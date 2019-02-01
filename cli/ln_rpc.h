#pragma once
#include "rpc.h"

namespace rpc
{
namespace ln
{
std::string def_dir();
json peers(const uds_rpc &ld);
json nodes(const uds_rpc &ld);
}

namespace btc {
json getnetworkinfo();
}

}
