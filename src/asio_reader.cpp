#include "f710_helpers.h"
#include "f710_time.h"
#include "f710_exceptions.h"
#include "asio_reader.h"
#include "model_defines.h"
#include <assert.h>
#include <cinttypes>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <rbl/simple_exit_guard.h>
#include <string>
#include <vector>

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

namespace f710 {
// void handle_init_event(js_event event, ControllerState* cstate);
// void handle_event(js_event event, ControllerState* cstate);

f710::Reader::Reader(std::string device_path, std::function<void(ControllerState&)> on_event_function)
        : m_fd(open_fd_non_blocking(device_path)),
            m_io_context(boost::asio::io_context()),
            m_serial_port(boost::asio::serial_port(m_io_context, m_fd)),
            m_timer(m_io_context, std::chrono::milliseconds(500)),
            m_controller_state(nullptr),
            m_on_event_function(on_event_function),
            m_js_event(),
            m_axis_count(0),
            m_button_count(0),
            m_initialize_done(false)
{
    m_joy_dev_name = device_path;
    m_is_open = false;

    // m_joy_dev = "";
    // m_fd = open_fd_non_blocking(m_joy_dev_name);
    m_controller_state = new ControllerState(
            AxisDevice(D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER),
            AxisDevice(D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER), ToggleButton(D_BUTTON_A));
}
void f710::Reader::start_read()
{
    auto b = boost::asio::mutable_buffer((char*)&m_js_event, sizeof(js_event));
    char bb[sizeof(js_event)];
    char* bbptr = &bb[0];

    boost::asio::async_read(m_serial_port, boost::asio::buffer(&(m_js_event), sizeof(js_event)), [this](const boost::system::error_code& ec, std::size_t length) {
        auto p = &this->m_js_event;
        if (!ec) {
            if (length != sizeof(js_event)) {
                throw std::runtime_error("Wrong size of js_event");
            }
            if(!m_initialize_done) {
                handle_init_event(m_js_event, m_controller_state);
            } else {
                handle_event(m_js_event, m_controller_state);
            }
        } else {
            throw std::runtime_error("read error");
        }
        boost::asio::post(m_io_context, [this]() {this->start_read();});
    });
}
void f710::Reader::operator()() {run();}

void f710::Reader::run()
{
    boost::asio::post(m_io_context, [this]() {this->start_read();});
    boost::asio::post(m_io_context, [this]() {
        m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
    });
    m_io_context.run();
}
void f710::Reader::handle_timer()
{
    // auto left_value = -1 * this->m_controller_state->m_left.get_latest_event().value;
    // auto right_value = -1 * this->m_controller_state->m_right.get_latest_event().value;
    // auto toggle = (1 == this->m_controller_state->m_button.get_latest_event().value);
    m_on_event_function(*m_controller_state);
    boost::asio::post(m_io_context, [this]() {
        m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
    });
    // tv = to_context.after_select_timedout();
}
void f710::Reader::handle_init_event(js_event event, ControllerState* cstate)
{
    switch (event.type) {
        case JS_EVENT_INIT | JS_EVENT_BUTTON:
            RBL_LOG_FMT("js_event_init_button time: %d number: %d value: %d type: %Xh",
                        event.time,
                        event.number, event.value, event.type);

            m_button_count++;
            break;
        case JS_EVENT_AXIS | JS_EVENT_INIT: {
            RBL_LOG_FMT("js_event_init_axis time:%f event number: %d value: %d type: %d",
                        event.time / 1000.0,
                        event.number, event.value, event.type);
            m_axis_count++;
        }
            break;
        default:
            throw F710WrongModeError();

    }
    RBL_LOG_FMT("initializing %d %d %d", ((int)m_initialize_done), m_button_count, m_axis_count)
    m_initialize_done = (m_button_count == 12)&&(m_axis_count == 6) ;
}
/**
 * Reads all events available on the f710_fd until an EAGAIN error in which case return 0
 * If any other io type error return -1
 */
void f710::Reader::handle_event(js_event event, ControllerState* cstate)
{
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
} // namespace f710}
