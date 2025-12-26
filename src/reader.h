#ifndef f710_reader_H
#define f710_reader_H
#include <string>
#include <functional>
#include <concepts>
#include <rbl/simple_exit_guard.h>
#include "timeout_context.h"
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
            int m_button_count;
            int m_axis_count;
            bool m_initialize_done;
            int m_output_interval_ms;
            std::function<void(ContState& csref)> m_on_event_function;
            std::string m_joy_dev;
            std::string m_joy_dev_name;
            ContState *m_controller_state;
        public:
            Reader() = delete;
            explicit Reader(
                std::string device_path,
                ContState* controller_state,
                std::function<void(ContState& csref)> on_event_function,
                int output_interval_ms = 500
            )
                        : m_fd(-1), m_axis_count(0), m_button_count(0),m_initialize_done(false),
                        m_on_event_function(on_event_function), m_controller_state(controller_state),
                        m_output_interval_ms(output_interval_ms)
            {
                m_joy_dev_name = device_path;
                m_is_open = false;
                m_joy_dev = "";
            }

            void run()
            {
                fd_set set;
                int f710_fd = open_fd_non_blocking(m_joy_dev_name);
                this->m_is_open = false;
                exit_guard::Guard guard([f710_fd]() {close(f710_fd);});
                SelectTimeoutContext to_context(CONST_SELECT_TIMEOUT_INTERVAL_MS, m_output_interval_ms);
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
                            js_event event;
#ifdef F710_READLOOP
                            ///
                            /// This block reads all available events and may get stuck here is the driver keeps moving
                            /// one of the axis controls
                            ///
                            bool eagain_break = false;
                            while (!eagain_break) {
                                int nread = read(f710_fd, &event, sizeof(js_event));
                                int save_errno = errno;
                                if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                                    throw F710ReadIOError();
                                } else if (nread > 0) {
                                    assert(nread == sizeof(js_event));
                                    m_controller_state->apply_event(event);
                                    tv = to_context.after_js_event();
                                } else if (nread == -1) {
                                    eagain_break = true;;
                                }
                            }
#else
                            ///
                            /// This reads only a single event before checking select - this increases the
                            /// number of select calls a lot
                            ///
                            int nread = read(f710_fd, &event, sizeof(js_event));
                            int save_errno = errno;
                            if ((nread == 0) || ((nread == -1) && save_errno != EAGAIN)) {
                                throw F710ReadIOError();
                            } else if (nread > 0) {
                                assert(nread == sizeof(js_event));
                                m_controller_state->apply_event(event);
                                tv = to_context.after_js_event();
                            }
#endif
                        }
                    }
                }
                close(f710_fd);
            }

            void operator()(){run();};

        };
} //namespace

#endif