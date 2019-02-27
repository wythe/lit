#include "channel.h"
#include "rpc_hosts.h"
#include <random>

namespace lit {

int connect_n(const hosts &hosts,
	      const node_list &nodes,
	      int n)
{
	return 0;
}

static void fund_all(const hosts & hosts,
		     const channel_list & channels,
		     const node_list & nodes, int n)
{
}

void bootstrap(const hosts &rpc)
{
	auto nodes = listnodes(rpc.ld);
	auto peers = listpeers(rpc.ld);

	while (nodes.size() < 400) {
		auto num = connections(peers);
		if (num < 10) {
			auto nodes_1ML = web::get_1ML_connected(rpc);
			auto n = connect_random(rpc.ld, nodes_1ML, 10 - num);
		}
		nodes = listnodes(rpc.ld);
		peers = listpeers(rpc.ld);
	}
	// Network is good, remove all non-channel connections.
	disconnect(rpc.ld, peers);
}

void autopilot(const hosts &hosts)
{
	auto nodes = listnodes(hosts.ld);
	auto channels = listchannels(hosts.ld);
	auto peers = listpeers(hosts.ld);

	bootstrap(hosts);
}
}
