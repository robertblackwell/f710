
// \author: Blaise Gassend
#include "f710.h"
#include "f710_helpers.h"
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>
#include <rbl/simple_exit_guard.h>
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
#include <functional>
#include <rbl/logger.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////
/// The following constants are applicable to Direct Mode - when the toggle on the front of the
/// controller points at the 'D'.
/// In this mode there are 3 AXIS input providers and 12 buttons.
///
/// #define values for event types are in <linux/joystick.h>
///
/// Events are identified by the pair (event_type, event_number)
///
//////////////////////////////////////////////////////////////////////////////////////////////////////
enum class DModeAxisEventNumber {
    RIGHT_STICK_LEFT_RIGHT = 2,
    RIGHT_STICK_FWD_BKWD = 3,
    LEFT_STICK_LEFT_RIGHT = 0,
    LEFT_STICK_FWD_BKWD = 1,
    CROSS_LEFT_RIGHT = 4,
    CROSS_FWD_BKWD = 5,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// The following constants are applicable to Direct Mode - when the toggle on the front of the
/// controller points at the 'D'.
/// In this mode there are 3 AXIS input providers and 12 buttons.
//////////////////////////////////////////////////////////////////////////////////////////////////////
// axis events
#define D_AXIS_CROSS_LEFT_RIGHT_NUMBER 4
#define D_AXIS_CROSS_FWD_BKWD_NUMBER 5

#define D_AXIS_RIGHT_STICK_LEFT_RIGHT_NUMBER 2
#define D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER 3
#define D_AXIS_LEFT_STICK_LEFT_RIGHT_NUMBER 0
#define D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER 1
enum class DModeButtonEventNumber {
    /// Button events only have 0 or 1 for values and the value reverts to 0
    /// as soon as the button is released. If you want to use a button as an on-off
    /// awitch you have to remember when it goes to 1
    ///
    ///
    /// Buttons in the circle on the left front of the coroller
    ///
    X = 0,
    A = 1,
    B = 2,
    Y = 3,
    ///
    /// Buttons on the RIGHT front/leading face of the controller.
    /// They are labelled LB and LT. Notice the labels are upside down
    /// and the one labelled LB is actually on the top while LT is on
    /// the bottom when holding the controller in the "useing" position.
    ///
    LB = 4,
    LT = 6,
    ///
    /// Buttons on the RIGHT front/leading face of the controller.
    /// They are labelled RB and RT. Notice the labels are upside down
    /// and the one labelled RB is actually on the top while RT is on
    /// the bottom when holding the controller in the "useing" position
    ///
    RB = 5,
    RT = 7,

    BACK = 8,
    START = 9,
    ///
    /// The left and right sticks have a push button function as given below
    ///
    LEFT_STICK_PUSH = 10,
    RIGHT_STICK_PUSH = 11,
};
// button events
/// Button events only have 0 or 1 for values and the value reverts to 0
/// as soon as the button is released. If you want to use a button as an on-off
/// awitch you have to remember when it goes to 1
///
///
/// Buttons in the circle on the left front of the coroller
///
#define D_BUTTON_X  0
#define D_BUTTON_A  1
#define D_BUTTON_B  2
#define D_BUTTON_Y  3
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled LB and LT. Notice the labels are upside down
/// and the one labelled LB is actually on the top while LT is on
/// the bottom when holding the controller in the "useing" position.
///
#define D_BUTTON_LB 4
#define D_BUTTON_LT 6
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled RB and RT. Notice the labels are upside down
/// and the one labelled RB is actually on the top while RT is on
/// the bottom when holding the controller in the "useing" position
///
#define D_BUTTON_RB 5
#define D_BUTTON_RT 7

#define D_BUTTON_BACK 8
#define D_BUTTON__START 9
///
/// The left and right sticks have a push button function as given below
///
#define D_BUTTON_LEFT_STICK_PUSH 10
#define D_BUTTON_RIGHT_STICK_PUSH_11
////////////////////////////////////////////////////////////////////////////////////////////////////////
/// When the button on the front of the conttroller is in the X position the controller and driver
/// will operate in X mode.
/// In this mode there are 8 axis controls -
/// -   left stick FWD-BWD, left stick LEFT-RIGHT, right stick FWD-BWD, right stick LEFT-RIGHT
/// -   cross LEFT-RIGHT, cros FWD-BCKWD
/// -   LT hold down (negative values only) RT hold down (negative values only)
/// and 10 buttons, they are:
/// -   A,B,X,Y, START, BACK, LB, RB, left stick PUSH, right stick PUSH
//////////////////////////////////////////////////////////////////////////////////////////////////////
// axis events

#define X_AXIS_LEFT_STICK_LEFT_RIGHT_NUMBER 0
#define X_AXIS_LEFT_STICK_FWD_BKWD_NUMBER 1

#define X_AXIS_RIGHT_STICK_LEFT_RIGHT_NUMBER 3
#define X_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER 4

#define X_AXIS_CROSS_LEFT_RIGHT_NUMBER 6
#define X_AXIS_CROSS_FWD_BKWD_NUMBER 7

#define X_AXIS_LT 4
#define X_AXIS_RT 5


// button events
/// Button events only have 0 or 1 for values and the value reverts to 0
/// as soon as the button is released. If you want to use a button as an on-off
/// awitch you have to remember when it goes to 1
///
///
/// Buttons in the circle on the left front of the coroller
///
#define X_BUTTON_X  2
#define X_BUTTON_A  0
#define X_BUTTON_B  1
#define X_BUTTON_Y  3
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled LB and LT. Notice the labels are upside down
/// and the one labelled LB is actually on the top while LT is on
/// the bottom when holding the controller in the "useing" position.
///
#define X_BUTTON_LB 4
///
/// Buttons on the RIGHT front/leading face of the controller.
/// They are labelled RB and RT. Notice the labels are upside down
/// and the one labelled RB is actually on the top while RT is on
/// the bottom when holding the controller in the "useing" position
///
#define X_BUTTON_RB 5

#define X_BUTTON_BACK 6
#define X_BUTTON_START 7
///
/// The left and right sticks have a push button function as given below
///
#define X_BUTTON_LEFT_STICK_PUSH  9
#define X_BUTTON_RIGHT_STICK_PUSH 10



#define CONST_SELECT_TIMEOUT_INTERVAL_MS 500
#define CONST_SELECT_TIMEOUT_EPSILON_MS 5

namespace f710 {
    AxisDevice::AxisDevice(int eventid)
            : event_number(eventid)
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
    void AxisDevice::add_js_event(js_event event)
    {
        if((event.type != JS_EVENT_AXIS) || (event.number != event_number))
            return;
        Time tnow = Time::now();
        if(!is_new_event) {
            is_new_event = true;
        }
        latest_event_time = event.time;
        latest_event_value = event.value;
    }
    /**
     *  Returns {} if there is not a new event since the last call to this function
     *  Returns the event if there has been one or more new events since the last call
     */
    js_event AxisDevice::get_latest_event()
    {
        is_new_event = false;
        js_event ev = {.time=latest_event_time, .value=latest_event_value, .type=JS_EVENT_AXIS, .number=(__u8) event_number};
        return ev;
    }

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
    struct AxisDevice {
        uint32_t latest_event_time;
        int16_t  latest_event_value;
        bool     is_new_event;
        int event_id;

        AxisDevice() = default;

        AxisDevice(int eventid)
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
         *  Returns {} if there is not a new event since the last call to this function
         *  Returns the event if there has been one or more new events since the last call
         */
        js_event get_latest_event()
        {
            is_new_event = false;
            js_event ev = {.time=latest_event_time, .value=latest_event_value, .type=JS_EVENT_AXIS, .number=(__u8) event_id};
            return ev;
        }

    };
    struct ControllerState {
        AxisDevice  m_left;
        AxisDevice  m_right;
        void apply_event(js_event event)
        {
            int ev_number = event.number;
            if (ev_number == m_left.event_id) {
                m_left.add_js_event(event.time, event.value);
            } else if (ev_number == m_right.event_id) {
                m_right.add_js_event(event.time, event.value);
            } else {
                // ignore these events
            }
        }
        void apply_init_event(js_event event)
        {
            int ev_number = event.number;
            if (ev_number == m_left.event_id) {
                m_left.add_js_event(event.time, event.value);
            } else if (ev_number == m_right.event_id) {
                m_right.add_js_event(event.time, event.value);
            } else {
                // ignore these events
            }
        }
    };
#endif
    f710::F710::F710(std::string device_path)
            : m_fd(-1), m_controller_state(nullptr), m_axis_count(0), m_button_count(0),m_initialize_done(false)
    {
        m_joy_dev_name = device_path;
        m_is_open = false;
        m_joy_dev = "";
        m_controller_state = new ControllerState(
                AxisDevice(D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER),
                AxisDevice(D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER), ToggleButton(D_BUTTON_A));
    }

    void f710::F710::run(std::function<void(int, int, bool)> on_event_function)
    {
        fd_set set;
        int f710_fd;
        this->m_is_open = false;
        f710_fd = open_fd_non_blocking(m_joy_dev_name);
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
                auto left_value = -1 * this->m_controller_state->m_left.get_latest_event().value;
                auto right_value = -1 * this->m_controller_state->m_right.get_latest_event().value;
                auto toggle = (1 == this->m_controller_state->m_button.get_latest_event().value);
                on_event_function(left_value, right_value, toggle);
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
    int f710::F710::read_init_events(int f710_fd, ControllerState* cstate)
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
    int f710::F710::read_events(int f710_fd, ControllerState* cstate)
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