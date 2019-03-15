#include "channel.h"
#include "rpc_hosts.h"
#include <random>
#include <unistd.h>
#include "logger.h"
#include <nlohmann/json.hpp>

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

	if (addressable(nodes) > 500)
		return;

	auto nodes_1ML = web::get_1ML_connected(rpc);
	l_info("connecting to peers");
	while (connections(peers) < 10) {
		connect_random(rpc.ld, nodes_1ML, 10 - connections(peers));
		peers = listpeers(rpc.ld);
	}

	l_info("building network");
	l_info("waiting for 500 addressable nodes...");
	do {
		sleep(2);
		nodes = listnodes(rpc.ld);
		l_info(nodes.size() << " nodes, " << addressable(nodes)
			  << " addressable.");
	} while (addressable(nodes) < 500);
	l_info("building network complete, disconnecting peers.");
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
