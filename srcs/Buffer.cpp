#include "Buffer.hpp"

Buffer::Buffer(size_t capacity, size_t block_size) : capacity_(capacity), block_size_(block_size) {}

void Buffer::readFd(int fd)
{
    if (isFull())
        return;

    std::vector<char> buf;
    buf.reserve(block_size_);

    size_t read_size = std::min(block_size_, capacity_ - size_);
    ssize_t read_bytes = read(fd, buf.data(), read_size);

    if (read_bytes <= 0) // TODO: if it's 0, what will happen?
        throw WebServErr::SysCallErrException("read failed");

    data_.push(std::move(buf));
    size_ += read_bytes;
}

void Buffer::writeFd(int fd)
{
    while (!data_.empty())
    {
        std::vector<char> block = data_.front();
        size_t write_bytes = write(fd, block.data()[writePos_], block.size() - writePos_);
        if (write_bytes <= 0)
            throw WebServErr::SysCallErrException("write failed");

        if (write_bytes < block.size())
            writePos_ += write_bytes;
        else
        {
            data_.pop();
            writePos_ = 0;
        }
    }
}

bool Buffer::isFull() const
{
    return size_ >= capacity_;
}
