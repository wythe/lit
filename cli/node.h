#pragma once

#include<vector>
#include<string>
#include "rpc.h"

enum channel_state {
	none,
	opening,
	normal,
	closing
};

// everything is a node
struct node {
	std::string nodeid;
	std::string alias;
	std::string address;

	// are we connected to this node?
	bool connected = false;

	// do we have an open channel with it?
	channel_state state = channel_state::none; 

	// how old is the channel?
	int confirmations = 0;
};

using node_list = std::vector<node>;

