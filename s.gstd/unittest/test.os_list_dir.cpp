#include "doctest.h"
import std;
import gs;

const string source_dir{__FILE__};
const string project_root = path::go_up_levels(source_dir, 1);
const string os_src_dir = string::fmt("{}/src/os", project_root);

TEST_CASE("test.os_list_dir") {
    bool saw_file = false;
    bool saw_directory = false;

    for (const auto& entry : os::list_dir(os_src_dir)) {
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
    CHECK(os::exists(os_src_dir));
    CHECK(os::exists(os_src_dir + "/read_lines.cppm"));
    CHECK(!os::exists("nonexistent/path"));
}
