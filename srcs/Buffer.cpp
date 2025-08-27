#include "Buffer.hpp"

Buffer::Buffer(size_t capacity, size_t block_size) : readPos_(0), writePos_(0), capacity_(capacity), size_(0), block_size_(block_size) {}

size_t Buffer::readFd(int fd)
{
    if (isFull())
        return BUFFER_FULL;

    if (data_.empty() || readPos_ == block_size_)
    {
        data_.push(std::string(block_size_, '\0'));
        readPos_ = 0;
    }

    std::string &buf = data_.back();

    size_t read_size = std::min(block_size_ - readPos_, capacity_ - size_);
    ssize_t read_bytes = read(fd, &(buf.data()[readPos_]), read_size);

    if (read_bytes < 0)
        return BUFFER_ERROR;

    if (read_bytes == 0)
        return EOF_REACHED;

    readPos_ += read_bytes;
    size_ += read_bytes;

    // TODO: Check EOF then return true;

    return read_bytes;
}

void Buffer::writeFd(int fd)
{

    if (data_.empty())
        return;

    std::string &block = data_.front();
    size_t write_bytes = write(fd, &(block.data()[writePos_]), block.size() - writePos_);
    if (write_bytes <= 0)
        return;

    if (write_bytes < block.size() - writePos_)
        writePos_ += write_bytes;
    else
    {
        data_.pop();
        writePos_ = 0;
    }

    size_ -= write_bytes;
}

bool Buffer::isFull() const
{
    return size_ >= capacity_;
}

std::queue<std::string> Buffer::getData() const
{
    return data_;
}