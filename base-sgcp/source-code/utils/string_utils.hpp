#ifndef _STRING_UTILS_HPP
#define _STRING_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

namespace sgcp {
    namespace string_utils {
        template<typename Out>
        void split(const std::string &s, char delim, Out result) {
            std::stringstream ss;
            ss.str(s);
            std::string item;
            while (std::getline(ss, item, delim)) {
                *(result++) = item;
            }
        }

        std::vector<std::string> split(const std::string &s, char delim) {
            std::vector<std::string> elems;
            split(s, delim, std::back_inserter(elems));
            return elems;
        }
    }
}

#endif