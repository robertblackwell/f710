#ifndef f710_H
#define f710_H
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>
#include <functional>
#include <dirent.h>
#include <boost/asio.hpp>
#include <fcntl.h>
#include <climits>
#include <vector>
#include <optional>
#include <assert.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <cmath>
#include <sys/stat.h>
#include <unistd.h>
#include "f710_time.h"
#include "f710_exceptions.h"

namespace f710 {

    struct AxisDevice;

    /**
     * This class represents one of the axis event sources on a F710
     *
     * The significant characteristic is that such a device sends a stream of events
     * as a stick is moved along one of its axes.
     *
     * Problem: the stream is very fast. That is interval between sucessive values can be
     * too fast for each event to be sent to a robot.
     *
     * The purpose of this class is to process that stream into a slower stream
     * of values/events.
     */
    struct AxisDevice {
        uint32_t latest_event_time;
        int16_t latest_event_value;
        bool is_new_event;
        int event_number;

        AxisDevice() = delete;
        explicit AxisDevice(int eventid);
        /**
         * Records the most recent event
         */
        void add_js_event(js_event event);
        /**
         *  Returns {} if there is not a new event since the last call to this function
         *  Returns the event if there has been one or more new events since the last call
         */
        js_event get_latest_event();
    };
        ///
        /// This class turns a button on an f710 controller into a toggle switch.
        /// one press and release will turn the button "on" and the next press and release
        /// will turn the button "off". The push and release can be fast or slow.
        /// a press will change the value but will not be recognised as a second press until there has been
        /// a release.
        ///
        struct ToggleButton {
            int event_number;
            __u8 event_type;
            int event_value;
            int event_state;
            uint32_t latest_event_time;
            bool event_toggle_value;
#define EVENT_STATE_A 11 //act on a 1 ignore a 0
#define EVENT_STATE_B 22 //ignore a 1 act on a 0

            ToggleButton(int button_event) {
                event_number = button_event;
                event_type = JS_EVENT_BUTTON;
                event_state = EVENT_STATE_A;
                event_toggle_value = false;
                latest_event_time = 0;
                event_value = 0;
            }

            void apply_event(js_event event) {
                if ((event.type != JS_EVENT_BUTTON) || (event.number != event_number))
                    return;
                latest_event_time = event.time;
                event_value = event.value;
                switch (event_state) {
                    case EVENT_STATE_A:
                        if (event.value == 1) {
                            event_state = EVENT_STATE_B;
                            event_toggle_value = !event_toggle_value;
                        }
                        break;
                    case EVENT_STATE_B:
                        if (event.value == 0) {
                            event_state = EVENT_STATE_A;
                        }
                        break;
                    default:
                        assert(0);
                }

            }

            void apply_init_event(js_event event) {
                apply_event(event);
            }

            js_event get_latest_event() {
                js_event ev = {.time=latest_event_time, .value=(__s16) ((event_toggle_value) ? 1
                                                                                             : 0), .type=event_type, .number=(__u8) event_number};
                return ev;
            }

        };

        ///
        /// This class holds a selection of Axis and ToggleButton that represent the function on the F710
        /// in which our application has an interest.
        ///
        struct ControllerState {
            AxisDevice m_left;
            AxisDevice m_right;
            ToggleButton m_button;

            ControllerState(AxisDevice left, AxisDevice right, ToggleButton button)
                    : m_left(left), m_right(right), m_button(button) {

            }

            void apply_event(js_event event) {
                m_left.add_js_event(event);
                m_right.add_js_event(event);
                m_button.apply_event(event);
            }

            void apply_init_event(js_event event) {
                m_left.add_js_event(event);
                m_right.add_js_event(event);
                m_button.apply_event(event);
            }
        };

        class F710 {
        public:
            F710() = delete;
            explicit F710(std::string device_path);

            void run(std::function<void(int, int, bool)> on_event_function);

        private:
            void start_read();
            void handle_read(const boost::system::error_code& ec, std::size_t length) const;
            void handle_timer();

            void handle_init_event(js_event event, ControllerState *cstate);
            void handle_event(js_event event, ControllerState *cstate);

            int read_init_events(int fd, ControllerState *cstate);
            int read_events(int fd, ControllerState *cstate);

            bool m_is_open;
            int m_fd;
            js_event m_js_event;
            std::function<void(int, int, bool)> m_on_event_function;
            boost::asio::io_context m_io_context;
            boost::asio::serial_port m_serial_port;
            boost::asio::steady_timer m_timer;
            int m_button_count;
            int m_axis_count;
            bool m_initialize_done;
            std::string m_joy_dev;
            std::string m_joy_dev_name;
            ControllerState *m_controller_state;
        };
} //namespace

#endif