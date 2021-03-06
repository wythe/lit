#pragma once
#include "rpc.h"
#include <vector>

namespace lit {
// lightning daemon
struct ld {
	ld() = default;
	~ld()
	{
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
	}

	// There can only be one
	ld(const ld &) = delete;
	ld &operator=(const ld &) = delete;
	static std::string def_dir();
	void connect_uds(std::string_view dir, std::string_view filename);
	int fd = -1;
};

namespace rpc {

// lightningd json rpc commands
json getinfo(const ld &ld);
json listpeers(const ld &ld);
json listnodes(const ld &ld, const std::string &id = "");
json listchannels(const ld &ld);
json listfunds(const ld &ld);
json newaddr(const ld &ld, bool bech32);
json connect(const ld &ld, std::string_view peer_id,
	     std::string_view peer_addr);
json disconnect(const ld &ld, const std::string & peer_id);
json openchannel(const ld &ld, const std::string &id, uint64_t sats);
json closechannel(const ld &ld, const std::string &id, bool force = false,
		  int timeout = 30);
}

// C++ API
struct node {
	node() = default;
	node(const node &) = default;
	node &operator=(const node &) = default;
	node(const json &j);
	node(const std::string nodeid, const std::string alias,
	     const std::string address);
	friend bool operator<(const node &a, const node &b)
	{
		return a.nodeid < b.nodeid;
	}
	friend bool operator==(const node &a, const node &b)
	{
		return a.nodeid == b.nodeid;
	}
	friend std::ostream &operator<<(std::ostream &os, const node &n);

	std::string nodeid;
	std::string alias;
	std::string address;
};

using node_list = std::vector<node>;

struct channel {
	channel() = default;
	channel(const channel &) = default;
	channel &operator=(const channel &) = default;
	channel(const json &j);
};

using channel_list = std::vector<channel>;

struct peer {
	peer() = default;
	peer(const peer &) = default;
	peer &operator=(const peer &) = default;
	peer(const json &j);
	friend std::ostream& operator<<(std::ostream& os, const peer &p);
	std::string id;
	bool connected;
	bool channel_open;
};

using peer_list = std::vector<peer>;

node_list listnodes(const ld &ld);
channel_list listchannels(const ld &ld);
peer_list listpeers(const ld &ld);
void connect(const ld &ld, const node &n);
int connect(const ld &ld, const node_list &nodes);
int disconnect(const ld &ld, const peer &p);
int disconnect(const ld &ld, const peer_list &peers);
void closechannel(const ld &ld, const peer &peer, bool force);
int closechannel(const ld &ld, const peer_list &peers, bool force);

// convenience
bool is_testnet(const ld &ld);
int connections(const peer_list &peers);
int addressable(const node_list &nodes);
void strip_non_addressable(node_list &nodes);
void strip_channel_open(peer_list &peers);
int connect_random(const ld &ld, const node_list &nodes, int n);

} // lightning
