#pragma once
#include <sstream>
#include <iostream>
#include <stdexcept>
#ifdef ANDROID
#include <android/log.h>
#endif

namespace wythe {
inline std::runtime_error create_exception(const std::string &desc)
{
	return std::runtime_error(desc);
}
inline std::runtime_error create_exception(const std::string &desc,
					   const char *file, int line)
{
	std::ostringstream os;
	os << '[' << file << ':' << line << "] " << desc;
	return std::runtime_error(os.str());
}
}

// Throw a std::runtime_error with description.
#define FATAL(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    throw wythe::create_exception(os.str()); \
} while (0)

// Throw a std::runtime_error with description and source file and line number added.
#define PANIC(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    throw wythe::create_exception(os.str(), __FILE__, __LINE__); \
} while (0)

