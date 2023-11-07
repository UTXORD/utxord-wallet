#include <iostream>
#include <filesystem>
#include <algorithm>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "nlohmann/json.hpp"

#include "util/translation.h"
#include "core_io.h"

#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "exechelper.hpp"
#include "create_inscription.hpp"
#include "inscription.hpp"
#include "core_io.h"

#include "test_case_wrapper.hpp"
#include "policy/policy.h"

using namespace l15;
using namespace l15::core;
using namespace utxord;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;


std::unique_ptr<TestcaseWrapper> w;
std::optional<Bech32> bech;

std::string GenRandomString(const int len) {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

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

struct Transfer
{
    std::string m_txid;
    uint32_t m_nout;
    CAmount m_amount;
    std::string m_addr;
};

struct CreateCondition
{
    std::vector<std::tuple<CAmount, std::string>> utxo;
    std::string market_fee;
    bool has_change;
    bool is_parent;
    bool has_parent;
    const char* comment;
};

std::string collection_id;
seckey collection_sk;
Transfer collection_utxo;


TEST_CASE("inscribe")
{
    KeyRegistry master_key(*bech, seed);
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("inscribe", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");

    KeyPair script_key = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair inscribe_key = master_key.Derive("m/86'/1'/2'/0/0", false);;
    KeyPair collection_key = master_key.Derive("m/86'/1'/2'/0/1", false);

    std::string destination_addr = w->btc().GetNewAddress();
    std::string market_fee_addr = w->btc().GetNewAddress();

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    std::string content_type = "image/svg+xml";
    const std::string svg = "<svg width=\"440\" height=\"101\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xml:space=\"preserve\" overflow=\"hidden\"><g transform=\"translate(-82 -206)\"><g><text fill=\"#777777\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 191.984 275)\">sample collection</text></g></g></svg>";
    auto content = hex(svg);

    CreateInscriptionBuilder test_inscription(*bech, INSCRIPTION);

    REQUIRE_NOTHROW(test_inscription.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(test_inscription.MarketFee("0", market_fee_addr));
    REQUIRE_NOTHROW(test_inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(test_inscription.Data(content_type, content));
    std::string inscription_amount = test_inscription.GetMinFundingAmount("");
    std::string child_amount = test_inscription.GetMinFundingAmount("collection");
    std::string segwit_child_amount = test_inscription.GetMinFundingAmount("collection,p2wpkh_utxo");

    std::clog << "Min funding: " << inscription_amount << std::endl;

//    EcdsaKeypair key1(master_key.Derive("m/84'/0'/0'/0/1").GetLocalPrivKey());
//    CreateCondition inscription {{{ ParseAmount(inscription_amount), w->bech32().Encode(l15::Hash160(key1.GetPubKey().as_vector()), bech32::Encoding::BECH32) }}, "0", false, true, false};
    KeyPair key1 = master_key.Derive("m/86'/1'/0'/0/1", false);
    CreateCondition inscription {{{ ParseAmount(inscription_amount), key1.GetP2TRAddress(*bech) }}, "0", false, true, false, "inscription"};
    KeyPair key2 = master_key.Derive("m/86'/1'/0'/0/2", false);
    CreateCondition inscription_w_change {{{ 10000, key2.GetP2TRAddress(*bech) }}, "0", true, false, false, "inscription_w_change"};
    KeyPair key3 = master_key.Derive("m/86'/1'/0'/0/3", false);
    CreateCondition inscription_w_fee {{{ ParseAmount(inscription_amount) + 43 + 1000, key3.GetP2TRAddress(*bech) }}, "0.00001", false, false, false, "inscription_w_fee"};
    KeyPair key4 = master_key.Derive("m/86'/1'/0'/0/4", false);
    KeyPair key4a = master_key.Derive("m/86'/1'/0'/1/4", false);
    CreateCondition inscription_w_change_fee {{{ 8000, key4.GetP2TRAddress(*bech) }, { 20000, key4a.GetP2TRAddress(*bech) }}, "0.00001", true, false, false, "inscription_w_change_fee"};

    KeyPair key5 = master_key.Derive("m/86'/1'/0'/0/5", false);
    CreateCondition child {{{ ParseAmount(child_amount), key5.GetP2TRAddress(*bech) }}, "0", false, false, true, "child"};
    KeyPair key6 = master_key.Derive("m/86'/1'/0'/0/6", false);
    CreateCondition child_w_change {{{ 10000, key6.GetP2TRAddress(*bech) }}, "0", true, false, true, "child_w_change"};
    KeyPair key7 = master_key.Derive("m/86'/1'/0'/0/7", false);
    CreateCondition child_w_fee {{{ ParseAmount(child_amount) + 43 + 1000, key7.GetP2TRAddress(*bech) }}, "0.00001", false, false, true, "child_w_fee"};
    KeyPair key8 = master_key.Derive("m/86'/1'/0'/0/8", false);
    CreateCondition child_w_change_fee {{{ 10000, key8.GetP2TRAddress(*bech) }}, "0.00001", true, false, true, "child_w_change_fee"};

    KeyPair key9(master_key.Derive("m/84'/1'/0'/0/9", false));
    CreateCondition segwit_child {{{ ParseAmount(segwit_child_amount), key9.GetP2WPKHAddress(*bech) }}, "0", false, false, true, "segwit_child"};
    KeyPair key10(master_key.Derive("m/84'/1'/0'/0/10", false));
    CreateCondition segwit_child_w_change {{{ 10000, key10.GetP2WPKHAddress(*bech) }}, "0", true, false, true, "segwit_child_w_change"};
    KeyPair key11(master_key.Derive("m/84'/1'/0'/0/11", false));
    CreateCondition segwit_child_w_fee {{{ ParseAmount(segwit_child_amount) + 43 + 1000, key11.GetP2WPKHAddress(*bech) }}, "0.00001", false, false, true, "segwit_child_w_fee"};
    KeyPair key12(master_key.Derive("m/84'/1'/0'/0/12", false));
    CreateCondition segwit_child_w_change_fee {{{ 15000, key12.GetP2WPKHAddress(*bech) }}, "0.00001", true, false, true, "segwit_child_w_change_fee"};

    auto condition = GENERATE_COPY(inscription,
                                   inscription_w_change, inscription_w_fee, inscription_w_change_fee,
                                   child, child_w_change, child_w_fee, child_w_change_fee,
                                   segwit_child, segwit_child_w_change, segwit_child_w_fee, segwit_child_w_change_fee);

    stringvector rawtxs;
    bool check_result = false;

    SECTION("Self inscribe") {
        std::clog << "Self inscribe: " << condition.comment << " ====================================================" << std::endl;

        check_result = true;

        CreateInscriptionBuilder builder_terms(*bech, INSCRIPTION);
        CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));

        std::string market_terms;
        REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(8, MARKET_TERMS));

        std::clog << "MARKET_TERMS:\n" << market_terms << std::endl;

        CreateInscriptionBuilder builder(*bech, INSCRIPTION);
        REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

        CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
        CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
        CHECK_NOTHROW(builder.Data(content_type, content));
        CHECK_NOTHROW(builder.InscribeAddress(condition.is_parent ? collection_key.GetP2TRAddress(*bech) : destination_addr));
        CHECK_NOTHROW(builder.ChangeAddress(destination_addr));

        for (const auto& utxo: condition.utxo) {
            string funds_txid = w->btc().SendToAddress(get<1>(utxo), FormatAmount(get<0>(utxo)));
            auto prevout = w->btc().CheckOutput(funds_txid, get<1>(utxo));
            CHECK_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, FormatAmount(get<0>(utxo)), get<1>(utxo)));
        }

        if (condition.has_parent) {
            CHECK_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount),
                                                  collection_utxo.m_addr));
        }

        CHECK_NOTHROW(builder.SignCommit(master_key, "fund", hex(script_key.PubKey())));
        CHECK_NOTHROW(builder.SignInscription(master_key, "inscribe"));
        if (condition.has_parent) {
            CHECK_NOTHROW(builder.SignCollection(master_key, "ord"));
        }

        std::string contract;
        REQUIRE_NOTHROW(contract = builder.Serialize(8, INSCRIPTION_SIGNATURE));
        std::clog << "INSCRIPTION_SIGNATURE:\n" << contract << std::endl;

        CreateInscriptionBuilder fin_contract(*bech, INSCRIPTION);
        REQUIRE_NOTHROW(fin_contract.Deserialize(contract, INSCRIPTION_SIGNATURE));

        REQUIRE_NOTHROW(rawtxs = fin_contract.RawTransactions());

//        CMutableTransaction tx;
//        REQUIRE(DecodeHexTx(tx, rawtxs[0]));
//
//        std::clog << "TX ============================================================" << '\n';
//        LogTx(tx);
//        std::clog << "===============================================================" << '\n';
//
//
//        std::vector<CTxOut> prevouts;
//        for (const auto& utxo: condition.utxo)
//            prevouts.emplace_back(get<0>(utxo), bech->PubKeyScript(get<1>(utxo)));
//
//        PrecomputedTransactionData txdata;
//        txdata.Init(tx, move(prevouts), /* force=*/ true);
//
//        for (size_t nin = 0; nin < condition.utxo.size(); ++nin) {
//            MutableTransactionSignatureChecker TxOrdChecker(&tx, nin, get<0>(condition.utxo[nin]), txdata, MissingDataBehavior::FAIL);
//            bool ok = VerifyScript(CScript(), bech->PubKeyScript(get<1>(condition.utxo[nin])), &tx.vin.front().scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
//            //REQUIRE(ok);
//        }
    }
    SECTION("lazy inscribe") {
        if (condition.has_parent) {
            std::clog << "Lazy inscribe: " << condition.comment << " ====================================================" << std::endl;

            check_result = true;

            CreateInscriptionBuilder builder_terms(*bech, LASY_INSCRIPTION);
            CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));
            CHECK_NOTHROW(builder_terms.Data(content_type, content));
            CHECK_NOTHROW(builder_terms.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount),
                                                  collection_utxo.m_addr));
            std::string market_terms;
            REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(8, LASY_COLLECTION_MARKET_TERMS));

            CreateInscriptionBuilder builder(*bech, LASY_INSCRIPTION);
            REQUIRE_NOTHROW(builder.Deserialize(market_terms, LASY_COLLECTION_MARKET_TERMS));

            CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
            CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
            CHECK_NOTHROW(builder.InscribeAddress(condition.is_parent ? bech->Encode(collection_key.PubKey()) : destination_addr));
            CHECK_NOTHROW(builder.ChangeAddress(destination_addr));

            for (const auto& utxo: condition.utxo) {
                string funds_txid = w->btc().SendToAddress(get<1>(utxo), FormatAmount(get<0>(utxo)));
                auto prevout = w->btc().CheckOutput(funds_txid, get<1>(utxo));
                CHECK_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, FormatAmount(get<0>(utxo)), get<1>(utxo)));
            }

            CHECK_NOTHROW(builder.SignCommit(master_key, "fund", hex(script_key.PubKey())));

            CHECK_NOTHROW(builder.SignInscription(master_key, "inscribe"));

            std::string contract;
            REQUIRE_NOTHROW(contract = builder.Serialize(8, LASY_COLLECTION_INSCRIPTION_SIGNATURE));
            std::clog << contract << std::endl;

            CreateInscriptionBuilder fin_contract(*bech, LASY_INSCRIPTION);
            REQUIRE_NOTHROW(fin_contract.Deserialize(contract, LASY_COLLECTION_INSCRIPTION_SIGNATURE));

            CHECK_NOTHROW(fin_contract.SignCollection(master_key, "ord"));

            REQUIRE_NOTHROW(rawtxs = fin_contract.RawTransactions());
        }
    }

    if (check_result) {

        CMutableTransaction commitTx, revealTx;

        REQUIRE(rawtxs.size() == 2);
        REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
        REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

        std::clog << condition.comment << " ^^^" << '\n';
        std::clog << "Funding TX ============================================================" << '\n';
        LogTx(commitTx);
        std::clog << "Genesis TX ============================================================" << '\n';
        LogTx(revealTx);
        std::clog << "=======================================================================" << '\n';

        if (condition.has_change && condition.has_parent) {
            CHECK(commitTx.vout.size() == 3);
        } else if (condition.has_change || condition.has_parent) {
            CHECK(commitTx.vout.size() == 2);
        } else {
            CHECK(commitTx.vout.size() == 1);
        }

        if (condition.has_parent) {
            CHECK(revealTx.vin.size() == 3);
            CHECK(revealTx.vout[1].nValue == 546);
        }

        if (condition.has_parent && ParseAmount(condition.market_fee)) {
            CHECK(revealTx.vout.size() == 3);
        } else if (condition.has_parent || ParseAmount(condition.market_fee)) {
            CHECK(revealTx.vout.size() == 2);
        } else {
            CHECK(revealTx.vout.size() == 1);
        }

        CHECK(revealTx.vout[0].nValue == 546);

        if (ParseAmount(condition.market_fee)) {
            CHECK(revealTx.vout.back().nValue == ParseAmount(condition.market_fee));
        }

        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

        if (condition.is_parent) {
            collection_id = revealTx.GetHash().GetHex() + "i0";
            collection_sk = collection_key.PrivKey();
            collection_utxo = {revealTx.GetHash().GetHex(), 0, revealTx.vout[0].nValue, w->bech32().Encode(collection_key.PubKey())};
        } else if (condition.has_parent) {
            collection_utxo.m_txid = revealTx.GetHash().GetHex();
            collection_utxo.m_nout = 1;
            collection_utxo.m_amount = revealTx.vout[1].nValue;
        }

        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
    }
}


struct InscribeWithMetadataCondition {
    bytevector metadata;
    bool save_as_parent;
    bool has_parent;
};

const InscribeWithMetadataCondition short_metadata = {nlohmann::json::to_cbor(nlohmann::json::parse(R"({"name":"sample inscription 1"})")), true, false};
const InscribeWithMetadataCondition exact_520_metadata = {
        nlohmann::json::to_cbor(nlohmann::json::parse(R"({"description":"very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong description","name":"sample inscription 2"})")),
        false, true};
const InscribeWithMetadataCondition long_metadata = {
    nlohmann::json::to_cbor(nlohmann::json::parse(
        "{\"name\":\"sample inscription 3\","
        "\"description\":\"very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong description\"}")),
        false, false};

TEST_CASE("metadata")
{
    KeyRegistry master_key(*bech, seed);
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("inscribe", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");

    KeyPair utxo_key = master_key.Derive("m/86'/1'/0'/0/1", false);
    KeyPair script_key = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair inscribe_key = master_key.Derive("m/86'/1'/2'/0/0", false);;

    string addr = utxo_key.GetP2TRAddress(*bech);

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    std::string content_type = "image/svg+xml";

    const string svg =
R"(<?xml version="1.0" encoding="utf-8"?><svg version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 175 175">
<path style="fill:#777777;" d="M129,151l-11.7-6.6l-7.2-12l-2.8-5.3l-12.4-13l-0.15,11.7l0.5,2.3l1.2,11.3l-8.3,11l-10.9-3.7l-5.9-9.6l-13-27.8l-3.3-3.9l-4.5,3.6l-0.4,4.6l-0.1,3.1l-5,7l-6.8-3.1l-2.3-6.7l-5.3-28.1l-2.4-6.4l-3.1-1.5l-2,0.7l-1.4,4.4l-2.8,5.9l-5.6-1.5
l-2.3-4.8l-2.8-15.8l7.6-14.2l15.1-0.4l8.6,4.2l4.6,2.4l12.5,0.7l3.5-10.5l0.1-1.5l0.1-1.2l0.6-4l13.7-13.3l15.7,12.1l0.3,0.5l11.4,11l16.5-5.5l4-3.6l11.2-8.4l18.3,0.1l11.8,14l-3.7,11.6l-8.4-0.6l-5.1-1.2l-5.6,2.9l-2.5,9.7l9.7,15.2l8.2,14
l-3.4,9.3l-6.6,2.3l-7.4-4.9l-2-2l-6.2-2.1l-2.2,1.4l0.6,5.9l6.8,17.2l2.9,11.6l-5.9,10.5zM94,112l0.3,0l14.3,14.2l2.8,5.4l7,11.7l13.1,5.2l4.7-8.7l-2.8-10.8l-6.8-17.2l0.4-7.6l3.6-2.4l7.8,2.6l2.1,2l6.3,4.3l4.9-1.7l2.8-7.7l-7.6-13l-10.2-16.2
l3-11.3l7.2-3.6l5.8,1.3l6.9,0.7l2.6-9.3l-10.7-12.7l-16.6-0.1l-10.65,8l-4,3.7l-18.4,5.9l-12.4-11.9l-0.3-0.5l-14-11.2l-11.8,11.8l-0.6,3.6l-0.1,1.2l-0.1,1.5l-4.3,12l-14.6-0.5l-4.7-2.5l-8.3-4l-13.4,0.2l-6.6,12.5l2.8,15.1l1.9,4.1l3.3,1
l1.7-4.3l2-5.6l3.5-1.2l4.6,2.2l2.9,7.3l5.3,28l2,6l4.7,2.3l3.4-5.7l0.1-3l0.5-5l7-5l4.6,5l13,27.8l5.5,9l9,3l6.8-9.3l-1-10.7l-0.5-2.3l0.7-13.8zM81,42c1.4,1,2,2.5,1.3,3.4c-0.65,1-2.3,1-3.7-0.07c-1.4-1-2-2.5-1.3-3.4zM73,47c-1,2.2-3.2,3.5-4.7,2.7
c-1.5-0.7-1.8-3-0.7-5.4c1-2.2,3.2-3.5,4.7-2.7z"/></svg>)";

    auto content = hex(svg);

    const auto& condition = GENERATE_REF(short_metadata, exact_520_metadata, long_metadata);

    std::string destination_addr = condition.save_as_parent ? w->bech32().Encode(inscribe_key.PubKey()) : w->btc().GetNewAddress();

    CreateInscriptionBuilder builder_terms(*bech, INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee("0", destination_addr));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(8, MARKET_TERMS));

    CreateInscriptionBuilder builder(*bech, INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.Data(content_type, content));
    REQUIRE_NOTHROW(builder.MetaData(hex(condition.metadata)));

    std::string min_fund = builder.GetMinFundingAmount(condition.has_parent ? "collection" : "");
    string funds_txid = w->btc().SendToAddress(addr, min_fund);
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    REQUIRE_NOTHROW(builder.InscribeAddress(destination_addr));
    REQUIRE_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, min_fund, addr));

    if (condition.has_parent) {
        CHECK_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount), collection_utxo.m_addr));
    }

    REQUIRE_NOTHROW(builder.SignCommit(master_key, "fund", hex(script_key.PubKey())));
    REQUIRE_NOTHROW(builder.SignInscription(master_key, "inscribe"));
    if (condition.has_parent) {
        CHECK_NOTHROW(builder.SignCollection(master_key, "ord"));
    }

//    stringvector rawtxs0;
//    CHECK_NOTHROW(rawtxs0 = builder.RawTransactions());

    std::string contract = builder.Serialize(8, INSCRIPTION_SIGNATURE);
    std::clog << "Contract JSON: " << contract << std::endl;

    CreateInscriptionBuilder builder2(*bech, INSCRIPTION);
    builder2.Deserialize(contract, INSCRIPTION_SIGNATURE);

    stringvector rawtxs;
    CHECK_NOTHROW(rawtxs = builder2.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);

    Inscription inscr(rawtxs[1]);
    const auto& result_metadata = inscr.GetMetadata();

    CHECK(result_metadata == condition.metadata);
    std::clog << "metadata:\n" << nlohmann::json::from_cbor(result_metadata).dump() << std::endl;

    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    CHECK(revealTx.vout[0].nValue == 546);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    if (condition.save_as_parent) {
        collection_sk = inscribe_key.PrivKey();
        collection_id = revealTx.GetHash().GetHex() + "i0";
        collection_utxo = {revealTx.GetHash().GetHex(), 0, 546, inscribe_key.GetP2TRAddress(*bech)};
    }

    std::clog << "Commit: ========================" << std::endl;
    LogTx(commitTx);
    std::clog << "Genesis: ========================" << std::endl;
    LogTx(revealTx);
    std::clog << "========================" << std::endl;

    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
}
