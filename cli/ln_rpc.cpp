#include "ln_rpc.h"
#include <wythe/common.h>
#include <string_view>

namespace rpc
{
namespace lightning
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
} // lightning

} // rpc
