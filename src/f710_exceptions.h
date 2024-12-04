#ifndef H_f710_exceptions_h
#define H_f710_exceptions_h
#include <string>
#include <stdexcept>

namespace f710 {
    class F710Exception: public std::exception
    {
        std::string what_message;
    public:
        F710Exception(const char* msg): what_message(msg){}
        const char* what() const noexcept override {
            return what_message.c_str();
        }
    };

    class F710SelectError: public F710Exception {
    public:
        F710SelectError() : F710Exception("io error during select call") {}
    };
    class F710ReadIOError: public F710Exception {
    public:
        F710ReadIOError() : F710Exception("io error while reading controller") {}
    };

} // namespace f710
#endif