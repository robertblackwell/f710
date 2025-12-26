#ifndef H_f710_model_H
#define H_f710_model_H
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
#include <assert.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <cmath>
#include <sys/stat.h>
#include <unistd.h>
// #include "f710_time.h"
// #include "f710_exceptions.h"
namespace f710 {

    /**
     * This class represents one of the axis event sources on a F710
     *
     * The significant characteristic is that such a device sends a stream of events
     * as a stick is moved along one of its axes.
     *
     * Problem: the stream is very fast. That is interval between sucessive values can be
     * too fast for each event to be sent to a robot.
     *
     * The purpose of this class is to remember the most recent value
     */
    class AxisDevice {
        public:
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
    /// a press will change the value but will not be recognized as a second press until there has been
    /// a release.
    ///
    class ToggleButton {
        public:
        int event_number;
        __u8 event_type;
        int event_value;
        int event_state;
        uint32_t latest_event_time;
        bool event_toggle_value;
#define EVENT_STATE_A 11 //act on a 1 ignore a 0
#define EVENT_STATE_B 22 //ignore a 1 act on a 0
        ToggleButton(int button_event);
        void apply_event(js_event event);
        void apply_init_event(js_event event);
        js_event get_latest_event();
    };

    ///
    /// This class holds a selection of Axis and ToggleButton that represent the function on the F710
    /// in which our application has an interest.
    ///
    struct ControllerState {
        int button_count;
        int axis_count;

        AxisDevice m_left;
        AxisDevice m_right;
        ToggleButton m_button;
        ControllerState(AxisDevice left, AxisDevice right, ToggleButton button);
        bool initialization_done();
        void apply_event(js_event event);
        void apply_init_event(js_event event);
    };

} //namespace

#endif