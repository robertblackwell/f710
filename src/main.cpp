#include "f710.h"
#include "f710_helpers.h"
int main(int argc, char **argv) {
    try {
        f710::F710 logitech_f710{"js0"};
        logitech_f710.run([](int left, int right) {
            printf("from main left: %d  right: %d\n", left, right);
        });
    } catch(const f710::F710Exception e) {
        printf("F710Exception");
    } catch(...) {
        printf("got an exception");
    }
    return 0;
}
