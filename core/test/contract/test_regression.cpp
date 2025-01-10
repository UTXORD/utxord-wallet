#include <iostream>
#include <filesystem>
#include <algorithm>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "nlohmann/json.hpp"

#include "util/translation.h"

#include "simple_transaction.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"

using namespace l15;
using namespace l15::core;
using namespace utxord;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

//std::unique_ptr<TestcaseWrapper> w;

int main(int argc, char* argv[])
{
    std::string configpath;
    Catch::Session session;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli
            = session.cli() // Get Catch's composite command line parser
              | Opt(configpath, "Config path") // bind variable to a new option, with a hint string
              ["--config"]    // the option names it will respond to
                      ("Path to L15 config");

    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;

    // if(configpath.empty())
    // {
    //     std::cerr << "Bitcoin config is not passed!" << std::endl;
    //     return 1;
    // }
    //
    // std::filesystem::path p(configpath);
    // if(p.is_relative())
    //     configpath = (std::filesystem::current_path() / p).string();
    //
    // w = std::make_unique<TestcaseWrapper>(configpath);
    // w->InitKeyRegistry("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");
    // w->keyreg().AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");

    int res = session.run();

    // w.reset();

    return res;
}


TEST_CASE("regression")
{
    std::string market_contract_cache;

    std::ifstream s("regression.json");
    auto regression_data = nlohmann::json::parse(s);

    for(const auto& json: regression_data["contracts"]) {
        std::string contract_string = json.dump();
        //std::clog << json["contract_type"] << " v. " << json["params"]["protocol_version"] << "\n" << contract_string << std::endl;

        if (json["contract_type"] == "transaction") {
            SimpleTransaction contract(REGTEST);
            CHECK_NOTHROW(contract.Deserialize(contract_string, SimpleTransaction::ParsePhase(json["params"]["phase"])));

            std::string contract_string2;
            CHECK_NOTHROW( contract_string2 = contract.Serialize(json["params"]["protocol_version"], SimpleTransaction::ParsePhase(json["params"]["phase"])) );

            auto json2 = nlohmann::json::parse(contract_string2);

            CHECK(json == json2);
        }
        else if (json["contract_type"] == "CreateInscription") {
            CreateInscriptionBuilder contract(REGTEST, json["params"]["phase"].get<std::string>().starts_with("LAZY") ? LAZY_INSCRIPTION : INSCRIPTION);

            CHECK_NOTHROW(contract.Deserialize(contract_string, CreateInscriptionBuilder::ParsePhase(json["params"]["phase"])));

            std::string contract_string2;
            CHECK_NOTHROW( contract_string2 = contract.Serialize(json["params"]["protocol_version"], CreateInscriptionBuilder::ParsePhase(json["params"]["phase"])) );

            auto json2 = nlohmann::json::parse(contract_string2);

            CHECK(json == json2);
        }
        else if (json["contract_type"] == "SwapInscription") {
            SwapInscriptionBuilder contract(REGTEST);

            SwapPhase phase = SwapInscriptionBuilder::ParsePhase(json["params"]["phase"]);

            if (phase == MARKET_PAYOFF_SIG) market_contract_cache = contract_string;

            if (phase == FUNDS_SWAP_SIG) {
                REQUIRE_NOTHROW(contract.Deserialize(market_contract_cache, MARKET_PAYOFF_SIG));
            }
            REQUIRE_NOTHROW(contract.Deserialize(contract_string, phase));

            std::string contract_string2;
            REQUIRE_NOTHROW( contract_string2 = contract.Serialize(json["params"]["protocol_version"], phase));


            auto json2 = nlohmann::json::parse(contract_string2);

            CHECK(json == json2);
        }
    }
}
