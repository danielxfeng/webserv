#include "Buffer.hpp"

Buffer::Buffer(size_t capacity, size_t block_size) : data_(), ref_(), data_view_(), capacity_(capacity), write_pos_(0), size_(0), block_size_(block_size), chunked_header_start_pos_(0), chunked_body_start_pos_(0), remain_chunk_size_(0), is_chunked_(false), is_eof_(false) {}

ssize_t Buffer::handleNextHeader(std::string_view data, ssize_t parsed, size_t skipped)
{
    size_t pos = data.find("\r\n");
    if (pos == std::string_view::npos)
    {
        chunked_header_start_pos_ = write_pos_ + parsed + skipped;
        return parsed;
    }

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
        if (chunk_size == 0)
        {
            std::string_view final_crlf = rest.substr(0, CRLF);
            if (final_crlf != "\r\n")
                return handleChunkedError();
            rest.remove_prefix(CRLF);
            skipped += CRLF;

            if (!rest.empty())
                return handleChunkedError();
            return handleChunkedEOF(parsed + pos);
        }

        chunked_body_start_pos_ = 0;
        remain_chunk_size_ = chunk_size;
        return handleBodyProcessing(rest, parsed, skipped);
    }
    catch (const std::exception &e)
    {
        return handleChunkedError();
    }
}

ssize_t Buffer::handleBodyProcessing(std::string_view data, ssize_t parsed, size_t skipped)
{
    ssize_t to_read = std::min(remain_chunk_size_ + CRLF, data.size());
    if (to_read >= remain_chunk_size_ && to_read < remain_chunk_size_ + CRLF)
    {
        to_read = remain_chunk_size_ - 1;
        chunked_body_start_pos_ = write_pos_ + parsed + to_read + skipped;
    }

    if (to_read == 0)
        return parsed;

    std::string_view body = data.substr(0, to_read);
    std::string_view rest = data.substr(to_read);

    data_view_.push_back(body);
    ref_.back() += 1;
    parsed += to_read;
    remain_chunk_size_ -= to_read;

    if (chunked_body_start_pos_ > 0 || remain_chunk_size_ > 0)
        return parsed;

    std::string_view crlf = rest.substr(0, CRLF);
    if (crlf != "\r\n")
        return handleChunkedError();
    rest.remove_prefix(CRLF);
    skipped += CRLF;

    if (rest.empty())
        return parsed;

    return handleNextHeader(rest, parsed, skipped);
}

ssize_t Buffer::handleChunkedEOF(ssize_t parsed)
{
    chunked_header_start_pos_ = 0;
    chunked_body_start_pos_ = 0;
    remain_chunk_size_ = 0;
    is_eof_ = true;
    return parsed;
}

ssize_t Buffer::handleChunkedError()
{
    chunked_header_start_pos_ = 0;
    chunked_body_start_pos_ = 0;
    remain_chunk_size_ = 0;
    return CHUNKED_ERR;
}

ssize_t Buffer::fsmSchduler(t_chunked_status status, std::string_view data)
{
    switch (status)
    {
    case NEXT_HEADER:
        return handleNextHeader(data, 0, 0);
    case BODY_PROCESSING:
        return handleBodyProcessing(data, 0, 0);
    default:
        throw WebServErr::ShouldNotBeHereException("Buffer::fsmSchduler: invalid state");
    }
}

/**
 * @brief Reads data from the file descriptor into the buffer in chunked mode.
 * @details
 * 1 Ensures there is enough space for chunk header and trailing CRLF,
 * if not, allocates a new block and copies any remaining header part.
 * 2 Reads data into the last block.
 * 3 Call fsmSchduler to parse the chunked data.
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

        if (chunked_header_start_pos_ > 0 && write_pos_ > chunked_header_start_pos_)
        {
            auto &curr = data_.back();
            remain_header = std::string_view(curr.data() + chunked_header_start_pos_, write_pos_ - chunked_header_start_pos_);
        }
        else if (chunked_body_start_pos_ > 0 && write_pos_ > chunked_body_start_pos_)
        {
            auto &curr = data_.back();
            remain_body = std::string_view(curr.data() + chunked_body_start_pos_, write_pos_ - chunked_body_start_pos_);
        }

        data_.push_back(std::string(block_size_, '\0'));
        ref_.push_back(0);
        auto &new_buf = data_.back();

        if (!remain_header.empty())
        {
            std::copy(remain_header.begin(), remain_header.end(), new_buf.begin());
            chunked_header_start_pos_ = 0;
        }
        else if (!remain_body.empty())
        {
            std::copy(remain_body.begin(), remain_body.end(), new_buf.begin());
            chunked_body_start_pos_ = 0;
        }
        else
        {
            chunked_header_start_pos_ = 0;
            chunked_body_start_pos_ = 0;
        }

        write_pos_ = remain_header.size() + remain_body.size();
    }

    // Now read into the last block
    std::string &buf = data_.back();
    size_t read_size = std::min(block_size_ - write_pos_, capacity_ - size_);
    ssize_t read_bytes = read(fd, &(buf.data()[write_pos_]), read_size);

    if (read_bytes < 0)
        return BUFFER_ERROR;

    if (read_bytes == 0)
        return EOF_REACHED;

    // Parse rest of the data (new chunk headers and bodies)
    std::string_view chunked_data(buf.data() + write_pos_, read_bytes);

    t_chunked_status status;

    if (remain_chunk_size_ > 0)
        status = BODY_PROCESSING;
    else
        status = NEXT_HEADER;

    ssize_t parsed = fsmSchduler(status, chunked_data);
    if (parsed == CHUNKED_ERR)
        return CHUNKED_ERR;

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
        ref_.pop_front();
    }
    else
        data_view_.front() = front;
    size_ -= size;

    // Also process the rest body if in chunked mode
    if (is_chunked_ && !data_view_.empty())
    {
        std::string_view rest = data_view_.front();
        data_view_.pop_front();
        ref_.front() = 0;
        ssize_t chunk_size = handleNextHeader(rest, 0, 0);
        if (chunk_size == CHUNKED_ERR)
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
    size_ += str.size();
    return true;
}
