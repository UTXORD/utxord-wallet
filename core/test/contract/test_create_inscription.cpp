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
    std::string fixed_change;
    bool has_change;
    bool is_parent;
    bool has_parent;
    bool return_collection;
    const char* comment;
};

std::string collection_id;
seckey collection_sk;
Transfer collection_utxo;


TEST_CASE("inscribe")
{
    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("inscribe", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'","4'"], "change":["0"], "index_range":"0-256"})");

    KeyPair market_script_key = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair script_key = master_key.Derive("m/86'/1'/3'/0/1", true);
    KeyPair int_key = master_key.Derive("m/86'/1'/4'/0/0", true);
    KeyPair fund_mining_fee_int_key = master_key.Derive("m/86'/1'/4'/0/1", true);
    KeyPair collection_key = master_key.Derive("m/86'/1'/2'/0/1", false);

    std::string destination_addr = w->btc().GetNewAddress();
    std::string market_fee_addr = w->btc().GetNewAddress();
    std::string author_fee_addr = w->btc().GetNewAddress();
    std::string return_addr = w->btc().GetNewAddress();

    std::string fee_rate;
//    try {
//        fee_rate = w->btc().EstimateSmartFee("1");
//    }
//    catch(...) {
        fee_rate = "0.00003";
//    }

    static const std::string avif_hex = "0000001c667479706d696631000000006d696631617669666d696166000000f16d657461000000000000002168646c72000000000000000070696374000000000000000000000000000000000e7069746d0000000000010000001e696c6f630000000004400001000100000000011500010000001e0000002869696e660000000000010000001a696e6665020000000001000061763031496d616765000000007069707270000000516970636f0000001469737065000000000000000100000001000000107061737000000001000000010000001561763143812000000a073800069010d002000000107069786900000000030808080000001769706d61000000000000000100010401028384000000266d6461740a073800069010d0023213164000004800000c066e6b60fb175753a17aa0";
    auto pixel_avif = std::tie("image/avif", avif_hex);

    static const std::string png_hex = "89504e470d0a1a0a0000000d494844520000000100000001010300000025db56ca00000003504c5445ffa500ca92419b0000000a49444154789c636000000002000148afa4710000000049454e44ae426082";
    auto pixel_png = std::tie("image/png", png_hex);

    static const std::string webp_hex = "524946463c000000574542505650382030000000d001009d012a0100010002003425a00274ba01f80003b000feef6497feef37ede6fdbcdff0d1ffcf4cd7983fa6800000";
    auto pixel_webp = std::tie("image/webp", webp_hex);

    static const std::string html_hex = "3c21444f43545950452068746d6c3e3c68746d6c3e3c686561643e3c7469746c653e546573743c2f7469746c653e3c2f686561643e3c626f64793e3c68313e41737365743c2f68313e3c2f626f64793e3c2f68746d6c3e";
    auto simple_html = std::tie("text/html", html_hex);

    static const std::string svg_hex = hex(std::string("<svg width=\"440\" height=\"101\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xml:space=\"preserve\" overflow=\"hidden\"><g transform=\"translate(-82 -206)\"><g><text fill=\"#777777\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 191.984 275)\">sample collection</text></g></g></svg>"));
    auto svg = std::tie("image/svg+xml", svg_hex);

    const auto content = GENERATE_COPY(pixel_avif, pixel_png, pixel_webp, simple_html, svg);

    std::clog << "Fee rate: " << fee_rate << std::endl;

    CreateInscriptionBuilder test_inscription(bech->GetChainMode(), INSCRIPTION);

    REQUIRE_NOTHROW(test_inscription.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(test_inscription.MarketFee("0", market_fee_addr));
    REQUIRE_NOTHROW(test_inscription.AuthorFee("0", author_fee_addr));
    REQUIRE_NOTHROW(test_inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(test_inscription.Data(get<0>(content), get<1>(content)));
    std::string inscription_amount = test_inscription.GetMinFundingAmount("");
    std::string change_amount = test_inscription.GetMinFundingAmount("change");
    std::string child_amount = test_inscription.GetMinFundingAmount("collection");
    std::string child_change_amount = test_inscription.GetMinFundingAmount("change,collection");
    std::string segwit_child_amount = test_inscription.GetMinFundingAmount("collection,p2wpkh_utxo");

    std::clog << "Min amount: " << inscription_amount << std::endl;
    std::clog << "Min amount w/change: " << change_amount << std::endl;
    std::clog << "Min child amount: " << child_amount << std::endl;
    std::clog << "Min child amount w/change: " << child_change_amount << std::endl;

    CreateInscriptionBuilder test_lazy_inscription(bech->GetChainMode(), LASY_INSCRIPTION);

    REQUIRE_NOTHROW(test_lazy_inscription.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(test_lazy_inscription.MarketFee("0", market_fee_addr));
    REQUIRE_NOTHROW(test_lazy_inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(test_lazy_inscription.Data(get<0>(content), get<1>(content)));
    REQUIRE_NOTHROW(test_lazy_inscription.AuthorFee("0.00001", author_fee_addr));
    CAmount lazy_add_amount = ParseAmount(test_lazy_inscription.GetMinFundingAmount("collection")) - ParseAmount(child_amount);

    std::clog << "Lazy add amount: " << FormatAmount(lazy_add_amount) << std::endl;

//    EcdsaKeypair key1(master_key.Derive("m/84'/0'/0'/0/1").GetLocalPrivKey());
//    CreateCondition inscription {{{ ParseAmount(inscription_amount), w->bech32().Encode(l15::Hash160(key1.GetPubKey().as_vector()), bech32::Encoding::BECH32) }}, "0", false, true, false};
    KeyPair key1 = master_key.Derive("m/86'/1'/0'/0/1", false);
    CreateCondition inscription {{{ ParseAmount(inscription_amount), key1.GetP2TRAddress(*bech) }}, "0", "0", false, true, false, false, "inscription"};
    KeyPair key2 = master_key.Derive("m/86'/1'/0'/0/2", false);
    CreateCondition inscription_w_change {{{ 10000, key2.GetP2TRAddress(*bech) }}, "0", "0", true, false, false, false, "inscription_w_change"};
    KeyPair key3 = master_key.Derive("m/86'/1'/0'/0/3", false);
    CreateCondition inscription_w_fee {{{ ParseAmount(inscription_amount) + 43 + 1000, key3.GetP2TRAddress(*bech) }}, "0.00001", "0", false, false, false, false, "inscription_w_fee"};
    KeyPair key4 = master_key.Derive("m/86'/1'/0'/0/4", false);
    KeyPair key4a = master_key.Derive("m/86'/1'/0'/1/4", false);
    CreateCondition inscription_w_change_fee {{{ 8000, key4.GetP2TRAddress(*bech) }, { 20000, key4a.GetP2TRAddress(*bech) }}, "0.00001", "0", true, false, false, false, "inscription_w_change_fee"};
    KeyPair key5 = master_key.Derive("m/86'/1'/0'/0/5", false);
    CreateCondition inscription_w_fix_change {{{ ParseAmount(inscription_amount) + 1043, key5.GetP2TRAddress(*bech) }}, "0", "0.00001", false, true, false, false, "inscription_w_fix_change"};

    KeyPair key6 = master_key.Derive("m/86'/1'/0'/0/6", false);
    CreateCondition child {{{ParseAmount(child_amount), key6.GetP2TRAddress(*bech) }}, "0", "0", false, false, true, false, "child"};
    KeyPair key7 = master_key.Derive("m/86'/1'/0'/0/7", false);
    CreateCondition child_w_change {{{10000, key7.GetP2TRAddress(*bech) }}, "0", "0", true, false, true, false, "child_w_change"};
    KeyPair key8 = master_key.Derive("m/86'/1'/0'/0/8", false);
    CreateCondition child_w_fee {{{ ParseAmount(child_amount) + 43 + 1000, key8.GetP2TRAddress(*bech) }}, "0.00001", "0", false, false, true, false, "child_w_fee"};
    KeyPair key9 = master_key.Derive("m/86'/1'/0'/0/9", false);
    CreateCondition child_w_change_fee {{{10000, key9.GetP2TRAddress(*bech) }}, "0.00001", "0", true, false, true, false, "child_w_change_fee"};
    KeyPair key10 = master_key.Derive("m/86'/1'/0'/0/10", false);
    CreateCondition child_w_change_fixed_change {{{10000, key10.GetP2TRAddress(*bech) }}, "0", "0.00005", true, false, true, false, "child_w_change_fixed_change"};

    KeyPair key11(master_key.Derive("m/84'/1'/0'/0/11", false));
    CreateCondition segwit_child {{{ParseAmount(segwit_child_amount), key11.GetP2WPKHAddress(*bech) }}, "0", "0", false, false, true, false, "segwit_child"};
    KeyPair key12(master_key.Derive("m/84'/1'/0'/0/12", false));
    CreateCondition segwit_child_w_change {{{10000, key12.GetP2WPKHAddress(*bech) }}, "0", "0", true, false, true, false, "segwit_child_w_change"};
    KeyPair key13(master_key.Derive("m/84'/1'/0'/0/13", false));
    CreateCondition segwit_child_w_fee {{{ ParseAmount(segwit_child_amount) + 43 + 1000, key13.GetP2WPKHAddress(*bech) }}, "0.00001", "0", false, false, true, false, "segwit_child_w_fee"};
    KeyPair key14(master_key.Derive("m/84'/1'/0'/0/14", false));
    CreateCondition segwit_child_w_change_fee {{{15000, key14.GetP2WPKHAddress(*bech) }}, "0.00001", "0", true, false, true, true, "segwit_child_w_change_fee"};

    auto condition = GENERATE_COPY(inscription,
                                   inscription_w_change, inscription_w_fee, inscription_w_change_fee, inscription_w_fix_change,
                                   child, child_w_change, child_w_fee, child_w_change_fee, child_w_change_fixed_change,
                                   segwit_child, segwit_child_w_change, segwit_child_w_fee, segwit_child_w_change_fee
                                   );

    stringvector rawtxs;
    bool check_result = false;
    bool lazy = false;

    SECTION("Self inscribe") {
        std::clog << "Self inscribe: " << condition.comment << " ====================================================" << std::endl;

        check_result = true;

        CreateInscriptionBuilder builder_terms(bech->GetChainMode(), INSCRIPTION);
        CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));

        std::string market_terms;
        REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(9, MARKET_TERMS));

        std::clog << "MARKET_TERMS:\n" << market_terms << std::endl;

        CreateInscriptionBuilder builder(bech->GetChainMode(), INSCRIPTION);
        REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

        CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
        CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
        CHECK_NOTHROW(builder.AuthorFee("0", author_fee_addr));
        CHECK_NOTHROW(builder.Data(get<0>(content), get<1>(content)));
        CHECK_NOTHROW(builder.InscribeScriptPubKey(hex(script_key.PubKey())));
        CHECK_NOTHROW(builder.InscribeInternalPubKey(hex(int_key.PubKey())));
        CHECK_NOTHROW(builder.InscribeAddress(condition.is_parent ? collection_key.GetP2TRAddress(*bech) : destination_addr));
        if (condition.fixed_change != "0") {
            CHECK_NOTHROW(builder.FixedChange(condition.fixed_change, destination_addr));
        }
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

        CHECK_NOTHROW(builder.SignCommit(master_key, "fund"));
        CHECK_NOTHROW(builder.SignInscription(master_key, "inscribe"));
        if (condition.has_parent) {
            CHECK_NOTHROW(builder.SignCollection(master_key, "ord"));
        }

        std::string contract;
        REQUIRE_NOTHROW(contract = builder.Serialize(9, INSCRIPTION_SIGNATURE));
        std::clog << "INSCRIPTION_SIGNATURE:\n" << contract << std::endl;

        CreateInscriptionBuilder fin_contract(bech->GetChainMode(), INSCRIPTION);
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
            lazy = true;

            CreateInscriptionBuilder builder_terms(bech->GetChainMode(), LASY_INSCRIPTION);
            CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));
            CHECK_NOTHROW(builder_terms.AuthorFee("0.00001", author_fee_addr));
            CHECK_NOTHROW(builder_terms.MarketInscribeScriptPubKey(hex(market_script_key.PubKey())));
            CHECK_NOTHROW(builder_terms.Collection(collection_id, FormatAmount(collection_utxo.m_amount), collection_utxo.m_addr));
            std::string market_terms;
            REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(9, LASY_INSCRIPTION_MARKET_TERMS));

            std::clog << "Market terms:\n" << market_terms << std::endl;

            CreateInscriptionBuilder builder(bech->GetChainMode(), LASY_INSCRIPTION);
            REQUIRE_NOTHROW(builder.Deserialize(market_terms, LASY_INSCRIPTION_MARKET_TERMS));

            CHECK_NOTHROW(builder.Data(get<0>(content), get<1>(content)));
            CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
            CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
            CHECK_NOTHROW(builder.InscribeInternalPubKey(hex(int_key.PubKey())));
            CHECK_NOTHROW(builder.InscribeScriptPubKey(hex(script_key.PubKey())));
            CHECK_NOTHROW(builder.FundMiningFeeInternalPubKey(hex(fund_mining_fee_int_key.PubKey())));
            CHECK_NOTHROW(builder.InscribeAddress(condition.is_parent ? bech->Encode(collection_key.PubKey()) : destination_addr));
            if (condition.fixed_change != "0") {
                CHECK_NOTHROW(builder.FixedChange(condition.fixed_change, destination_addr));
            }
            CHECK_NOTHROW(builder.ChangeAddress(destination_addr));

            get<0>(condition.utxo.back()) += lazy_add_amount;
            for (const auto& utxo: condition.utxo) {
                string funds_txid = w->btc().SendToAddress(get<1>(utxo), FormatAmount(get<0>(utxo)));
                auto prevout = w->btc().CheckOutput(funds_txid, get<1>(utxo));
                CHECK_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, FormatAmount(get<0>(utxo)), get<1>(utxo)));
            }

            uint32_t txcount = builder.TransactionCount(/*LASY_INSCRIPTION_SIGNATURE*/);
            for (uint32_t i = 0; i < txcount; ++i) {
                std::string rawtx;
                CHECK_NOTHROW(rawtx = builder.RawTransaction(i));

                CMutableTransaction tx;
                CHECK(DecodeHexTx(tx, rawtx));

                LogTx(tx);
            }

            CHECK_NOTHROW(builder.SignCommit(master_key, "fund"));
            CHECK_NOTHROW(builder.SignInscription(master_key, "inscribe"));

            std::string contract;
            REQUIRE_NOTHROW(contract = builder.Serialize(9, LASY_INSCRIPTION_SIGNATURE));
            std::clog << "{LASY_INSCRIPTION_SIGNATURE:" << contract << "\n}" << std::endl;

            CreateInscriptionBuilder fin_builder(bech->GetChainMode(), LASY_INSCRIPTION);
            REQUIRE_NOTHROW(fin_builder.Deserialize(contract, LASY_INSCRIPTION_SIGNATURE));

            CHECK_NOTHROW(fin_builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount), collection_utxo.m_addr));
            if (condition.return_collection) {
                CHECK_NOTHROW(fin_builder.OverrideCollectionAddress(return_addr));
            }
            CHECK_NOTHROW(fin_builder.MarketSignInscription(master_key, "inscribe"));
            CHECK_NOTHROW(fin_builder.SignCollection(master_key, "ord"));

            REQUIRE_NOTHROW(rawtxs = fin_builder.RawTransactions());

//            CMutableTransaction tx;
//            REQUIRE(DecodeHexTx(tx, rawtxs[1]));
//
//            size_t nin = 2;
//            auto spends = fin_builder.GetGenesisTxSpends();
//
//            //for (size_t nin = 0; nin < spends.size(); ++nin) {
//                CAmount amount = spends[nin].nValue;
//
//                PrecomputedTransactionData txdata;
//                txdata.Init(tx, fin_builder.GetGenesisTxSpends(), /* force=*/ true);
//
//                MutableTransactionSignatureChecker TxChecker(&tx, nin, amount, txdata, MissingDataBehavior::FAIL);
//                bool ok = VerifyScript(CScript(), spends[nin].scriptPubKey, &tx.vin[nin].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxChecker);
//                REQUIRE(ok);
//            //}
        }
    }

    if (check_result) {

        CMutableTransaction commitTx, revealTx;

        REQUIRE(rawtxs.size() == 2);
        REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
        REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

        std::clog << condition.comment << " ^^^" << '\n';
        std::clog << "Funding TX min fee: " << CalculateTxFee(1000, commitTx) << " ============================================================" << '\n';
        LogTx(commitTx);
        std::clog << "Genesis TX min fee: " << CalculateTxFee(1000, revealTx) << " ============================================================" << '\n';
        LogTx(revealTx);
        std::clog << "=======================================================================" << '\n';

//        if (condition.has_change && condition.has_parent) {
//            CHECK(commitTx.vout.size() == 3);
//        } else if (condition.has_change || condition.has_parent) {
//            CHECK(commitTx.vout.size() == 2);
//        } else {
//            CHECK(commitTx.vout.size() == 1);
//        }

        if (condition.has_parent) {
            CHECK(revealTx.vin.size() == 3);
            CHECK(revealTx.vout[1].nValue == 546);
        }

        size_t vout_size = 1;
        if (condition.has_parent) vout_size += 1;
        if (ParseAmount(condition.market_fee) > 0) vout_size += 1;
        if (lazy) vout_size += 1;

        CHECK(revealTx.vout.size() == vout_size);

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
    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("inscribe", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'","4'"], "change":["0"], "index_range":"0-256"})");

    KeyPair utxo_key = master_key.Derive("m/86'/1'/0'/0/1", false);
    KeyPair script_key = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair int_key = master_key.Derive("m/86'/1'/4'/0/0", true);
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

    CreateInscriptionBuilder builder_terms(bech->GetChainMode(), INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee("0", destination_addr));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(9, MARKET_TERMS));

    CreateInscriptionBuilder builder(bech->GetChainMode(), INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.Data(content_type, content));
    REQUIRE_NOTHROW(builder.MetaData(hex(condition.metadata)));
    CHECK_NOTHROW(builder.AuthorFee("0", destination_addr));
    CHECK_NOTHROW(builder.InscribeInternalPubKey(hex(int_key.PubKey())));
    CHECK_NOTHROW(builder.InscribeScriptPubKey(hex(script_key.PubKey())));

    std::string min_fund = builder.GetMinFundingAmount(condition.has_parent ? "collection" : "");
    string funds_txid = w->btc().SendToAddress(addr, min_fund);
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    REQUIRE_NOTHROW(builder.InscribeAddress(destination_addr));
    REQUIRE_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, min_fund, addr));

    if (condition.has_parent) {
        CHECK_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount), collection_utxo.m_addr));
    }

    stringvector rawtxs0;
    CHECK_NOTHROW(rawtxs0 = builder.RawTransactions());

    CMutableTransaction commitTx0, revealTx0;

    REQUIRE(DecodeHexTx(commitTx0, rawtxs0[0]));
    REQUIRE(DecodeHexTx(revealTx0, rawtxs0[1]));

//    std::clog << "Commit0: ========================" << std::endl;
//    LogTx(commitTx0);
//    std::clog << "Genesis0: ========================" << std::endl;
//    LogTx(revealTx0);
//    std::clog << "========================" << std::endl;

    REQUIRE_NOTHROW(builder.SignCommit(master_key, "fund"));
    REQUIRE_NOTHROW(builder.SignInscription(master_key, "inscribe"));
    if (condition.has_parent) {
        CHECK_NOTHROW(builder.SignCollection(master_key, "ord"));
    }

    std::string contract = builder.Serialize(9, INSCRIPTION_SIGNATURE);
    std::clog << "Contract JSON: " << contract << std::endl;

    CreateInscriptionBuilder builder2(bech->GetChainMode(), INSCRIPTION);
    builder2.Deserialize(contract, INSCRIPTION_SIGNATURE);

    stringvector rawtxs;
    CHECK_NOTHROW(rawtxs = builder2.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);

    Inscription inscr = ParseInscriptions(rawtxs[1]).front();
    auto result_metadata = inscr.GetMetadata();

    CHECK(result_metadata == condition.metadata);
    std::clog << "metadata:\n" << nlohmann::json::from_cbor(result_metadata).dump() << std::endl;

    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    std::clog << "Commit: ========================" << std::endl;
    LogTx(commitTx);
    std::clog << "Genesis: ========================" << std::endl;
    LogTx(revealTx);
    std::clog << "========================" << std::endl;

    CHECK(revealTx.vout[0].nValue == 546);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    if (condition.save_as_parent) {
        collection_sk = inscribe_key.PrivKey();
        collection_id = revealTx.GetHash().GetHex() + "i0";
        collection_utxo = {revealTx.GetHash().GetHex(), 0, 546, inscribe_key.GetP2TRAddress(*bech)};
    }

    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
}
