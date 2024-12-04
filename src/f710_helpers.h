#ifndef H_f710_helpers_h
#define H_f710_helpers_h
#include <string>
#include <stdexcept>

namespace f710 {

/*! \brief Returns the device path of the first joystick that matches joy_name.
 *  If no match is found, an empty string is returned.
 */
    std::string get_dev_by_joy_name(const std::string &joy_name);
    int open_fd_non_blocking(std::string device_name);
    class F710Exception: public std::exception
    {
        std::string what_message;
    public:
        const char* what() const noexcept override {
            return what_message.c_str();
        }
    };

} // namespace f710
#endif