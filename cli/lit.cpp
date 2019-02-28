#include <chrono>
#include <iomanip>
#include <locale>
#include <nlohmann/json.hpp>
#include <wythe/command.h>
#include <wythe/exception.h>

#include "rpc_hosts.h"
#include "channel.h"
#include "logger.h"

using satoshi = long long;
using json = nlohmann::json;

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

	lit::hosts rpc;
};

static std::string dollars(satoshi value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << "$" << std::fixed << std::setprecision(2) << value;
	return ss.str();
}

static double get_btcusd(lit::web::https &https)
{
	auto j = lit::web::priceinfo(https);
	auto last = j.at("last").get<std::string>();
	return std::stod(last);
}

static std::string to_dollars(lit::web::https &https, satoshi s)
{
	if (s == 0)
		return "$0.00";
	auto price = get_btcusd(https);
	auto btc = s / 100000000.0;
	return dollars(price * btc);
}

static satoshi get_funds(lit::ld &ld)
{
	auto res = lit::rpc::listfunds(ld);

	auto outputs = res["outputs"];
	if (outputs.size() == 0)
		return 0;
	satoshi total = 0;
	for (auto &j : res["outputs"]) {
		total += j["value"].get<long long>();
	}
	return total;
}

void getinfo(struct opts &opts)
{
	log_info << "network is " << (is_testnet(opts.rpc.ld) ? "testnet" : "mainnet");
	auto nodes = listnodes(opts.rpc.ld);
	log_info << nodes.size() << " nodes in network (" << addressable(nodes) << 
		" addressable).";
	auto channels = listchannels(opts.rpc.ld);
	log_info << channels.size() << " channels in network";
	auto peers = listpeers(opts.rpc.ld);
	log_info << peers.size() << " peers";
	auto mlnodes = lit::web::get_1ML_connected(opts.rpc);
	log_info << mlnodes.size() << " 1ML nodes";

	log_info << "block count is " << lit::rpc::getblockcount(opts.rpc.bd);
	log_info << "bitcoin price is " << to_dollars(opts.rpc.https, 100000000);
	log_info << "total funds: " << to_dollars(opts.rpc.https, get_funds(opts.rpc.ld));
}

void bootstrap(struct opts &opts)
{
	lit::bootstrap(opts.rpc);
}

void closeall(struct opts &opts)
{
	auto peers = listpeers(opts.rpc.ld);
	auto i = lit::closechannel(opts.rpc.ld, peers, true);
	log_info << "closed " << i << " channel" << ((i == 1) ? ".\n" : "s.\n");
}

void autopilot(struct opts &opts)
{
	autopilot(opts.rpc);
}

template <typename T>
wythe::cli::line<T> parse_opts(T &opts, int argc, char **argv)
{
	using namespace wythe::cli;
	line<T> line("0.0.1", "lit", "Lightning Things",
		     "lit [options] [command] [command-options]");

	add_opt(line, "lightning-dir", 'l', "lightning rpc dir", lit::ld::def_dir(),
		[&](std::string const &d) { opts.rpc_dir = d; });

	add_opt(line, "rpc-file", 'r', "lightning rpc file", "lightning-rpc",
		[&](std::string const &f) { opts.rpc_file = f; });

	add_opt(line, "trace", 't', "Display rpc json request and response",
		[&] { g_json_trace = true; });

	add_cmd(line, "getinfo", "Display summary information on channels",
		getinfo);
	add_cmd(line, "closeall", "Close all open channels, forcibly if necessary.",
		closeall);
	add_cmd(line, "bootstrap", "Get a new node connected",
		bootstrap);


	line.notes.emplace_back("Use at your own demise.\n");
	parse(line, argc, argv);
	return line;
}

int main(int argc, char **argv)
{
	try {
		opts opts;
		auto line = parse_opts(opts, argc, argv);
		opts.rpc.ld.connect_uds(opts.rpc_dir, opts.rpc_file);
		line.go(opts);

		if (!line.targets.empty())
			PANIC("unrecognized command: " << line.targets[0]);
	} catch (std::invalid_argument &e) {
		log_fatal << "invalid argument: " << e.what();
	} catch (std::exception &e) {
		log_fatal << e.what();
	}
}
