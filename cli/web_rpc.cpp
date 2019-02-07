#include "web_rpc.h"

namespace rpc
{
namespace web
{
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

json request(https &h, const std::string_view url)
{
	trace(url);
	curl_assert(curl_easy_setopt(h.c, CURLOPT_URL, std::string(url).c_str()));
	h.s.clear();
	curl_assert(curl_easy_perform(h.c));
	auto j = json::parse(h.s);
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
				     rpc::web::write_callback));
	curl_assert(curl_easy_setopt(c, CURLOPT_WRITEDATA, &s));
}

https::~https()
{
	curl_global_cleanup();
}
}
}
