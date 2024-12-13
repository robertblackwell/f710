#include "f710.h"
#include "f710_helpers.h"
#include <string>
#include <thread>
#include <chrono>
#include <cinttypes>
#include <rbl/simple_exit_guard.h>
#include <utility>
#include <optional>
#include <sys/time.h>
#include <linux/joystick.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rbl/std_format.h>
#define RBL_LOG_ENABLED
#define RBL_LOG_ALLOW_GLOBAL
#include <rbl/logger.h>


// axis events
#define AXIS_EVENT_CROSS_LEFT_RIGHT_EVENT_NUMBER 4
#define AXIS_EVENT_CROSS_FWD_BKWD_EVENT_NUMBER 5

#define AXIS_EVENT_RIGHT_STICK_LEFT_RIGHT_EVENT_NUMBER 2
#define AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER 3
#define AXIS_EVENT_LEFT_STICK_LEFT_RIGHT_EVENT_NUMBER 0
#define AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER 1
// button events
#define BUTTON_EVENT_NUMBER_Y 3
#define BUTTON_EVENT_NUMBER_B 2
#define BUTTON_EVENT_NUMBER_A 1
#define BUTTON_EVENT_NUMBER_X 0
#define BUTTON_EVENT_NUMBER_LT 6
#define BUTTON_EVENT_NUMBER_LB 4
#define BUTTON_EVENT_NUMBER_RT 7
#define BUTTON_EVENT_NUMBER_RB 5

#define CONST_SELECT_TIMEOUT_INTERVAL_MS 500
#define CONST_SELECT_TIMEOUT_EPSILON_MS 5

namespace f710 {
    /**
     * This struct is a convenient way to hold values that allow ongoing calculation of
     * what the timeout value should be for the next select call
     */
    struct SelectTimeoutContext {
        Time tnow;
        Time target_wakeup;
        Time last_target_wake_up;
        /**
         * This is the desired interval between select timeouts in millisecs
         */
        uint64_t desired_select_timeout_interval;
        /**
         * This is fudge factor.
         */
        uint64_t epsilon_value;
        Time computed_next_timeout_value_ms;
        SelectTimeoutContext(uint64_t timeout_interval, uint64_t epsilon)
            : tnow(Time::now()),
            target_wakeup(tnow.add_ms(timeout_interval)),
            last_target_wake_up(target_wakeup),
            desired_select_timeout_interval(timeout_interval),
            epsilon_value(epsilon),
            computed_next_timeout_value_ms(Time::from_ms(timeout_interval))


        {
            tnow = Time::now();
            computed_next_timeout_value_ms = tnow;
            target_wakeup = tnow;
            last_target_wake_up = tnow;
        }
        [[nodiscard]] timeval current_timeout() const
        {
            return computed_next_timeout_value_ms.as_timeval();
        }
        /**
         * Computes the next timeout interval as a `timeval` for the select call when we are processing a
         * select timeout
         */
        timeval after_select_timedout()
        {
            tnow = Time::now();
            target_wakeup = tnow.add_ms(desired_select_timeout_interval);
            computed_next_timeout_value_ms = Time::from_ms(desired_select_timeout_interval);
            last_target_wake_up = target_wakeup;
            return computed_next_timeout_value_ms.as_timeval();
        }
        /**
         * Calculate the next timeout value as a `timeval` when the most recent return from select was
         * because of a js_event and subsequent to that we got an EAGAIN.
         *
         * Reading js_events takes time. The goal here is to not let the reading of events extend the
         * period between select timeout expiries - at leats not "too much". Epsilon is the "too much"
         * factor.
         * So we try to calculate indirectly how much the next T/O
         * interval should be in order to get the T/O "back on schedule"
         */
        timeval after_js_event()
        {
            // compute the select timeout interval
            tnow = Time::now();
            if(Time::is_after(last_target_wake_up, tnow.add_ms(epsilon_value))) {
                // there is at least epsilon ms before the previously computed wakeup time

            } else if(Time::is_after(last_target_wake_up, tnow)) {
                // the last computed wake-up time is after tnow() but
                // there is less than epsilon ms between now and the last computed wakeup time
                // extend the wake-up time by epsilon ms

                last_target_wake_up = last_target_wake_up.add_ms(epsilon_value);
            } else {
                // last computed wake-up time is NOT after tnow. Set wakeup time to
                //tnow + epsilon ms - that is almost immediately.
                last_target_wake_up = tnow.add_ms(epsilon_value);
            }
            computed_next_timeout_value_ms = Time::diff_ms(last_target_wake_up, tnow);
            return computed_next_timeout_value_ms.as_timeval();
        }
    };

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
        uint32_t latest_event_time;
        int16_t  latest_event_value;
        bool     is_new_event;
        int event_id;

        StreamDevice() = default;

        explicit StreamDevice(int eventid)
                : event_id(eventid)
        {
            is_new_event = false;
            latest_event_time = 0;
            latest_event_value = 0;
        }
        void reset()
        {
            is_new_event = false;
            latest_event_time = 0;
            latest_event_value = 0;
        }
        /**
         * Records the most recent event
         * @param event_time  The time value in the most recent event
         * @param value       The stick position in the most recent event
         */
        void add_js_event(uint32_t event_time, int16_t value)
        {
            if(!is_new_event) {
                is_new_event = true;
            }
            latest_event_time = event_time;
            latest_event_value = value;
        }
        /**
         *  Returns {} if there is not a new event sicne the last call to this function
         *  Returns the event if there has been one or more new events since the last call
         */
        js_event get_latest_event()
        {
            is_new_event = false;
            js_event ev = {.time=latest_event_time, .value=latest_event_value, .type=JS_EVENT_AXIS, .number=(__u8) event_id};
            return ev;
        }

    };

    f710::F710::F710(std::string device_path)
            : m_f710_fd(-1), saved_left_value(-1),
              saved_right_value(-1),
              left_stick_fwd_bkwd(nullptr),
              right_stick_fwd_bkwd(nullptr)
    {
        m_joy_dev_name = std::move(device_path);
        m_is_open = false;
        m_joy_dev = "";
        left_stick_fwd_bkwd = new StreamDevice(AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER);
        right_stick_fwd_bkwd = new StreamDevice(AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER);
    }
    f710::F710::~F710()
    {
        if(m_f710_fd != -1) {
            close(m_f710_fd);
            m_f710_fd = -1;
        }
    }
    void f710::F710::run(std::function<void(int, int)> on_event_function)
    {
        while (true) {
            try {
                RBL_LOG_FMT("trying to find a Logitech F710 device at /dev/input/jsN");
                fd_set set;
                m_f710_fd = -1;
                auto fullpath = get_dev_by_joy_name();
                if(!fullpath) {
                    throw std::runtime_error("could not find path of the form /dev/imput/js");
                }
                m_f710_fd = open_fd_non_blocking(fullpath.value());
                if(m_f710_fd == -1) {
                    throw std::runtime_error(std_format("could not open the device path we found %s", fullpath.value().c_str()));
                }
                // this->left_stick_fwd_bkwd->reset();
                // this->right_stick_fwd_bkwd->reset();
                SelectTimeoutContext to_context(CONST_SELECT_TIMEOUT_INTERVAL_MS, CONST_SELECT_TIMEOUT_EPSILON_MS);
                struct timeval tv = to_context.current_timeout();
                while (true) {
                    FD_ZERO(&set);
                    FD_SET(m_f710_fd, &set);
                    int select_out = select(m_f710_fd + 1, &set, nullptr, nullptr, &tv);
                    if (select_out == -1) {
                        throw F710SelectError();
                    } else if (select_out == 0) {
                        auto left_value = -1 * this->left_stick_fwd_bkwd->get_latest_event().value;
                        auto right_value = -1 * this->right_stick_fwd_bkwd->get_latest_event().value;
//                        printf("left_value: %d right_value: %d\n", left_value, right_value);
                        on_event_function(left_value, right_value);
                        tv = to_context.after_select_timedout();
                    } else {
                        if (FD_ISSET(m_f710_fd, &set)) {
                            read_events(m_f710_fd, left_stick_fwd_bkwd, right_stick_fwd_bkwd);
                            tv = to_context.after_js_event();
                        }
                    }
                }
                close(m_f710_fd);
                m_f710_fd = -1;
            } catch (std::exception& e) {
                RBL_LOG_FMT("f710 caught an exception what: %s\n", e.what());
                if(m_f710_fd != -1) {
                    close(m_f710_fd);
                    m_f710_fd = -1;
                }
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(2000ms);
                continue;
            }
        }
    }
    /**
     * Reads all events available on the f710_fd until an EAGAIN error in which case return 0
     * If any other io type error return -1
     */
    int f710::F710::read_events(int f710_fd, StreamDevice *left, StreamDevice *right) {
        js_event event;
        while (true) {
            long nread = read(f710_fd, &event, sizeof(js_event));
            int save_errno = errno;
            if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                throw F710ReadIOError();
            } else if (nread == -1) {
                return 0;
            }
            switch (event.type) {
                case JS_EVENT_BUTTON | JS_EVENT_INIT:
//                    RBL_LOG_FMT("js_event_init \n");
                case JS_EVENT_BUTTON:
//                    RBL_LOG_FMT("Button event  time: %d number: %d value: %d type: %d\n", event.time,
//                                event.number, event.value, event.type);
                    break;
                case JS_EVENT_AXIS | JS_EVENT_INIT:{
                    if (event.number == left->event_id) {
                        left->add_js_event(event.time, 0);
                    } else if (event.number == right->event_id) {
                        right->add_js_event(event.time, 0);
                    } else {
                        // ignore these events
                    }
                    break;
                }
                case JS_EVENT_AXIS:{ 
                    int ev_number = event.number;
                    if (ev_number == left->event_id) {
                        left->add_js_event(event.time, event.value);
                    } else if (ev_number == right->event_id) {
                        right->add_js_event(event.time, event.value);
                    } else {
                        // ignore these events
                    }
                    break;
                }
                default:
//                    RBL_LOG_FMT("joy_node: Unknown event type. Please file a ticket. "
//                                "time=%u, value=%d, type=%Xh, number=%d", event.time, event.value,
//                                event.type,
//                                event.number);
                    break;
            }
        }
    }

} // namespace f710