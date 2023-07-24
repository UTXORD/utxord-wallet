#include <iostream>
#include <filesystem>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"
#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "channel_keys.hpp"
#include "exechelper.hpp"

#include "test_case_wrapper.hpp"

#include "simple_transaction.hpp"
#include "core_io.h"

using namespace l15;
using namespace l15::core;
using namespace l15::utxord;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

std::unique_ptr<TestcaseWrapper> w;

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

    if(configpath.empty())
    {
        std::cerr << "Bitcoin config is not passed!" << std::endl;
        return 1;
    }

    std::filesystem::path p(configpath);
    if(p.is_relative())
    {
        configpath = (std::filesystem::current_path() / p).string();
    }

    w = std::make_unique<TestcaseWrapper>(configpath, "bitcoin-cli");

    return session.run();
}


static const bytevector seed = unhex<bytevector>(
        "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");


TEST_CASE("singleinout")
{
    MasterKey master_key(seed);
    ChannelKeys utxo_key = master_key.Derive("m/86'/0'/1'/0/65535");

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());
    string funds_txid = w->btc().SendToAddress(addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    xonly_pubkey destination_pk = w->bech32().Decode(w->btc().GetNewAddress());

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    SimpleTransaction tx_contract;
    tx_contract.SetMiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(funds_txid, get<0>(prevout).n, 10000, utxo_key.GetLocalPubKey())));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(7000, destination_pk)));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize());

    std::clog << data << std::endl;

    SimpleTransaction tx_contract1;
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data));

    stringvector txs;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());

    CHECK(txs.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 1);
    CHECK(tx.vout.size() == 1);

    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx)));

}

TEST_CASE("2ins2outs")
{
    MasterKey master_key(seed);
    ChannelKeys utxo_key = master_key.Derive("m/86'/0'/1'/0/0");
    ChannelKeys utxo_key1 = master_key.Derive("m/86'/0'/1'/0/1");

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());
    string funds_txid = w->btc().SendToAddress(addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    string addr1 = w->bech32().Encode(utxo_key1.GetLocalPubKey());
    string funds_txid1 = w->btc().SendToAddress(addr1, FormatAmount(546));
    auto prevout1 = w->btc().CheckOutput(funds_txid1, addr1);

    xonly_pubkey destination_pk = w->bech32().Decode(w->btc().GetNewAddress());
    xonly_pubkey destination_pk1 = w->bech32().Decode(w->btc().GetNewAddress());

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    SimpleTransaction tx_contract;
    tx_contract.SetMiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(funds_txid, get<0>(prevout).n, 10000, utxo_key.GetLocalPubKey())));
    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(funds_txid1, get<0>(prevout1).n, 546, utxo_key1.GetLocalPubKey())));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(546, destination_pk)));
    REQUIRE_NOTHROW(tx_contract.AddChangeOutput(destination_pk1));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize());

    std::clog << data << std::endl;

    SimpleTransaction tx_contract1;
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data));

    stringvector txs;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());

    CHECK(txs.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 2);
    CHECK(tx.vout.size() == 2);

    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx)));
}
