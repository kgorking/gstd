export module gs:exec;

#ifdef _WIN32
import :exec_impl;
#else
import :exec_impl;
#endif
