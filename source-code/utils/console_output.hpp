#ifndef _CONSOLE_OUTPUT_HPP
#define _CONSOLE_OUTPUT_HPP

// When some shitty library decides it's ok to print out stuff on the console...
// I'll teach it with the following, ugly, non-portable code... yeah! (AS)

#include <cstdio>

#ifndef SUPPRESS_OUTPUT
    #define SUPPRESS_OUTPUT(x) \
    if(std::freopen("/dev/null", "w", stdout) && std::freopen("/dev/null", "w", stderr)) { \
        x \
        std::fclose(stdout); \
        std::fclose(stderr); \
        std::freopen("/dev/tty", "a", stdout); \
        std::freopen("/dev/tty", "a", stderr); \
    }
#endif

#endif