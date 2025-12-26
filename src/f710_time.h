#ifndef H_f710_TIME_H
#define H_f710_TIME_H

#include <cinttypes>
#include <cstring>
#include <memory>
#include <string>

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
#include <chrono>
#include <ctime>

namespace f710 {
    struct Time {
        uint64_t  millisecs;

        Time() = default;

        explicit Time(timeval tval) : millisecs(tval.tv_sec*1000 + tval.tv_usec/1000) {}

        [[nodiscard]] bool is_zero() const {
            return (millisecs == 0);
        }

        static Time from_ms(uint64_t ms) {
            Time t{};
            t.millisecs = ms;
            return t;
        }

        static Time now() {
            auto tm = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            Time t = Time::from_ms(tm);
            return t;
//            struct timeval tv{0};
//            Time t{};
//            gettimeofday(&tv, nullptr);
//            t.millisecs = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//            return t;
        }

        [[nodiscard]] Time add_ms(uint64_t ms) const
        {
            Time t{};
            t.millisecs = millisecs + ms;
            return t;
        }

        static Time diff_ms(Time t1, Time t2)
        {
            int64_t d = t1.millisecs - t2.millisecs;
            uint64_t d2 = (d < 0) ? 0: d;
            return Time::from_ms(d2);
        }

        static bool is_after(Time t1, Time t2)
        {
            return (t1.millisecs > t2.millisecs);
        }

        [[nodiscard]] timeval as_timeval() const
        {
            struct timeval tv = {.tv_sec = (__time_t)millisecs / 1000, .tv_usec = (__suseconds_t)(1000 * (millisecs % 1000))};
            return tv;
        }

    };

} // namespace f710
#endif