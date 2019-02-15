#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace rpc
{
struct hosts;
}

struct node {
	std::string nodeid;
	std::string alias;
	std::string address;
};

enum channel_state { none, connected, opening, normal, closing };

struct channel {
	node peer;
	channel_state state = channel_state::none;
	// how old is the channel?
	int confirmations = 0;
	double roi = 0;
};

using node_list = std::vector<node>;
using channel_list = std::vector<channel>;

channel_list unmarshal_channel_list(const rpc::hosts &hosts, const json &peers);
node_list unmarshal_node_list(const json &j);

// Make it so we have @n connections.
// New connections are chosen randomly from @nodes.
// This this may result in disconnections, but will never disconnect from an
// open channel.
// Returns number of connections.
int connect_n(const rpc::hosts &hosts, const channel_list &channels,
	      const node_list &nodes, int n);

// get at least 200 nodes.  If we are not connected, then go to 1ML.com.
node_list get_nodes(const rpc::hosts &hosts);

// fund all channels in connected state.
void fund_all(const rpc::hosts &hosts, const channel_list &channels);

// close underperforming channels.
void close(const channel_list &channels);

// autopilot algorithm
void autopilot(const rpc::hosts &hosts, const channel_list &channels,
	       const node_list &nodes);
