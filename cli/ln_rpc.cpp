#include <algorithm>
#include <iterator>
#include <random>
#include <string_view>
#include <wythe/common.h>
#include <poll.h>

#include "channel.h"
#include "ln_rpc.h"
#include "logger.h"

using string_view = std::string_view;

std::string lit::ld::def_dir()
{
	std::string path;

	const char *env = getenv("HOME");
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.lightning";
	return path;
}

void lit::ld::connect_uds(string_view dir, string_view filename)
{
	struct sockaddr_un addr;
	auto d = std::string(dir);
	if (chdir(d.c_str()) != 0)
		PANIC("cannot chdir to '" << dir << "'");

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		PANIC("socket() failed");
	strcpy(addr.sun_path, std::string(filename).c_str());
	addr.sun_family = AF_UNIX;

	auto e = ::connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (e != 0)
		PANIC("cannot connect to " << addr.sun_path << ": "
					   << strerror(errno));
}

namespace lit {
namespace rpc {
static bool write_all(int fd, const void *data, size_t size)
{
	while (size) {
		ssize_t done;

		done = write(fd, data, size);
		if (done < 0 && errno == EINTR)
			continue;
		if (done <= 0)
			return false;
		data = (const char *)data + done;
		size -= done;
	}

	return true;
}

/* Read an entire json rpc message and return. */
static json read_json(int fd, int timeout_ms)
{
	std::vector<char> resp;
	resp.resize(1024);
	bool part = true;
	int off = 0;
	int i;
	int brackets = 0;

	struct pollfd pfd = { fd, POLLIN, 0};

	while (part) {
		auto ret = poll(&pfd, 1, timeout_ms);
		if (ret == -1) {
			perror("poll()");
			PANIC("error reading from lightningd");
		} else if (ret == 0) {
			PANIC("reading timeout from lightningd (" <<
			timeout_ms << "ms)");
		}
		i = read(fd, &resp[0] + off, resp.size() - 1 - off);
		if (i <= 0)
			PANIC("error reading from lightningd");
		off += i;
		/* NUL terminate */
		resp[off] = '\0';

		// count the { and } chars
		brackets = 0; // FIXME fix this to not start over every time
		for (auto &c : resp) {
			if (c == '{')
				brackets++;
			else if (c == '}')
				brackets--;
		}

		if (brackets > 0) {
			// resize and read more
			if (off == resp.size() - 1)
				resp.resize(resp.size() * 2);
		} else {
			// done
			part = false;
			resp.resize(off);
		}
	}
	return json::parse(resp.begin(), resp.end());
}

static json request(const ld &ld, string_view method, const json &params,
		    int timeout_ms = -1)
{
	std::string raw;
	json j{{"jsonrpc", "2.0"},
	       {"id", name()},
	       {"method", std::string(method)},
	       {"params", params}};

	trace(j);
	std::string s{j.dump()};
	write_all(ld.fd, s.data(), s.size());
	auto r = read_json(ld.fd, timeout_ms);
	trace(r);
	if (r.count("error"))
		PANIC(r.at("error"));
	return r.at("result");
}

json getinfo(const ld &ld)
{
	return request(ld, "getinfo", json::array());
}

json listpeers(const ld &ld)
{
	return request(ld, "listpeers", json::array());
}

json listnodes(const ld &ld, const std::string& id)
{
	return request(ld, "listnodes", id.empty() ? json::array() : json{ id });
}

json listchannels(const ld &ld)
{
	return request(ld, "listchannels", json::array());
}

json listfunds(const ld &ld)
{
	return request(ld, "listfunds", json::array());
}

json newaddr(const ld &ld, bool bech32)
{
	std::string type = bech32 ? "bech32" : "p2sh-segwit";
	return request(ld, "newaddr", {type});
}

json connect(const ld &ld, string_view peer_id, string_view peer_addr)
{
	auto addr = std::string(peer_id);
	if (!peer_addr.empty()) {
		addr += '@';
		addr += peer_addr;
	}
	return request(ld, "connect", {addr}, 3000);
}

json disconnect(const ld &ld, const std::string & peer_id)
{
	return request(ld, "disconnect", {peer_id});
}

json openchannel(const ld &ld, const std::string &id, uint64_t sats)
{
	return request(ld, "fundchannel", {id, sats});
}

json closechannel(const ld &ld, const std::string &id, bool force, int timeout)
{
	return request(ld, "close", {id, force, timeout});
}
} // rpc

channel::channel(const json &j)
{
}

node::node(const json &j)
{
	nodeid = j.at("nodeid").get<std::string>();
	if (j.count("alias"))
		alias = j.at("alias").get<std::string>();
	if (j.count("addresses") && j.at("addresses").size() > 0) {
		address =
		    j.at("addresses").at(0).at("address").get<std::string>();
		address += ":";
		address += std::to_string(
		    j.at("addresses").at(0).at("port").get<int>());
	}
}

node::node(const std::string nodeid, const std::string alias,
	   const std::string address)
    : nodeid(nodeid), alias(alias), address(address)
{
}

std::ostream &operator<<(std::ostream &os, const node &n)
{
	os << n.nodeid.substr(0, 6) << "... " << n.address;
	return os;
}

std::ostream& operator<<(std::ostream& os, const peer &p)
{
	os << p.id.substr(0, 6) << "...";
	return os;
}

peer::peer(const json &j)
{
	id = j.at("id").get<std::string>();
	connected = j.at("connected").get<bool>();
	channel_open = false;
}

node_list listnodes(const ld &ld)
{
	auto j = rpc::listnodes(ld);
	node_list nodes;
	for (auto &jn : j.at("nodes"))
		nodes.emplace_back(jn);
	return nodes;
}

channel_list listchannels(const ld &ld)
{
	auto j = rpc::listchannels(ld);
	channel_list channels;
	for (auto &ch : j.at("channels"))
		channels.emplace_back(ch);
	return channels;
}

peer_list listpeers(const ld &ld) 
{
	auto j = rpc::listpeers(ld);
	peer_list peers;
	for (auto &jp : j.at("peers"))
		peers.emplace_back(jp);
	return peers;
}

template <typename T, typename Op>
int for_try(const ld &ld, std::string_view text, T list, Op op)
{
	auto i = 0;
	for (auto &n : list) {
		try {
			l_info(text << n);
			op(ld, n);
			++i;
		} catch (std::exception &e) {
			l_warn(e.what());
		}
	}
	return i;
}

void connect_to(const ld &ld, const node &n)
{
	rpc::connect(ld, n.nodeid, n.address);
}

int connect(const ld &ld, const node_list & nodes)
{
	return for_try(ld, "connecting to ", nodes, connect_to);
}

int disconnect_from(const ld &ld, const peer &p)
{
	rpc::disconnect(ld, p.id);
}

int disconnect(const ld &ld, const peer_list & peers)
{
	return for_try(ld, "disconnecting from ", peers, disconnect_from);
}

void closechannel(const ld &ld, const peer & peer, bool force)
{
	rpc::closechannel(ld, peer.id, force, 30);
}

int closechannel(const ld &ld, const peer_list &peers, bool force)
{
	return for_try(ld, "closing ", peers,
		       [](auto &ld, auto &p) { closechannel(ld, p, true); });
}

bool is_testnet(const ld &ld)
{
	auto j = rpc::getinfo(ld);
	return j.at("network").get<std::string>() == "testnet";
}

int connections(const peer_list &peers)
{
	return std::count_if(peers.begin(), peers.end(),
			     [](auto p) { return p.connected; });
}

int addressable(const node_list &nodes)
{
	return std::count_if(nodes.begin(), nodes.end(),
			     [](auto n) { return !n.address.empty(); });
}

void strip_non_addressable(node_list &nodes)
{
	nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
				   [](auto n) { return n.address.empty(); }),
		    nodes.end());
}

void strip_channel_open(peer_list &peers)
{
	peers.erase(std::remove_if(peers.begin(), peers.end(),
				   [](auto n) { return n.channel_open; }),
		    peers.end());
}

int connect_random(const ld &ld, const node_list &nodes, int n)
{
	l_trace(" connecting to " << n << " random nodes");
	node_list rnd_nodes;
	int i = 0;
	int tries = 0;
	while (i < n) {
		tries += n - i;
		std::sample(nodes.begin(),
			    nodes.end(),
			    std::back_inserter(rnd_nodes),
			    n - i,
			    std::mt19937{std::random_device{}()});
		int count = connect(ld, rnd_nodes);
		i += count;
		rnd_nodes.clear();
	}
	l_info(i << " connections made after " << tries << " tries");
	return i;
}

int open_channel(const ld &ld, const peer &peer, uint64_t sats)
{
	rpc::openchannel(ld, peer.id, sats);
}

int open_channel(const ld &ld, const peer_list &peers, uint64_t sats)
{
#if 0
	l_trace(" opening " << n << " channels");
	return for_try(ld, "opening ", peers,
		       [](auto &ld, auto &p) { open_channel(ld, p, sats); });
#endif
}

} // lit
