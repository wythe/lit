#pragma once
#include <iosfwd>

void spdlog_info(std::ostringstream const &os);
void spdlog_warn(std::ostringstream const &os);
void spdlog_fatal(std::ostringstream const &os);

void set_log_level(const std::string & level);

#define l_info(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    spdlog_info(os); \
} while (0)

#define l_warn(desc) \
do { \
    std::ostringstream os; \
    os << desc; \
    spdlog_warn(os); \
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
    spdlog_fatal(os); \
} while (0)

