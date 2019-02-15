#include "bc_rpc.h"
#include <wythe/common.h>
#include <string_view>

using string_view = std::string_view;

namespace rpc
{
namespace bitcoin
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

/*
 * Do this in lieu of a proper config file loader.
 */
static std::string get_config_value(auto first, auto last, string_view key)
{
	auto it{std::find_if(first, last, [&](string_view l){
		return wythe::starts_with(l, key);
	})};

	if (it == last)
		PANIC("no " << key << "found");
	auto l = wythe::split(*it, '=');
	if (l.size() != 2)
		PANIC("config value parse error for key " << key);
	return l[1];
}

static std::string def_dir()
{
	std::string path;

	const char *env{getenv("HOME")};
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.bitcoin";
	return path;
}

static std::string userpass()
{
	auto fn{def_dir() + "/bitcoin.conf"};
	auto file{wythe::read_file(fn)};
	auto lines{wythe::line_split(file.begin(), file.end())};
	auto v{get_config_value(lines.begin(), lines.end(), "rpcuser")};
	v += ':';
	v += get_config_value(lines.begin(), lines.end(), "rpcpassword");

	return v;
}

bd::bd()
{
	curl_assert(curl_global_init(CURL_GLOBAL_DEFAULT));
	c = curl_easy_init();
	if (!c)
		PANIC("curl init error");
	curl_assert(curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_callback));
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "content-type: text/plain;");
	curl_assert(curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers));
	curl_assert(
	    curl_easy_setopt(c, CURLOPT_URL, "http://127.0.0.1:18332/"));
	curl_assert(
	    curl_easy_setopt(c, CURLOPT_USERPWD, userpass().c_str()));
	curl_assert(curl_easy_setopt(c, CURLOPT_USE_SSL, CURLUSESSL_TRY));
}

bd::~bd()
{
	curl_global_cleanup();
}

static json request(const bd &bd, string_view method, const json &params)
{
	std::string raw;
	json j{{"jsonrpc", "1.0"},
	       {"id", name()},
	       {"method", std::string(method)},
	       {"params", params}};

	std::string s{j.dump()};

	curl_assert(curl_easy_setopt(bd.c, CURLOPT_POSTFIELDSIZE, s.size()));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_POSTFIELDS, s.data()));

	curl_assert(curl_easy_setopt(bd.c, CURLOPT_WRITEDATA, &raw));
	curl_assert(curl_easy_perform(bd.c));
	auto r = json::parse(raw);
	trace(r);

	auto err = r.at("error");
	if (err != nullptr)
		PANIC(method << " error: " << std::setw(4) << r);
	return r.at("result");
}

json getnetworkinfo(const bd &bd)
{
	return request(bd, "getnetworkinfo", json::array());
}

json getblockcount(const bd &bd)
{
	return request(bd, "getblockcount", json::array());
}

json gettxout(const bd &bd, string_view txid, int count)
{
	json j{std::string{txid}, count};
	return request(bd, "gettxout", j);
}

json getrawtransaction(const bd &bd, string_view txid)
{
	json j{std::string{txid}, 1};
	return request(bd, "getrawtransaction", j);
}

int confirmations(const bd &bd, string_view txid)
{
	auto h = getrawtransaction(bd, txid);
	return h.at("confirmations").get<int>();
}

} // bitcoin
} // rpc
