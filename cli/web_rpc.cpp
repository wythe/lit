#include "web_rpc.h"

namespace rpc
{
namespace web
{
static size_t write_callback(void *contents, size_t size, size_t n,
			     std::string *s)
{
	size_t bytes = size * n;
	size_t old_size = s->size();
	s->resize(old_size + bytes);
	std::copy((char *)contents, (char *)contents + bytes,
		  s->begin() + old_size);
	return bytes;
}

json request(https &h, const std::string url)
{
	trace(url);
	curl_easy_setopt(h.c, CURLOPT_URL, url.c_str());
	auto r = curl_easy_perform(h.c);
	if (r != CURLE_OK)
		PANIC("curl fail: " << curl_easy_strerror(r));
	auto j = json::parse(h.s);
	if (j.find("result") != j.end()) {
		if (j.at("result").get<std::string>() == "error")
			PANIC(url << " error: " << std::setw(4) << j);
	}
	trace(j);
	return j;
}

https::https()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	c = curl_easy_init();
	if (!c)
		PANIC("internet error");
	curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, rpc::web::write_callback);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, &s);
}

https::~https() { curl_global_cleanup(); }
}
}
