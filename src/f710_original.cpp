
// \author: Blaise Gassend

#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <vector>
#include <optional>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int64_t TimeInMilliSecs;
TimeInMilliSecs time_in_ms()
{
    struct timeval tv{0};
    gettimeofday(&tv, nullptr);
    return (TimeInMilliSecs)(tv.tv_sec*1000 + (tv.tv_usec/1000));
}
namespace f710 {
    struct Time {
        time_t      secs;
        suseconds_t u_secs;
        Time()=default;
        explicit Time(timeval tval): secs(tval.tv_sec), u_secs(tval.tv_usec) {}
        [[nodiscard]] bool is_zero() const {
            return (secs == 0) && (u_secs == 0);
        }
        static Time from_ms(uint64_t add_ms) {
            Time t{};
            t.secs = (time_t)add_ms / 1000;
            t.u_secs = ((suseconds_t)add_ms % 1000)*1000;
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
            t.secs += (time_t)ms / 1000;
            suseconds_t u = t.u_secs + (time_t)(ms % 1000) * 1000;
            t.secs += u / 1000000;
            t.u_secs = u % 1000000;
            return t;
        }
        static bool is_after(Time t1, Time t2)
        {
            return (t1.secs > t2.secs) || ((t1.secs == t2.secs) && (t1.u_secs > t2.u_secs));
        }
    };
}
namespace ros {
    struct Time {
        uint32_t secs;
        uint32_t nsecs;
    };

    Time get_time()
    {
        return {1,0};
    }
}

struct F710MessageHeader {
    ros::Time stamp;
};
struct F710Button {

};
struct F710Message {
    F710MessageHeader header;
    std::vector<uint32_t> buttons;
    std::vector<double>   axes;
};

/*! \brief Returns the device path of the first joystick that matches joy_name.
 *  If no match is found, an empty string is returned.
 */
std::string get_dev_by_joy_name(const std::string &joy_name) {
    const char path[] = "/dev/input";  // no trailing / here
    struct dirent *entry;
    struct stat stat_buf;

    DIR* dev_dir = opendir(path);
    if (dev_dir == nullptr) {
        printf("Couldn't open %s. Error %i: %s.", path, errno, strerror(errno));
        return "";
    }

    while ((entry = readdir(dev_dir)) != nullptr) {
        // filter entries
        if (strncmp(entry->d_name, "js", 2) != 0)  // skip device if it's not a joystick
        {
            continue;
        }
        std::string current_path = std::string(path) + "/" + entry->d_name;
        if (stat(current_path.c_str(), &stat_buf) == -1) {
            continue;
        }
        if (!S_ISCHR(stat_buf.st_mode))  // input devices are character devices, skip other
        {
            continue;
        }

        // get joystick name
        int joy_fd = open(current_path.c_str(), O_RDONLY);
        if (joy_fd == -1) {
            continue;
        }

        char current_joy_name[128];
        if (ioctl(joy_fd, JSIOCGNAME(sizeof(current_joy_name)), current_joy_name) < 0) {
            strncpy(current_joy_name, "Unknown", sizeof(current_joy_name));
        }

        close(joy_fd);

        printf("Found joystick: %s (%s).\n", current_joy_name, current_path.c_str());
        closedir(dev_dir);
        return current_path;
    }
    closedir(dev_dir);
    return "";
}
namespace f710 {
/**
 * This class represents one of the axis of one of the sticks on a F710
 *
 * The significant characteristic is that such a device sends a stream of events
 * as a stick is moved along one of its axes.
 *
 * Problem: the stream is very fast. That is interval between adjacent values
 * if too fast for each event to be sent to a robot.
 *
 * The purpose of this class is to process that stream into a slower stream
 * of values/events.
 */
    struct StreamDevice {
        /**
         * The system time of the end of the next interval.
         * Will be zero if no interval timer is running
         */
        ::f710::Time interval_end_time;
        /**
         * The system time of the end of the next interval
         */
        ::f710::Time time_of_last_reading;

        uint32_t previous_event_time;
        int16_t previous_event_value;

        uint64_t interval_length_in_ms;
        bool enabled;
        int state;
        int event_id;

        StreamDevice() = default;

        StreamDevice(int eventid, uint64_t interval_ms)
                : event_id(eventid), interval_end_time({0, 0}),
                  time_of_last_reading({0, 0}),
                  interval_length_in_ms(interval_ms) {

            state = 0;
            previous_event_time = 0;
            previous_event_value = 0;
            enabled = true;
        }

        /**
         * Records the most recent event
         * @param event_time  The time value in the most recent event
         * @param value       The stick position in the most recent event
         */
        void add_js_event(uint32_t event_time, int16_t value) {
            previous_event_time = event_time;
            previous_event_value = value;
            if (interval_end_time.is_zero()) {
                interval_end_time = Time::now().add_ms(interval_length_in_ms);
            }
        }

        std::optional<js_event> interval_expired(long time_value) {
            Time t = Time::now();
            if((!interval_end_time.is_zero()) && Time::is_after(t, interval_end_time)) {
                interval_end_time = Time({0,0});
                js_event ev = {.time=previous_event_time, .value=previous_event_value, .type=JS_EVENT_AXIS, .number=(__u8) event_id};
                return ev;
            };
            return {};
        }

    };
}
// axis events
#define AXIS_EVENT_CROSS_LEFT_RIGHT_EVENT_NUMBER 0
#define AXIS_EVENT_CROSS_FWD_BKWD_EVENT_NUMBER 1

#define AXIS_EVENT_RIGHT_STICK_LEFT_RIGHT_EVENT_NUMBER 2
#define AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER 3
#define AXIS_EVENT_LEFT_STICK_LEFT_RIGHT_EVENT_NUMBER 4
#define AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER 5
// button events
#define BUTTON_EVENT_NUMBER_Y 3
#define BUTTON_EVENT_NUMBER_B 2
#define BUTTON_EVENT_NUMBER_A 1
#define BUTTON_EVENT_NUMBER_X 0
#define BUTTON_EVENT_NUMBER_LT 6
#define BUTTON_EVENT_NUMBER_LB 4
#define BUTTON_EVENT_NUMBER_RT 7
#define BUTTON_EVENT_NUMBER_RB 5

/// \brief Opens, reads from and publishes joystick events
struct F710 {
    bool m_is_open;
    bool m_sticky_buttons;
    bool m_default_trig_value;
    std::string m_joy_dev;
    std::string m_joy_dev_name;
    double m_deadzone;
    double m_autorepeat_rate;    // in Hz.  0 for no repeat.
    double m_coalesce_interval;  // Defaults to 100 Hz rate limit.
    int m_event_count;
    int m_pub_count;
    f710::StreamDevice *left_stick_fwd_bkwd;
    f710::StreamDevice *right_stick_fwd_bkwd;

    F710(F710 *me, std::string device_path, double auto_repeat_rate, double coalesce_interval, double dead_zone)
            : left_stick_fwd_bkwd(nullptr), right_stick_fwd_bkwd(nullptr) {
        m_joy_dev_name = device_path;
        m_is_open = false;
        m_joy_dev = "";
        m_sticky_buttons = false;
        m_default_trig_value = false;
        m_event_count = 0;
        m_pub_count = 0;
        m_autorepeat_rate = auto_repeat_rate;
        m_deadzone = dead_zone;
        m_coalesce_interval = coalesce_interval;




        // Checks on parameters
        if (!me->m_joy_dev_name.empty()) {
            std::string joy_dev_path = get_dev_by_joy_name(me->m_joy_dev_name);
            if (joy_dev_path.empty()) {
                printf("Couldn't find a joystick with name %s. Falling back to default device.",
                       me->m_joy_dev_name.c_str());
            } else {
                printf("Using %s as joystick device.", joy_dev_path.c_str());
                me->m_joy_dev = joy_dev_path;
            }
        }
        if (me->m_autorepeat_rate < 0) {
            printf("joy_node: autorepeat_rate (%f) less than 0, setting to 0.", me->m_autorepeat_rate);
            me->m_autorepeat_rate = 0;
        }

        if (me->m_coalesce_interval < 0) {
            printf("joy_node: coalesce_interval (%f) less than 0, setting to 0.", me->m_coalesce_interval);
            me->m_coalesce_interval = 0;
        }

        if (me->m_autorepeat_rate > 1 / me->m_coalesce_interval) {
            printf("joy_node: autorepeat_rate (%f Hz) > 1/coalesce_interval (%f Hz) "
                   "does not make sense. Timing behavior is not well defined.", me->m_autorepeat_rate,
                   1 / me->m_coalesce_interval);
        }

        if (me->m_deadzone >= 1) {
            printf("joy_node: deadzone greater than 1 was requested. The semantics of deadzone have changed. "
                   "It is now related to the range [-1:1] instead of [-32767:32767]. For now I am dividing your deadzone "
                   "by 32767, but this behavior is deprecated so you need to update your launch file.");
            me->m_deadzone /= 32767;
        }

        if (me->m_deadzone > 0.9) {
            printf("joy_node: deadzone (%f) greater than 0.9, setting it to 0.9", me->m_deadzone);
            me->m_deadzone = 0.9;
        }

        if (me->m_deadzone < 0) {
            printf("joy_node: m_deadzone (%f) less than 0, setting to 0.", me->m_deadzone);
            me->m_deadzone = 0;
        }
    }

    void open_fd() {
        int f710_fd;
        this->m_is_open = false;
        bool first_fault = true;
        while (true) {
            f710_fd = open(this->m_joy_dev.c_str(), O_RDONLY);
            if (f710_fd != -1) {
                // There seems to be a bug in the driver or something where the
                // initial events that are to define the initial state of the
                // joystick are not the values of the joystick when it was opened
                // but rather the values of the joystick when it was last closed.
                // Opening then closing and opening again is a hack to get more
                // accurate initial state data.
                close(f710_fd);
                f710_fd = open(this->m_joy_dev.c_str(), O_RDONLY);
            }
            if (f710_fd != -1) {
                break;
            }
            if (first_fault) {
                printf("Couldn't open joystick %s. Will retry every second.", this->m_joy_dev.c_str());
                first_fault = false;
            }
            sleep(1.0);
        }
        // here if we got a device - file descriptor is in joy_fd

        char current_joy_name[128];
        if (ioctl(f710_fd, JSIOCGNAME(sizeof(current_joy_name)), current_joy_name) < 0) {
            strncpy(current_joy_name, "Unknown", sizeof(current_joy_name));
        }

        printf("Opened joystick: %s (%s). m_deadzone: %f.", this->m_joy_dev.c_str(), current_joy_name,
               this->m_deadzone);
        this->m_is_open = true;

    }

    int f710_run()
    {
        // Parameter conversions
        double autorepeat_interval = 1 / this->m_autorepeat_rate;
        double scale = -1. / (1. - this->m_deadzone) / 32767.;
        double unscaled_deadzone = 32767. * this->m_deadzone;

        js_event event;
        struct timeval tv;
        fd_set set;
        int joy_fd;
        this->m_event_count = 0;
        this->m_pub_count = 0;

        // Big while loop opens, publishes
        while (true) {
            this->m_is_open = false;
            open_fd();

            bool tv_set = false;
            bool publication_pending = false;
            tv.tv_sec = 100;
            tv.tv_usec = 0;
            F710Message f710_msg;  // Here because we want to reset it on device close.
            double val;  // Temporary variable to hold event values
            while (true) {
                FD_ZERO(&set);
                FD_SET(joy_fd, &set);
                int select_out = select(joy_fd + 1, &set, nullptr, nullptr, nullptr);
                if (select_out == -1) {
                    // process error
                } else if (select_out == 0) {
                    std::optional<js_event> left_stick_event = this->left_stick_fwd_bkwd.interval_expired();
                    std::optional<js_event> right_stick_event = this->right_stick_fwd_bkwd.interval_expired();
                } else {
                    if (FD_ISSET(joy_fd, &set)) {
                        if (read(joy_fd, &event, sizeof(js_event)) == -1 && errno != EAGAIN) {
                            break;  // Joystick is probably closed. Definitely occurs.
                        }

                        f710_msg.header.stamp = ros::get_time();
                        this->m_event_count++;
                        //                    printf("Button event type: %d value: %d number: %d\n", event.type, event.value, event.number);
                        switch (event.type) {
                            case JS_EVENT_BUTTON:
                            case JS_EVENT_BUTTON | JS_EVENT_INIT:
                                printf("Button event  time: %d number: %d value: %d type: %d\n", event.time,
                                       event.number, event.value, event.type);
                                break;
                            case JS_EVENT_AXIS:
                            case JS_EVENT_AXIS | JS_EVENT_INIT: {
                                printf("Axes time:%f event number: %d value: %d type: %d\n", event.time / 1000.0,
                                       event.number, event.value, event.type);
                                int ev_number = event.number;
                                if (ev_number == AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER) {
                                    this->left_stick_fwd_bkwd.add_js_event(event.time, event.value);
                                } else if (ev_number == AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER) {
                                    this->right_stick_fwd_bkwd.add_js_event(event.time, event.value);
                                } else {
                                    // ignore these events
                                }
                            }
                                break;
                            default:
                                printf("joy_node: Unknown event type. Please file a ticket. "
                                       "time=%u, value=%d, type=%Xh, number=%d", event.time, event.value, event.type,
                                       event.number);
                                break;
                        }
                    }
                }

                // If an axis event occurred, start a timer to combine with other
                // events.
                if (!publication_pending && publish_soon) {
                    tv.tv_sec = trunc(this->m_coalesce_interval);
                    tv.tv_usec = (this->m_coalesce_interval - tv.tv_sec) * 1e6;
                    publication_pending = true;
                    tv_set = true;
                }

                // If nothing is going on, start a timer to do auto repeat.
                if (!tv_set && this->m_autorepeat_rate > 0) {
                    tv.tv_sec = trunc(autorepeat_interval);
                    tv.tv_usec = (autorepeat_interval - tv.tv_sec) * 1e6;
                    tv_set = true;
                }

                if (!tv_set) {
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                }
            }  // End of joystick open loop.

            //            close(m_ff_fd);
            close(joy_fd);
            //            ros::spinOnce();
            //            if (nh_.ok()) {
            //                printf("Connection to joystick device lost unexpectedly. Will reopen.");
            //            }
        }

        cleanup:
        printf("joy_node shut down.");

        return 0;
    }
}
int main(int argc, char **argv) {
//    ros::init(argc, argv, "joy_node");
    std::string controller_path = get_dev_by_joy_name("js0");
    F710 f710;
    f710_init(&f710, "js0", 0.5, 0.5, 0.0);
    return f710_run(&f710);
}
