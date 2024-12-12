#include "iobuffer.h"
#include <exception>
#include <cassert>
#define IOB_TERM_CHAR (char)0x00;

namespace rbl {

void IoBuffer::init(std::size_t cap)
{
    m_memPtr = malloc(cap);
    m_start_offset = 0;
    m_cPtr = (char*) m_memPtr;
    m_length = 0;
    m_capacity = cap;
    set_cstr_terminator();
}
IoBuffer::IoBuffer(const std::size_t cap)
{
    std::size_t tmp_cap = cap;
    m_memPtr = malloc(tmp_cap);
    m_start_offset = 0;
    m_cPtr = (char*) m_memPtr;
    m_length = 0;
    m_capacity = tmp_cap;
    set_cstr_terminator();
}
IoBuffer::IoBuffer(std::string& str, std::size_t cap)
{
    std::size_t tmp_cap = cap;
    m_memPtr = malloc(tmp_cap);
    m_start_offset = 0;
    m_cPtr = (char*) m_memPtr;
    m_length = 0;
    m_capacity = tmp_cap;
    this->append((void*)str.c_str(), str.size());
}
IoBuffer::IoBuffer(IoBuffer&& other) noexcept
{
    m_capacity = other.m_capacity;
    m_memPtr = other.m_memPtr;
    m_length = other.m_length;
    m_start_offset = other.m_start_offset;
    m_cPtr = (char*) m_memPtr;
    other.init(m_capacity);
}
IoBuffer& IoBuffer::operator =(IoBuffer&& other) noexcept
{
    if (&other == this) {
        return *this;
    }
    m_memPtr = other.m_memPtr;
    m_cPtr = (char*)m_memPtr;
    m_capacity = other.m_capacity;
    m_length = other.m_length;
    m_start_offset = other.m_start_offset;

    other.init(m_capacity);
    return *this;
}

IoBuffer::~IoBuffer()
{
    if( (m_memPtr != nullptr) && (m_capacity > 0) ){
        free(m_memPtr);
    }
}
void* IoBuffer::raw_memory()
{
    return m_memPtr;
}

void* IoBuffer::data()
{
    char* start_of_content = &((char*)m_memPtr)[m_start_offset];
    return (void*)start_of_content;
}
char* IoBuffer::get_first_char_ptr()
{
    char* start_of_content = &((char*)m_memPtr)[m_start_offset];
    return start_of_content;
}
char* IoBuffer::c_str() {return get_first_char_ptr();}

std::size_t IoBuffer::size() const
{
    return m_length;
}
bool IoBuffer::empty() const
{
    return (m_length == 0);
}
/**
 * capacity of the buffer - max value of size
*/
std::size_t IoBuffer::capacity() const
{
    return m_capacity;
}
/**
 * returns a pointer to the next available unused position in the buffer
*/
void* IoBuffer::nextAvailable()
{   if(m_cPtr == nullptr) {
        throw std::runtime_error("m_cPtr is null ");
    } else {
        return (void *) (get_first_char_ptr() + m_length);
    }
}
/**
 * Make sure that the byte after the last active data bayte is set to (char*)0
 * This makes the active data a valid c string
 */
inline void IoBuffer::set_cstr_terminator()
{
   *(m_cPtr + (m_start_offset + m_length)) = IOB_TERM_CHAR;
}
/**
 * Resets the buffer so that it is again an empty buffer
 */
void IoBuffer::clear()
{
    m_start_offset = 0;
    m_length = 0;
    set_cstr_terminator();
}

void IoBuffer::append(void* data, std::size_t len)
{
    if ( ( (m_start_offset + m_length + len) >= m_capacity )  ) {
        std::size_t new_capacity = std::max(2 * m_capacity, m_capacity + 2*len);
        void* tmp = realloc(m_memPtr, new_capacity);
        m_memPtr = tmp;
        m_cPtr = (char*) m_memPtr;
        m_capacity = new_capacity;
    }
    void* na = nextAvailable();
    memcpy(na, data, len);
    m_length = m_length + len;
    m_cPtr = (char*) m_memPtr;
    set_cstr_terminator();
}
void IoBuffer::append(std::string const & str)
{
    append((void*)str.c_str(), str.size());
}
void IoBuffer::append(std::string&& str)
{
    append((void*)str.c_str(), str.size());
}
void IoBuffer::append(std::string* str)
{
    append((void*)str->c_str(), str->size());
}
void IoBuffer::setSize(std::size_t n)
{
    m_length = n;
    set_cstr_terminator();
}
void* IoBuffer::space_ptr()
{
    return &((char*)m_memPtr)[m_start_offset + m_length];
}
std::size_t IoBuffer::space_len() const
{
    size_t x = m_capacity - 1 - (m_start_offset + m_length);
    return x;
}
void IoBuffer::commit(std::size_t bytes_used)
{
    if(m_start_offset + m_length + bytes_used >= m_capacity) {
        throw std::invalid_argument("bytes_used ");
    }
    m_length += bytes_used;
    *((char*)nextAvailable())= IOB_TERM_CHAR;
}
void IoBuffer::consume(std::size_t byte_count)
{
    if(byte_count > m_length) {
        throw std::invalid_argument("byte_count");
    }
    m_start_offset += byte_count;
    m_length -= byte_count;
    if(m_length == 0) {
        m_start_offset = 0;
    }

}
std::string IoBuffer::to_string()
{
    char* p = &(m_cPtr[m_start_offset]);
    std::string s(p, m_length);
    return s;
}
std::string IoBuffer::substr(std::size_t start, std::size_t len)
{
    if(start + len > m_capacity) {
        throw std::runtime_error("IoBuffer::substr call is too large");
    }
    char* p = &(m_cPtr[start]);
    std::string s(p, len);
    return s;
}

bool IoBuffer::contains(void* ptr)
{
    char* p = (char*) ptr;
    return contains(p);
}
bool IoBuffer::contains(const char* ptr)
{
    char* endPtr = m_cPtr + (long)m_capacity;
    char* sPtr = m_cPtr;
//    bool r1 = ptr <= endPtr;
//    bool r2 = ptr >= sPtr;
    bool r = ( ptr <= endPtr && ptr >= sPtr);
    return r;
}


} //namespace Marvin
