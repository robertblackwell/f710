//
// Created by robert on 12/4/24.
//

#ifndef F710_SIMPLE_EXIT_GUARD_H
#define F710_SIMPLE_EXIT_GUARD_H
#include <functional>
namespace exit_guard {
    struct Guard {
        std::function<void()> f;

        Guard(std::function<void()> myf) : f(myf) {}

        ~Guard() {
            f();
        }
    };
}

#endif //F710_SIMPLE_EXIT_GUARD_H
