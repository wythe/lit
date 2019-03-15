#pragma once
#include <iosfwd>

void spdlog_info(std::ostringstream &os);

void set_log_level(const std::string & level);

#define l_info(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    spdlog_info(os); \
} while (0)

#define l_trace(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    spdlog_info(os); \
} while (0)

#define l_fatal(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    spdlog_info(os); \
} while (0)

