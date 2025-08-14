#include "Server.hpp"

void Server::addConn(int fd)
{
    auto now = time(NULL);

    // TODO: replace the hardcoded values with actual configuration values.
    t_conn conn = {fd, -1, -1, "", UNKNOWN, HEADER_PARSING, now, now, 0, 999999999, Buffer(1024, 256), Buffer(1024, 256), std::unordered_map<std::string, std::string>()};

    conn_vec_.push_back(conn);
    conn_map_.emplace(fd, &conn_vec_.back());
}

t_msg_from_serv Server::handleDataIn(int fd)
{
    if (!conn_map_.contains(fd))
        throw WebServErr::ShouldNotBeHereException("Connection not found");

    t_conn *conn = conn_map_.at(fd);
    if (fd == conn->socket_fd)
    {

        size_t bytes_read = conn->read_buf.readFd(fd);

        if (bytes_read == ERROR || bytes_read == BUFFER_FULL)
            return {false, -1, IN, -1}; // Just skip for now.

        conn->bytes_received += bytes_read;

        if (conn->status == HEADER_PARSING)
        {
            // TODO: Call headerparser.
            // Assign the value to header.
        }

        const bool is_max_length_exceeded = (conn->headers.contains("Content-Length") && conn->bytes_received > std::stoul(conn->headers.at("Content-Length")));

        const bool is_eof = (bytes_read == EOF_REACHED || is_max_length_exceeded);

        switch (conn->status)
        {
        case HEADER_PARSING:
            if (conn->bytes_received >= max_header_size_)
                return handleError(conn, BAD_REQUEST, "Header size exceeded");
            if (is_eof)
                return handleError(conn, BAD_REQUEST, "Header not found");
            break;
        case READING:
			// TODO: Construct full path fron Config + Path
            // TODO: Handle the methods.
			// TODO: Generate Header
            break;
        default:
            return {false, -1, IN, -1}; // Just skip for now.
        }

        if (is_eof)
        {
            conn->status = WRITING;
            const bool keep_alive = conn->headers.contains("Connection") && conn->headers.at("Connection") == "keep-alive";
            return {true, -1, IN, keep_alive ? -1 : fd};
        }
    }
    else
    {
        if (fd != conn->inner_fd_in)
            return handleError(conn, INTERNAL_SERVER_ERROR, "Inner fd mismatch");

        if (conn->status != WRITING)
            return handleError(conn, INTERNAL_SERVER_ERROR, "Connection not in writing state");

        size_t bytes_read = conn->read_buf.readFd(fd);
        if (bytes_read == ERROR)
            return handleError(conn, INTERNAL_SERVER_ERROR, "Read error.");

        if (bytes_read == BUFFER_FULL)
            return {false, -1, IN, -1}; // Buffer is full, skip

        if (bytes_read == EOF_REACHED)
            return {true, -1, IN, fd};

        return handleError(conn, INTERNAL_SERVER_ERROR, "Should not reach here");
    }
}

t_msg_from_serv Server::handleDataOut(int fd) { /* TODO: implement */ }

t_msg_from_serv Server::handleError(t_conn *conn, t_error_code error_code, const std::string &error_message)
{
    // TODO: Handle the error based on the error_code.
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
            handleDataEnd(it->socket_fd); // TODO: need a way to unregister the fd from epoll.
        }
        else
        {
            ++it;
        }
    }
}
