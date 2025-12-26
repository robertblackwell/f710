#include "f710_helpers.h"
#include "f710_time.h"
#include "f710_exceptions.h"
#include "reader.h"
#include "model_defines.h"
#include <assert.h>
#include <boost/asio/io_context.hpp>
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

namespace f710 {

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

} // namespace f710