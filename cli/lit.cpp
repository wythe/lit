#include <chrono>
#include <iomanip>
#include <locale>
#include <nlohmann/json.hpp>
#include <wythe/exception.h>
#include <CLI/CLI.hpp>

#include "rpc_hosts.h"
#include "channel.h"
#include "logger.h"

using satoshi = long long;
using json = nlohmann::json;

bool g_json_trace = false; // trace json commands

struct opts {
	opts() {
		rpc_dir = lit::ld::def_dir();
		rpc_file = "lightning-rpc";
	}
	opts(const opts &) = delete;
	opts &operator=(const opts &) = delete;

	bool show_price = false;
	bool bech32 = true;
	satoshi sats = 0;
	std::string peer_id, peer_addr;
	std::string rpc_dir, rpc_file;
	std::string brpc_dir, brpc_file;

	// timeout for network requests (ms)
	int timeout = 3000;

	std::string log_level = "info";

	// connect/disconnect/fundchannel/close: number of nodes 
	int node_count = 1;

	lit::hosts rpc;
};

class cli : public CLI::App {
	public:
	cli(struct opts & opts) : opts(opts) {}
	virtual void pre_callback() {
		opts.rpc.ld.connect_uds(opts.rpc_dir, opts.rpc_file);
		set_log_level(opts.log_level);
	}
	struct opts & opts;
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

void status(struct opts &opts)
{
	l_info("network is " << (is_testnet(opts.rpc.ld) ? "testnet" : "mainnet"));
	auto nodes = listnodes(opts.rpc.ld);
	l_info(nodes.size() << " nodes in network (" << addressable(nodes) << 
		" addressable).");
	auto channels = listchannels(opts.rpc.ld);
	l_info(channels.size() << " channels in network");
	auto peers = listpeers(opts.rpc.ld);
	l_info(peers.size() << " peers");

	l_info("bitcoin price is " << to_dollars(opts.rpc.https, 100000000));
	l_info("total funds: " << to_dollars(opts.rpc.https, get_funds(opts.rpc.ld)));
	l_info("block count is " << lit::rpc::getblockcount(opts.rpc.bd));
}

void connectn(struct opts &opts)
{
#if 1
	lit::connect(opts.rpc, opts.node_count);
#else
	l_info("connecting to " << opts.node_count << " nodes");
	auto nodes = listnodes(opts.rpc.ld);
	lit::strip_non_addressable(nodes);
	if (opts.node_count > nodes.size())
		PANIC("requesting " << opts.node_count
				    << " connections but there are only "
				    << nodes.size() << " nodes in network.");
	connect_random2(opts.rpc.ld, nodes, opts.node_count);
#endif
}

void closeall(struct opts &opts)
{
	auto peers = listpeers(opts.rpc.ld);
	auto i = lit::closechannel(opts.rpc.ld, peers, true);
	l_info("closed " << i << " channel" << ((i == 1) ? ".\n" : "s.\n"));
}

void bootstrap(struct opts &opts)
{
	lit::bootstrap(opts.rpc);
}

void autopilot(struct opts &opts)
{
	autopilot(opts.rpc);
}

int CLI11_parse(opts &opts, int argc, char **argv)
{
	cli app(opts);
	app.add_option("--lightning-dir", opts.rpc_dir, "lightningd rpc dir",
		       true);
	app.add_option("-f,--rpc_file", opts.rpc_file, "lightningd rpc file",
		       true);
	app.add_flag("-t,--trace", g_json_trace,
		     "Display rpc json requests and responses");
	app.add_option("-l,--log-level", opts.log_level,
		       "log level (io, debug, info, error, fatal)", true)
	    ->check(CLI::IsMember({"io", "debug", "info", "error", "fatal"},
				  CLI::ignore_case));

	app.require_subcommand(1);

	app.add_subcommand("status", "Display summary information.")
	    ->callback([&]() { status(opts); });
	app.add_subcommand("closeall",
			   "Close all open channels, forcibly if necessary.")
	    ->callback([&]() { closeall(opts); });

	auto c = app.add_subcommand("connect", "Connect to random nodes.");
	c->callback([&]() { connectn(opts); });
	c->add_option("-t,--timeout", opts.timeout, "conenction timeout", true);
	c->add_option("count", opts.node_count, "node count");

	app.add_subcommand("bootstrap", "Get a new node connected")
	    ->callback([&]() { bootstrap(opts); });

	CLI11_PARSE(app, argc, argv);
	return 0;
}

int main(int argc, char **argv)
{
	try {
		opts opts;
		return CLI11_parse(opts, argc, argv);
	} catch (std::exception &e) {
		l_fatal(e.what());
	}
}

