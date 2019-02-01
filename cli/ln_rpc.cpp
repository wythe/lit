#include "ln_rpc.h"

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
	    {"method", "listpeers"}, {"id", ld.id}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}

json nodes(const uds_rpc &ld)
{
	json j = {
	    {"method", "listnodes"}, {"id", ld.id}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}
} //ln

namespace btc
{
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

void curl_get()
{
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
	const char *data =
	    "{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"getnetworkinfo\", \"params\": [] }";

	headers = curl_slist_append(headers, "content-type: text/plain;");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:18332/");

	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(data));
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

	curl_easy_setopt(curl, CURLOPT_USERPWD, "wythe:cbxGk7SVS2Ruz5L2");

	curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

	if (curl_easy_perform(curl) != CURLE_OK)
		PANIC("curl fail");
    }
}

json getnetworkinfo()
{
#if 1
	curl_get();
	return {{"method", "getnetworkinfo"}, {"id", "1"}, {"params", {nullptr}}};
#else
	std::string id = "bitcoin-cli-";
	id += getpid();
	json j = {
	    {"method", "getnetworkinfo"}, {"id", id}, {"params", {nullptr}}};
	return j;
#endif
}

} // btc

} // rpc
