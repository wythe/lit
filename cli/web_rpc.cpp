#include "web_rpc.h"
#include "rpc_hosts.h"

namespace lit {
namespace web {
static size_t write_callback(void *contents, size_t size, size_t n,
			     std::string *s)
{
	size_t bytes{size * n};
	size_t old_size{s->size()};
	s->resize(old_size + bytes);
	std::copy((char *)contents, (char *)contents + bytes,
		  s->begin() + old_size);
	return bytes;
}

json request(const https &h, const std::string_view url)
{
	std::string s;
	trace(url);
	curl_assert(
	    curl_easy_setopt(h.c, CURLOPT_URL, std::string(url).c_str()));
	curl_assert(curl_easy_setopt(h.c, CURLOPT_WRITEDATA, &s));
	curl_assert(curl_easy_perform(h.c));
	auto j = json::parse(s);
	auto it = j.find("result");
	if (it != j.end() && *it == "error")
		PANIC(url << " error:\n" << std::setw(4) << j);
	trace(j);
	return j;
}

https::https()
{
	curl_assert(curl_global_init(CURL_GLOBAL_DEFAULT));
	c = curl_easy_init();
	if (!c)
		PANIC("curl init error");

	curl_assert(curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L));
	curl_assert(curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L));
	curl_assert(curl_easy_setopt(c, CURLOPT_WRITEFUNCTION,
				     write_callback));
}

https::~https()
{
	curl_global_cleanup();
}

json priceinfo(const https &https)
{
	return request(https, "https://api.gemini.com/v1/pubticker/btcusd");
}

node_list get_1ML_connected(const lit::hosts &hosts)
{
	auto url = std::string{"https://1ml.com"};
	if (is_testnet(hosts.ld))
		url += "/testnet";

	node_list nodes;
	url += "/node?order=channelcount&active=true&public=true&json=true";
	auto j = request(hosts.https, url);
	for (auto &n : j) {
		nodes.emplace_back(
		    n.at("pub_key").get<std::string>(),
		    n.at("alias").get<std::string>(),
		    n.at("addresses").at(0).at("addr").get<std::string>());
	}
	return nodes;
}
}
}
