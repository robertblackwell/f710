#ifndef f710_H
#define f710_H
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>
#include <functional>
#include <dirent.h>
#include <fcntl.h>
#include <climits>
#include <vector>
#include <optional>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <cmath>
#include <sys/stat.h>
#include <unistd.h>
#include "f710_time.h"
#include "f710_exceptions.h"

namespace f710 {
    struct StreamDevice;

    class F710 {
    public:
        explicit F710(std::string device_path);
        ~F710();
        void run(std::function<void(int, int)> on_event_function);

    private:
        void reset();
        int read_events(int fd, StreamDevice* left, StreamDevice* right);
        bool m_is_open;
//        bool m_sticky_buttons;
//        bool m_default_trig_value;
        int  m_f710_fd;
        std::string m_joy_dev;
        std::string m_joy_dev_name;
//        double m_deadzone;
//        double m_autorepeat_rate;    // in Hz.  0 for no repeat.
//        double m_coalesce_interval;  // Defaults to 100 Hz rate limit.
//        int m_event_count;
//        int m_pub_count;
        int          saved_left_value;
        int          saved_right_value;
        StreamDevice *left_stick_fwd_bkwd;
        StreamDevice *right_stick_fwd_bkwd;
    };
}
#endif