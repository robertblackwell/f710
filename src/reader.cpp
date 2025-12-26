#include "f710_helpers.h"
#include "f710_time.h"
#include "f710_exceptions.h"
#include "reader.h"
#include "model_defines.h"
#include <assert.h>
#include <boost/asio/io_context.hpp>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <linux/input.h>
#include <linux/joystick.h>
#include <memory>
#include <optional>
#include <rbl/logger.h>
#include <rbl/simple_exit_guard.h>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

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
        : desired_select_timeout_interval(timeout_interval), epsilon_value(epsilon)
        {
            tnow = Time::now();
            computed_next_timeout_value_ms = tnow;
            target_wakeup = tnow;
            last_target_wake_up = tnow;
        }
        timeval current_timeout()
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
            Time tmp = last_target_wake_up.add_ms(100);
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
#if 0
    f710::Reader::Reader(std::string device_path, std::function<void(int, int, bool)> on_event_function)
            : m_fd(-1), m_controller_state(nullptr), m_axis_count(0), m_button_count(0),m_initialize_done(false),
            m_on_event_function(on_event_function)
    {
        m_joy_dev_name = device_path;
        m_is_open = false;
        m_joy_dev = "";
        m_controller_state = new ControllerState(
                AxisDevice(D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER),
                AxisDevice(D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER), ToggleButton(D_BUTTON_A));
    }
#endif
f710::Reader::Reader(std::string device_path, std::function<void(ControllerState& statref)> on_event_function)
        : m_fd(-1), m_axis_count(0), m_button_count(0),m_initialize_done(false),
        m_on_event_function(on_event_function)
    {
        m_joy_dev_name = device_path;
        m_is_open = false;
        m_joy_dev = "";
        m_controller_state = new ControllerState(
                AxisDevice(D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER),
                AxisDevice(D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER), ToggleButton(D_BUTTON_A));
    }
void f710::Reader::operator()() {run();}
    void f710::Reader::run()
    {
        fd_set set;
        int f710_fd = open_fd_non_blocking(m_joy_dev_name);
        this->m_is_open = false;
        exit_guard::Guard guard([f710_fd]() {close(f710_fd);});
        SelectTimeoutContext to_context(CONST_SELECT_TIMEOUT_INTERVAL_MS, CONST_SELECT_TIMEOUT_EPSILON_MS);
        struct timeval tv = to_context.current_timeout();
        while (true) {
            FD_ZERO(&set);
            FD_SET(f710_fd, &set);
            int select_out = select(f710_fd + 1, &set, nullptr, nullptr, &tv);
            if (select_out == -1) {
                throw F710SelectError();
            } else if (select_out == 0) {
                m_on_event_function(*m_controller_state);
                tv = to_context.after_select_timedout();
            } else {
                if (FD_ISSET(f710_fd, &set)) {
                    if(!m_initialize_done) {
                        read_init_events(f710_fd, m_controller_state);
                    } else {
                        read_events(f710_fd, m_controller_state);
                    }
                    tv = to_context.after_js_event();
                }
            }
        }
        close(f710_fd);
    }
    int f710::Reader::read_init_events(int f710_fd, ControllerState* cstate)
    {
        js_event event;
        while (true) {
            int nread = read(f710_fd, &event, sizeof(js_event));
            int save_errno = errno;
            if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                throw F710ReadIOError();
            } else if (nread == -1) {
                return 0;
            }
            assert(nread == sizeof(js_event));
            switch (event.type) {
                case JS_EVENT_INIT | JS_EVENT_BUTTON:
                    RBL_LOG_FMT("js_event_init_button time: %d number: %d value: %d type: %Xh",
                                event.time,
                                event.number, event.value, event.type);

//                    cstate->apply_init_event(event);
                    m_button_count++;
                    break;
                case JS_EVENT_AXIS | JS_EVENT_INIT: {
                    RBL_LOG_FMT("js_event_init_axis time:%f event number: %d value: %d type: %d",
                                event.time / 1000.0,
                                event.number, event.value, event.type);
//                    cstate->apply_init_event(event);
                    m_axis_count++;
                }
                    break;
                default:
                    throw F710WrongModeError();

            }
            RBL_LOG_FMT("initializing %d %d %d", ((int)m_initialize_done), m_button_count, m_axis_count)
            m_initialize_done = (m_button_count == 12)&&(m_axis_count == 6) ;
        }
    }
    /**
     * Reads all events available on the f710_fd until an EAGAIN error in which case return 0
     * If any other io type error return -1
     */
    int f710::Reader::read_events(int f710_fd, ControllerState* cstate)
    {
        js_event event;
        assert(m_initialize_done);
        while (true) {
            int nread = read(f710_fd, &event, sizeof(js_event));
            int save_errno = errno;
            if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                throw F710ReadIOError();
            } else if (nread == -1) {
                return 0;
            }
            switch (event.type) {
                case  JS_EVENT_INIT | JS_EVENT_BUTTON:
                    RBL_LOG_FMT("js_event_init_button time: %d number: %d value: %d type: %Xh",
                                event.time,
                                event.number, event.value, event.type);
                    break;
                case JS_EVENT_AXIS | JS_EVENT_INIT:
                    RBL_LOG_FMT("js_event_init_axis time:%f event number: %d value: %d type: %d",
                                event.time / 1000.0,
                                event.number, event.value, event.type);
                    break;
                case JS_EVENT_BUTTON:
                    RBL_LOG_FMT("Button event  time: %d number: %d value: %d type: %d", event.time,
                                event.number, event.value, event.type);
                    cstate->apply_event(event);
                break;
                case JS_EVENT_AXIS: {
                    RBL_LOG_FMT("Axis time:%f event number: %d value: %d type: %d", event.time / 1000.0,
                                event.number, event.value, event.type);
                    cstate->apply_event(event);
                }
                break;
                default:
                    RBL_LOG_FMT("joy_node: Unknown event type. Please file a ticket. "
                                "time=%u, value=%d, type=%Xh, number=%d", event.time, event.value,
                                event.type,
                                event.number);
                break;
            }
        }
    }
} // namespace f710