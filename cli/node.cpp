#include "node.h"
#include "rpc_hosts.h"
#include <random>

namespace ln = rpc::lightning;
namespace bc = rpc::bitcoin;

channel unmarshal_channel(const rpc::hosts &hosts, const json &peer)
{
	auto id = peer.at("id").get<std::string>();
	auto a = ln::listnodes(hosts.ld, id).at("nodes").at(0);
	// WARN(a);
	channel ch;
	ch.peer = ln::unmarshal_node(a);
	ch.confirmations = bc::confirmations(
	    hosts.bd,
	    peer.at("channels").at(0).at("funding_txid").get<std::string>());
	return ch;
}

node_list unmarshal_node_list(const json &j)
{
	node_list nodes;
	for (auto &n : j.at("nodes"))
		// ignore if we can't address it
		if (n["addresses"] > 0)
			nodes.emplace_back(ln::unmarshal_node(n));
	return nodes;
}

channel_list unmarshal_channel_list(const rpc::hosts &hosts, const json &peers)
{
	channel_list channels;
	for (auto &p : peers.at("peers"))
		channels.emplace_back(unmarshal_channel(hosts, p));
	return channels;
}

node_list get_nodes(const rpc::hosts &hosts)
{
	auto nodes = ln::get_nodes(hosts.ld);
	node_list ml_nodes;
	if (nodes.size() < 200)
		ml_nodes = rpc::web::get_1ML_connected(hosts.https);

	nodes.insert(nodes.end(), ml_nodes.begin(), ml_nodes.end());
}

int connect_n(const rpc::hosts &hosts, const channel_list &channels,
	      const node_list &nodes, int n)
{
	return 0;
}

void fund_all(const rpc::hosts &hosts, const channel_list &channels,
	      const node_list &nodes, int n)
{
}

void autopilot(const rpc::hosts &hosts, const channel_list &channels,
	       const node_list &nodes)
{
}
