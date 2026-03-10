export module gs:pipes;

#ifdef _WIN32
import :pipes_impl;
#else
import :pipes_impl;
#endif
