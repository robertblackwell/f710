

#include "model.h"
#include "f710_time.h"
// #include "f710_helpers.h"
// #include "reader.h"
#include <assert.h>
// #include <boost/asio/io_context.hpp>
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
f710::AxisDevice::AxisDevice(int eventid) : event_number(eventid) {
    is_new_event = false;
    latest_event_time = 0;
    latest_event_value = 0;
}
/**
 * Records the most recent event
 * @param event_time  The time value in the most recent event
 * @param value       The stick position in the most recent event
 */
void f710::AxisDevice::add_js_event(js_event event) {
    if ((event.type != JS_EVENT_AXIS) || (event.number != event_number))
        return;
    Time tnow = Time::now();
    if (!is_new_event) {
        is_new_event = true;
    }
    latest_event_time = event.time;
    latest_event_value = event.value;
}
/**
 *  Returns {} if there is not a new event since the last call to this function
 *  Returns the event if there has been one or more new events since the last
 * call
 */
js_event f710::AxisDevice::get_latest_event() {
    is_new_event = false;
    js_event ev = {.time = latest_event_time,
                 .value = latest_event_value,
                 .type = JS_EVENT_AXIS,
                 .number = (__u8)event_number};
  return ev;
}

f710::ToggleButton::ToggleButton(int button_event) {
    event_number = button_event;
    event_type = JS_EVENT_BUTTON;
    event_state = EVENT_STATE_A;
    event_toggle_value = false;
    latest_event_time = 0;
    event_value = 0;
}

void f710::ToggleButton::apply_event(js_event event) {
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
void f710::ToggleButton::apply_init_event(js_event event) {
    apply_event(event);
}
js_event f710::ToggleButton::get_latest_event() {
    js_event ev = {.time = latest_event_time,
                 .value = (__s16)((event_toggle_value) ? 1 : 0),
                 .type = event_type,
                 .number = (__u8)event_number};
    return ev;
}

f710::ControllerState::ControllerState(AxisDevice left, AxisDevice right, ToggleButton button)
        : m_left(left), m_right(right), m_button(button)
{
}

void f710::ControllerState::apply_event(js_event event)
{

    m_left.add_js_event(event);
    m_right.add_js_event(event);
    m_button.apply_event(event);
}

void f710::ControllerState::apply_init_event(js_event event)
{
    m_left.add_js_event(event);
    m_right.add_js_event(event);
    m_button.apply_event(event);
}
