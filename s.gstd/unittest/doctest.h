#include "doctest_impl.h"
import gs;

namespace doctest {
    template <>
    struct StringMaker<string> {
        static String convert(string const& value) {
            return String(string::fmt("\"{}\"", value).c_str());
        }
    };
}