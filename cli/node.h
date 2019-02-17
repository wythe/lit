#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "ln_rpc.h"

using json = nlohmann::json;

namespace rpc {
struct hosts;
}

enum channel_state { none, connected, opening, normal, closing };

struct channel {
	rpc::lightning::node peer;
	channel_state state = channel_state::none;
	// how old is the channel?
	int confirmations = 0;
	double roi = 0;
};

using channel_list = std::vector<channel>;

channel_list unmarshal_channel_list(const rpc::hosts &hosts, const json &peers);

// Make it so we have @n connections.
// New connections are chosen randomly from @nodes.
// This this may result in disconnections, but will never disconnect from an
// open channel.
// Returns number of connections.
int connect_n(const rpc::hosts &hosts, const channel_list &channels,
	      const rpc::lightning::node_list &nodes, int n);

// get at least 200 nodes.  If we are not connected, then go to 1ML.com and get
// the top 50 most connected nodes. If still not 200, throw exception
rpc::lightning::node_list get_nodes(const rpc::hosts &hosts);

// fund all channels in connected state.
void fund_all(const rpc::hosts &hosts, const channel_list &channels);

// close underperforming channels.
void close(const channel_list &channels);

// autopilot algorithm
void autopilot(const rpc::hosts &hosts, const channel_list &channels,
	       const rpc::lightning::node_list &nodes);
