#include "node.h"

node unmarshal_node(const json &j)
{ 
	node n;
	n.nodeid = j.at("nodeid");
	n.alias = j.at("alias");
	n.address = j["addresses"][0]["address"];
	n.address += ":";
	n.address += j["addresses"][0]["port"].get<int>();
	return n;
}

node unmarshal_peer(const json &peer)
{
#if 1
	return node();
#else
	auto p = j.at("id").get<std::string>();
	auto n = unmarshal_node(ln::listnodes(p.nodeid));
#endif

}

node_list unmarshal_node_list(const json &j)
{
	node_list nodes;
	for (auto &n : j.at("nodes"))
		// ignore if we can't address it
		if (n["addresses"] > 0)
			nodes.emplace_back(unmarshal_node(n));
	return nodes;
}

node_list unmarshal_peer_list(const json &j)
{
	node_list peers;
	for (auto &p : j.at("peers"))
		peers.emplace_back(unmarshal_peer(p));
	return peers;
}
