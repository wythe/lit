#include <string>
#include <unistd.h>
#include "rpc.h"

void trace(const json &j)
{
	if (g_json_trace)
		std::cerr << std::setw(4) << j << '\n';
}

std::string rpc::def_dir() 
{
	std::string path;

	const char *env = getenv("HOME");
	if (!env)
		PANIC("HOME environment undefined");

	path = env;
	path += "/.lightning";
	return path;
}

rpc::lightningd::lightningd()
{
	id = "lit-cli-" + std::to_string(getpid());
}

void rpc::lightningd::connect(std::string const &dir, std::string const &filename)
{
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
}


rpc::lightningd::~lightningd()
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

rpc::https::https()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	c = curl_easy_init();
	if (!c)
		PANIC("internet error");
	curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, &s);
}

rpc::https::~https()
{
	curl_global_cleanup();
}

size_t rpc::https::write_callback(void *contents, size_t size, size_t n,
			     std::string *s)
{
	size_t bytes = size * n;
	size_t old_size = s->size();
	s->resize(old_size + bytes);
	std::copy((char *)contents, (char *)contents + bytes,
		  s->begin() + old_size);
	return bytes;
}


json rpc::request_local(rpc::lightningd &c, const json &req)
{
	trace(req);
	std::string s = req.dump();
	rpc::write_all(c.fd, s.data(), s.size());
	auto j = read_all(c);
	trace(j);
	return j;
}
