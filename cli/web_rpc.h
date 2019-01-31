#pragma once
#include "rpc.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace rpc
{
namespace web
{
struct https {
	https();
	https(const https &) = delete;
	https &operator=(const https &) = delete;
	~https();
	static size_t write_callback(void *contents, size_t size, size_t n,
				     std::string *s);
	CURL *c;
	std::string s;
};

json request(https &h, const std::string url);

}
}
