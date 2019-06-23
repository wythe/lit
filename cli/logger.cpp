#include "logger.h"
#include "spdlog/spdlog.h"
#include <sstream>
void spdlog_info(std::ostringstream const &os)
{
	spdlog::info(os.str());
}

void spdlog_warn(std::ostringstream const &os)
{
	spdlog::warn(os.str());
}

void spdlog_fatal(std::ostringstream const &os)
{
	spdlog::error(os.str());
}

void set_log_level(const std::string &level)
{
}
