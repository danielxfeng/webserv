#include "../includes/Server.hpp"

t_msg_from_serv defaultMsg()
{
    return {std::vector<RaiiFd>{}, std::vector<int>{}};
}

Server::Server(EpollHelper &epoll, const t_server_config &config) : epoll_(epoll), config_(config) {}

const t_server_config &Server::getConfig() const { return config_; }

void Server::addConn(int fd)
{
    auto now = time(NULL);

    t_conn conn = make_conn(fd, config_.max_request_size);

    conns_.push_back(std::move(conn));
    conn_map_.emplace(fd, &conns_.back());
    LOG_INFO("New connection added: ", fd);
}

//
// Handle data in
//

t_msg_from_serv Server::handleDataInFromSocketParsingHeader(int fd, t_conn *conn, bool is_eof)
{
    try
    {
        conn->request->httpParser(conn->read_buf->getData().front());
        t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
        if (conn->request->getrequestLineMap().contains("Content-Length"))
            conn->content_length = static_cast<size_t>(stoull(conn->request->getrequestLineMap().at("Content-Length")));
        else
        {
            if (method == GET || method == DELETE)
                conn->content_length = 0;
            else if ((method == POST || method == CGI) && conn->request->isChunked())
            {
                conn->content_length = config_.max_request_size;
            }
            else
                throw WebServErr::ShouldNotBeHereException("Content-Length not found for method requiring body");
        }

        LOG_INFO("Header parsed successfully for fd: ", fd);
        conn->status = READING;

        // TODO: remove the header from buffer
        conn->bytes_received = 0; // Reset bytes_received for body reading
        try
        {
            t_file output = MethodHandler(epoll_).handleRequest(config_, conn->request->getrequestLineMap(), conn->request->getrequestHeaderMap(), conn->request->getrequestBodyMap());
            switch (method)
            {
            case GET:
            case DELETE:
                LOG_INFO("Switching to writing state for fd: ", fd);
                return switchToWritingState(conn, output);
            case POST:
            case CGI:
            {

                t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};
                msg.fds_to_register.push_back(output.fileDescriptor);
                conn->inner_fd_in = output.fileDescriptor.get();
                conn_map_.emplace(conn->inner_fd_in, conn);
                LOG_INFO("Switching to processing state for fd: ", fd);
                return msg;
            }
            case SRV_ERROR:
                return defaultMsg();
            default:
                throw WebServErr::ShouldNotBeHereException("Unhandled method after parsing header");
            }
        }
        catch (const WebServErr::MethodException &e)
        {
            LOG_ERROR("Method exception occurred: ", e.code(), e.what());
            conn->status = SRV_ERROR;
            return handleError(conn, e.code(), e.what());
        }
    }
    catch (const WebServErr::InvalidRequestHeader &e)
    {
        if (is_eof)
        {
            conn->status = SRV_ERROR;
            LOG_ERROR("Invalid request header for fd: ", fd, e.what());
            return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, didn't find end of header when EOF reached");
        }

        const bool is_max_length_reached = (conn->bytes_received >= config_.max_headers_size);
        if (is_max_length_reached)
        {
            conn->status = SRV_ERROR;
            LOG_ERROR("Header size exceeded for fd: ", fd);
            return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, header size exceeded");
        }

        return defaultMsg(); // Ignore the error since the header might be incomplete.
    }
    catch (const WebServErr::BadRequestException &e)
    {
        conn->status = SRV_ERROR;
        LOG_ERROR("Bad request for fd: ", fd, e.what());
        return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, parsing failed");
    }
}

t_msg_from_serv Server::handleDataInFromSocketReadingBody(int fd, t_conn *conn, bool is_eof)
{
    const bool is_content_length_reached = conn->bytes_received == conn->content_length;
    const bool is_chunked_eof_reached = conn->request->isChunked() && is_eof;

    if (is_chunked_eof_reached || is_content_length_reached)
    {
        conn->status = PROCESSING;
        return defaultMsg(); // Wait for the main loop to notify when inner fd is ready.
    }

    const bool is_content_length_exceeded = conn->bytes_received > conn->content_length;
    if (is_content_length_exceeded || is_eof)
    {
        conn->status = SRV_ERROR;
        LOG_ERROR("Request size exceeded or EOF reached but content length not reached for fd: ", fd);
        // TODO: delete the created file.
        return handleError(conn, ERR_400_BAD_REQUEST, "Request size exceeded or EOF reached but content length not reached");
    }

    return defaultMsg(); // Continue reading
}

t_msg_from_serv Server::handleDataInFromSocket(int fd, t_conn *conn)
{
    ssize_t bytes_read = conn->read_buf->readFd(fd);

    if (bytes_read == BUFFER_FULL || bytes_read == BUFFER_ERROR)
        return defaultMsg(); // Checking error is disallowed in subject.

    // Update heartbeat
    conn->last_heartbeat = time(NULL);

    conn->bytes_received += bytes_read;
    LOG_INFO("Data read from fd: ", fd, " bytes: ", bytes_read, " total: ", conn->bytes_received);

    bool is_eof = (bytes_read == EOF_REACHED);

    switch (conn->status)
    {
    case HEADER_PARSING:
        return handleDataInFromSocketParsingHeader(fd, conn, is_eof);
    case READING:
        return handleDataInFromSocketReadingBody(fd, conn, is_eof);
    case SRV_ERROR:
        return defaultMsg();
    default:
        return handleDataEnd(fd);
    }
}

t_msg_from_serv Server::handleDataInFromInternal(int fd, t_conn *conn)
{
    if (fd != conn->inner_fd_out)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    if (conn->status != WRITING && conn->status != PREPARING_RESPONSE_HEADER)
    {
        LOG_DEBUG("Connection not in writing state for fd: ", fd);
        return defaultMsg();
    }

    ssize_t bytes_read = conn->write_buf->readFd(fd);
    switch (bytes_read)
    {
    case SRV_ERROR:
        LOG_INFO("Error reading from internal fd for fd: ", fd);
        return handleDataEnd(fd);
    case EOF_REACHED:
    {
        t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};
        msg.fds_to_unregister.push_back(fd);
        conn_map_.erase(fd);
        conn->inner_fd_out = -1;
        LOG_INFO("Finished reading from internal fd for fd: ", fd);
        return msg;
    }
    default:
        LOG_INFO("Reading from internal fd for fd: ", fd);
        return defaultMsg();
    }
}

t_msg_from_serv Server::handleDataIn(int fd)
{
    // We close the conn first, then un-register the fds from epoll.
    // So there may be a delay between the two operations.
    if (!conn_map_.contains(fd))
    {
        LOG_WARN("Connection not found for fd: ", fd);
        return defaultMsg();
    }

    t_conn *conn = conn_map_.at(fd);

    if (fd == conn->socket_fd) // Reading from the client socket
    {
        return handleDataInFromSocket(fd, conn);
    }
    else
    {
        return handleDataInFromInternal(fd, conn);
    }
}

//
// Handle data out
//

t_msg_from_serv Server::handleDataOutToSocketDone(int fd, t_conn *conn)
{
    const bool is_output_exceeded = conn->bytes_sent > conn->output_length;
    if (is_output_exceeded)
    {
        LOG_ERROR("Output length exceeded for fd: ", fd);
        return handleDataEnd(fd);
    }

    const bool is_output_complete = (conn->bytes_sent == conn->output_length);
    if (is_output_complete)
    {
        const bool is_close = conn->request->getrequestHeaderMap().contains("Connection") && conn->request->getrequestHeaderMap().at("Connection") == "close";
        if (conn->status == SRV_ERROR || is_close)
        {
            conn->status = DONE;
            LOG_INFO("Finished writing error response to socket for fd: ", fd);
            return handleDataEnd(fd);
        }
        else
        {
            t_conn new_conn = make_conn(fd, config_.max_request_size);
            conns_.push_back(std::move(new_conn));
            conn_map_.at(fd) = &conns_.back();
            conn->status = HEADER_PARSING;
            LOG_INFO("Finished writing response to socket for fd: ", fd);
            return resetConn(conn);
        }
    }
}

t_msg_from_serv Server::handleDataOutToSocket(int fd, t_conn *conn)
{
    if (fd != conn->socket_fd)
    {
        LOG_ERROR("Socket fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Socket fd mismatch");
    }

    if (conn->status == PREPARING_RESPONSE_HEADER)
    {
        LOG_DEBUG("Connection in waiting header state for fd: ", fd);
        return handleWaitingHeaderStage(conn);
    }

    if (conn->status != WRITING && conn->status != SRV_ERROR)
    {
        return defaultMsg();
    }

    ssize_t bytes_written = conn->write_buf->writeFd(fd);

    switch (bytes_written)
    {
    case BUFFER_ERROR:
        LOG_ERROR("Error writing to socket for fd: ", fd);
        return handleDataEnd(fd);
    case BUFFER_EMPTY:
        return defaultMsg();
    }

    conn->bytes_sent += (bytes_written > 0 ? static_cast<size_t>(bytes_written) : 0);
    LOG_INFO("Data written to fd: ", fd, " bytes: ", bytes_written, " total: ", conn->bytes_sent);

    if (conn->bytes_sent >= conn->output_length) // Check if done
        return handleDataOutToSocketDone(fd, conn);

    return defaultMsg();
}

t_msg_from_serv Server::handleDataOutToInternal(int fd, t_conn *conn)
{
    if (fd != conn->inner_fd_in)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    if (conn->status != PROCESSING)
    {
        LOG_DEBUG("Connection not in processing state for fd: ", fd);
        return defaultMsg();
    }

    ssize_t bytes_written = conn->read_buf->writeFd(fd);
    switch (bytes_written)
    {
    case BUFFER_ERROR:
        LOG_ERROR("Error writing to internal fd for fd: ", fd);
        return handleDataEnd(fd);
    case BUFFER_FULL:
    case EOF_REACHED:
        LOG_ERROR("Internal buffer full when writing to internal fd for fd: ", fd);
        return handleError(conn, ERR_500_INTERNAL_SERVER_ERROR, "Internal buffer full when writing to internal fd");
    case BUFFER_EMPTY:
    {
        LOG_INFO("Finished writing to internal fd for fd: ", fd);
        t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};
        msg.fds_to_unregister.push_back(fd);
        conn_map_.erase(fd);
        conn->inner_fd_in = -1;
        conn->status = PREPARING_RESPONSE_HEADER;
        return msg;
    }
    default:
        LOG_INFO("Writing to internal fd for fd: ", fd);
        return defaultMsg();
    }
}

t_msg_from_serv Server::handleDataOut(int fd)
{
    // We close the conn first, then un-register the fds from epoll.
    // So there may be a delay between the two operations.
    if (!conn_map_.contains(fd))
    {
        LOG_WARN("Connection not found for fd: ", fd);
        return defaultMsg();
    }

    t_conn *conn = conn_map_.at(fd);

    if (fd == conn->socket_fd) // Writing to the client socket
    {
        return handleDataOutToSocket(fd, conn);
    }
    else
    {
        return handleDataOutToInternal(fd, conn);
    }
}

t_msg_from_serv Server::handleError(t_conn *conn, const t_status_error_codes error_code, const std::string &error_message)
{
    conn->status = SRV_ERROR;
    conn->response = std::make_shared<HttpResponse>();
    conn->write_buf.append(conn->response->failedResponse(*conn, error_code, error_message));
}

//
// Helpers for FSM
//

t_msg_from_serv Server::resetConnMap(t_conn *conn)
{
    t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};

    if (conn->inner_fd_in != -1)
    {
        conn_map_.erase(conn->inner_fd_in);
        msg.fds_to_unregister.push_back(conn->inner_fd_in);
        conn->inner_fd_in = -1;
    }

    if (conn->inner_fd_out != -1)
    {
        conn_map_.erase(conn->inner_fd_out);
        msg.fds_to_unregister.push_back(conn->inner_fd_out);
        conn->inner_fd_out = -1;
    }

    return msg;
}

t_msg_from_serv Server::closeConn(t_conn *conn)
{
    t_msg_from_serv msg = resetConnMap(conn);

    int fd = conn->socket_fd;

    if (conn->socket_fd != -1)
    {
        conn_map_.erase(conn->socket_fd);
        msg.fds_to_unregister.push_back(conn->socket_fd);
        conn->socket_fd = -1;
    }
    conns_.remove_if([fd](const t_conn &c)
                     { return c.socket_fd == fd; });
    conn->status = TERMINATED;
    LOG_INFO("Connection closed: ", fd);
    return msg;
}

t_msg_from_serv Server::timeoutKiller()
{
    auto now = time(NULL);

    t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};

    for (auto it = conns_.begin(); it != conns_.end();)
    {
        if (difftime(now, it->last_heartbeat) > config_.max_heartbeat_timeout ||
            difftime(now, it->start_timestamp) > config_.max_request_timeout)
        {
            int fd = it->socket_fd;
            t_msg_from_serv temp = closeConn(&*it);
            msg.fds_to_unregister.insert(
                msg.fds_to_unregister.end(),
                temp.fds_to_unregister.begin(),
                temp.fds_to_unregister.end());

            it = conns_.erase(it);
            LOG_INFO("Connection timed out and closed: ", fd);
        }
        else
        {
            ++it;
        }
    }

    return msg;
}

//
// Finite State Machine
//

t_msg_from_serv Server::req_header_parsing_handler(int fd, t_conn *conn)
{
    try
    {
        std::string_view buf = conn->read_buf->peek();
        conn->request->httpParser(buf);

        // After successful parsing, determine the method and content length
        t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
        if (conn->request->getrequestLineMap().contains("Content-Length"))
            conn->content_length = static_cast<size_t>(stoull(conn->request->getrequestLineMap().at("Content-Length")));
        else
        {
            if (method == GET || method == DELETE)
                conn->content_length = 0;
            else if ((method == POST || method == CGI) && conn->request->isChunked())
            {
                conn->content_length = config_.max_request_size;
            }
            else
                throw WebServErr::ShouldNotBeHereException("Content-Length not found for method requiring body");
        }

        LOG_INFO("Header parsed successfully for fd: ", fd);
        conn->read_buf->insertHeaderAndSetChunked(conn->request->getupToBodyCounter(), conn->request->isChunked());

        return req_header_processing_handler(fd, conn);
    }
    catch (const WebServErr::InvalidRequestHeader &e)
    {
        if (is_eof)
        {
            return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, didn't find end of header when EOF reached");
        }

        const bool is_max_length_reached = (conn->bytes_received >= config_.max_headers_size);
        if (is_max_length_reached)
        {
            return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, header size exceeded");
        }

        return defaultMsg(); // Ignore the error since the header might be incomplete.
    }
    catch (const WebServErr::BadRequestException &e)
    {
        return handleError(conn, ERR_400_BAD_REQUEST, "Invalid request header, parsing failed");
    }
}

t_msg_from_serv Server::req_header_processing_handler(int fd, t_conn *conn)
{
    try
        {
            t_file output = MethodHandler(epoll_).handleRequest(config_, conn->request->getrequestLineMap(), conn->request->getrequestHeaderMap(), conn->request->getrequestBodyMap());
            t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
            switch (method)
            {
            case GET:
            case DELETE:
                conn->res = output;
                return res_header_processing_handler(fd, conn);
            case POST:
            case CGI:
            {

                t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};
                msg.fds_to_register.push_back(output.fileDescriptor);
                conn->inner_fd_in = output.fileDescriptor.get();
                conn_map_.emplace(conn->inner_fd_in, conn);
                LOG_INFO("Switching to processing state for fd: ", fd);
                return msg;
            }
            case SRV_ERROR:
                return defaultMsg();
            default:
                throw WebServErr::ShouldNotBeHereException("Unhandled method after parsing header");
            }
        }
        catch (const WebServErr::MethodException &e)
        {
            LOG_ERROR("Method exception occurred: ", e.code(), e.what());
            return handleError(conn, e.code(), e.what());
        }
}

t_msg_from_serv Server::req_body_processing_in_handler(int fd, t_conn *conn)
{
    const bool is_content_length_reached = conn->bytes_received == conn->content_length;
    const bool is_chunked_eof_reached = conn->request->isChunked() && is_eof;

    if (is_chunked_eof_reached || is_content_length_reached)
    {
        conn->status = PROCESSING;
        return defaultMsg(); // Wait for the main loop to notify when inner fd is ready.
    }

    const bool is_content_length_exceeded = conn->bytes_received > conn->content_length;
    if (is_content_length_exceeded || is_eof)
    {
        conn->status = SRV_ERROR;
        LOG_ERROR("Request size exceeded or EOF reached but content length not reached for fd: ", fd);
        // TODO: delete the created file.
        return handleError(conn, ERR_400_BAD_REQUEST, "Request size exceeded or EOF reached but content length not reached");
    }

    return defaultMsg(); // Continue reading
}

t_msg_from_serv Server::req_body_processing_out_handler(int fd, t_conn *conn)
{
    if (fd != conn->inner_fd_out)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    if (conn->status != WRITING && conn->status != PREPARING_RESPONSE_HEADER)
    {
        LOG_DEBUG("Connection not in writing state for fd: ", fd);
        return defaultMsg();
    }

    ssize_t bytes_read = conn->write_buf->readFd(fd);
    switch (bytes_read)
    {
    case SRV_ERROR:
        LOG_INFO("Error reading from internal fd for fd: ", fd);
        return handleDataEnd(fd);
    case EOF_REACHED:
    {
        t_msg_from_serv msg = {std::vector<RaiiFd>{}, std::vector<int>{}};
        msg.fds_to_unregister.push_back(fd);
        conn_map_.erase(fd);
        conn->inner_fd_out = -1;
        LOG_INFO("Finished reading from internal fd for fd: ", fd);
        return msg;
    }
    default:
        LOG_INFO("Reading from internal fd for fd: ", fd);
        return defaultMsg();
    }
}

t_msg_from_serv Server::res_header_processing_handler(int fd, t_conn *conn)
{
    return defaultMsg();
}

t_msg_from_serv Server::response_in_handler(int fd, t_conn *conn)
{
    return defaultMsg();
}

t_msg_from_serv Server::response_out_handler(int fd, t_conn *conn)
{
    return defaultMsg();
}

t_msg_from_serv Server::done_handler(int fd, t_conn *conn)
{
    const bool keep_alive = conn->request->getrequestHeaderMap().contains("Connection") && conn->request->getrequestHeaderMap().at("Connection") == "keep-alive";

    // Terminate the connection if error occurred or not keep-alive
    if (conn->error_code != ERR_NO_ERROR || !keep_alive)
    {
        conn->status = DONE;
        LOG_INFO("Connection closed: ", fd);
        return closeConn(conn);
    }

    // Reset the connection for next request if keep-alive
    t_msg_from_serv msg = resetConnMap(conn);
    resetConn(conn, conn->socket_fd, config_.max_request_size);
    LOG_INFO("Keep-alive: ready for next request on fd: ", fd);
    return msg;
}

t_msg_from_serv Server::terminated_handler(int fd, t_conn *conn)
{
    return closeConn(conn);
}

//
// Scheduler
//

t_msg_from_serv Server::scheduler(int fd, t_event_type event_type)
{
    if (!conn_map_.contains(fd))
    {
        LOG_WARN("Connection not found for fd: ", fd);
        return defaultMsg();
    }

    t_conn *conn = conn_map_.at(fd);
    t_status status = conn->status;

    if (event_type == ERROR_EVENT)
        return terminated_handler(fd, conn);

    switch (status)
    {
    case REQ_HEADER_PARSING:
        if (fd != conn->socket_fd)
        {
            LOG_WARN("Invalid fd for REQ_HEADER_PARSING for fd: ", fd);
            return defaultMsg();
        }
        switch (event_type)
        {
        case READ_EVENT:
            return req_header_parsing_handler(fd, conn);
        case WRITE_EVENT:
            return defaultMsg(); // Ignore write event in this state
        default:
            LOG_ERROR("Invalid event type for REQ_HEADER_PARSING for fd: ", fd);
            throw WebServErr::ShouldNotBeHereException("Invalid event type for REQ_HEADER_PARSING");
        }
    case REQ_BODY_PROCESSING:
        switch (event_type)
        {
        case READ_EVENT:
            if (fd != conn->socket_fd)
            {
                LOG_WARN("Invalid fd for REQ_BODY_PROCESSING READ_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return req_body_processing_in_handler(fd, conn);
        case WRITE_EVENT:
            if (fd != conn->inner_fd_in)
            {
                LOG_WARN("Invalid fd for REQ_BODY_PROCESSING WRITE_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return req_body_processing_out_handler(fd, conn);
        default:
            LOG_ERROR("Invalid event type for REQ_BODY_PROCESSING for fd: ", fd);
            throw WebServErr::ShouldNotBeHereException("Invalid event type for REQ_BODY_PROCESSING");
        }
    case RESPONSE:
        switch (event_type)
        {
        case READ_EVENT:
            if (fd != conn->inner_fd_out)
            {
                LOG_WARN("Invalid fd for RESPONSE READ_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return response_in_handler(fd, conn);
        case WRITE_EVENT:
            if (fd != conn->socket_fd)
            {
                LOG_WARN("Invalid fd for RESPONSE WRITE_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return response_out_handler(fd, conn);
        default:
            LOG_ERROR("Invalid event type for RESPONSE for fd: ", fd);
            throw WebServErr::ShouldNotBeHereException("Invalid event type for RESPONSE");
        }
    case REQ_HEADER_PROCESSING:
    case RES_HEADER_PROCESSING:
        LOG_ERROR("Invalid event type for REQ_HEADER_PROCESSING or RES_HEADER_PROCESSING for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Invalid event type for REQ_HEADER_PROCESSING or RES_HEADER_PROCESSING");
    case DONE:
    case TERMINATED:
        LOG_WARN("Connection in terminal state for fd: ", fd);
        return defaultMsg();
    default:
        LOG_ERROR("Unhandled status in scheduler for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Unhandled status in scheduler");
    }
}
