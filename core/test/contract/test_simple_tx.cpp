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
std::optional<Bech32> bech;

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
    if (w->mMode == "regtest") {
        bech = Bech32(utxord::Hrp<REGTEST>());
    }
    else if (w->mMode == "testnet") {
        bech = Bech32(utxord::Hrp<TESTNET>());
    }
    else if (w->mMode == "mainnet") {
        bech = Bech32(utxord::Hrp<MAINNET>());
    }

    return session.run();
}


static const bytevector seed = unhex<bytevector>(
        "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

struct TestCondition {
    seckey keypair;
    std::string address;
};

TEST_CASE("singleinout")
{
    MasterKey master_key(seed);

    ChannelKeys p2tr_utxo_key = master_key.Derive("m/86'/0'/1'/0/65535");
    seckey p2tr_utxo_sk = p2tr_utxo_key.GetLocalPrivKey();

    std::clog << "P2TR UTXO pubkey: " << hex(p2tr_utxo_key.GetLocalPubKey()) << std::endl;

    seckey p2wpkh_utxo_sk = master_key.Derive("m/84'/0'/0'/0/10").GetLocalPrivKey();
    EcdsaKeypair p2wpkh_utxo_key(p2wpkh_utxo_sk);

    TestCondition p2tr_cond = {p2tr_utxo_sk, w->bech32().Encode(p2tr_utxo_key.GetLocalPubKey())};
    TestCondition p2wpkh_cond = {p2wpkh_utxo_sk, w->bech32().Encode(l15::Hash160(p2wpkh_utxo_key.GetPubKey().as_vector()), bech32::Encoding::BECH32)};

    auto cond = GENERATE_COPY(/*p2tr_cond, */p2wpkh_cond);

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

    SimpleTransaction tx_contract(*bech);
    tx_contract.MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(*bech, funds_txid, get<0>(prevout).n, 10000, cond.address)));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(*bech, 7000, destination_addr)));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize());

    std::clog << "singleinout:\n"
              << data << std::endl;

    SimpleTransaction tx_contract1(*bech);
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data));

    stringvector txs;
    REQUIRE_NOTHROW(txs = tx_contract1.RawTransactions());

    CHECK(txs.size() == 1);

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, txs[0]));

    CHECK(tx.vin.size() == 1);
    CHECK(tx.vout.size() == 1);

    PrecomputedTransactionData txdata;
    txdata.Init(tx, {CTxOut {10000, bech->PubKeyScript(cond.address)}}, /* force=*/ true);

    MutableTransactionSignatureChecker TxOrdChecker(&tx, 0, 10000, txdata, MissingDataBehavior::FAIL);
    bool ok = VerifyScript(CScript(), bech->PubKeyScript(cond.address), &tx.vin.front().scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
    REQUIRE(ok);


//    ECC_Start();
//
//    CKey keypair;
//    keypair.Set(p2wpkh_utxo_sk.begin(), p2wpkh_utxo_sk.end(), true);
//    uint256 hash = SignatureHash(bech->PubKeyScript(cond.address), tx, 0, SIGHASH_ALL, 10000, SigVersion::WITNESS_V0);
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
    MasterKey master_key(seed);
    ChannelKeys utxo_key = master_key.Derive("m/86'/0'/1'/0/0");
    ChannelKeys utxo_key1 = master_key.Derive("m/86'/0'/1'/0/1");

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());
    string funds_txid = w->btc().SendToAddress(addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    string addr1 = w->bech32().Encode(utxo_key1.GetLocalPubKey());
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

    SimpleTransaction tx_contract(*bech);
    tx_contract.MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(*bech, funds_txid, get<0>(prevout).n, 10000, bech->Encode(utxo_key.GetLocalPubKey()))));
    REQUIRE_NOTHROW(tx_contract.AddInput(std::make_shared<UTXO>(*bech, funds_txid1, get<0>(prevout1).n, 546, bech->Encode(utxo_key1.GetLocalPubKey()))));
    REQUIRE_NOTHROW(tx_contract.AddOutput(std::make_shared<P2TR>(*bech, 546, destination_addr)));
    REQUIRE_NOTHROW(tx_contract.AddChangeOutput(destination_addr1));

    REQUIRE_NOTHROW(tx_contract.Sign(master_key));

    std::string data;
    REQUIRE_NOTHROW(data = tx_contract.Serialize());

    std::clog << data << std::endl;

    SimpleTransaction tx_contract1(*bech);
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

TEST_CASE("txchain")
{
    MasterKey master_key(seed);
    ChannelKeys utxo_key = master_key.Derive("m/86'/0'/1'/0/100");
    ChannelKeys intermediate_key = master_key.Derive("m/86'/0'/1'/0/101");

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());
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

    std::shared_ptr<SimpleTransaction> tx_contract = std::make_shared<SimpleTransaction>(*bech);
    std::shared_ptr<SimpleTransaction> tx1_contract = std::make_shared<SimpleTransaction>(*bech);
    tx_contract->MiningFeeRate(fee_rate);
    tx1_contract->MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(tx1_contract->AddInput(tx_contract));
    REQUIRE_NOTHROW(tx1_contract->AddOutput(std::make_shared<P2TR>(*bech, 546, destination_addr)));

    REQUIRE_NOTHROW(tx_contract->AddInput(std::make_shared<UTXO>(*bech, funds_txid, get<0>(prevout).n, 10000, bech->Encode(utxo_key.GetLocalPubKey()))));
    REQUIRE_NOTHROW(tx_contract->AddOutput(std::make_shared<P2TR>(*bech, ParseAmount(tx1_contract->GetMinFundingAmount("")), bech->Encode(intermediate_key.GetLocalPubKey()))));
    REQUIRE_NOTHROW(tx_contract->AddChangeOutput(change_addr));

    REQUIRE_NOTHROW(tx_contract->Sign(master_key));
    REQUIRE_NOTHROW(tx1_contract->Sign(master_key));

    std::string data, data1;
    REQUIRE_NOTHROW(data = tx_contract->Serialize());
    REQUIRE_NOTHROW(data1 = tx1_contract->Serialize());

    std::clog << data << std::endl;
    std::clog << data1 << std::endl;

    SimpleTransaction tx_contract1(*bech);
    REQUIRE_NOTHROW(tx_contract1.Deserialize(data));

    SimpleTransaction tx1_contract1(*bech);
    REQUIRE_NOTHROW(tx1_contract1.Deserialize(data1));

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

