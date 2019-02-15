#include "ln_rpc.h"
#include <wythe/common.h>
#include <string_view>
#include "node.h"

using string_view = std::string_view;
namespace rpc
{
namespace lightning
{

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

static json read_all(int fd)
{
	std::string v;
	v.reserve(360);
	char ch;
	int r;
	while (r = read(fd, &ch, 1)) {
		if (r <= 0)
			PANIC("error talking to fd: " << r);
		v.push_back(ch);
		int avail;
		ioctl(fd, FIONREAD, &avail);
		if (avail == 0)
			break;
	}
	return json::parse(v);
}

int connect_uds(string_view dir, string_view filename)
{
	int fd;
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
	return fd;
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
	auto r = read_all(ld.fd);
	trace(r);
	if (r.count("error"))
		PANIC(r.at("error"));
	return r.at("result");
}

std::string def_dir()
{
	std::string path;

	const char *env = getenv("HOME");
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.lightning";
	return path;
}

json listpeers(const ld &ld)
{
	return request(ld, "listpeers", json::array());
}

json listnodes(const ld &ld, const std::string& id)
{
	return request(ld, "listnodes", id.empty() ? json::array() : json{ id });
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

json fundchannel(const ld &ld, const std::string &id, uint64_t amount)
{
	return request(ld, "fundchannel", {id, amount});
}

node unmarshal_node(const json &j)
{
	node n;
	n.nodeid = j.at("nodeid").get<std::string>();
	n.alias = j.at("alias").get<std::string>();
	n.address = j.at("addresses").at(0).at("address").get<std::string>();
	n.address += ":";
	n.address += j.at("addresses").at(0).at("port").get<int>();
	return n;
}

node_list get_nodes(const ld &ld)
{
	auto j = listnodes(ld);
	node_list nodes;
	for (auto &jn : j.at("nodes")) {
		// ignore if we can't address it
		if (jn.count("addresses") && jn.at("addresses").size() > 0)
			nodes.emplace_back(unmarshal_node(jn));
	}
	return nodes;
}

} // lightning
} // rpc
