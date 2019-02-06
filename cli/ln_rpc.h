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
std::string def_dir();
std::string userpass();

struct btc_rpc {
	btc_rpc();
	btc_rpc(const btc_rpc &) = delete;
	btc_rpc &operator=(const btc_rpc &) = delete;
	~btc_rpc();
	CURL *c;
	std::string id;
};

json getblockcount(const btc_rpc &bd);
json getnetworkinfo(const btc_rpc &bd);
json gettxout(const btc_rpc &bd, const std::string &txid, int count);
json getrawtransaction(const btc_rpc &bd, const std::string &txid);
}

}
