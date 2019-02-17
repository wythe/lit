#include <chrono>
#include <iomanip>
#include <locale>
#include <nlohmann/json.hpp>
#include <wythe/command.h>
#include <wythe/exception.h>

#include "rpc_hosts.h"
#include "node.h"

using satoshi = long long;
using json = nlohmann::json;
namespace ln = rpc::lightning;
namespace bc = rpc::bitcoin;

bool g_json_trace = false; // trace json commands

struct opts {
	opts() = default;
	opts(const opts &) = delete;
	opts &operator=(const opts &) = delete;

	bool show_price = false;
	bool bech32 = true;
	satoshi sats = 0;
	std::string peer_id, peer_addr;
	std::string rpc_dir, rpc_file;
	std::string brpc_dir, brpc_file;

	// FIXME Just make this a global?
	rpc::hosts rpc;
};

static std::string dollars(satoshi value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << "$" << std::fixed << std::setprecision(2) << value;
	return ss.str();
}

static double get_btcusd(rpc::web::https &https)
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
		auto j = rpc::web::priceinfo(https);
		auto last = j.at("last").get<std::string>();
		v = std::stod(last);
	}
	return v;
}

static std::string to_dollars(rpc::web::https &https, satoshi s)
{
	if (s == 0)
		return "$0.00";
	auto price = get_btcusd(https);
	auto btc = s / 100000000.0;
	return dollars(price * btc);
}

static satoshi get_funds(ln::ld &ld)
{
	auto res = ln::listfunds(ld);

	auto outputs = res["outputs"];
	if (outputs.size() == 0)
		return 0;
	satoshi total = 0;
	for (auto &j : res["outputs"]) {
		total += j["value"].get<long long>();
	}
	return total;
}

void new_addr(opts &opts)
{
	auto res = ln::newaddr(opts.rpc.ld, opts.bech32);
	std::cout << res["address"].get<std::string>() << '\n';
}

void list_funds(struct opts &opts)
{
	std::cout << std::setw(4)
		  << to_dollars(opts.rpc.https, get_funds(opts.rpc.ld))
		  << '\n';
}

void list_nodes(struct opts &opts)
{
	std::cout << std::setw(4) << ln::listnodes(opts.rpc.ld);
}

void getnetworkinfo(struct opts &opts)
{
	std::cout << "getting network info:\n";
	std::cout << std::setw(4) << bc::getnetworkinfo(opts.rpc.bd) << '\n';
}

void list_peers(struct opts &opts)
{
	std::cout << std::setw(4) << ln::listpeers(opts.rpc.ld);
}

void getinfo(struct opts &opts)
{
	auto peers = ln::listpeers(opts.rpc.ld);
	auto nodes = ln::get_nodes(opts.rpc.ld);
	auto mlnodes = rpc::web::get_1ML_connected(opts.rpc.https);
	WARN(nodes.size() << " addressable nodes in network");
	WARN(mlnodes.size() << " 1ML nodes");
	WARN(peers["peers"].size() << " peers");
	auto channel_list = unmarshal_channel_list(opts.rpc, peers);

	for (auto &ch : channel_list) {
		WARN("id is " << ch.peer.nodeid);
		WARN("state is " << ch.state);
		WARN("confirmations is " << ch.confirmations);
	}

	WARN("block count is " << bc::getblockcount(opts.rpc.bd));
	WARN("bitcoin price is " << to_dollars(opts.rpc.https, 100000000));
	WARN("total funds: " << to_dollars(opts.rpc.https, get_funds(opts.rpc.ld)));
}

void autopilot(struct opts &opts)
{
	auto channels = unmarshal_channel_list(opts.rpc, ln::listpeers(opts.rpc.ld));
	auto nodes = get_nodes(opts.rpc.ld);
	autopilot(opts.rpc, channels, nodes);
}

template <typename T>
wythe::cli::line<T> parse_opts(T &opts, int argc, char **argv)
{
	using namespace wythe::cli;
	line<T> line("0.0.1", "lit", "Lightning Stuff",
		     "lit [options] [command] [command-options]");

	add_opt(line, "ln-dir", 'L', "lightning rpc dir", ln::def_dir(),
		[&](std::string const &d) { opts.rpc_dir = d; });

	add_opt(line, "ln-rpc-file", 'f', "lightning rpc file", "lightning-rpc",
		[&](std::string const &f) { opts.rpc_file = f; });

	add_opt(line, "trace", 't', "Display rpc json request and response",
		[&] { g_json_trace = true; });

	add_cmd(line, "listfunds", "Show funds available for opening channels",
		list_funds);
	add_cmd(line, "listnodes", "List all the nodes we see", list_nodes);
	add_cmd(line, "getnetworkinfo",
		"Show bitcoin and lightningnetwork info.", getnetworkinfo);

	add_cmd(line, "listpeers", "List our peers", list_peers);
	auto c =
	    emp_cmd(line, "newaddr",
		    "Request a new bitcoin address for use in funding channels",
		    new_addr);
	add_opt(*c, "p2sh", 'p', "Use p2sh-segwit address (default is bech32)",
		[&] { opts.bech32 = false; });

	add_cmd(line, "getinfo", "Display summary information on channels",
		getinfo);

	line.notes.emplace_back("Use at your own demise.\n");
	parse(line, argc, argv);
	return line;
}

int main(int argc, char **argv)
{
	try {
		opts opts;
		auto line = parse_opts(opts, argc, argv);
		opts.rpc.ld.fd = ln::connect_uds(opts.rpc_dir, opts.rpc_file);
		line.go(opts);

		if (!line.targets.empty())
			PANIC("unrecognized command: " << line.targets[0]);
	} catch (std::invalid_argument &e) {
		std::cerr << "invalid argument: " << e.what() << '\n';
	} catch (std::exception &e) {
		std::cerr << e.what() << '\n';
	}
}
