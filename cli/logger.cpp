#include "logger.h"
#include "spdlog/spdlog.h"
#include <sstream>
void spdlog_info(std::ostringstream &os)
{
	spdlog::info(os.str());
}

void set_log_level(const std::string &level)
{
}
