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

void trace(const json &j);

namespace rpc
{
std::string def_dir();
std::string name();

struct lightningd {
	lightningd();
	lightningd(const lightningd &) = delete;
	lightningd& operator=(const lightningd&) = delete;
	~lightningd();
	std::string id;
	void connect(std::string const &dir, std::string const &filename);
	int fd = -1;
	struct sockaddr_un addr;
};

struct https {
	https();
	https(const https &) = delete;
	https& operator=(const https&) = delete;
	~https();
	static size_t write_callback(void *contents, size_t size, size_t n,
				     std::string * s);
	CURL *c;
	std::string s;
};

json request_local(lightningd &c, const json &req);

inline json request_remote(https & h, const std::string url)
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

inline bool write_all(int fd, const void *data, size_t size)
{
	while (size) {
		ssize_t done;

		done = write(fd, data, size);
		if (done < 0 && errno == EINTR)
			continue;
		if (done <= 0)
			return false;
		data = (const char *)data + done;
		size -= done;
	}

	return true;
}

inline json read_all(lightningd &c)
{
	std::string v;
	v.reserve(360);
	char ch;
	int r;
	while (r = read(c.fd, &ch, 1)) {
		if (r <= 0)
			PANIC("error talking to lightningd");
		v.push_back(ch);
		int avail;
		ioctl(c.fd, FIONREAD, &avail);
		if (avail == 0)
			break;
	}
	return json::parse(v);
}

inline bool has_error(const json &j) { return (j.find("error") != j.end()); }

inline std::string error_message(const json &j)
{
	if (has_error(j))
		return j["error"]["message"];
}


}

