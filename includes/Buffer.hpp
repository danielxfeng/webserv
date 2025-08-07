#pragma once

#include <vector>
#include <queue>
#include <unistd.h>
#include "WebServErr.hpp"

class Buffer
{
    private:
        std::queue<std::vector<char>> data_;
        size_t readPos_;
        size_t writePos_;
        size_t capacity_;
        size_t size_;
        size_t block_size_;

    public:
        Buffer() = default;
        Buffer(const Buffer&) = default;
        Buffer& operator=(const Buffer&) = default;
        ~Buffer() = default;

        Buffer(size_t capacity, size_t block_size);

        void readFd(int fd);
        void writeFd(int fd);
        bool isFull() const;
};