module;
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
#endif
export module gs:dir;

import std;
import :string;
import :sequence;

namespace os {
    export sequence<string> list_dir(string path, bool recursive = false) {
#ifdef _WIN32
        std::string search{ path.to_string() };
        if (!search.empty() && search.back() != '\\' && search.back() != '/')
            search += "\\";
        search += "*";

        WIN32_FIND_DATAA data;
        HANDLE h = FindFirstFileA(search.c_str(), &data);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                string name(data.cFileName);
                if (name != "." && name != "..") {
                    co_yield name;
                    if (recursive) {
                        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            co_yield std::ranges::elements_of(list_dir(name, true));
                        }
                    }
                }
            } while (FindNextFileA(h, &data));
            FindClose(h);
        }
#else
        DIR* dir = opendir(path.c_str());
        if (dir) {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr) {
                string name(ent->d_name);
                if (name != "." && name != "..") {
                    co_yield name;

                    if (recursive && ent->d_type == DT_DIR) {
                        co_yield std::ranges::elements_of(list_dir(name, true));
                    }
                }
            }
            closedir(dir);
        }
#endif
    }

    export bool exists(string path) {
#ifdef _WIN32
        DWORD attr = GetFileAttributesA(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES;
#else
        return access(path.c_str(), F_OK) == 0;
#endif
    }
}
