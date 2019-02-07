#pragma once
#include "rpc.h"
namespace rpc {
namespace bitcoin {
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
json getrawtransaction(const btc_rpc &bd, std::string_view txid);
}
}
