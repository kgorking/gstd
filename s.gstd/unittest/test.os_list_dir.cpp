#include "doctest.h"
import std;
import gs;

TEST_CASE("test.os_list_dir") {
    bool saw = false;
    for (string name : os::list_dir("s.gstd/src/os")) {
        if (name == "read_lines.cppm") {
            saw = true;
            break;
        }
    }
    CHECK(saw);
}

TEST_CASE("test.os_exists") {
    CHECK(os::exists("s.gstd/src/os"));
    CHECK(os::exists("s.gstd/src/os/read_lines.cppm"));
    CHECK(!os::exists("nonexistent/path"));
}
