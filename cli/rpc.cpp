#include "rpc.h"
#include <string>
#include <unistd.h>

namespace rpc
{
	using string_view = std::string_view;

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

int connect(string_view dir, string_view filename)
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

json request_local(int fd, const json &req)
{
	trace(req);
	std::string s{req.dump()};
	rpc::write_all(fd, s.data(), s.size());
	auto j = read_all(fd);
	trace(j);
	return j;
}

bool has_error(const json &j) { return (j.find("error") != j.end()); }

std::string error_message(const json &j)
{
	if (has_error(j))
		return j["error"]["message"];
}

}
