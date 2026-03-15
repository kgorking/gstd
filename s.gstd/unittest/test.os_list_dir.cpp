#include "doctest.h"
import std;
import gs;

TEST_CASE("test.os_list_dir") {
    bool saw_file = false;
    bool saw_directory = false;

    const std::filesystem::path source_dir{__FILE__};
    const std::filesystem::path project_root = source_dir.parent_path().parent_path();
    const std::filesystem::path os_src_dir = project_root / "src" / "os";

    for (const auto& entry : os::list_dir(os_src_dir.string())) {
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
    const std::filesystem::path source_dir{__FILE__};
    const std::filesystem::path project_root = source_dir.parent_path().parent_path();
    const std::filesystem::path os_src_dir = project_root / "src" / "os";

    CHECK(os::exists(os_src_dir.string()));
    CHECK(os::exists((os_src_dir / "read_lines.cppm").string()));
    CHECK(!os::exists("nonexistent/path"));
}
