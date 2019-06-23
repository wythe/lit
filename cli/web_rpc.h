#pragma once
#include "rpc.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "ln_rpc.h"


namespace lit {
struct hosts;

namespace web {
struct https {
	https();
	https(const https &) = delete;
	https &operator=(const https &) = delete;
	~https();
	CURL *c;
};

json request(https &h, std::string_view url);

json priceinfo(const https &https);

node_list get_1ML_connected(const hosts &hosts);
}
}
