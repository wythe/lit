#include <wythe/command.h>
#include <wythe/exception.h>
#include <rpc.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct command_flags {
    bool listfunds = false;
} flags;

std::string name = "litcli";

json get_btcusd() {
    return rpc::request_remote("https://api.gemini.com/v1/pubticker/btcusd");
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

        line.add_note("Use at your own demise.\n");
        line.parse(argc, argv);

    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
}

