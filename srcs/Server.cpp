#include "../includes/Server.hpp"

t_msg_from_serv defaultMsg()
{
    return {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};
}

Server::Server(EpollHelper &epoll, const t_server_config &config) : epoll_(epoll), config_(config) {}

const t_server_config &Server::getConfig() const { return config_; }

void Server::addConn(int fd)
{
    t_conn conn = make_conn(fd, config_.max_request_size);

    conns_.push_back(std::move(conn));
    conn_map_.emplace(fd, &conns_.back());
    LOG_INFO("New connection added: ", fd);
}

//
// Helpers for FSM
//

t_msg_from_serv Server::resetConnMap(t_conn *conn)
{
    t_msg_from_serv msg = {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};

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

    if (conn->socket_fd != -1)
    {
        conn_map_.erase(conn->socket_fd);
        msg.fds_to_unregister.push_back(conn->socket_fd);
    }

    conn->status = TERMINATED;
    LOG_INFO("Connection closed: ", conn->socket_fd);
    return msg;
}

t_msg_from_serv Server::timeoutKiller()
{
    auto now = time(NULL);

    t_msg_from_serv msg = {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};

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

t_msg_from_serv Server::reqHeaderParsingHandler(int fd, t_conn *conn)
{
    if (fd != conn->socket_fd)
    {
        LOG_WARN("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    const ssize_t bytes_read = conn->read_buf->readFd(fd);

    // Handle read errors and special conditions
    if (bytes_read == BUFFER_ERROR)
    {
        LOG_ERROR("Error reading from socket for fd: ", fd);
        conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
        return resheaderProcessingHandler(conn);
    }

    // Buffer full, wait for the next read event
    if (bytes_read == BUFFER_FULL)
        return defaultMsg(); // Wait for the next read event.

    // EOF reached, should not be here since header has not been parsed yet.
    if (bytes_read == EOF_REACHED)
    {
        LOG_ERROR("EOF reached while reading from socket for fd: ", fd);
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }

    conn->bytes_received += bytes_read;

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
            // For GET/DELETE, no body is expected
            if (method == GET || method == DELETE)
                conn->content_length = 0;
            else if ((method == POST || method == CGI) && conn->request->isChunked())
            {
                // Chunked transfer encoding, content length is not known in advance
                conn->content_length = config_.max_request_size;
            }
            else
                throw WebServErr::ShouldNotBeHereException("Content-Length not found for method requiring body");
        }

        LOG_INFO("Header parsed successfully for fd: ", fd);
        bool ok = conn->read_buf->removeHeaderAndSetChunked(conn->request->getupToBodyCounter(), conn->request->isChunked());

        if (!ok)
        {
            LOG_ERROR("Failed to remove header from buffer for fd: ", fd);
            conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
            return resheaderProcessingHandler(conn);
        }

        // Remove the header size from bytes_received
        conn->bytes_received -= conn->request->getupToBodyCounter(); // Adjust bytes_received after removing header

        return reqHeaderProcessingHandler(fd, conn);
    }
    catch (const WebServErr::InvalidRequestHeader &e) // Incomplete header
    {
        if (conn->read_buf->isEOF()) // EOF reached but header not complete
        {
            LOG_ERROR("Invalid request header for fd: ", fd, e.what());
            conn->error_code = ERR_400_BAD_REQUEST;
            return resheaderProcessingHandler(conn);
        }

        // Check if max header size is exceeded
        const bool is_max_length_reached = (conn->bytes_received >= config_.max_headers_size);
        if (is_max_length_reached)
        {
            LOG_ERROR("Header size exceeded for fd: ", fd);
            conn->error_code = ERR_400_BAD_REQUEST;
            return resheaderProcessingHandler(conn);
        }

        return defaultMsg(); // Ignore the error since the header might be incomplete.
    }
    catch (const WebServErr::BadRequestException &e)
    {
        LOG_ERROR("Bad request for fd: ", fd, e.what());
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Exception during header parsing for fd: ", fd, e.what());
        conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
        return resheaderProcessingHandler(conn);
    }
}

t_msg_from_serv Server::reqHeaderProcessingHandler(int fd, t_conn *conn)
{
    conn->status = REQ_HEADER_PROCESSING;
    try
    {
        conn->res = MethodHandler(epoll_).handleRequest(config_, conn->request->getrequestLineMap(), conn->request->getrequestHeaderMap(), conn->request->getrequestBodyMap());
        t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
        switch (method)
        {
        case GET:
        case DELETE:
            return resheaderProcessingHandler(conn);
        case POST:
        case CGI:
        {
            t_msg_from_serv msg = {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};
            conn->inner_fd_in = conn->res.fileDescriptor.get()->get();
            msg.fds_to_register.push_back(std::move(conn->res.fileDescriptor));

            conn_map_.emplace(conn->inner_fd_in, conn);
            LOG_INFO("Switching to processing state for fd: ", fd);
            conn->status = REQ_BODY_PROCESSING;
            return msg;
        }
        default:
            throw WebServErr::ShouldNotBeHereException("Unhandled method after parsing header");
        }
    }
    catch (const WebServErr::MethodException &e)
    {
        LOG_ERROR("Method exception occurred: ", e.code(), e.what());
        conn->error_code = e.code();
        return resheaderProcessingHandler(conn);
    }
}

t_msg_from_serv Server::reqBodyProcessingInHandler(int fd, t_conn *conn)
{
    if (fd != conn->socket_fd)
    {
        LOG_WARN("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    // Update heartbeat
    conn->last_heartbeat = time(NULL);

    const ssize_t bytes_read = conn->read_buf->readFd(fd);

    // Handle read errors and special conditions
    if (bytes_read == BUFFER_ERROR)
    {
        LOG_ERROR("Error reading from socket for fd: ", fd);
        conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
        return resheaderProcessingHandler(conn);
    }

    // Buffer full, wait for the next read event
    if (bytes_read == BUFFER_FULL)
        return defaultMsg(); // Wait for the next read event.

    // EOF reached, should not be here since header has not been parsed yet.
    if (bytes_read == EOF_REACHED)
    {
        LOG_ERROR("EOF reached while reading from socket for fd: ", fd);
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }

    conn->bytes_received += bytes_read;

    const bool is_content_length_reached = conn->bytes_received == conn->content_length;
    const bool is_chunked_eof_reached = conn->request->isChunked() && conn->read_buf->isEOF();

    if (is_content_length_reached)
    {
        LOG_INFO("Request body fully received for fd: ", fd);
        return defaultMsg(); // Wait for the main loop to notify when inner fd is ready.
    }

    if (is_chunked_eof_reached)
    {
        LOG_INFO("Chunked request body fully received for fd: ", fd);
        conn->content_length = conn->read_buf->size();
        return defaultMsg(); // Wait for the main loop to notify when inner fd is ready.
    }

    const bool is_content_length_exceeded = conn->bytes_received > conn->content_length;
    if (is_content_length_exceeded || conn->read_buf->isEOF())
    {
        LOG_ERROR("Request size exceeded or EOF reached but content length not reached for fd: ", fd);
        // TODO: delete the created file.
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }

    return defaultMsg(); // Continue reading
}

t_msg_from_serv Server::reqBodyProcessingOutHandler(int fd, t_conn *conn)
{
    // Should be the inner fd for writing request body to the file
    if (fd != conn->inner_fd_in)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    // Update heartbeat
    conn->last_heartbeat = time(NULL);

    // Wait for the next write event if buffer is empty
    if (conn->read_buf->isEmpty())
    {
        LOG_INFO("Read buffer empty for fd: ", fd);
        return defaultMsg();
    }

    ssize_t bytes_write = conn->read_buf->writeFd(fd);

    LOG_INFO("Data written to internal fd for fd: ", fd, " bytes: ", bytes_write);

    // Handle write errors and special conditions
    if (bytes_write == BUFFER_ERROR || bytes_write == EOF_REACHED)
    {
        LOG_ERROR("Error writing to internal fd: ", fd);
        conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
        return resheaderProcessingHandler(conn);
    }

    conn->bytes_sent += bytes_write;
    LOG_INFO("Data written to internal fd for fd: ", fd, " bytes: ", bytes_write, " total: ", conn->bytes_sent);

    // Check if content length is exceeded
    if (conn->bytes_sent > conn->content_length)
    {
        LOG_ERROR("Content length exceeded for fd: ", fd);
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }

    // Check if content length is reached
    if (conn->bytes_sent == conn->content_length)
    {
        LOG_INFO("Request body fully received for fd: ", fd);
        return resheaderProcessingHandler(conn);
    }

    return defaultMsg();
}

t_msg_from_serv Server::resheaderProcessingHandler(t_conn *conn)
{
    conn->status = RES_HEADER_PROCESSING;
    const std::string header = (conn->error_code == ERR_NO_ERROR)
                                   ? conn->response->successResponse(conn, conn->res.fileSize)
                                   : conn->response->failedResponse(conn, conn->error_code, "Error");

    conn->status = RESPONSE;

    conn->bytes_sent = 0;

    t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));

    if (method != CGI)
        conn->write_buf->insertHeader(header);

    switch (method)
    {
    case GET:
        conn->output_length = conn->res.fileSize + header.size();
        break;
    case DELETE:
    case POST:
        conn->output_length = header.size();
        break;
    case CGI:
        conn->status = RES_HEADER_PROCESSING;
        conn->output_length = config_.max_request_size; // Unknown length, send until EOF
        break;
    }

    // Unregister and close the inner fd for reading request body if exists
    t_msg_from_serv msg = {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};
    if (method != CGI && conn->inner_fd_in != -1) // For CGI, we use same fd for in and out
    {
        conn_map_.erase(conn->inner_fd_in);
        msg.fds_to_unregister.push_back(conn->inner_fd_in);
        conn->inner_fd_in = -1;
    }

    // Register the inner fd for writing response body if GET method
    if (method == GET)
    {
        conn->inner_fd_out = conn->res.fileDescriptor.get()->get();
        msg.fds_to_register.push_back(std::move(conn->res.fileDescriptor));
        conn_map_.emplace(conn->inner_fd_out, conn);
    }
    return msg;
}

t_msg_from_serv Server::responseInHandler(int fd, t_conn *conn)
{
    if (fd != conn->inner_fd_out)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    conn->last_heartbeat = time(NULL);

    if (conn->write_buf->isFull())
    {
        return defaultMsg();
    }

    ssize_t bytes_read = conn->write_buf->readFd(fd);

    LOG_INFO("Data read from internal fd for fd: ", fd, " bytes: ", bytes_read);

    if (bytes_read == BUFFER_ERROR)
    {
        LOG_ERROR("Error reading from internal fd for fd: ", fd);
        return terminatedHandler(fd, conn);
    }

    if (bytes_read == EOF_REACHED)
    {
        t_msg_from_serv msg = defaultMsg();
        if (conn->inner_fd_out != -1)
        {
            conn_map_.erase(conn->inner_fd_out);
            msg.fds_to_unregister.push_back(conn->inner_fd_out);
            conn->inner_fd_out = -1;
        }
        return msg;
    }

    if (bytes_read == BUFFER_FULL)
    {
        return defaultMsg();
    }

    t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
    if (conn->status == RES_HEADER_PROCESSING && method == CGI)
    {
        // TODO: Implement CGI response header parsing
        conn->status = RESPONSE;
    }

    // Check if done
    if (conn->write_buf->size() > conn->output_length)
    {
        LOG_ERROR("Output length exceeded for fd: ", fd);
        return terminatedHandler(fd, conn);
    }

    return defaultMsg();
}

t_msg_from_serv Server::responseOutHandler(int fd, t_conn *conn)
{
    // Should be the socket fd
    if (fd != conn->socket_fd)
    {
        LOG_ERROR("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    conn->last_heartbeat = time(NULL);

    // Skip when buffer is empty
    if (conn->write_buf->isEmpty())
    {
        return defaultMsg();
    }

    // Write data to socket
    ssize_t bytes_written = conn->write_buf->writeFd(fd);
    if (bytes_written == BUFFER_ERROR || bytes_written == EOF_REACHED)
    {
        LOG_ERROR("Error writing to socket for fd: ", fd);
        return terminatedHandler(fd, conn);
    }

    conn->bytes_sent += bytes_written;
    LOG_INFO("Data written to fd: ", fd, " bytes: ", bytes_written, " total: ", conn->bytes_sent);

    // Check if done
    if (conn->bytes_sent == conn->output_length)
    {
        // If done, buffer should be empty
        if (conn->write_buf->isEmpty())
        {
            return doneHandler(fd, conn);
        }
        else
        {
            LOG_ERROR("Output length reached but content length not matched for fd: ", fd);
            return terminatedHandler(fd, conn);
        }
    }

    // Check if exceeded
    if (conn->bytes_sent > conn->output_length)
    {
        LOG_ERROR("Output length exceeded for fd: ", fd);
        return terminatedHandler(fd, conn);
    }

    // Not done yet
    return defaultMsg();
}

t_msg_from_serv Server::doneHandler(int fd, t_conn *conn)
{
    conn->status = DONE;
    const bool keep_alive = conn->request->getrequestHeaderMap().contains("Connection") && conn->request->getrequestHeaderMap().at("Connection") != "close";

    // Terminate the connection if error occurred or not keep-alive
    if (conn->error_code != ERR_NO_ERROR || !keep_alive)
    {
        conn->status = DONE;
        LOG_INFO("Connection closed: ", fd);
        t_msg_from_serv msg = closeConn(conn);
        int fd = conn->socket_fd;
        conns_.remove_if([fd](const t_conn &c)
                         { return c.socket_fd == fd; });
        return msg;
    }

    // Reset the connection for next request if keep-alive
    t_msg_from_serv msg = resetConnMap(conn);
    resetConn(conn, conn->socket_fd, config_.max_request_size);
    LOG_INFO("Keep-alive: ready for next request on fd: ", fd);
    return msg;
}

t_msg_from_serv Server::terminatedHandler(int fd, t_conn *conn)
{
    conn->status = TERMINATED;

    int fd = conn->socket_fd;
    LOG_INFO("Connection terminated: ", conn->socket_fd);
    t_msg_from_serv msg = closeConn(conn);
    conns_.remove_if([fd](const t_conn &c)
                     { return c.socket_fd == fd; });
    return msg;
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
        return terminatedHandler(fd, conn);

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
            return reqHeaderParsingHandler(fd, conn);
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
            return reqBodyProcessingInHandler(fd, conn);
        case WRITE_EVENT:
            if (fd != conn->inner_fd_in)
            {
                LOG_WARN("Invalid fd for REQ_BODY_PROCESSING WRITE_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return reqBodyProcessingOutHandler(fd, conn);
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
            return responseInHandler(fd, conn);
        case WRITE_EVENT:
            if (fd != conn->socket_fd)
            {
                LOG_WARN("Invalid fd for RESPONSE WRITE_EVENT for fd: ", fd);
                return defaultMsg();
            }
            return responseOutHandler(fd, conn);
        default:
            LOG_ERROR("Invalid event type for RESPONSE for fd: ", fd);
            throw WebServErr::ShouldNotBeHereException("Invalid event type for RESPONSE");
        }
    case REQ_HEADER_PROCESSING:
        return defaultMsg(); // Ignore events in these states
    case RES_HEADER_PROCESSING:
        if (event_type == READ_EVENT && fd == conn->inner_fd_out)
            return responseInHandler(fd, conn);
        return defaultMsg();
    case DONE:
    case TERMINATED:
        LOG_WARN("Connection in terminal state for fd: ", fd);
        return defaultMsg();
    default:
        LOG_ERROR("Unhandled status in scheduler for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Unhandled status in scheduler");
    }
}
