#pragma once
#include "rpc.h"

namespace rpc
{
namespace ln
{
struct lightningd {
	lightningd();
	lightningd(const lightningd &) = delete;
	lightningd &operator=(const lightningd &) = delete;
	~lightningd();
	std::string id;
	int fd = -1;
};

json peers(const lightningd &ld);
json nodes(const lightningd &ld);
}
}
