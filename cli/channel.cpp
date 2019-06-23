#include "channel.h"
#include "rpc_hosts.h"
#include <random>
#include <unistd.h>
#include "logger.h"
#include <nlohmann/json.hpp>

namespace lit {

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

void connect(const hosts &rpc, int count)
{
	auto nodes = listnodes(rpc.ld);
	lit::strip_non_addressable(nodes);
	if (count > nodes.size()) {
		l_info("total number of nodes is only " << nodes.size());
		count = nodes.size();
	}
	l_info("connecting to " << count << " random nodes");
	connect_random(rpc.ld, nodes, count);
}

void open_channel(const hosts &rpc, int count, uint64_t sats)
{
	auto peers = listpeers(rpc.ld);
	//lit::strip_nonfunded();
	//open_channel(rpc.ld, count, sats);
}

void autopilot(const hosts &hosts)
{
	auto nodes = listnodes(hosts.ld);
	auto channels = listchannels(hosts.ld);
	auto peers = listpeers(hosts.ld);

	bootstrap(hosts);
}
}
