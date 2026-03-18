module;
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif
export module gs:dir;

import std;
import :string;
import :sequence;

namespace os {
    export class DirectoryEntry {
    public:
        string name;
        std::chrono::system_clock::time_point last_write_time;
        bool is_directory = false;
        bool is_symlink = false;

        DirectoryEntry() = default;
        
        explicit DirectoryEntry(string n) : name(std::move(n)) {}
    };

    export sequence<DirectoryEntry> list_dir(string path, bool recursive = false) {
#ifdef _WIN32
        string search;
        if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
            search = path + "*";
        else
            search = path + "\\*";

        WIN32_FIND_DATAA data;
        HANDLE h = FindFirstFileA(search.c_str(), &data);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                string name(data.cFileName);
                if (name != "." && name != "..") {
                    DirectoryEntry entry(name);
                    entry.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    entry.is_symlink = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                    
                    // Convert Windows FILETIME to system_clock::time_point
                    ULARGE_INTEGER uli;
                    uli.LowPart = data.ftLastWriteTime.dwLowDateTime;
                    uli.HighPart = data.ftLastWriteTime.dwHighDateTime;
                    // Windows FILETIME is 100-nanosecond intervals since 1601-01-01
                    // Convert to Unix epoch (1970-01-01)
                    const auto windows_tick = 10000000LL;
                    const auto sec_to_unix_epoch = 11644473600LL;
                    auto timestamp = (uli.QuadPart / windows_tick) - sec_to_unix_epoch;
                    entry.last_write_time = std::chrono::system_clock::from_time_t(timestamp);
                    
                    co_yield entry;
                    if (recursive && entry.is_directory) {
                        co_yield std::ranges::elements_of(list_dir(name, true));
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
                    DirectoryEntry entry(name);
                    entry.is_directory = (ent->d_type == DT_DIR);
                    entry.is_symlink = (ent->d_type == DT_LNK);
                    
                    // Get detailed file metadata using stat
                    string full_path = path + "/" + name;
                    struct stat stat_buf;
                    if (stat(full_path.c_str(), &stat_buf) == 0) {
                        entry.last_write_time = std::chrono::system_clock::from_time_t(stat_buf.st_mtime);
                    }
                    
                    co_yield entry;
                    if (recursive && entry.is_directory) {
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
