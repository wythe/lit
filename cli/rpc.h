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

inline std::string id(void)
{
	static std::string id{"lit-" + std::to_string(getpid())};
	return id;
}

// unix domain socket json rpc
struct uds_rpc {
	uds_rpc() {
		id = rpc::id();
	}
	~uds_rpc()
	{
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
	}

	uds_rpc(const uds_rpc &) = delete;
	uds_rpc &operator=(const uds_rpc &) = delete;
	std::string id;
	int fd = -1;
};

template <typename T>
void trace(T &j)
{
	if (g_json_trace)
		std::cerr << std::setw(4) << j << '\n';
}

std::string def_dir();
std::string name();

json request_local(int fd, const json &req);

bool has_error(const json &j);

std::string error_message(const json &j);

int connect(std::string_view dir, std::string_view filename);

} // rpc
