#pragma once

#include <vector>
#include <queue>
#include <unistd.h>
#include "WebServErr.hpp"

class Buffer
{
private:
    std::queue<std::vector<char>> data_; // The data structure of the buffer. We split the buffer into fixed-size blocks, new block is appended when the prev one is full.
    size_t readPos_;                     // The position of the next read in the current block.
    size_t writePos_;                    // The position of the next write in the current block.
    size_t capacity_;                    // The maximum size of the buffer.
    size_t size_;                        // The current size of the buffer.
    size_t block_size_;                  // The size of each block in the buffer.

public:
    Buffer() = default;
    Buffer(const Buffer &) = default;
    Buffer &operator=(const Buffer &) = default;
    ~Buffer() = default;

    Buffer(size_t capacity, size_t block_size);

    /**
     * @brief Reads data from the file descriptor(a file/socket/pipe) into the buffer.
     * @return true if EOF is reached, false otherwise.
     */
    bool readFd(int fd);

    /**
     * @brief Writes data from the buffer to the file descriptor(a file/socket/pipe).
     */
    void writeFd(int fd);

    /**
     * @brief Checks if the buffer is full.
     */
    bool isFull() const;
};