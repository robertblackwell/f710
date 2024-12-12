#define GNU_SOURCE
#include "std_format.h"
#include <stdarg.h>

std::string std_format(const char* fmt, ...)
{
    char* bufptr;
    va_list args;
    va_start(args, fmt);
    size_t len = asprintf(&bufptr, fmt, args);
    va_end(args);
    auto s = std::string(bufptr, len);
    free(bufptr);
    return s;
}
