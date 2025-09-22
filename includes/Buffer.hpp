#pragma once

#include <vector>
#include <deque>
#include <unistd.h>
#include <string>
#include <string_view>
#include "WebServErr.hpp"
#include "SharedTypes.hpp"

static constexpr size_t MAX_CHUNK_HEADER_SPACE = 20;
static constexpr size_t CRLF = 2; // CRLF size

typedef enum e_chunked_status
{
    NEXT_HEADER,
    PARTIAL_HEADER,
    BODY_PROCESSING,
    WAITING_DELIMITER,
    CHUNKED_EOF,
    CHUNKED_ERROR
} t_chunked_status;

/**
 * @brief A buffer class with reference-counted blocks and optional chunked transfer parsing.
 * @details
 * This buffer manages blocks of strings with string_view references, allowing zero-copy reads/writes.
 *
 * ## Normal mode
 * - Supports reading from and writing to file descriptors.
 * - Blocks are appended as needed, views track the readable region, reference counting ensures blocks are freed when no longer used.
 *
 * ## Chunked transfer encoding mode
 * It's the tricky part.
 * For each read, we may receive several chunks, or a partial chunk.
 * For a partial chunk, it may be a partial header, or a partial body, or a partial CRLF delimiter.
 *
 * It would be difficult to parse the chunked data across buffer blocks,
 * so we use another string_view, a FSM, and some state variables to handle the chunked parsing.
 *
 * And we also apply 2 rules during the processing:
 * 1. Ensure chunk header and trailing CRLF are not split across blocks.
 * 2. Chunked bodies can span multiple blocks.
 *
 * ## Why we couple the chunked parsing with the buffer class?
 * - We cannot defer parsing with an large buffer, since input may exceed available memory.
 * - Using a temporary file introduces an extra data copy.
 * - imo, handling chunked parsing in Buffer is efficient and nearly zero-copy,
 *   tho brings lots of complexity, and does not follow single-responsibility, as a cost.
 *   But I don't have better ideas now.
 */
class Buffer
{
private:
    std::deque<std::string> data_;           // The data blocks in the buffer, for adding.
    std::deque<size_t> ref_;                 // Reference counts for how many views point to each block.
    std::deque<std::string_view> data_view_; // The view of data blocks, for reading.
    size_t capacity_;                        // The maximum size of the buffer.
    size_t write_pos_;                       // The current write position in the last block
    size_t size_;                            // The current size of the buffer.
    size_t block_size_;                      // The size of each block in the buffer.
    size_t remain_header_size_;              // The remain size of the incomplete chunk header left in data_.
    size_t remain_body_size_;                // The remain size of the chunk body which is waiting for the delimiter in data_.
    size_t remain_chunk_size_;               // The remain size of the chunk in chunked mode.
    bool is_chunked_;                        // Whether the buffer is in chunked mode.
    bool is_eof_;                            // Whether the EOF has been reached in chunked mode.

    ssize_t handleNextHeader(std::string_view data, ssize_t parsed, size_t skipped);
    ssize_t handleBodyProcessing(std::string_view data, ssize_t parsed, size_t skipped);
    ssize_t handleChunkedEOF(ssize_t parsed);
    ssize_t handleChunkedError();

    /**
     * @brief The finite state machine scheduler for parsing chunked data.
     */
    ssize_t fsmScheduler(t_chunked_status status, std::string_view data);

    /**
     * @brief Reads data from the file descriptor into the buffer in chunked mode.
     */
    ssize_t readFdChunked(int fd);

public:
    Buffer();
    Buffer(const Buffer &) = default;
    Buffer &operator=(const Buffer &) = default;
    ~Buffer() = default;

    Buffer(size_t capacity, size_t block_size);

    /**
     * @brief Reads data from the file descriptor(a file/socket/pipe) into the buffer.
     */
    ssize_t readFd(int fd);

    /**
     * @brief Writes data from the buffer to the socket descriptor(a socket/pipe).
     */
    ssize_t writeSocket(int fd);

    /**
     * @brief Writes data from the buffer to the file descriptor(a file).
     */
    ssize_t writeFile(int fd);

    /**
     * @brief Checks if the buffer is full.
     */
    bool isFull() const;

    /**
     * @brief Checks if the buffer is empty.
     */
    bool isEmpty() const;

    /**
     * @brief Checks if the buffer is eof.
     */
    bool isEOF() const;

    /**
     * @brief Returns the current size of the buffer.
     */
    size_t size() const;

    /**
     * @brief Returns the first string in the buffer without removing it.
     */
    const std::string_view peek() const;

    /**
     * @brief Removes the first `size` bytes from the buffer, and sets the chunked mode.
     * @details
     * It is used for removing request header after parsing.
     * At that time, there should be only one block in the buffer.
     * If there are multiple blocks, or the given size is larger than the size of the first block,
     * it returns false.
     * If the buffer is in chunked mode, it also processes the rest body in chunked mode.
     */
    bool removeHeaderAndSetChunked(const std::size_t size, bool is_chunked);

    /**
     * @brief Inserts a header string into the buffer.
     */
    bool insertHeader(std::string str);
};