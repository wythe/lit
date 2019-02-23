#include "channel.h"
#include "rpc_hosts.h"
#include <random>

namespace lit {

#if 0
channel::channel(const hosts &hosts, const peer &peer) : peer(peer)
{
#if 0
	confirmations = bc::confirmations(
	    hosts.bd,
	    peer.at("channels").at(0).at("funding_txid").get<std::string>());
#endif
}

channel_list unmarshal_channel_list(const hosts &hosts, const peer_list &peers)
{
	channel_list channels;
#if 0
	for (auto &p : peers)
		channels.emplace_back(unmarshal_channel(hosts, p));
#endif
	return channels;
}
#endif

int connect_n(const hosts &hosts,
	      const node_list &nodes, int n)
{
	return 0;
}

static void fund_all(const hosts & hosts,
		     const channel_list & channels,
		     const node_list & nodes, int n)
{
}

static void bootstrap(const hosts &hosts,
		      const node_list &nodes,
		      const peer_list &peers)
{
	// If our graph is too small, connect to 1ML and then exit.
	if (nodes.size() < 200) {
		if (connections(hosts.ld, peers) < 10) {
			auto nodes_1ML = web::get_1ML_connected(hosts);
			connect_random(hosts.ld, nodes_1ML, 10);
		}
		PANIC("Too few nodes in network: " << nodes.size()
						   << " (200 required).");
	}
	// Network is good, remove all non-channel connections so we can proceed
	// to next phase.
	disconnect(hosts.ld, peers);
}

void autopilot(const hosts &hosts)
{
	auto nodes = listnodes(hosts.ld);
	auto channels = listchannels(hosts.ld);
	auto peers = listpeers(hosts.ld, nodes);

	bootstrap(hosts, nodes, peers);
}
}
