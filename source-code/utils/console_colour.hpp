#ifndef _CONSOLE_COLOUR_HPP
#define _CONSOLE_COLOUR_HPP

#include <sstream>
#include <string>
#include <iostream>

namespace sgcp {
    namespace Console {
        enum class Colour {
            Red = 31,
            Green = 32,
            Yellow = 33,
            Blue = 34,
            Magenta = 35,
            Default = 39
        };

        typename std::underlying_type<Colour>::type colour_code(const Colour& c);

        template<typename T>
        std::string colour(Colour c, T w) {
            std::stringstream ss;
            ss << "\033[" << colour_code(c) << "m" << w << "\033[" << colour_code(Colour::Default) << "m";
            return ss.str();
        }

        template<typename T>
        std::string colour_red(T w) { return colour(Colour::Red, w); }

        template<typename T>
        std::string colour_green(T w) { return colour(Colour::Green, w); }

        template<typename T>
        std::string colour_yellow(T w) { return colour(Colour::Yellow, w); }

        template<typename T>
        std::string colour_blue(T w) { return colour(Colour::Blue, w); }

        template<typename T>
        std::string colour_magenta(T w) { return colour(Colour::Magenta, w); }

        std::string yellow_separator();

        std::ostream& operator<<(std::ostream& out, const Colour& c);
    }
}

#endif
