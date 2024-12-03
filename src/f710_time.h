#ifndef f710_TIME_H
#define f710_TIME_H

#include "f710.h"
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>

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
#if 0
    struct Time {
        time_t secs;
        suseconds_t u_secs;

        Time() = default;

        explicit Time(timeval tval) : secs(tval.tv_sec), u_secs(tval.tv_usec) {}

        [[nodiscard]] bool is_zero() const {
            return (secs == 0) && (u_secs == 0);
        }

        static Time from_ms(uint64_t add_ms) {
            Time t{};
            t.secs = (time_t) add_ms / 1000;
            t.u_secs = ((suseconds_t) add_ms % 1000) * 1000;
            return t;
        }

        static Time now() {
            struct timeval tv{0};
            Time t{};
            gettimeofday(&tv, nullptr);
            t.secs = tv.tv_sec;
            t.u_secs = tv.tv_usec;
            return t;
        }

        Time add_ms(uint64_t ms) {
            Time t({this->secs, this->u_secs});
            t.secs += (time_t) ms / 1000;
            suseconds_t u = t.u_secs + (time_t) (ms % 1000) * 1000;
            t.secs += u / 1000000;
            t.u_secs = u % 1000000;
            return t;
        }

        static bool is_after(Time t1, Time t2) {
            return (t1.secs > t2.secs) || ((t1.secs == t2.secs) && (t1.u_secs > t2.u_secs));
        }
    };
#endif
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