#include <wythe/command.h>
#include <wythe/exception.h>
#include <rpc.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <locale>

using json = nlohmann::json;

struct command_flags {
    bool listfunds = false;
} flags;

std::string name = "litcli";

template <typename T>
std::string dollars(T value) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << "$" << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

std::string get_btcusd() {
    auto j = rpc::request_remote("https://api.gemini.com/v1/pubticker/btcusd");
    auto v = j.at("last").get<std::string>();
	return dollars(std::stod(v));
}

json get_funds() {
    json j = { 
        {"method", "listfunds"},
        {"id", name},
        {"params", nullptr}
    };

    if (chdir("/home/wythe/.lightning") != 0) PANIC("cannot chdir to ~/.lightning");
    return rpc::request_local(j);
}

void list_funds() {

    std::cout << std::setw(4) << get_funds() << '\n';
    std::cout << std::setw(4) << get_btcusd() << '\n';

}

int main(int argc, char ** argv) {
    try {
        wythe::command line("lit", "CLI Bitcoin Lightning Wallet", "lit [options]");
        line.add(wythe::option("listfunds", 'f', "Show funds available for opening channels", [&]{ list_funds(); } ));
        line.add(wythe::option("price", 'p', "Show current BTC price in USD", [&]{ std::cout << get_btcusd() << '\n'; } ));

        line.add_note("Use at your own demise.\n");
        line.parse(argc, argv);

    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
}

