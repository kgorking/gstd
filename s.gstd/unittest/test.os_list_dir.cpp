#include "doctest.h"
import std;
import gs;

TEST_CASE("test.os_list_dir") {
    bool saw_file = false;
    bool saw_directory = false;

    for (const auto& entry : os::list_dir("s.gstd/src/os")) {
        if (entry.name == "read_lines.cppm") {
            saw_file = true;
            CHECK(!entry.is_directory);
            CHECK(!entry.is_symlink);
        }

        if (entry.name == "windows") {
            // Parent directory entry
            CHECK (entry.is_directory);
            saw_directory = true;
        }
    }

    CHECK(saw_file);
    CHECK(saw_directory);
}

TEST_CASE("test.os_exists") {
    CHECK(os::exists("s.gstd/src/os"));
    CHECK(os::exists("s.gstd/src/os/read_lines.cppm"));
    CHECK(!os::exists("nonexistent/path"));
}
