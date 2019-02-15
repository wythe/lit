#pragma once
#include "rpc.h"
namespace rpc
{
namespace bitcoin
{
// bitcoin daemon
struct bd {
	bd();
	bd(const bd &) = delete;
	bd &operator=(const bd &) = delete;
	~bd();
	CURL *c;
};

// bitcoind rpc commands
json getblockcount(const bd &bd);
json getnetworkinfo(const bd &bd);
json gettxout(const bd &bd, std::string_view txid, int count);
json getrawtransaction(const bd &bd, std::string_view txid);

// convenience functions
int confirmations(const bd &bd, std::string_view txid);
}
}
