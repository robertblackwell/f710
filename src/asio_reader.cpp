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

template <class ContState>
f710::Reader<ContState>::Reader(std::string device_path,
    ContState* controller_state,
    std::function<void(ContState&)> on_event_function)
        : m_fd(open_fd_non_blocking(device_path)),
            m_io_context(boost::asio::io_context()),
            m_serial_port(boost::asio::serial_port(m_io_context, m_fd)),
            m_timer(m_io_context, std::chrono::milliseconds(500)),
            m_on_event_function(on_event_function),
            m_js_event(),
            m_axis_count(0),
            m_button_count(0),
            m_initialize_done(false),
            m_controller_state(controller_state)
{
    m_joy_dev_name = device_path;
    m_is_open = false;
    // m_controller_state = new ControllerState(
    //         AxisDevice(D_AXIS_LEFT_STICK_FWD_BKWD_NUMBER),
    //         AxisDevice(D_AXIS_RIGHT_STICK_FWD_BKWD_NUMBER), ToggleButton(D_BUTTON_A));
}
template <class ContState>
void f710::Reader<ContState>::start_read()
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
            m_controller_state->apply_event(m_js_event);
        } else {
            throw std::runtime_error("read error");
        }
        boost::asio::post(m_io_context, [this]() {this->start_read();});
    });
}
template <class ContState>
void f710::Reader<ContState>::operator()()
{
    boost::asio::post(m_io_context, [this]() {this->start_read();});
    boost::asio::post(m_io_context, [this]() {
        m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
    });
    m_io_context.run();
}

template <class ContState>
void f710::Reader<ContState>::run()
{
    boost::asio::post(m_io_context, [this]() {this->start_read();});
    boost::asio::post(m_io_context, [this]() {
        m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
    });
    m_io_context.run();
}
template <class ContState>
void f710::Reader<ContState>::handle_timer()
{
    m_on_event_function(*m_controller_state);
    boost::asio::post(m_io_context, [this]() {
        m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
    });
}
} // namespace f710}
