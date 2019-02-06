#include "ln_rpc.h"
#include <wythe/common.h>

namespace rpc
{
namespace ln
{

std::string def_dir()
{
	std::string path;

	const char *env = getenv("HOME");
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.lightning";
	return path;
}

json peers(const uds_rpc &ld)
{
	json j{{"id", id()}, {"method", "listpeers"}, {"params", {nullptr}}};
	return request_local(ld.fd, j)["result"];
}

json nodes(const uds_rpc &ld)
{
	json j{{"id", id()},
	       {"jsonrpc", "2.0"},
	       {"method", "listnodes"},
	       {"params", {nullptr}}};
	return request_local(ld.fd, j)["result"];
}
} //ln

namespace btc
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

btc_rpc::btc_rpc() {
	curl_assert(curl_global_init(CURL_GLOBAL_DEFAULT));
	c = curl_easy_init();
	if (!c)
		PANIC("curl init error");
	curl_assert(curl_easy_setopt(c, CURLOPT_WRITEFUNCTION,
				     write_callback));

}

btc_rpc::~btc_rpc()
{
	curl_global_cleanup();
}

std::string def_dir()
{
	std::string path;

	const char *env{getenv("HOME")};
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.bitcoin";
	return path;
}

/*
 * Do this in lieu of a proper config file loader.
 */
static std::string get_config_value(auto first, auto last, const std::string & key)
{
	auto it{std::find_if(first, last, [&](const std::string &l){
		return wythe::starts_with(l, key);
	})};

	if (it == last)
		PANIC("no " << key << "found");
	auto l = wythe::split(*it, '=');
	if (l.size() != 2)
		PANIC("config value parse error for key " << key);
	return l[1];
}

std::string userpass()
{
	auto fn{def_dir() + "/bitcoin.conf"};
	auto file{wythe::read_file(fn)};
	auto lines{wythe::line_split(file.begin(), file.end())};
	auto v{get_config_value(lines.begin(), lines.end(), "rpcuser")};
	v += ':';
	v += get_config_value(lines.begin(), lines.end(), "rpcpassword");

	return v;
}

static json request(const btc_rpc &bd, const std::string &method,
		    const json &params)
{
	std::string raw;
	json j = {{"jsonrpc", "2.0"},
		  {"id", id()},
		  {"method", method},
		  {"params", params}};

	std::string s{j.dump()};
	struct curl_slist *headers = NULL;

	headers = curl_slist_append(headers, "content-type: text/plain;");
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_HTTPHEADER, headers));

	curl_assert(
	    curl_easy_setopt(bd.c, CURLOPT_URL, "http://127.0.0.1:18332/"));

	curl_assert(curl_easy_setopt(bd.c, CURLOPT_POSTFIELDSIZE, s.size()));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_POSTFIELDS, s.data()));

	curl_assert(
	    curl_easy_setopt(bd.c, CURLOPT_USERPWD, userpass().c_str()));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_USE_SSL, CURLUSESSL_TRY));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_WRITEDATA, &raw));
	curl_assert(curl_easy_perform(bd.c));
	auto r = json::parse(raw);
	trace(r);
	auto err{r.find("error")};
	if (err != r.end()) {
		if (*err != nullptr)
			PANIC(method << " error: " << std::setw(4) << r);
	} else
		PANIC("no error found in bitcoind response\n"
		      << std::setw(4) << r);

	auto res = r["result"];
	if (res == nullptr)
		PANIC("no result found in bitcoind response\n"
		      << std::setw(4) << r);
	return res;
}

json getnetworkinfo(const btc_rpc &bd)
{
	return request(bd, "getnetworkinfo", json::array());
}

json getblockcount(const btc_rpc &bd)
{
	return request(bd, "getblockcount", json::array());
}

json gettxout(const btc_rpc &bd, const std::string &txid, int count)
{
	json j{txid, count};
	return request(bd, "gettxout", j);
}

json getrawtransaction(const btc_rpc &bd, const std::string &txid)
{
	json j{txid, 1};
	return request(bd, "getrawtransaction", j);
}



} // btc
} // rpc
