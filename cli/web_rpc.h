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
	CURL *c;
	std::string s;
};

json request(https &h, std::string_view url);
}
}
