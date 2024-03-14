#include <iostream>
#include <filesystem>
#include <algorithm>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"
#include "core_io.h"
#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "channel_keys.hpp"

#include "exechelper.hpp"

#include "test_case_wrapper.hpp"
#include "simple_transaction.hpp"

#include "key.h"
#include "policy/policy.h"

using namespace l15;
using namespace l15::core;
using namespace utxord;

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

    w = std::make_unique<TestcaseWrapper>(configpath);

    return session.run();
}


static const bytevector seed = unhex<bytevector>(
        "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

struct TestCondition {
    KeyPair keypair;
    std::string address;
};

TEST_CASE("singleinout")
{
    KeyRegistry master_key(w->chain(), hex(seed));
    master_key.AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");

    KeyPair p2tr_utxo_key = master_key.Derive("m/86'/1'/0'/0/255", false);

    std::clog << "P2TR UTXO pubkey: " << hex(p2tr_utxo_key.PubKey()) << std::endl;

    KeyPair p2wpkh_utxo_key = master_key.Derive("m/84'/1'/0'/0/10", false);

    TestCondition p2tr_cond = {p2tr_utxo_key, p2tr_utxo_key.GetP2TRAddress(Bech32(w->chain()))};
    TestCondition p2wpkh_cond = {p2wpkh_utxo_key, p2wpkh_utxo_key.GetP2WPKHAddress(Bech32(w->chain()))};

    auto cond = GENERATE_COPY(p2tr_cond, p2wpkh_cond);

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    string funds_txid = w->btc().SendToAddress(cond.address, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, cond.address);

    std::string destination_addr = w->btc().GetNewAddress();

    SimpleTransaction tx_contract(w->chain());
    tx_contract.MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(Bech32(w->chain()), funds_txid, get<0>(prevout).n, 10000, cond.address)));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(w->chain(), 7000, destination_addr)));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key, "funds"));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize(2, TX_SIGNATURE));

    std::clog << "singleinout:\n"
              << data << std::endl;

    SimpleTransaction tx_contract1(w->chain());
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data, TX_SIGNATURE));

    stringvector txs;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());

    CHECK(txs.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 1);
    CHECK(tx.vout.size() == 1);

    PrecomputedTransactionData txdata;
    txdata.Init(tx, {CTxOut {10000, Bech32(w->chain()).PubKeyScript(cond.address)}}, /* force=*/ true);

    MutableTransactionSignatureChecker TxOrdChecker(&tx, 0, 10000, txdata, MissingDataBehavior::FAIL);
    bool ok = VerifyScript(CScript(), Bech32(w->chain()).PubKeyScript(cond.address), &tx.vin.front().scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
    REQUIRE(ok);


//    ECC_Start();
//
//    CKey keypair;
//    keypair.Set(p2wpkh_utxo_sk.begin(), p2wpkh_utxo_sk.end(), true);
//    uint256 hash = SignatureHash(Bech32(w->chain()).PubKeyScript(cond.address), tx, 0, SIGHASH_ALL, 10000, SigVersion::WITNESS_V0);
//
//    std::clog << "Test sighash: " << hash.GetHex() << std::endl;
//
//    std::vector<unsigned char> sig;
//    sig.resize(72);
//    CHECK(keypair.Sign(hash, sig));
//
//    ECC_Stop();
//
//    sig.push_back((unsigned char)SIGHASH_ALL);
//
//    tx.vin.front().scriptWitness.stack.front() = sig;

    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx)));

}

TEST_CASE("2ins2outs")
{
    KeyRegistry master_key(w->chain(), hex(seed));
    master_key.AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");
    KeyPair utxo_key = master_key.Derive("m/86'/1'/1'/0/0", false);
    KeyPair utxo_key1 = master_key.Derive("m/86'/1'/1'/0/1", false);

    string addr = utxo_key.GetP2TRAddress(Bech32(w->chain()));
    string funds_txid = w->btc().SendToAddress(addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    string addr1 = utxo_key1.GetP2TRAddress(Bech32(w->chain()));
    string funds_txid1 = w->btc().SendToAddress(addr1, FormatAmount(546));
    auto prevout1 = w->btc().CheckOutput(funds_txid1, addr1);

    std::string destination_addr = w->btc().GetNewAddress();
    std::string destination_addr1 = w->btc().GetNewAddress();

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    SimpleTransaction tx_contract(w->chain());
    tx_contract.MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(Bech32(w->chain()), funds_txid, get<0>(prevout).n, 10000, utxo_key.GetP2TRAddress(Bech32(w->chain())))));
    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(Bech32(w->chain()), funds_txid1, get<0>(prevout1).n, 546, utxo_key1.GetP2TRAddress(Bech32(w->chain())))));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(w->chain(), 546, destination_addr)));
    REQUIRE_NOTHROW(tx_contract.AddChangeOutput(destination_addr1));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key, "funds"));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize(2, TX_SIGNATURE));

    std::clog << data << std::endl;

    SimpleTransaction tx_contract1(w->chain());
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data, TX_SIGNATURE));

    stringvector txs;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());

    CHECK(txs.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 2);
    CHECK(tx.vout.size() == 2);

    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx)));
}

TEST_CASE("txchain")
{
    KeyRegistry master_key(w->chain(), hex(seed));
    master_key.AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");
    KeyPair utxo_key = master_key.Derive("m/86'/1'/1'/0/100", false);
    KeyPair intermediate_key = master_key.Derive("m/86'/1'/1'/0/101", false);

    string addr = utxo_key.GetP2TRAddress(Bech32(w->chain()));
    string funds_txid = w->btc().SendToAddress(addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    std::string destination_addr = w->btc().GetNewAddress();
    std::string change_addr = w->btc().GetNewAddress();

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    std::shared_ptr<SimpleTransaction> tx_contract = std::make_shared<SimpleTransaction>(w->chain());
    std::shared_ptr<SimpleTransaction> tx1_contract = std::make_shared<SimpleTransaction>(w->chain());
    tx_contract->MiningFeeRate(fee_rate);
    tx1_contract->MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract->AddInput(std::make_shared<UTXO>(Bech32(w->chain()), funds_txid, get<0>(prevout).n, 10000, addr)));
    REQUIRE_NOTHROW(tx_contract->AddOutput(std::make_shared<P2TR>(w->chain(), 10000, intermediate_key.GetP2TRAddress(Bech32(w->chain())))));

    REQUIRE_NOTHROW(tx1_contract->AddInput(make_shared<ContractOutput>(tx_contract, 0)));
    REQUIRE_NOTHROW(tx1_contract->AddOutput(std::make_shared<P2TR>(w->chain(), 546, destination_addr)));

    REQUIRE_NOTHROW(tx_contract->Outputs().back()->Amount(ParseAmount(tx1_contract->GetMinFundingAmount(""))));
    REQUIRE_NOTHROW(tx_contract->AddChangeOutput(change_addr));

    REQUIRE_NOTHROW(tx_contract->Sign(master_key, "funds"));
    REQUIRE_NOTHROW(tx1_contract->Sign(master_key, "funds"));

    std::string data, data1;
    REQUIRE_NOTHROW(data = tx_contract->Serialize(2, TX_SIGNATURE));
    REQUIRE_NOTHROW(data1 = tx1_contract->Serialize(2, TX_SIGNATURE));

    std::clog << data << std::endl;
    std::clog << data1 << std::endl;

    SimpleTransaction tx_contract1(w->chain());
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data, TX_SIGNATURE));

    SimpleTransaction tx1_contract1(w->chain());
    REQUIRE_NOTHROW(tx1_contract1.Deserialize(data1, TX_SIGNATURE));

    stringvector txs, txs1;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());
    REQUIRE_NOTHROW(txs1 = tx1_contract1.RawTransactions());

    CHECK(txs.size() == 1);
    CHECK(txs1.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 1);
    CHECK(tx.vout.size() == 2);

    CMutableTransaction tx1;
    REQUIRE(DecodeHexTx(tx1, txs1[0]));

    CHECK(tx1.vin.size() == 1);
    CHECK(tx1.vout.size() == 1);

    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx)));
    CHECK_NOTHROW(w->btc().SpendTx(CTransaction(tx1)));
}

