#ifndef f710_reader_H
#define f710_reader_H
#include <string>
#include <functional>
#include "model.h"

namespace f710 {

        class ControllerStateInterface {
          virtual ~ControllerStateInterface();
          virtual void apply_event(js_event);
          virtual void apply_init_event(js_event);
        };

        class Reader {
        public:
            Reader() = delete;

            explicit Reader(std::string device_path,std::function<void(ControllerState& csref)> on_event_function);

            void run();
            void operator()();

        private:
            int read_init_events(int fd, ControllerState* cstate);
            int read_events(int fd, ControllerState* cstate);

            bool m_is_open;
            int m_fd;
            int m_button_count;
            int m_axis_count;
            bool m_initialize_done;
            std::function<void(ControllerState& csref)> m_on_event_function;
            std::string m_joy_dev;
            std::string m_joy_dev_name;
            f710::ControllerState *m_controller_state;
        };
} //namespace

#endif