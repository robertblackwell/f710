#ifndef f710_asio_reader_H
#define f710_asio_reader_H
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
#include "model.h"

namespace f710 {

    template <typename ContState>
    concept HasApplyEvent = requires(ContState csref, js_event  arg) {
        {csref.apply_event(arg)} -> std::same_as<void>;
    };

    template <HasApplyEvent ContState>
    class Reader {
        bool m_is_open;
        int m_fd;
        js_event m_js_event;
        std::function<void(ContState&)>& m_on_event_function;
        boost::asio::io_context m_io_context;
        boost::asio::serial_port m_serial_port;
        boost::asio::steady_timer m_timer;
        int m_button_count;
        int m_axis_count;
        bool m_initialize_done;
        int m_output_interval_ms;
        std::string m_joy_dev;
        std::string m_joy_dev_name;
        ContState *m_controller_state;
    public:
        Reader() = delete;
        explicit Reader(
            std::string device_path,
            ContState* controller_state,
            std::function<void(ContState& state)> on_event_function,
            int output_interval_ms = 500
        )
                : m_fd(open_fd_non_blocking(device_path)),
                m_io_context(boost::asio::io_context()),
                m_serial_port(boost::asio::serial_port(m_io_context, m_fd)),
                m_timer(m_io_context, std::chrono::milliseconds(output_interval_ms)),
                m_on_event_function(on_event_function),
                m_js_event(),
                m_axis_count(0),
                m_button_count(0),
                m_initialize_done(false),
                m_controller_state(controller_state),
                m_output_interval_ms(output_interval_ms)
        {
            m_joy_dev_name = device_path;
            m_is_open = false;
        }

        void run()
        {
            boost::asio::post(m_io_context, [this]() {this->start_read();});
            boost::asio::post(m_io_context, [this]() {
                m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
            });
            m_io_context.run();
        }
        void operator()(){run();};

    private:
        void start_read()
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

        void handle_timer()
        {
            m_on_event_function(*m_controller_state);
            const std::chrono::milliseconds ms{m_output_interval_ms};
            m_timer.expires_at(m_timer.expiry() + ms);
            m_timer.async_wait([this](const boost::system::error_code& ec){this->handle_timer();});
        }
    };
} //namespace

#endif