#ifndef _STREAM_UTILS_HPP
#define _STREAM_UTILS_HPP

namespace sgcp {
    namespace stream_utils {
        template<class T>
        struct finally_close {
            T& s;
            finally_close(T& s) : s{s} {}
            ~finally_close() { s.close(); }
        };

        template<class T>
        finally_close<T> remember_to_close(T& s) { return finally_close<T>{s}; }
    }
}

#endif