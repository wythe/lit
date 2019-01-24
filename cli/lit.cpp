#include <chrono>
#include <iomanip>
#include <locale>
#include <nlohmann/json.hpp>
#include <rpc.h>
#include <wythe/command.h>
#include <wythe/exception.h>

using json = nlohmann::json;
using satoshi = long long;

std::string name = "litcli";

bool g_json_trace = false; // trace json commands

struct command_flags {
	bool listfunds = false;
} flags;

template <typename T> std::string dollars(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << "$" << std::fixed << std::setprecision(2) << value;
	return ss.str();
}

double get_btcusd()
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
		auto j = rpc::request_remote(
		    "https://api.gemini.com/v1/pubticker/btcusd");
		auto last = j.at("last").get<std::string>();
		v = std::stod(last);
	}
	return v;
}

std::string to_dollars(const satoshi &s)
{
	if (s == 0)
		return "$0.00";
	auto price = get_btcusd();
	auto btc = s / 100000000.0;
	return dollars(price * btc);
}

satoshi get_funds()
{
	json j = {{"method", "listfunds"}, {"id", name}, {"params", nullptr}};

	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
	auto res = rpc::request_local(j);
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

std::string new_addr()
{
	std::string type = bech32 ? "bech32" : "p2sh-segwit";
	json j = {{"method", "newaddr"}, {"id", name}, {"params", {type}}};
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
	auto res = rpc::request_local(j);
	return res["result"]["address"];
}

void list_funds()
{
	std::cout << std::setw(4) << to_dollars(get_funds()) << '\n';
}

json list_nodes()
{
	json j = {{"method", "listnodes"}, {"id", name}, {"params", {nullptr}}};
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
	return rpc::request_local(j);
}

json list_peers()
{
	json j = {{"method", "listpeers"}, {"id", name}, {"params", {nullptr}}};
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
	return rpc::request_local(j);
}

template <typename T> json dev_fail(T args)
{
	WARN("ok");
	if (args.size() != 1)
		PANIC("one argument expected, the peer id");
	json j = {{"method", "dev-fail"}, {"id", name}, {"params", {args[0]}}};
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
	WARN("fail");
	return rpc::request_local(j);
}

json connect(const std::string &peer_id, const std::string &peer_addr)
{
	auto id = peer_id;
	if (!peer_addr.empty()) {
		id += '@';
		id += peer_addr;
	}
	json req = {{"method", "connect"}, {"id", name}, {"params", {id}}};
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
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
	if (chdir("/home/wythe/.lightning") != 0)
		PANIC("cannot chdir to ~/.lightning");
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

int main(int argc, char **argv)
{

	bool show_price = false;
	satoshi sats = 0;
	std::string peer_id, peer_addr;

	try {
		using namespace wythe::cli;
		line line("0.1", "lit", "Bitcoin Lightning Wallet",
			  "lit [options] [command] [command-options]");
		line.global_opts.emplace_back(
		    "trace", 't', "Display rpc json request and response",
		    [&] { g_json_trace = true; });

		line.commands.emplace_back(
		    "listfunds", "Show funds available for opening channels",
		    [&] { list_funds(); });
		line.commands.emplace_back("listnodes",
					   "List all the nodes we see",
					   [&] { list_nodes(); });
		line.commands.emplace_back("listpeers", "List our peers",
					   [&] { list_peers(); });
		line.commands.emplace_back("dev-fail", "Fail with peer",
					   [&] { dev_fail(line.targets); });

		auto c = line.commands.emplace(
		    line.commands.end(), "newaddr",
		    "Request a new bitcoin address for use in funding channels",
		    [&] { std::cout << new_addr() << '\n'; });
		c->opts.emplace_back(
		    "p2sh", 'p', "Use p2sh-segwit address (default is bech32)",
		    [&] { bech32 = false; });

		c = line.commands.emplace(line.commands.end(), "connect",
					  "Connect to another node",
					  [&] { connect(peer_id, peer_addr); });
		c->opts.emplace_back("id", 'i', "id of peer to connect to", "",
				     [&](auto s) { peer_id = s; });
		c->opts.emplace_back("host", 'h',
				     "ip address of peer to connect to", "",
				     [&](auto s) { peer_addr = s; });

		c = line.commands.emplace(line.commands.end(), "fundchannel",
					  "Fund channel",
					  [&] { fund_channel(peer_id, sats); });
		c->opts.emplace_back("id", 'i', "id of peer to fund", "",
				     [&](auto s) { peer_id = s; });
		c->opts.emplace_back(option("satoshi", 's',
					    "Amount in satoshis", "0",
					    [&](auto s) { sats = stoll(s); }));

		// higher level commands
		c = line.commands.emplace(
		    line.commands.end(), "fundfirst",
		    "Fund first available compatible node",
		    [&] { fund_first(sats); });
		c->opts.emplace_back(option("satoshi", 's',
					    "Amount in satoshis", "0",
					    [&](auto s) { sats = stoll(s); }));

		line.commands.emplace_back(
		    "price", "Show current BTC price in USD", [&] {
			    show_price = true;
			    std::cout << dollars(get_btcusd()) << '\n';
		    });

		line.notes.emplace_back("Use at your own demise.\n");
		line.parse(argc, argv);

		line.exec(); // perform action

		if (show_price) {
			for (auto &n : line.targets) {
				std::cout
				    << dollars(std::stod(n) * get_btcusd())
				    << '\n';
			}
		}

		// if (!line.targets.empty()) PANIC("unrecognize command line
		// argument: " << line.targets[0]);
	} catch (std::invalid_argument &e) {
		std::cerr << "invalid argument: " << e.what() << std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
}
