#include "ln_rpc.h"
#include <wythe/common.h>
#include <string_view>

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

static json request(int fd, const json &req)
{
	trace(req);
	std::string s{req.dump()};
	write_all(fd, s.data(), s.size());
	auto j = read_all(fd);
	trace(j);
	if (j.count("error"))
		PANIC(j["error"]);
	return j;
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
	json j{{"id", id()},
	       {"jsonrpc", "2.0"},
	       {"method", "listpeers"},
	       {"params", {nullptr}}};
	return request(ld.fd, j)["result"];
}

json listnodes(const ld &ld)
{
	json j{{"id", id()},
	       {"jsonrpc", "2.0"},
	       {"method", "listnodes"},
	       {"params", {nullptr}}};
	return request(ld.fd, j)["result"];
}

json listfunds(const ld &ld)
{
	json j{{"id", ld.id},
	       {"jsonrpc", "2.0"},
	       {"method", "listfunds"},
	       {"params", json::array()}};
	return request(ld.fd, j)["result"];
}

json newaddr(const ld &ld, bool bech32)
{
	std::string type = bech32 ? "bech32" : "p2sh-segwit";
	json j{{"id", ld.id},
	       {"jsonrpc", "2.0"},
	       {"method", "newaddr"},
	       {"params", {type}}};
	return request(ld.fd, j)["result"];
}

json connect(const ld &ld, string_view peer_id, string_view peer_addr)
{
	auto addr = std::string(peer_id);
	if (!peer_addr.empty()) {
		addr += '@';
		addr += peer_addr;
	}
	json req{{"id", ld.id},
		 {"jsonrpc", "2.0"},
		 {"method", "connect"},
		 {"params", {addr}}};
	return request(ld.fd, req);
}

bool fundchannel(const ld &ld, const std::string &id, uint64_t amount)
{
	json req{{"id", ld.id},
		 {"jsonrpc", "2.0"},
		 {"method", "fundchannel"},
		 {"params", {id, amount}}};
	return request(ld.fd, req);
}
} // lightning
} // rpc
