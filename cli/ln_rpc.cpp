#include <algorithm>
#include <iterator>
#include <random>
#include <string_view>
#include <wythe/common.h>

#include "channel.h"
#include "ln_rpc.h"

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
	if (chdir(std::string(dir).c_str()) != 0)
		PANIC("cannot chdir to " << dir);

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
static json read_json(int fd)
{
	std::vector<char> resp;
	resp.resize(1024);
	bool part = true;
	int off = 0;
	int i;
	int brackets = 0;

	while (part) {
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
			if (c == '}')
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

static json request(const ld &ld, string_view method, const json &params)
{
	std::string raw;
	json j{{"jsonrpc", "2.0"},
	       {"id", name()},
	       {"method", std::string(method)},
	       {"params", params}};

	trace(j);
	std::string s{j.dump()};
	write_all(ld.fd, s.data(), s.size());
	auto r = read_json(ld.fd);
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
	return request(ld, "connect", {addr});
}

json disconnect(const ld &ld, const std::string & peer_id)
{
	return request(ld, "disconnect", {peer_id});
}

json fundchannel(const ld &ld, const std::string &id, uint64_t amount)
{
	return request(ld, "fundchannel", {id, amount});
}

json closechannel(const ld &ld, const std::string &id, bool force, int timeout)
{
	return request(ld, "fundchannel", {id, force, timeout});
}
} // rpc

channel::channel(const json &j)
{
}

node::node(const json &j)
{
	nodeid = j.at("nodeid").get<std::string>();
	alias = j.at("alias").get<std::string>();
	address = j.at("addresses").at(0).at("address").get<std::string>();
	address += ":";
	address += j.at("addresses").at(0).at("port").get<int>();
}

node::node(const std::string nodeid, const std::string alias,
	   const std::string address)
    : nodeid(nodeid), alias(alias), address(address)
{
}

peer::peer(const json &j, const node_list &nodes)
{
	auto i = std::find_if(nodes.begin(), nodes.end(), [&](auto n) {
		return n.nodeid == j.at("id").get<std::string>();
	});
	if (i != nodes.end())
		n = *i;
}

node_list listnodes(const ld &ld)
{
	auto j = rpc::listnodes(ld);
	node_list nodes;
	for (auto &jn : j.at("nodes")) {
		// ignore if we can't address it
		if (jn.count("addresses") && jn.at("addresses").size() > 0)
			nodes.emplace_back(jn);
	}
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

peer_list listpeers(const ld &ld, const node_list &nodes) 
{
	auto j = rpc::listpeers(ld);
	peer_list peers;
	for (auto &jp : j.at("peers"))
		peers.emplace_back(jp, nodes);
	return peers;
}

void connect(const ld &ld, const node_list & nodes)
{
	for (auto &n : nodes)
		rpc::connect(ld, n.nodeid, n.address);
}

void disconnect(const ld &ld, const peer_list & peers)
{
	for (auto &p : peers)
		rpc::disconnect(ld, p.n.nodeid);
}

void closechannel(const ld &ld, const peer & peer, bool force)
{
	rpc::closechannel(ld, peer.n.nodeid, force, 30);
}

void closechannel(const ld &ld, const peer_list & peers, bool force)
{
	for (auto &ch : peers)
		closechannel(ld, ch, force);
}

bool is_testnet(const ld &ld)
{
	auto j = rpc::getinfo(ld);
	return j.at("network").get<std::string>() == "testnet";
}

int connections(const ld &ld, const peer_list &peers)
{
	return std::count_if(peers.begin(), peers.end(),
			     [](auto p) { return p.connected; });
}

void connect_random(const ld &ld, const node_list &nodes, int n)
{
	node_list rnd_nodes;
	std::sample(nodes.begin(),
		    nodes.end(),
		    std::back_inserter(rnd_nodes),
		    n,
		    std::mt19937{std::random_device{}()});
	connect(ld, rnd_nodes);
}
} // lit
