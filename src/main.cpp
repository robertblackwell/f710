#include "f710_helpers.h"
#include "f710_exceptions.h"
#include <format>
#include <chrono>
#ifdef ASIO_READER
#include "asio_reader.h"
#else
#include "reader.h"
#endif
float scale(bool high_gear, int value) {
    if (value == 0) {
        return 0.0;
    }
    auto multiplier = (value < 0) ? -1: 1;
    value = value * multiplier;
    float pwm;
    if (high_gear) {
        pwm = multiplier * (round(((float) value / (float) INT16_MAX) * (85.0 - 50.0)) + 50.0);
    } else {
        pwm = multiplier * (round(((float) value / (float) INT16_MAX) * (60.0 - 30.0)) + 30.0);
    }
    return pwm;
}
std::string format_time_now()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%M:%S", tm);
    return buffer;
}
void cb(f710::ControllerState& state) {
    auto left = state.m_left.latest_event_value;
    auto right = state.m_right.latest_event_value;
    auto onoff = state.m_button.event_toggle_value;

    auto pwm_left = scale(onoff, left);
    auto pwm_right = scale(onoff, right);

    auto tn = format_time_now();
    printf("from main %s left: %d pwm_left: %f  right: %d pwm_right: %f toggle: %d\n",
        tn.c_str(),
        left, pwm_left, right, pwm_right, (int)onoff);
}
int main(int argc, char **argv) {
    try {
        std::string js_name = "js";
        f710::Reader logitech_f710{js_name, cb};
        logitech_f710();

    } catch(const f710::F710Exception e) {
        printf("F710Exception %s", e.what());
    } catch(...) {
        printf("got an exception");
    }
    return 0;
}
