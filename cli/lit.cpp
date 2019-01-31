#include <chrono>
#include <iomanip>
#include <locale>
#include <nlohmann/json.hpp>
#include <wythe/command.h>
#include <wythe/exception.h>

#include "ln_rpc.h"
#include "rpc.h"
#include "web_rpc.h"

using json = nlohmann::json;
using satoshi = long long;

bool g_json_trace = false; // trace json commands

struct opts {
	opts() = default;
	opts(const opts &) = delete;
	opts &operator=(const opts &) = delete;

	bool show_price = false;
	satoshi sats = 0;
	std::string peer_id, peer_addr;
	std::string rpc_dir, rpc_file;
	rpc::web::https https;
	rpc::ln::lightningd ld;
};

template <typename T> std::string dollars(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << "$" << std::fixed << std::setprecision(2) << value;
	return ss.str();
}

double get_btcusd(rpc::web::https &https)
{
	static double v = -1;
	// Timer used here to cache last values for 5 seconds between calls.
	// This way we don't have to worry about
	// hammering gemini.
	static auto last_time = std::chrono::system_clock::now();
	auto now = std::chrono::system_clock::now();
	double secs =
	    std::chrono::duration_cast<std::chrono::seconds>(now - last_time)
		.count();
	if (v < 0 || secs > 5) {
		auto j = rpc::web::request(
		    https, "https://api.gemini.com/v1/pubticker/btcusd");

		auto last = j.at("last").get<std::string>();
		v = std::stod(last);
	}
	return v;
}

std::string to_dollars(rpc::web::https &https, const satoshi &s)
{
	if (s == 0)
		return "$0.00";
	auto price = get_btcusd(https);
	auto btc = s / 100000000.0;
	return dollars(price * btc);
}

satoshi get_funds(rpc::ln::lightningd &ld, rpc::web::https &https)
{
	json j = {
	    {"method", "listfunds"}, {"id", ld.id}, {"params", json::array()}};

	auto res = rpc::request_local(ld.fd, j);
	auto outputs = res["result"]["outputs"];
	if (outputs.size() == 0)
		return 0;
	satoshi total = 0;
	for (auto &j : res["result"]["outputs"]) {
		total += j["value"].get<long long>();
	}
	return total;
}

bool bech32 = true;

void new_addr(opts &opts)
{
	std::string type = bech32 ? "bech32" : "p2sh-segwit";
	json j = {
	    {"method", "newaddr"}, {"id", opts.ld.id}, {"params", {type}}};
	auto res = rpc::request_local(opts.ld.fd, j);
	std::cout << res["result"]["address"] << '\n';
}

void list_funds(struct opts &opts)
{
	std::cout << std::setw(4)
		  << to_dollars(opts.https, get_funds(opts.ld, opts.https))
		  << '\n';
}
void list_nodes(struct opts &opts)
{
	std::cout << std::setw(4) << rpc::ln::nodes(opts.ld);
}

void list_peers(struct opts &opts)
{
	std::cout << std::setw(4) << rpc::ln::peers(opts.ld);
}

void getinfo(struct opts &opts)
{
	// show number of peers and nodes

	auto peers = rpc::ln::peers(opts.ld);
	auto nodes = rpc::ln::nodes(opts.ld);

	WARN(nodes["result"]["nodes"].size() << " nodes");
	WARN(peers["result"]["peers"].size() << " peers");
}

#if 0
json connect(const std::string &peer_id, const std::string &peer_addr)
{
	auto id = peer_id;
	if (!peer_addr.empty()) {
		id += '@';
		id += peer_addr;
	}
	json req = {{"method", "connect"}, {"id", name}, {"params", {id}}};
	auto j = rpc::request_local(req);
	auto m = rpc::error_message(j);
	if (!m.empty())
		PANIC(m);
	return j;
}

bool fund_channel(const std::string &id, satoshi amount)
{
	json req = {
	    {"method", "fundchannel"}, {"id", name}, {"params", {id, amount}}};
	auto j = rpc::request_local(req);
	auto m = rpc::error_message(j);
	if (!m.empty())
		PANIC(m);
	return j;
}

void fund_first(satoshi sats)
{
	std::cout << "funding first available compatible node with " << sats
		  << " satoshis (" << to_dollars(sats) << ").\n";
	auto nodes = list_nodes();
	// WARN(nodes);
	for (auto &n : nodes["result"]["nodes"]) {
		// std::cout << n["alias"] << '\n';
		for (auto &a : n["addresses"]) {
			if (a.size()) { // TODO This check is because one of the
					// values was set to {}. Problem with
					// lightningd?
				std::string id = n["nodeid"];
				std::string ip = a["address"];
				std::string addr = ip;
				addr += ':';
				addr += std::to_string(a["port"].get<int>());
				std::cout << "trying " << id << " " << addr
					  << '\n';
				// connect(id, addr);
				// return;
			}
		}
	}
}
#endif

int main(int argc, char **argv)
{
	try {
		using namespace wythe::cli;
		struct opts opts;
		line<struct opts> line(
		    "0.0.1", "lit", "Lightning Stuff",
		    "lit [options] [command] [command-options]");

		line.global_opts.emplace_back(
		    "lightning-dir", 'L', "lightning rpc dir", rpc::def_dir(),
		    [&](std::string const &d) { opts.rpc_dir = d; });

		line.global_opts.emplace_back(
		    "rpc-file", 'R', "lightning rpc file", "lightning-rpc",
		    [&](std::string const &f) { opts.rpc_file = f; });

		line.global_opts.emplace_back(
		    "trace", 't', "Display rpc json request and response",
		    [&] { g_json_trace = true; });

		line.commands.emplace_back(
		    "listfunds", "Show funds available for opening channels",
		    list_funds);
		line.commands.emplace_back(
		    "listnodes", "List all the nodes we see", list_nodes);
		line.commands.emplace_back("listpeers", "List our peers",
					   list_peers);

		auto c = line.commands.emplace(
		    line.commands.end(), "newaddr",
		    "Request a new bitcoin address for use in funding channels",
		    new_addr);
		c->opts.emplace_back(
		    "p2sh", 'p', "Use p2sh-segwit address (default is bech32)",
		    [&] { bech32 = false; });

		line.commands.emplace_back(
		    "getinfo", "Display summary information on channels",
		    getinfo);

		line.notes.emplace_back("Use at your own demise.\n");
		line.parse(argc, argv);

		rpc::connect(opts.ld, opts.rpc_dir, opts.rpc_file);

		line.go(opts);

		// if (!line.targets.empty()) PANIC("unrecognize command line
		// argument: " << line.targets[0]);
	} catch (std::invalid_argument &e) {
		std::cerr << "invalid argument: " << e.what() << std::endl;
	} catch (std::exception &e) {
		WARN(e.what());
	}
}
