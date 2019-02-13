#pragma once
#include "rpc.h"

namespace rpc
{
namespace lightning
{
// lightning daemon
struct ld {
	ld() {
		id = rpc::id();
	}
	~ld()
	{
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
	}

	// There can only be one
	ld(const ld &) = delete;
	ld &operator=(const ld &) = delete;
	std::string id;
	int fd = -1;
};

// control
std::string def_dir();
int connect_uds(std::string_view dir, std::string_view filename);

// rpc commands
json listpeers(const ld &ld);
json listnodes(const ld &ld);
json listfunds(const ld &ld);
json newaddr(const ld &ld, bool bech32);
json connect(const ld &ld, std::string_view peer_id, std::string_view peer_addr);
}
}
