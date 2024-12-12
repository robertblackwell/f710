#ifndef marvin_contig_buffer_t_hpp
#define marvin_contig_buffer_t_hpp
#include <cstddef>
#include <cstring> // memcpy
#include <cstdlib> //realloc

#include <memory>
#include <iostream>
#include <iterator>

namespace rbl {

/**
 *
 * \brief IoBuffer provides a template class representing a contiguous expanding block of memory.
 *
 * IoBuffer class wraps a contiguous buffer and provides manipulation methods.
 * Once constructed the IoBuffer instance "own" the raw memory.
 * IoBuffer destructor releases the raw memory.
 *
 * IoBuffers are designed to be used for partial input/output without the need for copying data.
 *
 * The active data in a buffer is in the memory range between
 *
 *  m_memPtr + m_start_ptr and m_memPtr + m_start_ptr + m_length.
 *
 * The functions space_ptr() and space_len() give programmers access to the unused space at
 * the end of the buffer.
 *
 * Hence if reading into an already in-use buffer one would use the following call
 *
 *      int n = read(fd, iobuffer.space_ptr(), iobuffer.space_len())
 *
 * Data can be removed from the front of a buffer with the consume() function.
 * This is typically used when a buffer has data from a read operation and subsequent processing can
 * only handle some but not all of that data.
 *
 *      int num_processed = some_process(iobuffer)
 *      iobuffer.consume(num_processed)
 *
 *
 *
*/
class IoBuffer
{
protected:
    union {
        void *m_memPtr;      /// points to the start of the memory slab managed by the instance
        char *m_cPtr_xx;        /// same as memPtr but makes it easier in debugger to see whats in the buffer
    };
    char *m_cPtr;        /// same as memPtr but makes it easier in debugger to see whats in the buffer
    std::size_t m_start_offset;/// offset from m_memPtr of the first byte of the active data in the buffer
                               /// because of the consume() function this may not always be the
                               /// the same as m_memPtr
    std::size_t m_length;      ///
    std::size_t m_capacity;    /// the capacity of the buffer, the value used for the malloc call
//    std::size_t m_size;        /// size of the currently filled portion of the memory slab
    void init(std::size_t cap);
    /**
    * returns a pointer to the next available unused position in the buffer
    */
    void* nextAvailable();
    void set_cstr_terminator();

public:
    using SPtr = std::shared_ptr<IoBuffer>;
    using UPtr = std::unique_ptr<IoBuffer>;
    static std::size_t min_buffer_size;
#if 0
    static SPtr makeSPtr(std::size_t capacity)
    {
        std::size_t sz = (capacity > IoBuffer<S>::min_buffer_size) ? capacity : IoBuffer<S>::min_buffer_size ;
        SPtr mbp = std::make_shared<IoBuffer>((sz));
        return mbp;
    }
    static SPtr makeSPtr(std::string s)
    {
        SPtr mbp = std::make_shared<IoBuffer>((s.size()));
        mbp->append((void*) s.c_str(), s.size());
        return mbp;
    }
    static SPtr makeSPtr(void* mem, std::size_t size)
    {
        SPtr mbp = std::make_shared<IoBuffer>((size));
        mbp->append(mem, size);
        return mbp;
    }
    static SPtr makeSPtr(IoBuffer& mb)
    {
        SPtr mbp = std::make_shared<IoBuffer>((mb.capacity()));
        mbp->append(mb.data(), mb.size());
        return mbp;
    }
#endif
    explicit IoBuffer(std::size_t cap = 128);
    explicit IoBuffer(std::string& sin, std::size_t cap = 128);
    IoBuffer(IoBuffer& other) = delete;
    IoBuffer& operator =(IoBuffer& other) = delete;
    IoBuffer(IoBuffer&& other) noexcept;
    IoBuffer& operator =(IoBuffer&& other) noexcept;
    ~IoBuffer();
    /**
     * gets a pointer to the start of the active contents of the buffer
     */
    void* data();
    /**
     * Get the first byte/char of the content.
     * @return nullptr if buffer is empty()
     */
    char* get_first_char_ptr();
    char* c_str();
    /**
     * gets a pointer to the start of the memory slab being managed by the instance
     */
    void* raw_memory();
    /**
     * gets the size of used portion of the buffer
    */
    [[nodiscard]] std::size_t size() const;
    /**
     * Returns true if the buffer is empty
     * @return bool
     */
    [[nodiscard]] bool empty() const;
    /**
     * capacity of the buffer - max value of size
    */
    [[nodiscard]] std::size_t capacity() const;
    /**
     * Resets the buffer so that it is again an empty buffer
     */
    void clear();

    void append(void* data, std::size_t len);
    void append(std::string const & str);
    void append(std::string&& str);
    void append(std::string* str);
    void setSize(std::size_t n);
    /**
     * @brief Returns a pointer to the start of unused memory space after the last
     * active content in the buffer. This is the start of a memory where more data could be placed.
     * The memory pointed into is owned by the IoBuffer. Do not free
     * @param this
     * @return void*
     */
    void* space_ptr();
    /**
     * @brief Returns a the length of available space in the buffer after
     * the active data.
     * @param this
     * @return int
     */
    [[nodiscard]] std::size_t space_len() const;
    /**
     * @brief Updates the IoBuffer so that the bytes_used bytes of memory area after the active content
     * is also considered to be active data. The memory added is not updated as it is expected
     * that data has already been put into that memory area.
     * @param this
     * @param bytes_used
     */
    void commit(std::size_t bytes_used);
    /**
     * @brief Updates the IoBuffer so that the first byte_count bytes of the active data are now
     * considered not active data. IE Increments the start pointer. This allows for partial
     * processing of the data in the buffer and the 'discarding' of the processed data without
     * discarding the buffer or copying the unprocessed data
     * @param this
     * @param byte_count
     */
    void consume(std::size_t byte_count);
    /**
     * Returns a string that has the same value as the used portion of the buffer
     */
    std::string to_string();
    std::string substr(std::size_t start, std::size_t len);
    /**
     * Determines if an address value (pointer) is within the address range of the
     * the buffer ie
     *      buffer.dada() < = ptr < buffer.data() + buffer.capacity();
     *  or, should it be
     *      buffer.dada() < = ptr < buffer.data() + buffer.size();
     *
     */
    bool contains(void* ptr);
    bool contains(const char* ptr);

};


} //namespace Marvin
#endif
