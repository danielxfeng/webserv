#pragma once

#include <vector>
#include <queue>
#include <unistd.h>
#include "WebServErr.hpp"

typedef enum e_buff_error_code
{
    EOF_REACHED = 0,
    BUFFER_ERROR = -1,
    BUFFER_FULL = -2
} t_buff_error_code;

class Buffer
{
private:
    std::queue<std::string> data_; // The data structure of the buffer. We split the buffer into fixed-size blocks, new block is appended when the prev one is full.
    size_t readPos_;               // The position of the next read in the current block.
    size_t writePos_;              // The position of the next write in the current block.
    size_t capacity_;              // The maximum size of the buffer.
    size_t size_;                  // The current size of the buffer.
    size_t block_size_;            // The size of each block in the buffer.

public:
    Buffer() = delete;
    Buffer(const Buffer &) = default;
    Buffer &operator=(const Buffer &) = default;
    ~Buffer() = default;

    Buffer(size_t capacity, size_t block_size);

    /**
     * @brief Reads data from the file descriptor(a file/socket/pipe) into the buffer.
     * @return The number of bytes read, 0 if EOF is reached, -1 if an error occurs, -2 if the buffer is full.
     */
    size_t readFd(int fd);

    /**
     * @brief Writes data from the buffer to the file descriptor(a file/socket/pipe).
     */
    void writeFd(int fd);

    /**
     * @brief Checks if the buffer is full.
     */
    bool isFull() const;

    std::queue<std::string> getData() const;
};