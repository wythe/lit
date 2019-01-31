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
void trace(const json &j);
std::string def_dir();
std::string name();

json request_local(int fd, const json &req);

bool write_all(int fd, const void *data, size_t size);

json read_all(int fd);

bool has_error(const json &j);

std::string error_message(const json &j);

int connect_local(std::string const &dir, std::string const &filename);

void connect(auto &d, std::string const &dir, std::string const &filename) {
	d.fd = connect_local(dir, filename);
}
} // rpc
