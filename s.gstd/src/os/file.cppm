export module gs:file;

#ifdef _WIN32
import :file_impl;
#else
import :file_impl;
#endif
