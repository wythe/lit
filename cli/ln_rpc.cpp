#include "ln_rpc.h"

namespace rpc
{
namespace ln
{

lightningd::lightningd() { id = "lit-cli-" + std::to_string(getpid()); }

lightningd::~lightningd()
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

json peers(const lightningd &ld)
{
	json j = {
	    {"method", "listpeers"}, {"id", ld.id}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}

json nodes(const lightningd &ld)
{
	json j = {
	    {"method", "listnodes"}, {"id", ld.id}, {"params", {nullptr}}};
	return request_local(ld.fd, j);
}
}
}
