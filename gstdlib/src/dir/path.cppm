export module gs:path;
import :types;
import :string;
import :fmt;

namespace path {
    // Get the platform-specific path separator
    export consteval char get_path_separator() {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }

    // Check if a character is a path separator (handles both / and \ on Windows)
    inline bool is_separator(char c) {
        return c == '/' || c == '\\';
    }
    inline bool is_separator(string s) {
        if (s.size() != 1) return false;
        return s == "/" || s == "\\";
    }

    export string join(string a, string b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        char sep = get_path_separator();
        if (a.back() == sep) return a + b;
        return fmt("{}{}{}", a, sep, b);
    }

    // Extract the filename (last component) from a path
    export string get_filename(string path) {
        if (path.empty()) return path;
        
        int64 pos = path.find_last_of("/\\");
        if (pos == string::npos) return path;
        
        return path.substr(pos + 1);
    }

    // Extract the directory part of a path (everything before the last separator)
    export string get_directory(string path) {
        if (path.empty()) return path;
        
        int64 pos = path.find_last_of("/\\");
        if (pos == string::npos) return "";
        if (pos == 0) return path.substr(0, 1);  // Root directory
        
        return path.substr(0, pos);
    }

    // Extract the file extension (including the dot)
    export string get_extension(string path) {
        string filename = get_filename(path);
        if (filename.empty()) return "";
        
        int64 pos = filename.find_last_of('.');
        if (pos == string::npos || pos == 0) return "";  // No extension or hidden file without extension
        
        return filename.substr(pos);
    }

    // Remove the extension from a path
    export string remove_extension(string path) {
        string filename = get_filename(path);
        string extension = get_extension(path);
        
        if (extension.empty()) return path;
        
        // Remove extension from the end of the path
        return path.substr(0, path.count() - extension.count());
    }

    // Check if a path has an extension
    export bool has_extension(string path) {
        return !get_extension(path).empty();
    }

    // Decompose a path into directory and filename
    export struct PathInfo {
        string directory;
        string filename;
    };

    export PathInfo split_path(string path) {
        return PathInfo {
            get_directory(path),
            get_filename(path)
        };
    }

    // Decompose a filename into name and extension
    export struct FilenameInfo {
        string name;
        string extension;
    };

    export FilenameInfo split_filename(string path) {
        string filename = get_filename(path);
        string extension = get_extension(path);
        
        return FilenameInfo {
            filename.substr(0, filename.count() - extension.count()),
            extension
        };
    }

    // Normalize a path (handle both / and \ as separators, works on any platform)
    /*export string normalize_path(string path) {
        string result = path;
        char sep = get_path_separator();
        
        // Replace the non-preferred separator with the platform separator
        char other_sep = (sep == '/') ? '\\' : '/';
        for (int i = 0; i < result.count(); ++i) {
            // TODO
            if (result[i] == other_sep) {
                result[i] = sep;
            }
        }
        
        return result;
    }*/

    // Go up N levels in a path
    // Example: go_up_levels("/aa/bb/cc/dd/", 2) returns "/aa/bb/"
    export string go_up_levels(string path, int64 levels) {
        if (path.empty() || levels == 0) return path;

        string result = get_directory(path);
        bool had_trailing_sep = false;
        
        // Check if path ends with separator
        if (!result.empty() && is_separator(result.back())) {
            had_trailing_sep = true;
            result.pop_back();  // Remove trailing separator
        }
        
        // Go up the specified number of levels
        for (int64 i = 0; i < levels && !result.empty(); ++i) {
            // Find the last separator
            int64 pos = result.find_last_of("/\\");
            if (pos == string::npos) {
                // No more separators, we're at the top
                result.clear();
                break;
            }
            // Remove everything after the last separator
            result = result.substr(0, pos);
        }
        
        // Add back trailing separator if there was one and result is not empty
        if (had_trailing_sep && !result.empty()) {
            return result + get_path_separator();
        }
        
        return result;
    }
}
