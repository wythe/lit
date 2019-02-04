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
	json j = {
	    {"id", id()}, {"method", "listpeers"}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}

json nodes(const uds_rpc &ld)
{
	json j = {
	    {"id", id()}, {"jsonrpc", "2.0"}, {"method", "listnodes"}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}
} //ln

namespace btc
{

btc_rpc::btc_rpc() {
	curl_assert(curl_global_init(CURL_GLOBAL_DEFAULT));
	c = curl_easy_init();
	if (!c)
		PANIC("curl init error");

}

std::string def_dir()
{
	std::string path;

	const char *env = getenv("HOME");
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
	auto it = std::find_if(first, last, [&](const std::string &l){
		return wythe::starts_with(l, key);
	});

	if (it == last)
		PANIC("no " << key << "found");
	auto l = wythe::split(*it, '=');
	if (l.size() != 2)
		PANIC("config value parse error for key " << key);
	return l[1];
}

std::string userpass()
{
	auto fn = def_dir();
	fn += "/bitcoin.conf";
	auto file = wythe::read_file(fn);
	auto lines = wythe::line_split(file.begin(), file.end());
	auto v = get_config_value(lines.begin(), lines.end(), "rpcuser");
	v+= ':';
	v += get_config_value(lines.begin(), lines.end(), "rpcpassword");

	return v;
}

static void request(const btc_rpc &bd, const std::string &method, const json &params)
{
	json j = {
		{"jsonrpc", "2.0"}, {"id", id()}, {"method", method}, {"params", params}};
	WARN(j);

	std::string s = j.dump();
	struct curl_slist *headers = NULL;

	headers = curl_slist_append(headers, "content-type: text/plain;");
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_HTTPHEADER, headers));

	curl_assert(
	    curl_easy_setopt(bd.c, CURLOPT_URL, "http://127.0.0.1:18332/"));

	curl_assert(
	    curl_easy_setopt(bd.c, CURLOPT_POSTFIELDSIZE, s.size()));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_POSTFIELDS, s.data()));

	curl_assert(
	    curl_easy_setopt(bd.c, CURLOPT_USERPWD, userpass().c_str()));
	curl_assert(curl_easy_setopt(bd.c, CURLOPT_USE_SSL, CURLUSESSL_TRY));

	curl_assert(curl_easy_perform(bd.c));
}

json getnetworkinfo(const btc_rpc &bd)
{
	request(bd, "getnetworkinfo", json::array());
	return "{}";
}

btc_rpc::~btc_rpc()
{
	curl_global_cleanup();
}
} // btc
} // rpc
