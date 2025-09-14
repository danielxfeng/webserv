#include "Buffer.hpp"

Buffer::Buffer(size_t capacity, size_t block_size) : data_(), ref_(), data_view_(), capacity_(capacity), write_pos_(0), size_(0), block_size_(block_size), remain_header_size_(0), remain_body_size_(0), remain_chunk_size_(0), is_chunked_(false), is_eof_(false) {}

/**
 * @details
 * This function handles the chunk header parsing.
 * The given `data` should contain the full chunk header,
 * and may also contain the chunk body, or next chunks.
 *
 * If the chunk size is zero, it means the last chunk has been reached.
 */
ssize_t Buffer::handleNextHeader(std::string_view data, ssize_t parsed, size_t skipped)
{
    size_t pos = data.find("\r\n");
    if (pos == std::string_view::npos)
    {
        remain_header_size_ = data.size();
        return parsed;
    }

    remain_header_size_ = 0;
    std::string_view header = data.substr(0, pos);
    std::string_view crlf = data.substr(pos, CRLF);
    if (crlf != "\r\n")
        return handleChunkedError();
    std::string_view rest = data.substr(pos + CRLF);
    skipped += CRLF;

    size_t chunk_size = 0;
    try
    {
        chunk_size = std::stoul(std::string(header), nullptr, 16);
        // Handle the zero-size chunk (last chunk)
        if (chunk_size == 0)
        {
            std::string_view final_crlf = rest.substr(0, CRLF);
            if (final_crlf != "\r\n")
                return handleChunkedError();
            rest.remove_prefix(CRLF);

            // Headers are not included in size_.
            skipped += chunk_size + CRLF;

            // Should be no more data after the last chunk
            if (!rest.empty())
                return handleChunkedError();

            // Mark EOF and return
            return handleChunkedEOF(parsed);
        }

        remain_body_size_ = 0;
        remain_chunk_size_ = chunk_size;
        return handleBodyProcessing(rest, parsed, skipped);
    }
    catch (const std::exception &e)
    {
        return handleChunkedError();
    }
}

/**
 * @details
 * This function handles the chunk body processing.
 * The given `data` should contain part or all of the chunk body,
 * or the next chunks. But it should contain the full CRLF if the chunk body is complete.
 *
 * If the chunk body is not fully available, it will read as much as possible,
 * and update the remain_chunk_size_ accordingly.
 * If the chunk body is fully available, it will check for the trailing CRLF,
 * and then call handleNextHeader to process the next chunk header.
 */
ssize_t Buffer::handleBodyProcessing(std::string_view data, ssize_t parsed, size_t skipped)
{
    ssize_t to_read = std::min(remain_chunk_size_ + CRLF, data.size());

    // We need to be sure CRLF is not splitted, so at least 1 byte of body stays with CRLF.
    if (to_read >= static_cast<ssize_t>(remain_chunk_size_))
    {
        if (to_read < static_cast<ssize_t>(remain_chunk_size_ + CRLF))
        {
            // Not enough to keep body and full CRLF together: leave 1 byte to join with CRLF next time
            to_read = remain_chunk_size_ - 1;
            remain_body_size_ = 1;
        }
        else
        {
            // We have full body + CRLF available: read body only here
            to_read = remain_chunk_size_;
        }
    }

    if (to_read == 0)
        return parsed;

    std::string_view body = data.substr(0, to_read);
    std::string_view rest = data.substr(to_read);

    data_view_.push_back(body);
    ref_.back() += 1;
    parsed += to_read;
    remain_chunk_size_ -= to_read;

    // There are more data to read, so we wait for next call.
    if (remain_chunk_size_ > 0)
        return parsed;

    // We have read the whole chunk, now we need to check the trailing CRLF.
    std::string_view crlf = rest.substr(0, CRLF);
    if (crlf != "\r\n")
        return handleChunkedError();
    rest.remove_prefix(CRLF);
    skipped += CRLF;

    // If there is no more data, we wait for next call.
    if (rest.empty())
        return parsed;

    // Otherwise, we start to parse the next chunk header.
    return handleNextHeader(rest, parsed, skipped);
}

ssize_t Buffer::handleChunkedEOF(ssize_t parsed)
{
    remain_header_size_ = 0;
    remain_body_size_ = 0;
    is_eof_ = true;
    return parsed;
}

ssize_t Buffer::handleChunkedError()
{
    remain_header_size_ = 0;
    remain_body_size_ = 0;
    return CHUNKED_ERROR;
}

ssize_t Buffer::fsmScheduler(t_chunked_status status, std::string_view data)
{
    switch (status)
    {
    case NEXT_HEADER:
        return handleNextHeader(data, 0, 0);
    case BODY_PROCESSING:
        return handleBodyProcessing(data, 0, 0);
    default:
        throw WebServErr::ShouldNotBeHereException("Buffer::fsmScheduler: invalid state");
    }
}

/**
 * @details
 * This function handles the epollin event on the socket.
 * The most important responsibility of this function is to ensure
 * that no incomplete-chunk-header/chunk-body-awaiting-its-trailing-delimiter
 * is sent to the FSM scheduler.
 * So if the buffer block may not have enough space, we create a new one here,
 * and copy any remaining partial header/body to the string_view, then send to FSM.
 */
ssize_t Buffer::readFdChunked(int fd)
{
    // Ensure enough space for chunk header and trailing CRLF
    const size_t remain_space = block_size_ - write_pos_;
    if (data_.empty() || remain_space < MAX_CHUNK_HEADER_SPACE                                   // Ensure enough space for chunk header
        || ((remain_chunk_size_ <= remain_space) && (remain_space < remain_chunk_size_ + CRLF))) // Ensure the trailing CRLF will not be split
    {
        std::string_view remain_header;
        std::string_view remain_body;

        // If we need to create a new block, we need to copy any remaining header/body part to the new block.
        if (remain_header_size_ > 0)
        {
            auto &curr = data_.back();
            remain_header = std::string_view(curr.data() + write_pos_ - remain_header_size_, remain_header_size_);
        }
        else if (remain_body_size_ > 0)
        {
            auto &curr = data_.back();
            remain_body = std::string_view(curr.data() + write_pos_ - remain_body_size_, remain_body_size_);
        }

        data_.push_back(std::string(block_size_, '\0'));
        ref_.push_back(0);
        auto &new_buf = data_.back();

        if (!remain_header.empty())
        {
            std::copy(remain_header.begin(), remain_header.end(), new_buf.begin());
        }
        else if (!remain_body.empty())
        {
            std::copy(remain_body.begin(), remain_body.end(), new_buf.begin());
        }

        write_pos_ = remain_body_size_ + remain_header_size_;
    }

    // Now read into the last block
    std::string &buf = data_.back();
    size_t read_size = std::min(block_size_ - write_pos_, capacity_ - size_);
    ssize_t read_bytes = read(fd, &(buf.data()[write_pos_]), read_size);

    if (read_bytes < 0)
        return RW_ERROR;

    if (read_bytes == 0)
    {
        is_eof_ = true;
        return EOF_REACHED;
    }

    // Parse read data, + any unprocessed partial header/body.
    size_t prefix_offset = remain_body_size_ + remain_header_size_;
    std::string_view chunked_data(buf.data() + write_pos_ - prefix_offset, read_bytes + prefix_offset);

    // Reset the remain sizes, since we have updated the view to include them.
    remain_body_size_ = 0;
    remain_header_size_ = 0;

    t_chunked_status status;

    // The chunked_data view always starts from the last unparsed byte
    // (including any partial header/body), so FSM never sees a truncated token.
    if (remain_chunk_size_ > 0)
        status = BODY_PROCESSING;
    else
        status = NEXT_HEADER;

    ssize_t parsed = fsmScheduler(status, chunked_data);
    if (parsed == CHUNKED_ERROR)
        return CHUNKED_ERROR;

    write_pos_ += read_bytes;
    size_ += parsed;

    return parsed;
}

ssize_t Buffer::readFd(int fd)
{
    if (isFull())
        return BUFFER_FULL;

    if (is_chunked_)
        return readFdChunked(fd);

    bool new_block = false;

    // Allocate a new block if needed
    if (data_.empty() || write_pos_ == block_size_)
    {
        data_.push_back(std::string(block_size_, '\0'));
        write_pos_ = 0;
        new_block = true;
    }

    // Now read into the last block
    std::string &buf = data_.back();

    size_t read_size = std::min(block_size_ - write_pos_, capacity_ - size_);
    ssize_t read_bytes = read(fd, &(buf.data()[write_pos_]), read_size);

    if (read_bytes < 0)
        return RW_ERROR;

    if (read_bytes == 0)
    {
        is_eof_ = true;
        return EOF_REACHED;
    }

    // Update buffer state
    write_pos_ += read_bytes;
    size_ += read_bytes;

    if (new_block)
    {
        data_view_.push_back(std::string_view(buf.data(), write_pos_));
        ref_.push_back(1);
    }
    else
        data_view_.back() = std::string_view(buf.data(), write_pos_);

    return read_bytes;
}

ssize_t Buffer::writeSocket(int fd)
{
    if (isEmpty())
        return BUFFER_EMPTY;

    auto &block = data_view_.front();
    ssize_t write_bytes = write(fd, block.data(), block.size());
    if (write_bytes < 0)
        return RW_ERROR;
    if (write_bytes == 0)
    {
        is_eof_ = true;
        return EOF_REACHED;
    }

    if (write_bytes < static_cast<ssize_t>(block.size()))
        block.remove_prefix(write_bytes);
    else
    {
        data_view_.pop_front();
        auto &ref = ref_.front();
        if (ref == 1)
        {
            data_.pop_front();
            ref_.pop_front();
        }
        else
            --ref;
    }

    if (static_cast<size_t>(write_bytes) > size_)
        throw WebServErr::ShouldNotBeHereException("Buffer::writeFd: size underflow");

    size_ -= write_bytes;
    return write_bytes;
}

/**
 * @details
 * Since NON-BLOCKING does not apply to regular files,
 * we can simply drain all data in the buffer to the file.
 *
 * Extra copy here since we are not allowed to call `write` in a loop
 * in this project.
 */
ssize_t Buffer::writeFile(int fd)
{
    if (isEmpty())
        return BUFFER_EMPTY;

    ssize_t write_bytes = 0;

    if (data_view_.size() > 1)
    {
        std::string block;
        block.reserve(size_);
        while (!data_view_.empty())
        {
            block.append(data_view_.front());
            data_view_.pop_front();
        }

        write_bytes = write(fd, block.data(), block.size());
        if (write_bytes < 0 || write_bytes != static_cast<ssize_t>(block.size()))
            write_bytes = RW_ERROR;
    }
    else
    {
        write_bytes = write(fd, data_view_.front().data(), data_view_.front().size());
        if (write_bytes < 0 || write_bytes != static_cast<ssize_t>(data_view_.front().size()))
            write_bytes = RW_ERROR;
        data_view_.clear();
    }

    data_.clear();
    ref_.clear();
    size_ = 0;
    write_pos_ = 0;

    return write_bytes;
}

bool Buffer::isFull() const
{
    return size_ >= capacity_;
}

bool Buffer::isEmpty() const
{
    return size_ == 0;
}

bool Buffer::isEOF() const
{
    return is_eof_;
}

size_t Buffer::size() const
{
    return size_;
}

const std::string_view Buffer::peek() const
{
    if (data_view_.empty())
        return std::string_view();
    return data_view_.front();
}

bool Buffer::removeHeaderAndSetChunked(const std::size_t size, bool is_chunked)
{
    is_chunked_ = is_chunked;

    if (data_.size() != 1 || data_view_.size() != 1 || size > data_view_.front().size())
        throw WebServErr::ShouldNotBeHereException("Buffer::removeHeader: invalid state");

    // Remove the header
    auto front = data_view_.front();
    front.remove_prefix(size);
    if (front.empty())
    {
        data_.pop_front();
        data_view_.pop_front();
        ref_.pop_front();
    }
    else
        data_view_.front() = front;
    size_ -= size;

    // Also process the rest body if in chunked mode,
    // bc if the buffer contains all the chunked data, we have no chance to handle it again.
    if (is_chunked_ && !data_view_.empty())
    {
        std::string_view rest = data_view_.front();
        data_view_.pop_front();
        ref_.front() = 0;

        // The good thing is that we can be sure we can start from a new chunk header.
        ssize_t chunk_size = handleNextHeader(rest, 0, 0);
        if (chunk_size == CHUNKED_ERROR)
            return false;
        size_ = chunk_size;
    }

    return true;
}

bool Buffer::insertHeader(const std::string str)
{
    if (str.size() + size_ > capacity_)
        return false;

    data_.push_front(str);
    data_view_.push_front(std::string_view(data_.front()));
    ref_.push_front(1);
    size_ += str.size();
    return true;
}
