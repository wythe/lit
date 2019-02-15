#pragma once
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wythe/exception.h>

#define curl_assert(r) do {\
if (r != CURLE_OK) \
	PANIC("curl fail: " << curl_easy_strerror(r)); \
} while(0); 

using json = nlohmann::json;

extern bool g_json_trace;

namespace rpc
{

// the unique name of the program, used as id for json.
inline std::string name()
{
	static std::string id{"lit-" + std::to_string(getpid())};
	return id;
}

template <typename T>
void trace(T &j)
{
	if (g_json_trace)
		std::cerr << std::setw(4) << j << '\n';
}

} // rpc
