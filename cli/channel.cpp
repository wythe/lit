#include "channel.h"
#include "rpc_hosts.h"
#include <random>
#include <unistd.h>

namespace lit {

static void fund_all(const hosts & hosts,
		     const channel_list & channels,
		     const node_list & nodes, int n)
{
}

void bootstrap(const hosts &rpc)
{
	auto nodes = listnodes(rpc.ld);
	auto peers = listpeers(rpc.ld);

	if (addressable(nodes) > 400)
		return;

	auto nodes_1ML = web::get_1ML_connected(rpc);
	std::cerr << "connecting to peers\n";
	while (connections(peers) < 10) {
		connect_random(rpc.ld, nodes_1ML, 10 - connections(peers));
		peers = listpeers(rpc.ld);
	}

	std::cerr << "building network...\n";
	while (addressable(nodes) < 400) {
		std::cerr << nodes.size() << " nodes, " << addressable(nodes)
			  << " addressable.\n";
		sleep(2);
		nodes = listnodes(rpc.ld);
		peers = listpeers(rpc.ld);
	}
	std::cerr << "building network complete, disconnecting peers.n";
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
