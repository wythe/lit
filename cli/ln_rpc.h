#pragma once
#include <vector>
#include "rpc.h"

namespace rpc
{
namespace lightning
{
// lightning daemon
struct ld {
	ld() = default;
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
	int fd = -1;
};

struct node {
	std::string nodeid;
	std::string alias;
	std::string address;
	friend bool operator<(const node &a, const node &b)
	{
		return a.nodeid < b.nodeid;
	}
	friend bool operator==(const node &a, const node &b)
	{
		return a.nodeid == b.nodeid;
	}
};

using node_list = std::vector<node>;

// control
std::string def_dir();
int connect_uds(std::string_view dir, std::string_view filename);

// lightningd json rpc commands
json listpeers(const ld &ld);
json listnodes(const ld &ld, const std::string &id = "");
json listfunds(const ld &ld);
json newaddr(const ld &ld, bool bech32);
json connect(const ld &ld, std::string_view peer_id, std::string_view peer_addr);
json fundchannel(const ld &ld, const std::string &id, uint64_t amount);

// unmarshaling functions
node unmarshal_node(const json &j);

// C++ API
node_list get_nodes(const ld &ld);
}
}
