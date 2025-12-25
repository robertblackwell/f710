#include "f710.h"
#include "f710_helpers.h"
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
int main(int argc, char **argv) {
    try {
        f710::F710 logitech_f710{"js0"};
        logitech_f710.run([](int left, int right, bool onoff) {
            printf("from main left: %d right: %d toggle: %d\n", left, right, (int)onoff);
#if 1
            auto pwm_left = scale(onoff, left);
            auto pwm_right = scale(onoff, right);
            printf("from main left: %d pwm_left: %f  right: %d pwm_right: %f toggle: %d\n", left, pwm_left, right, pwm_right, (int)onoff);
#endif
        });
    } catch(const f710::F710Exception e) {
        printf("F710Exception %s", e.what());
    } catch(...) {
        printf("got an exception");
    }
    return 0;
}
