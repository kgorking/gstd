import std;
import gs;
import gs.testing;

const string source_dir{__FILE__};
const string project_root = path::go_up_levels(source_dir, 1);
const string os_src_dir = string::fmt("{}/src/os", project_root);

auto os_list_dir_test = [] -> test {
    bool saw_file = false;
    bool saw_directory = false;

    for (const auto& entry : os::list_dir(os_src_dir)) {
        if (entry.name == "read_lines.cppm") {
            saw_file = true;
            test::assert(!entry.is_directory, "read_lines.cppm should not be a directory");
            test::assert(!entry.is_symlink, "read_lines.cppm should not be a symlink");
        }

        if (entry.name == "windows") {
            // Parent directory entry
            test::assert(entry.is_directory, "windows should be a directory");
            saw_directory = true;
        }
    }

    test::assert(saw_file, "should have seen read_lines.cppm");
    test::assert(saw_directory, "should have seen windows directory");
};

auto os_exists_test = [] -> test {
    test::assert(os::exists(os_src_dir), "os_src_dir should exist");
    test::assert(os::exists(os_src_dir + "/read_lines.cppm"), "read_lines.cppm should exist");
    test::assert(!os::exists("nonexistent/path"), "nonexistent path should not exist");
};
