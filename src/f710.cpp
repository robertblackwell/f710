
// \author: Blaise Gassend
#include "f710.h"
#include "f710_helpers.h"
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
#include <fcntl.h>
#include <assert.h>
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
#define CONST_SELECT_TIMEOUT_EPSILON_MS 50

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
         * period between select timeout expiries. So we try to calculate indirectly how much the next T/O
         * interval should be in order to get the T/O "back on schedule"
         */
        timeval after_js_event()
        {
            // compute the select timeout interval
            tnow = Time::now();
            Time tmp = last_target_wake_up.add_ms(100);
            if(Time::is_after(last_target_wake_up, tnow.add_ms(epsilon_value))) {
                // there is at least 100ms before the previously computed wakeup time

            } else if(Time::is_after(last_target_wake_up, tnow)) {
                // the last computed wake-up time is after tnow() but
                // there is less than 100 ms between now and the last computed wakeup time
                // extend the wake-up time

                last_target_wake_up = last_target_wake_up.add_ms(epsilon_value);
            } else {
                // last computed wake-up time is NOT after tnow. Set wakeup time to
                //tnow + 100ms
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

        StreamDevice(int eventid)
                : event_id(eventid)
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
            Time tnow = Time::now();
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
            : m_fd(-1), left_stick_fwd_bkwd(nullptr), right_stick_fwd_bkwd(nullptr),
              saved_left_value(-1),
              saved_right_value(-1)
    {
        m_joy_dev_name = device_path;
        m_is_open = false;
        m_joy_dev = "";
        left_stick_fwd_bkwd = new StreamDevice(AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER);
        right_stick_fwd_bkwd = new StreamDevice(AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER);
    }

    int f710::F710::run() {
        js_event event;
        fd_set set;
        int joy_fd;
        // Big while loop opens, publishes
        while (true) {
            this->m_is_open = false;
            joy_fd = open_fd(m_joy_dev_name);
            int status = fcntl(joy_fd, F_SETFL, fcntl(joy_fd, F_GETFL, 0) | O_NONBLOCK);
            if (status == -1){
                perror("calling fcntl");
                return -1;
            }

            bool tv_set = false;
            bool publication_pending = false;
//            tv.tv_sec = 100;
//            tv.tv_usec = 0;
            double val;  // Temporary variable to hold event values
#define XTO_OLD
#ifdef TO_OLD
            Time tnow = Time::now();
            Time target_wakeup = Time::now();
            Time last_target_wake_up = Time::now();
            Time select_timeout_interval = Time::from_ms(CONST_SELECT_TIMEOUT_INTERVAL_MS);
            struct timeval tv = select_timeout_interval.as_timeval();
#else
            SelectTimeoutContext to_context(CONST_SELECT_TIMEOUT_INTERVAL_MS, CONST_SELECT_TIMEOUT_EPSILON_MS);
            struct timeval tv = to_context.current_timeout();
#endif
            while (true) {
                FD_ZERO(&set);
                FD_SET(joy_fd, &set);
                RBL_LOG_FMT("starting interval calc last_target_wake_up: %ld target_wake_up: %ld  now: %ld\n", last_target_wake_up.millisecs, target_wakeup.millisecs, tnow.millisecs);
                RBL_LOG_FMT("\t target-last %ld target-now: %ld\n", target_wakeup.millisecs - last_target_wake_up.millisecs, target_wakeup.millisecs - tnow.millisecs);
                RBL_LOG_FMT("select tv calc: tv secs: %ld tv u_secs: %ld\n", tv.tv_sec, tv.tv_usec);
//                printf("tv %ld %ld  tv2 %ld %ld\n", tv.tv_sec, tv.tv_usec, tv2.tv_sec, tv2.tv_usec);
                int select_out = select(joy_fd + 1, &set, nullptr, nullptr, &tv);
                if (select_out == -1) {
                    // process error
                } else if (select_out == 0) {
                    RBL_LOG_FMT("select_out == 0 timeout\n");
                    Time t = Time::now();
                    js_event left_stick_event = this->left_stick_fwd_bkwd->get_latest_event();
                    js_event right_stick_event = this->right_stick_fwd_bkwd->get_latest_event();
                    int left_value = -left_stick_event.value;
                    int right_value = -right_stick_event.value;
//                    if((left_value != saved_left_value) || (right_value != saved_right_value)) {
                        saved_left_value = left_value;
                        saved_right_value = right_value;
                        printf("XXXXXXXXemit left: %d right: %d\n", saved_left_value, saved_right_value);
//                    }
                    // compute the next timeout interval
#ifdef TO_OLD
                    tnow = Time::now();
                    target_wakeup = tnow.add_ms(CONST_SELECT_TIMEOUT_INTERVAL_MS);
                    select_timeout_interval = Time::from_ms(CONST_SELECT_TIMEOUT_INTERVAL_MS);
                    last_target_wake_up = target_wakeup;
                    tv = select_timeout_interval.as_timeval();
#else
                    tv = to_context.after_select_timedout();
#endif
                } else {
                    if (FD_ISSET(joy_fd, &set)) {
                        while (true) {
                            int nread = read(joy_fd, &event, sizeof(js_event));
                            int save_errno = errno;
                            if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                                // f710 probably closed itself
                                return -1;
                            } else if (nread == -1) {
//                                printf("eagain\n");
                                // compute the select timeout interval
#ifdef TO_OLD
                                Time tnow = Time::now();
                                Time tmp = last_target_wake_up.add_ms(100);
                                if(Time::is_after(last_target_wake_up, tnow.add_ms(CONST_SELECT_TIMEOUT_EPSILON_MS))) {
                                    // there is at least 100ms before the previously computed wakeup time

                                } else if(Time::is_after(last_target_wake_up, tnow)) {
                                    // the last computed wake up time is after tnow() but
                                    // there is less than 100 ms between now and the last computed wakeup time
                                    // extend the wakeup time

                                    last_target_wake_up = last_target_wake_up.add_ms(CONST_SELECT_TIMEOUT_EPSILON_MS);
                                } else {
                                    // last computed wake up time is NOT after tnow. Set wakeup time to
                                    //tnow + 100ms
                                    last_target_wake_up = tnow.add_ms(CONST_SELECT_TIMEOUT_EPSILON_MS);
                                }
                                select_timeout_interval = Time::diff_ms(last_target_wake_up, tnow);
                                tv = select_timeout_interval.as_timeval();
#else
                                tv = to_context.after_js_event();
#endif
                                break;
                            }
#if 0
                            if (read(joy_fd, &event, sizeof(js_event)) == -1 && errno != EAGAIN) {
                                break;  // Joystick is probably closed. Definitely occurs.
                            }
#endif
                            //                    printf("Button event type: %d value: %d number: %d\n", event.type, event.value, event.number);
                            switch (event.type) {
                                case JS_EVENT_BUTTON | JS_EVENT_INIT:
                                    RBL_LOG_FMT("js_event_init \n");
                                case JS_EVENT_BUTTON:
                                    RBL_LOG_FMT("Button event  time: %d number: %d value: %d type: %d\n", event.time,
                                           event.number, event.value, event.type);
                                    break;
                                case JS_EVENT_AXIS | JS_EVENT_INIT:
                                    RBL_LOG_FMT("js_event_init \n");
                                case JS_EVENT_AXIS: {
#if 1
                                    RBL_LOG_FMT("Axes time:%f event number: %d value: %d type: %d\n", event.time / 1000.0,
                                           event.number, event.value, event.type);
#endif
                                    int ev_number = event.number;
                                    if (ev_number == AXIS_EVENT_LEFT_STICK_FWD_BKWD_EVENT_NUMBER) {
                                        this->left_stick_fwd_bkwd->add_js_event(event.time, event.value);
                                    } else if (ev_number == AXIS_EVENT_RIGHT_STICK_FWD_BKWD_EVENT_NUMBER) {
                                        this->right_stick_fwd_bkwd->add_js_event(event.time, event.value);
                                    } else {
                                        // ignore these events
                                    }
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
                }
            }

            close(joy_fd);
        }

        cleanup:
        printf("joy_node shut down.");
    }
} // namespace f710