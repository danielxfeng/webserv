#include "Buffer.hpp"

Buffer::Buffer(size_t capacity, size_t block_size) : data_(), ref_(), data_view_(), capacity_(capacity), write_pos_(0), size_(0), block_size_(block_size), chunked_header_start_pos_(0), remain_chunk_size_(0), is_chunked_(false) {}

ssize_t Buffer::returnHelperForProcessChunkedData(std::vector<std::string_view> &new_chunks)
{
    size_t total = 0;
    for (size_t j = 0; j < new_chunks.size(); ++j)
    {
        data_view_.push_back(new_chunks[j]);
        ref_.back() += 1;
        total += new_chunks[j].size();
    }

    return total;
}

/**
 * @brief Processes chunked data from the given string view, extracting complete chunks.
 * @details
 * We only handle the chunked data that starts from the chunked header.
 */
ssize_t Buffer::processChunkedData(std::string_view &data)
{
    size_t i = 0;
    std::vector<std::string_view> new_chunks;

    while (i < data.size())
    {
        auto pos = data.find("\r\n", i);
        if (pos == std::string_view::npos)
        {
            chunked_header_start_pos_ = write_pos_ + i;
            return returnHelperForProcessChunkedData(new_chunks);
        }

        std::string_view header = data.substr(i, pos - i);
        size_t chunk_size = 0;
        try
        {
            chunk_size = std::stoul(std::string(header), nullptr, 16);
            if (chunk_size == 0)
            {
                // Last chunk, process remaining data
                is_eof_ = true;
                return returnHelperForProcessChunkedData(new_chunks);
            }
        }
        catch (const std::exception &e)
        {
            return CHUNKED_ERR; // Invalid chunk size
        }

        i = pos + CRLF; // Move past the header and CRLF

        if (i + chunk_size + CRLF < data.size())
        {
            std::string_view chunk = data.substr(i, chunk_size);
            new_chunks.push_back(chunk);
            i += chunk_size + CRLF;
            remain_chunk_size_ = 0;
            chunked_header_start_pos_ = 0;
            continue;
        }
        else if (i + chunk_size + CRLF >= data.size())
        {
            size_t available = data.size() - i;
            if (available >= chunk_size)
            {
                available = chunk_size - 1; // Leave space for CRLF
                chunked_body_start_pos_ = write_pos_ + i + available;
            }

            std::string_view chunk = data.substr(i, available);
            new_chunks.push_back(chunk);
            remain_chunk_size_ = chunk_size - available;
            chunked_header_start_pos_ = 0;
            return returnHelperForProcessChunkedData(new_chunks);
        }
    }
}

/**
 * @brief Reads data from the file descriptor into the buffer in chunked mode.
 * @details
 * 1 Ensures there is enough space for chunk header and trailing CRLF,
 * if not, allocates a new block and copies any remaining header part.
 * 2 Reads data into the last block.
 * 3 If there is any remaining chunk body to be read (`remain_chunk_size_ > 0`),
 * reads them, and sends to `data_view_`.
 * 4 Call helper functions to parse rest of the data (start from chunked header).
 */
ssize_t Buffer::readFdChunked(int fd)
{
    bool new_block = false;

    // Ensure enough space for chunk header and trailing CRLF
    const size_t remain_space = block_size_ - write_pos_;
    if (data_.empty() || remain_space < MAX_CHUNK_HEADER_SPACE                                   // Ensure enough space for chunk header
        || ((remain_chunk_size_ <= remain_space) && (remain_space < remain_chunk_size_ + CRLF))) // Ensure the trailing CRLF will not be split
    {
        std::string_view remain_header;

        if (write_pos_ > chunked_header_start_pos_)
        {
            auto &curr = data_.back();
            remain_header = std::string_view(curr.data() + chunked_header_start_pos_, write_pos_ - chunked_header_start_pos_);
        }

        data_.push_back(std::string(block_size_, '\0'));
        ref_.push_back(0);
        auto &new_buf = data_.back();

        if (!remain_header.empty())
        {
            std::copy(remain_header.begin(), remain_header.end(), new_buf.begin());
            chunked_header_start_pos_ = 0;
        }

        write_pos_ = remain_header.size();
        new_block = true;
    }

    // Now read into the last block
    std::string &buf = data_.back();
    size_t read_size = std::min(block_size_ - write_pos_, capacity_ - size_);
    ssize_t read_bytes = read(fd, &(buf.data()[write_pos_]), read_size);

    if (read_bytes < 0)
        return BUFFER_ERROR;

    if (read_bytes == 0)
        return EOF_REACHED;

    size_t new_write_size = 0;

    // Parse the remaining chunk body if any
    if (remain_chunk_size_ > 0)
    {
        const size_t left_body = chunked_body_start_pos_ > 0 ? write_pos_ - chunked_body_start_pos_
                                                             : write_pos_; // In case the chunk body is started in this block
        const size_t start_pos = write_pos_ - left_body;
        // In case the chunk body is ended by the read
        if (read_bytes + left_body >= remain_chunk_size_ + CRLF)
        {
            std::string_view new_data(buf.data() + start_pos, remain_chunk_size_);

            const char *crlf = buf.data() + start_pos + remain_chunk_size_;
            if (!(crlf[0] == '\r' && crlf[1] == '\n'))
                return CHUNKED_ERR; // Invalid chunk ending

            data_view_.push_back(new_data);
            ref_.back() += 1;

            write_pos_ += remain_chunk_size_ + CRLF - left_body;
            new_write_size += remain_chunk_size_;
            remain_chunk_size_ = 0;
            chunked_body_start_pos_ = 0;
        }
        else // In case we have only part of the chunked body
        {
            std::string_view new_data(buf.data() + write_pos_);
            data_view_.push_back(new_data);
            ref_.back() += 1;
            write_pos_ += read_bytes;
            remain_chunk_size_ -= read_bytes;
            return read_bytes;
        }
    }

    // Parse rest of the data (new chunk headers and bodies)
    std::string_view chunked_data(buf.data() + write_pos_, read_bytes - new_write_size - CRLF);
    ssize_t parsed = processChunkedData(chunked_data);
    if (parsed == CHUNKED_ERR)
        return CHUNKED_ERR;

    return new_write_size + parsed;
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
        return BUFFER_ERROR;

    if (read_bytes == 0)
        return EOF_REACHED;

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

ssize_t Buffer::writeFd(int fd)
{
    if (isEmpty())
        return BUFFER_EMPTY;

    auto &block = data_view_.front();
    ssize_t write_bytes = write(fd, block.data(), block.size());
    if (write_bytes < 0)
        return BUFFER_ERROR;
    if (write_bytes == 0)
        return EOF_REACHED;

    if (write_bytes < block.size())
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

bool Buffer::isFull() const
{
    return size_ >= capacity_;
}

bool Buffer::isEmpty() const
{
    return size_ == 0;
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

    auto front = data_view_.front();
    front.remove_prefix(size);
    if (front.empty())
    {
        data_.pop_front();
        data_view_.pop_front();
    }
    else
        data_view_.front() = front;
    size_ -= size;

    // Also process the rest body if in chunked mode
    if (is_chunked_ && !data_view_.empty())
    {
        ssize_t chunk_size = processChunkedData(data_view_.front());
        if (chunk_size == CHUNKED_ERR)
            throw WebServErr::ShouldNotBeHereException("Buffer::removeHeader: chunked parse error");
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
    size_ += str.size();
    return true;
}
