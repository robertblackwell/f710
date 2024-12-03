#include "f710.h"

int main(int argc, char **argv)
{
    f710::F710 collector{"js0"};
    int ix = collector.run();
    return 0;
}
