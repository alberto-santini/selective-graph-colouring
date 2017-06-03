#include "console_colour.hpp"

namespace sgcp {
    namespace Console {
        typename std::underlying_type<Colour>::type colour_code(const Colour& c) {
            return static_cast<typename std::underlying_type<Colour>::type>(c);
        }

        std::ostream& operator<<(std::ostream& out, const Colour& c) {
            return out << "\033[" << colour_code(c) << "m";
        }

        std::string yellow_separator() {
            return colour_yellow("====================");
        }
    }
}
