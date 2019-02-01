#include "rpc.h"
#include <string>
#include <unistd.h>

namespace rpc
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

void trace(const json &j)
{
	if (g_json_trace)
		std::cerr << std::setw(4) << j << '\n';
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

int connect_local(std::string const &dir, std::string const &filename)
{
	int fd;
	struct sockaddr_un addr;
	if (chdir(dir.c_str()) != 0)
		PANIC("cannot chdir to " << dir);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		PANIC("socket() failed");
	strcpy(addr.sun_path, filename.c_str());
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
	std::string s = req.dump();
	rpc::write_all(fd, s.data(), s.size());
	auto j = read_all(fd);
	trace(j);
	return j;
}

json read_all(int fd)
{
	std::string v;
	v.reserve(360);
	char ch;
	int r;
	while (r = read(fd, &ch, 1)) {
		if (r <= 0)
			PANIC("error talking to fd");
		v.push_back(ch);
		int avail;
		ioctl(fd, FIONREAD, &avail);
		if (avail == 0)
			break;
	}
	return json::parse(v);
}

bool has_error(const json &j) { return (j.find("error") != j.end()); }

std::string error_message(const json &j)
{
	if (has_error(j))
		return j["error"]["message"];
}
}
