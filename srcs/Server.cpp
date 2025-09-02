#include "../includes/Server.hpp"

Server::Server(const t_server_config &config) : config_(config) {}

const t_server_config &Server::getConfig() const { return config_; }

void Server::addConn(int fd)
{
    auto now = time(NULL);

    t_conn conn = {fd, -1, -1, HEADER_PARSING, now, now, config_.max_request_size, 0, std::make_unique<Buffer>(1024, 256), std::make_unique<Buffer>(1024, 256), std::make_shared<HttpRequests>(), std::make_shared<HttpResponse>()};

    conn_vec_.push_back(std::move(conn));
    conn_map_.emplace(fd, &conn_vec_.back());
    LOG_DEBUG("New connection added: ", fd);
}

t_msg_from_serv Server::handleDataIn(int fd)
{
    if (!conn_map_.contains(fd))
        throw WebServErr::ShouldNotBeHereException("Connection not found");

    t_conn *conn = conn_map_.at(fd);
    if (fd == conn->socket_fd)
    {
        LOG_DEBUG("Data read: ", fd);
        ssize_t bytes_read = static_cast<ssize_t>(conn->read_buf->readFd(fd));

        if (bytes_read == SRV_ERROR || bytes_read == BUFFER_FULL)
            return {false, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // Just skip for now.

        conn->bytes_received += bytes_read;

        if (conn->status == HEADER_PARSING)
        {
            try
            {
                conn->request->httpParser(conn->read_buf->getData().front());
                if (conn->request->getrequestLineMap().contains("Content-Length"))
                    conn->content_length = static_cast<size_t>(stoull(conn->request->getrequestLineMap().at("Content-Length")));

                conn->status = READING;
                LOG_DEBUG("Header parsed successfully for fd: ", fd);
                try
                {
                    t_file output = MethodHandler().handleRequest(config_, conn->request->getrequestLineMap(), conn->request->getrequestBodyMap());
                    const auto response_header = conn->response->successResponse(*conn, output.fileSize);
                    conn->write_buf->getData().push(response_header);
                    conn->status = WRITING;
                    auto fd_to_register = std::vector<std::pair<int, t_direction>>{{fd, OUT}};
                    if (output.isDynamic)
                    {
                        conn->write_buf->getData().push(output.dynamicPage);
                        return {true, fd_to_register, std::vector<std::pair<int, t_direction>>{}};
                    }
                    conn->inner_fd_in = output.fd;
                    fd_to_register.push_back(std::make_pair(conn->inner_fd_in, IN));
                    return {false, fd_to_register, std::vector<std::pair<int, t_direction>>{}};
                }
                catch (const WebServErr::MethodException &e)
                {
                    LOG_ERROR("Method exception occurred: ", e.what());
                    return handleError(conn, e.what());
                }
            }
            catch (const WebServErr::InvalidRequestHeader &e)
            {
                LOG_ERROR("Invalid request header for fd: ", fd, e.what());
            } // Ignore the error since the header might be incomplete.
            catch (const WebServErr::BadRequestException &e)
            {
                LOG_ERROR("Bad request for fd: ", fd);
                return handleError(conn, e.what());
            }
        }

        const bool is_max_length_reached = (conn->bytes_received == conn->content_length);

        if (bytes_read == EOF_REACHED && !is_max_length_reached)
        {
            LOG_ERROR("EOF reached but content length not reached", " weird eh?");
            return handleError(conn, "EOF reached but content length not reached");
        }

        if (conn->bytes_received > config_.max_request_size)
        {
            LOG_ERROR("Request size exceeded: ", conn->bytes_received);
            return handleError(conn, "Request size exceeded");
        }

        switch (conn->status)
        {
        case HEADER_PARSING:
            if (conn->bytes_received >= config_.max_headers_size)
                return handleError(conn, "Header size exceeded");
            if (is_max_length_reached)
                return handleError(conn, "Header not found");
            break;
        case READING:
            // TODO: Construct full path fron Config + Path
            // TODO: Handle the methods.
            // TODO: Generate Header
            break;
        default:
            return {false, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // Just skip for now.
        }

        if (is_max_length_reached)
        {
            conn->status = WRITING;
            const bool keep_alive = !conn->request->getrequestHeaderMap().contains("Connection") || conn->request->getrequestHeaderMap().at("Connection") == "keep-alive";
            (void) keep_alive;
            return {false, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // todo
        }
    }
    else
    {
        if (fd != conn->inner_fd_in)
            return handleError(conn, "Inner fd mismatch");

        if (conn->status != WRITING)
            return handleError(conn, "Connection not in writing state");

        ssize_t bytes_read = conn->read_buf->readFd(fd);
        if (bytes_read == SRV_ERROR)
            return handleError(conn, "Read error.");

        if (bytes_read == BUFFER_FULL)
            return {false, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // Buffer is full, skip

        if (bytes_read == EOF_REACHED)
            return {true, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // todo

        return handleError(conn, "Should not reach here");
    }

    LOG_DEBUG("Data read: ", fd);
    throw WebServErr::ShouldNotBeHereException("Should not reach here, here"); // TODO: remove this after implementation
}

t_msg_from_serv Server::handleDataOut(int fd)
{
    (void)fd;
    return {false, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // TODO: Implement this.
}

t_msg_from_serv Server::handleError(t_conn *conn, const std::string &error_message)
{
    (void)error_message;
    (void)conn;
    return {true, std::vector<std::pair<int, t_direction>>{}, std::vector<std::pair<int, t_direction>>{}}; // TODO: Handle the error based on the error_code.
}

void Server::timeoutKiller()
{
    auto now = time(NULL);
    for (auto it = conn_vec_.begin(); it != conn_vec_.end();)
    {
        // TODO: Replace the hardcoded value with actual configuration value.
        if (difftime(now, it->last_heartbeat) > 99999 || difftime(now, it->start_timestamp) > 99999)
        {
            it = conn_vec_.erase(it);
            // handleDataEnd(it->socket_fd); // TODO: need a way to unregister the fd from epoll.
        }
        else
        {
            ++it;
        }
    }
}
