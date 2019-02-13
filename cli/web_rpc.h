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
};

json request(https &h, std::string_view url);

json priceinfo(const https &https);
}
}
