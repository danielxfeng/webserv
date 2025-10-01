#include "../includes/Server.hpp"

void resetConn(t_conn *conn, int socket_fd, size_t max_request_size)
{
    conn->socket_fd = socket_fd;
    conn->inner_fd_in = -1;
    conn->inner_fd_out = -1;
    conn->config_idx = -1;
    conn->is_cgi = false;
    conn->status = REQ_HEADER_PARSING;
    conn->start_timestamp = time(NULL);
    conn->last_heartbeat = conn->start_timestamp;
    conn->content_length = max_request_size;
    conn->bytes_received = 0;
    conn->output_length = max_request_size;
    conn->bytes_sent = 0;
    conn->res = t_file{nullptr, nullptr, 0, 0, false, "", ""};
    conn->read_buf = std::make_unique<Buffer>();
    conn->write_buf = std::make_unique<Buffer>();
    conn->request = std::make_shared<HttpRequests>();
    conn->response = std::make_shared<HttpResponse>();
    conn->error_code = ERR_NO_ERROR;
}

/**
 * @brief Helper function to create and initialize a t_conn structure.
 */
t_conn make_conn(int socket_fd, size_t max_request_size)
{
    t_conn conn;
    resetConn(&conn, socket_fd, max_request_size);
    return conn;
}

t_msg_from_serv defaultMsg()
{
    return {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};
}

Server::Server(EpollHelper &epoll, const std::vector<t_server_config> &configs) : epoll_(epoll), configs_(configs), cookies_(), conns_(), conn_map_(), inner_fd_map_() 
{
    for (size_t i = 0; i < configs_.size(); ++i)
        cookies_.emplace_back();

    for (auto &config: configs_)
    {
        LOG_INFO("server config:", config.server_name, config.err_pages[ERR_404_NOT_FOUND]);
    }
}

const std::vector<t_server_config> &Server::getConfigs() const { return configs_; }

const t_server_config &Server::getConfig(size_t idx) const
{
    if (idx >= configs_.size())
        throw std::out_of_range("Config index out of range");
    return configs_.at(idx);
}

void Server::addConn(int fd)
{
    t_conn conn = make_conn(fd, MAX_REQUEST_SIZE); // here we use the global limit, since we don't know the config yet

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
        if (conn->is_cgi)
            msg.fds_to_unregister.push_back(conn->inner_fd_in);
        else
            inner_fd_map_.erase(conn->inner_fd_in);
        
        conn->inner_fd_in = -1;
    }

    if (conn->inner_fd_out != -1)
    {
        conn_map_.erase(conn->inner_fd_out);
        if (conn->is_cgi)
            msg.fds_to_unregister.push_back(conn->inner_fd_out);
        else
            inner_fd_map_.erase(conn->inner_fd_out);
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
        size_t max_heartbeat_timeout = it->config_idx != -1 ? configs_[it->config_idx].max_heartbeat_timeout : GLOBAL_HEARTBEAT_TIMEOUT;
        size_t max_request_timeout = it->config_idx != -1 ? configs_[it->config_idx].max_request_timeout : GLOBAL_REQUEST_TIMEOUT;
        if (difftime(now, it->last_heartbeat) > max_heartbeat_timeout || difftime(now, it->start_timestamp) > max_request_timeout)
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
            ++it;
    }

    return msg;
}

//
// Finite State Machine
//

/**
 * @details
 * Reads data into the request buffer; if the buffer is full, waits for the next read event.
 * On read error, sets error code 500 and transitions to resheaderProcessing.
 * On unexpected EOF before header completion, sets error code 400 and transitions to resheaderProcessing.
 * Attempts to parse the header:
 *    - If complete and valid:
 *        - Determines the method and calculates content_length
 *          (special handling for GET/DELETE and chunked POST/CGI).
 *        - Removes the parsed header from the buffer and adjusts counters.
 *        - Transitions to reqHeaderProcessingHandler for method-specific setup.
 *    - If incomplete:
 *        - Continues waiting unless the header size exceeds the configured limit,
 *          in which case sets error code 400 and transitions to resheaderProcessing.
 *    - On invalid or malformed headers, sets error code 400.
 *    - On unexpected exceptions, sets error code 500.
 */
t_msg_from_serv Server::reqHeaderParsingHandler(int fd, t_conn *conn)
{
    if (fd != conn->socket_fd)
    {
        LOG_WARN("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    const ssize_t bytes_read = conn->read_buf->readFd(fd);

    if (bytes_read == RW_ERROR)
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
        if (conn->bytes_received == 0)
        {
            LOG_INFO("Client requested to close the conn", fd);
            return terminatedHandler(fd, conn);
        }
        LOG_ERROR("EOF reached while reading from socket for fd: ", fd);
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }

    conn->bytes_received += bytes_read;

    try
    {
        std::string_view buf = conn->read_buf->peek();
        conn->request->httpParser(buf);

        if (conn->config_idx == -1)
        {
            if (conn->request->getrequestHeaderMap().contains("servername"))
            {
                std::string host = conn->request->getrequestHeaderMap().at("servername");
                for (size_t i = 0; i < configs_.size(); ++i)
                {
                    if (configs_[i].server_name == toLower(host))
                    {
                        conn->config_idx = i;
                        break;
                    }
                }
            }

            if (conn->config_idx == -1)
                conn->config_idx = 0;
        }
        LOG_INFO("select config_idx", configs_[conn->config_idx].server_name);
        conn->content_length = configs_[conn->config_idx].max_request_size;
        conn->output_length = configs_[conn->config_idx].max_request_size;
        // TODO: add err page.
        //  After successful parsing, determine the method and content length
        t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
        if (conn->request->getrequestHeaderMap().contains("content-length"))
            conn->content_length = static_cast<size_t>(stoull(conn->request->getrequestHeaderMap().at("content-length")));
        else
        {
            // For GET/DELETE, no body is expected
            if (method == GET || method == DELETE)
                conn->content_length = 0;
            else if ((method == POST || method == CGI) && conn->request->isChunked())
                conn->content_length = configs_[conn->config_idx].max_request_size; // Chunked transfer encoding, content length is not known in advance
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

        for (const auto &pair : conn->request->getrequestHeaderMap())
            LOG_INFO("Header: ", pair.first + ": " + pair.second);
        for (const auto &pair : conn->request->getrequestLineMap())
            LOG_INFO("Request Line: ", pair.first + ": " + pair.second);
        LOG_INFO("Adjusted bytes received: ", conn->bytes_received);
        LOG_INFO("Buffer size after removing header: ", conn->read_buf->size());

        return reqHeaderProcessingHandler(fd, conn);
    }
    catch (const WebServErr::InvalidRequestHeader &e) // Incomplete header
    {
        if (conn->read_buf->isEOF()) // EOF reached but header not complete
        {
            LOG_ERROR("Invalid request header for fd: ", fd, e.what());
            conn->config_idx = 0;
            conn->error_code = ERR_400_BAD_REQUEST;
            return resheaderProcessingHandler(conn);
        }

        // Check if max header size is exceeded
        const size_t max_header_size = conn->config_idx == -1 ? MAX_HEADERS_SIZE : configs_[conn->config_idx].max_headers_size;
        const bool is_max_length_reached = (conn->bytes_received >= max_header_size);
        if (is_max_length_reached)
        {
            LOG_ERROR("Header size exceeded for fd: ", fd);
            conn->config_idx = 0;
            conn->error_code = ERR_400_BAD_REQUEST;
            return resheaderProcessingHandler(conn);
        }

        return defaultMsg(); // Ignore the error since the header might be incomplete.
    }
    catch (const WebServErr::BadRequestException &e)
    {
        LOG_ERROR("Bad request for fd: ", fd, e.what());
        conn->config_idx = 0;
        conn->error_code = ERR_400_BAD_REQUEST;
        return resheaderProcessingHandler(conn);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Exception during header parsing for fd: ", fd, e.what());
        conn->config_idx = 0;
        conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
        return resheaderProcessingHandler(conn);
    }
}

/**
 * @details
 * Processes the parsed request headers and prepares for response handling.
 *
 * - GET:
 *   Registers the response file descriptor as `inner_fd_out`,
 *   stores it in `inner_fd_map_`, and transitions directly to resheaderProcessing.
 *
 * - DELETE:
 *   No internal fd setup needed, transitions directly to resheaderProcessing.
 *
 * - POST:
 *   Registers the response file descriptor as `inner_fd_in`,
 *   stores it in `inner_fd_map_`, sets status to REQ_BODY_PROCESSING,
 *   and waits for the request body to be received.
 *
 * - CGI:
 *   Registers the CGI input pipe fd as `inner_fd_in`, pushes it to
 *   `fds_to_register` and `conn_map_`, sets status to REQ_BODY_PROCESSING,
 *   and waits for the request body to be sent to CGI.
 *
 * On method handling error, sets the connection error_code and
 * transitions to resheaderProcessing.
 */
t_msg_from_serv Server::reqHeaderProcessingHandler(int fd, t_conn *conn)
{
    conn->status = REQ_HEADER_PROCESSING;
    try
    {
        LOG_INFO("Processing request header for fd: ", fd);
        conn->res = MethodHandler(epoll_).handleRequest(configs_[conn->config_idx], conn->request->getrequestLineMap(), conn->request->getrequestHeaderMap(), conn->request->getrequestBodyMap(), epoll_);
        LOG_INFO("Resource prepared for fd: ", fd, " file size: ", conn->res.fileSize, " isDynamic: ", conn->res.isDynamic);
        t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));
        conn->is_cgi = (method == CGI);
        LOG_INFO("Method determined: ", conn->request->getrequestLineMap().at("Method"), " for fd: ", fd);
        switch (method)
        {
        case GET:
            LOG_INFO("FILE: ", conn->res.FD_handler_OUT.get()->get());
            if (conn->res.isDynamic)
                return resheaderProcessingHandler(conn);
            inner_fd_map_.emplace(conn->res.FD_handler_OUT.get()->get(), conn->res.FD_handler_OUT);
            conn->inner_fd_out = conn->res.FD_handler_OUT.get()->get();
            LOG_INFO("Switching to response header processing for fd: ", fd);
            return resheaderProcessingHandler(conn);
        case DELETE:
            return resheaderProcessingHandler(conn);
        case POST:
        {
            LOG_INFO("FILE IN: ", conn->res.FD_handler_IN.get()->get(), " FILE OUT: ", conn->res.FD_handler_OUT.get()->get());
            inner_fd_map_.emplace(conn->res.FD_handler_OUT.get()->get(), conn->res.FD_handler_OUT);
            conn->inner_fd_in = conn->res.FD_handler_OUT.get()->get();
            conn->status = REQ_BODY_PROCESSING;
            LOG_INFO("Switching to processing state for fd: ", fd);
            return reqBodyProcessingInHandler(fd, conn, true);
        }
        case CGI:
        {
            t_msg_from_serv msg = {std::vector<std::shared_ptr<RaiiFd>>{}, std::vector<int>{}};
            conn->inner_fd_in = conn->res.FD_handler_IN.get()->get();
            msg.fds_to_register.push_back(std::move(conn->res.FD_handler_IN));
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
        conn->config_idx = 0;
        conn->error_code = e.code();
        conn->error_message = e.what();
        return resheaderProcessingHandler(conn);
    }
}

/**
 * @details
 * Reads data from the client socket into the request read buffer.
 * Writes data to the internal fd for POST requests.
 * If the buffer is full, waits for the next read event.
 * If data is successfully read, updates the byte counter,
 *   and transitions to resheaderProcessing.
 * If the read size exceeds the declared Content-Length,
 *   sets error code 400 and transitions to resheaderProcessing.
 * If the full Content-Length is reached, transitions to resheaderProcessing.
 * On read error or unexpected EOF, sets error code 500 and
 *   transitions to resheaderProcessing.
 */
t_msg_from_serv Server::reqBodyProcessingInHandler(int fd, t_conn *conn, bool is_initial)
{
    if (fd != conn->socket_fd)
    {
        LOG_WARN("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    if (!is_initial)
    {
        // Update heartbeat
        conn->last_heartbeat = time(NULL);

        const ssize_t bytes_read = conn->read_buf->readFd(fd);

        // Handle read errors and special conditions
        if (bytes_read == RW_ERROR)
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
    }

    if (!conn->is_cgi)
    {
        ssize_t written_bytes = conn->read_buf->writeFile(conn->inner_fd_in);
        if (written_bytes == RW_ERROR)
        {
            LOG_ERROR("Error writing to internal fd for fd: ", fd);
            conn->error_code = ERR_500_INTERNAL_SERVER_ERROR;
            return resheaderProcessingHandler(conn);
        }
        LOG_INFO("Data written to internal fd for fd: ", fd, " bytes: ", written_bytes);
        conn->bytes_sent += written_bytes;
    }

    const bool is_content_length_reached = conn->bytes_received == conn->content_length;
    const bool is_chunked_eof_reached = conn->request->isChunked() && conn->read_buf->isEOF();

    if (is_content_length_reached)
    {
        LOG_INFO("Request body fully received for fd: ", fd);
        if (conn->is_cgi)
            return defaultMsg(); // Wait for the main loop to notify when inner fd is ready.
        inner_fd_map_.erase(conn->inner_fd_in);
        conn->inner_fd_in = -1;
        return resheaderProcessingHandler(conn);
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

/**
 * @details
 * Writes data from the request read buffer to the CGI input pipe.
 * If the read buffer is empty, waits for the next write event.
 * If data is successfully written, updates the byte counter.
 * If the written size exceeds the declared Content-Length,
 *   sets error code 400 and transitions to resheaderProcessing.
 * If the full Content-Length is reached, transitions to resheaderProcessing.
 * On write error or unexpected EOF, sets error code 500 and
 *   transitions to resheaderProcessing.
 */
t_msg_from_serv Server::reqBodyProcessingOutHandler(int fd, t_conn *conn)
{
    // Should be the inner fd for writing request body to the file
    if (fd != conn->inner_fd_in || !conn->is_cgi)
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

    ssize_t bytes_write = conn->read_buf->writeSocket(fd);

    LOG_INFO("Data written to internal fd for fd: ", fd, " bytes: ", bytes_write);

    // Handle write errors and special conditions
    if (bytes_write == RW_ERROR || bytes_write == EOF_REACHED)
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

/**
 * @details
 * Prepares the response headers based on the request processing outcome.
 * Sets the connection status to `RESPONSE`.
 * Initializes the output length based on the request method and response size.
 * For CGI, sets the inner fd for reading
 */
t_msg_from_serv Server::resheaderProcessingHandler(t_conn *conn)
{
    LOG_TRACE("Response Header Processing: ", "Starting...");
    conn->status = RES_HEADER_PROCESSING;

    size_t size_error_page = 0;

    if (conn->error_code != ERR_NO_ERROR && conn->error_code != ERR_301_REDIRECT)
    {
        try
        {
            t_file err_page = ErrorResponse(epoll_).getErrorPage(configs_[conn->config_idx].err_pages, conn->error_code);
            inner_fd_map_.emplace(err_page.FD_handler_OUT.get()->get(), err_page.FD_handler_OUT);
            conn->inner_fd_out = err_page.FD_handler_OUT.get()->get();
            size_error_page += err_page.fileSize;
        }
        catch (const WebServErr::ErrorResponseException &)
        {}
    }

    LOG_INFO("the size_error_page", size_error_page, "inner_fd_out", conn->inner_fd_out);

    const std::string header = (conn->error_code == ERR_NO_ERROR)
                                   ? conn->response->successResponse(conn, cookies_[conn->config_idx])
                                   : conn->response->failedResponse(conn, conn->error_code, conn->error_message, size_error_page, cookies_[conn->config_idx]);

    

    LOG_INFO("Response header prepared for fd: ", conn->socket_fd, "\n", header);

    conn->status = RESPONSE;
    conn->bytes_sent = 0;

    conn->write_buf->insertHeader(header);

    LOG_INFO("Response header size for fd: ", conn->socket_fd, " size: ", header.size());
    LOG_INFO("Header content:\n", header);

    if (conn->error_code != ERR_NO_ERROR)
    {
        LOG_ERROR("Error occurred, preparing error response: ", conn->error_code);
        conn->output_length = header.size() + size_error_page;
        return defaultMsg();
    }
    t_method method = convertMethod(conn->request->getrequestLineMap().at("Method"));

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
        conn->output_length = configs_[conn->config_idx].max_request_size; // Unknown length, send until EOF
        break;
    default:
        throw WebServErr::ShouldNotBeHereException("Unhandled method in response header processing");
    }

    // Register the inner fd for writing response body if CGI method
    if (method == CGI)
        conn->inner_fd_out = conn->inner_fd_in;
    LOG_INFO(" REMOVE DEBUG MK TEST: ", conn->output_length, "\n", header);
    return defaultMsg();
}

/**
 * @details
 * Reads data from the pipe into the write buffer.
 * If the data is chunked, updates the `output_length` when eof is reached,
 * and transitions to the `response` state.
 * If the entire response has been read, closes the pipe.
 * If buffer is full, waits for the next read event.
 * If an error occurs during reading, transitions to the terminated state.
 * Errors includes: size exceeds, read error.
 */
t_msg_from_serv Server::responseInHandler(int fd, t_conn *conn)
{
    if (fd != conn->inner_fd_out || !conn->is_cgi)
    {
        LOG_ERROR("Inner fd mismatch for fd: ", fd);
        throw WebServErr::ShouldNotBeHereException("Inner fd mismatch");
    }

    conn->last_heartbeat = time(NULL);

    if (conn->write_buf->isFull())
        return defaultMsg();

    ssize_t bytes_read = conn->write_buf->readFd(fd);

    LOG_INFO("Data read from internal fd for fd: ", fd, " bytes: ", bytes_read);

    if (bytes_read == RW_ERROR)
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
        return defaultMsg();

    if (conn->status == RES_HEADER_PROCESSING)
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

/**
 * @details
 * Reads required data from file server for GET / Static error page.
 * Sends data from the write buffer to the client socket.
 * If the entire response has been sent, transitions to the done state.
 * If buffer is empty, waits for the next write event.
 * If an error occurs during reading or sending, transitions to the terminated state.
 * Errors includes: size exceeds, write error.
 */
t_msg_from_serv Server::responseOutHandler(int fd, t_conn *conn)
{
    // Should be the socket fd
    if (fd != conn->socket_fd)
    {
        LOG_ERROR("Socket fd mismatch for fd: ", fd);
        return defaultMsg();
    }

    conn->last_heartbeat = time(NULL);

    if (!conn->is_cgi && conn->inner_fd_out != -1)
    {
        ssize_t read_bytes = conn->write_buf->readFd(conn->inner_fd_out);
        if (read_bytes == RW_ERROR)
        {
            LOG_ERROR("Error reading from internal fd for fd: ", fd);
            return terminatedHandler(fd, conn);
        }
    }

    // Skip when buffer is empty
    if (conn->write_buf->isEmpty())
        return defaultMsg();

    // Write data to socket
    ssize_t bytes_written = conn->write_buf->writeSocket(fd);
    if (bytes_written == RW_ERROR || bytes_written == EOF_REACHED)
    {
        LOG_ERROR("Error writing to socket for fd: ", fd);
        return terminatedHandler(fd, conn);
    }

    conn->bytes_sent += bytes_written;
    LOG_INFO("Data written to fd: ", fd, " bytes: ", bytes_written, " total: ", conn->bytes_sent, " / ", conn->output_length);

    // Check if done
    if (conn->bytes_sent == conn->output_length)
    {
        // If done, buffer should be empty
        if (conn->write_buf->isEmpty())
        {
            if (!conn->is_cgi && conn->inner_fd_out != -1)
            {
                inner_fd_map_.erase(conn->inner_fd_out);
                conn->inner_fd_out = -1;
            }
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

/**
 * @details
 * Closes and removes any internal fds.
 * Resets the connection to initial state for keep-alive.
 * Otherwise, closes the connection.
 */
t_msg_from_serv Server::doneHandler(int fd, t_conn *conn)
{
    conn->status = DONE;
    const bool keep_alive = !conn->request->getrequestHeaderMap().contains("connection") || conn->request->getrequestHeaderMap().at("connection") != "close";
    LOG_INFO("Request fully processed for fd: ", fd, " keep-alive: ", keep_alive);

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
    size_t config_idx = conn->config_idx;
    resetConn(conn, conn->socket_fd, configs_[conn->config_idx].max_request_size);
    conn->config_idx = config_idx;
    LOG_INFO("Keep-alive: ready for next request on fd: ", fd, "config_idx", config_idx);
    return msg;
}

/**
 * @details
 * Removes the conn from all the maps, and lists, fds are closed by RaiiFd.
 * Be called in scheduler when event_type is ERROR_EVENT, or when a connection should be terminated in FSM.
 * Will not send any response to the client.
 */
t_msg_from_serv Server::terminatedHandler(int fd, t_conn *conn)
{
    (void)fd; // Unused parameter
    conn->status = TERMINATED;

    int sock_fd = conn->socket_fd;
    LOG_INFO("Connection terminated: ", sock_fd);
    t_msg_from_serv msg = closeConn(conn);
    conns_.remove_if([sock_fd](const t_conn &c)
                     { return c.socket_fd == sock_fd; });
    return msg;
}

//
// Scheduler
//

t_msg_from_serv Server::scheduler(int fd, t_event_type event_type)
{
    if (!conn_map_.contains(fd))
        return defaultMsg();

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
                // LOG_WARN("Invalid fd for REQ_BODY_PROCESSING WRITE_EVENT for fd: ", fd);
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
                return defaultMsg();
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
