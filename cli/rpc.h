#pragma once
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wythe/exception.h>

using json = nlohmann::json;

extern bool g_json_trace;

namespace rpc
{

// unix domain socket json rpc
struct uds_rpc {
	uds_rpc(const std::string & prefix);
	uds_rpc(const uds_rpc &) = delete;
	uds_rpc &operator=(const uds_rpc &) = delete;
	~uds_rpc();
	std::string id;
	int fd = -1;
};

void trace(const json &j);
std::string def_dir();
std::string name();

json request_local(int fd, const json &req);

json read_all(int fd);

bool has_error(const json &j);

std::string error_message(const json &j);

int connect(std::string const &dir, std::string const &filename);

} // rpc
