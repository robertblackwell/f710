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
        class Reader {
        public:
            Reader() = delete;
            explicit Reader(std::string device_path, std::function<void(ControllerState& state)> on_event_function);

            void run();
            void operator()();

        private:
            void start_read();
            void handle_timer();

            void handle_init_event(js_event event, ControllerState *cstate);
            void handle_event(js_event event, ControllerState *cstate);

            bool m_is_open;
            int m_fd;
            js_event m_js_event;
            std::function<void(ControllerState&)>& m_on_event_function;
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