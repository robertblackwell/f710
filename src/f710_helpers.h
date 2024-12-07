#ifndef H_f710_helpers_h
#define H_f710_helpers_h
#include <string>
#include <stdexcept>
#include <optional>

namespace f710 {

/** Tries to find a device whose path has the following string as a prefix "/dev/input/js"
 *
 *  If some are found chooses the first of those and returns the full path.
 *
 *  If no such path is found return {}
 */
    std::optional<std::string> get_dev_by_joy_name();
    int open_fd_non_blocking(std::string device_name);

//    class F710Exception: public std::exception
//    {
//        std::string what_message;
//    public:
//        F710Exception(const char* msg): what_message(msg){}
//        const char* what() const noexcept override {
//            return what_message.c_str();
//        }
//    };
//
//    class F710SelectError: public F710Exception {
//    public:
//        F710SelectError() : F710Exception("io error during select call") {}
//    };
//    class F710ReadIOError: public F710Exception {
//    public:
//        F710ReadIOError() : F710Exception("io error while reading controller") {}
//    };

} // namespace f710
#endif