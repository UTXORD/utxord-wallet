#include <iostream>
#include <filesystem>
#include <algorithm>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "nlohmann/json.hpp"

#include "util/translation.h"
#include "core_io.h"

#include "chain_api.hpp"
#include "simple_transaction.hpp"
#include "create_inscription.hpp"
#include "runes.hpp"
#include "inscription.hpp"

#include "test_case_wrapper.hpp"
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
        configpath = (std::filesystem::current_path() / p).string();

    w = std::make_unique<TestcaseWrapper>(configpath);
    w->InitKeyRegistry("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");
    w->keyreg().AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    w->keyreg().AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    w->keyreg().AddKeyType("inscribe", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'","4'"], "change":["0" , "1"], "index_range":"0-256"})");

    int res = session.run();

    w.reset();

    return res;
}


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
    CAmount market_fee;
    CAmount fixed_change;
    bool has_change;
    bool has_parent;
    bool return_collection;
    uint32_t ext_fee_count;
    uint32_t min_version;
    const char* comment;
};

std::string collection_id;
std::string delegate_id;
Transfer collection_utxo;
CAmount fee_rate;

static const auto pixel_avif = std::tuple<std::string, bytevector>("image/avif", unhex<bytevector>("0000001c667479706d696631000000006d696631617669666d696166000000f16d657461000000000000002168646c72000000000000000070696374000000000000000000000000000000000e7069746d0000000000010000001e696c6f630000000004400001000100000000011500010000001e0000002869696e660000000000010000001a696e6665020000000001000061763031496d616765000000007069707270000000516970636f0000001469737065000000000000000100000001000000107061737000000001000000010000001561763143812000000a073800069010d002000000107069786900000000030808080000001769706d61000000000000000100010401028384000000266d6461740a073800069010d0023213164000004800000c066e6b60fb175753a17aa0"));
static const auto pixel_png = std::tuple<std::string, bytevector>("image/png", unhex<bytevector>("89504e470d0a1a0a0000000d494844520000000100000001010300000025db56ca00000003504c5445ffa500ca92419b0000000a49444154789c636000000002000148afa4710000000049454e44ae426082"));
static const auto pixel_webp = std::tuple<std::string, bytevector>("image/webp", unhex<bytevector>("524946463c000000574542505650382030000000d001009d012a0100010002003425a00274ba01f80003b000feef6497feef37ede6fdbcdff0d1ffcf4cd7983fa6800000"));
static const auto simple_html = std::tuple<std::string, bytevector>("text/html", unhex<bytevector>("3c21444f43545950452068746d6c3e3c68746d6c3e3c686561643e3c7469746c653e546573743c2f7469746c653e3c2f686561643e3c626f64793e3c68313e41737365743c2f68313e3c2f626f64793e3c2f68746d6c3e"));
static const auto no_content = std::tuple<std::string, bytevector>("", {});

static const char* svg_text = "<svg width=\"440\" height=\"101\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xml:space=\"preserve\" overflow=\"hidden\"><g transform=\"translate(-82 -206)\"><g><text fill=\"#777777\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 191.984 275)\">sample collection</text></g></g></svg>";
static const bytevector svg_bytes = bytevector(svg_text, svg_text + strlen(svg_text));
static const auto svg = std::tuple<std::string, bytevector>("image/svg+xml", svg_bytes);


TEST_CASE("simple_inscribe")
{
    std::string change_addr = w->btc().GetNewAddress();
    std::string market_fee_addr = w->btc().GetNewAddress();
    std::string author_fee_addr = w->btc().GetNewAddress();

    fee_rate = 3000;

    CreateInscriptionBuilder inscription(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(inscription.MarketFee(0, market_fee_addr));
    REQUIRE_NOTHROW(inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(inscription.OrdOutput(546, w->p2tr(2,0,1)));
    REQUIRE_NOTHROW(inscription.ChangeAddress(change_addr));
    REQUIRE_NOTHROW(inscription.InscribeScriptPubKey(w->derive(86,3,0,1).GetSchnorrKeyPair().GetPubKey()));
    REQUIRE_NOTHROW(inscription.InscribeInternalPubKey(w->derive(86,4,0,0).GetSchnorrKeyPair().GetPubKey()));
    REQUIRE_NOTHROW(inscription.AddInput(w->fund(10000, w->p2tr(0,0,0))));

    REQUIRE_NOTHROW(inscription.SignCommit(w->keyreg(), "fund"));
    REQUIRE_NOTHROW(inscription.SignInscription(w->keyreg(), "inscribe"));

    stringvector rawtxs;
    REQUIRE_NOTHROW(rawtxs = inscription.RawTransactions());

    CMutableTransaction commitTx, genesisTx;

    REQUIRE(rawtxs.size() == 2);
    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(genesisTx, rawtxs[1]));


    std::clog << "\nCommitTX:\n";
    LogTx(w->chain(), commitTx);
    std::clog << "\nGenesisTX:\n";
    LogTx(w->chain(), genesisTx);

    CHECK(commitTx.vin.size() == 1);
    CHECK(commitTx.vout.size() == 2);

    CHECK(genesisTx.vin.size() == 1);
    CHECK(genesisTx.vout.size() == 1);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(genesisTx)));

    w->confirm(1, genesisTx.GetHash().GetHex());

    collection_id = genesisTx.GetHash().GetHex() + "i0";
    collection_utxo = {genesisTx.GetHash().GetHex(), 0, genesisTx.vout[0].nValue, w->p2tr(2,0,1)};

}



TEST_CASE("inscribe")
{
    std::string destination_addr = w->btc().GetNewAddress();
    std::string market_fee_addr = w->btc().GetNewAddress();
    std::string author_fee_addr = w->btc().GetNewAddress();
    std::string extra_fee_addr = w->btc().GetNewAddress();
    std::string return_addr = w->btc().GetNewAddress();

    fee_rate = 3000;

    auto content = GENERATE_COPY(pixel_avif/*, pixel_png, pixel_webp, simple_html*/, svg, no_content);

    std::clog << "Fee rate: " << fee_rate << std::endl;

    CreateInscriptionBuilder test_inscription(w->chain(), INSCRIPTION);

    REQUIRE_NOTHROW(test_inscription.OrdOutput(546, destination_addr));
    REQUIRE_NOTHROW(test_inscription.MarketFee(0, market_fee_addr));
    REQUIRE_NOTHROW(test_inscription.AuthorFee(0, author_fee_addr));
    REQUIRE_NOTHROW(test_inscription.MiningFeeRate(fee_rate));
    if (get<1>(content).empty()) {
        if (delegate_id.empty()) {
            REQUIRE_NOTHROW(test_inscription.Data(get<0>(simple_html), get<1>(simple_html)));
        }
        else {
            REQUIRE_NOTHROW(test_inscription.Delegate(delegate_id));
        }
    }
    else {
        REQUIRE_NOTHROW(test_inscription.Data(get<0>(content), get<1>(content)));
    }
    CAmount inscription_amount = test_inscription.GetMinFundingAmount("");
    CAmount child_amount = test_inscription.GetMinFundingAmount("collection");
    CAmount segwit_child_amount = test_inscription.GetMinFundingAmount("collection,p2wpkh_utxo");

    CreateInscriptionBuilder test_lazy_inscription(w->chain(), LAZY_INSCRIPTION);

    REQUIRE_NOTHROW(test_lazy_inscription.OrdOutput(546, destination_addr));
    REQUIRE_NOTHROW(test_lazy_inscription.MarketFee(0, market_fee_addr));
    REQUIRE_NOTHROW(test_lazy_inscription.MiningFeeRate(fee_rate));
    if (get<1>(content).empty()) {
        if (delegate_id.empty()) {
            REQUIRE_NOTHROW(test_lazy_inscription.Data(get<0>(simple_html), get<1>(simple_html)));
        }
        else {
            REQUIRE_NOTHROW(test_lazy_inscription.Delegate(delegate_id));
        }
    }
    else {
        REQUIRE_NOTHROW(test_lazy_inscription.Data(get<0>(content), get<1>(content)));
    }
    REQUIRE_NOTHROW(test_lazy_inscription.AuthorFee(1000, author_fee_addr));
    CAmount lazy_add_amount = test_lazy_inscription.GetMinFundingAmount("collection") - child_amount;

//    CreateCondition inscription {{{ ParseAmount(inscription_amount), w->p2wpkh(0,0,1) }}, 0, false, true, false};
    CreateCondition inscription {{{ inscription_amount, w->p2tr(0,0,1) }}, 0, 0, false, false, false, 0, 8, "inscription"};
    CreateCondition inscription_w_change {{{ 10000, w->p2tr(0,0,2) }}, 0, 0, true, false, false, 0, 8, "inscription_w_change"};
    CreateCondition inscription_w_fee {{{ inscription_amount + (43 * fee_rate / 1000) + 1000, w->p2tr(0,0,3) }}, 1000, 0, false, false, false, 0, 8, "inscription_w_fee"};
    CreateCondition inscription_w_change_fee {{{ 8000, w->p2tr(0, 0, 4) }, { 20000, w->p2tr(0, 1, 4) }}, 1000, 0, true, false, false, 0, 8, "inscription_w_change_fee"};
    CreateCondition inscription_w_fix_change {{{ inscription_amount + 1043, w->p2tr(0, 0,5) }}, 0, 1000, false, false, false, 0, 9, "inscription_w_fix_change"};

    CreateCondition child {{{child_amount, w->p2tr(0, 0, 6) }}, 0, 0, false, true, false, 0, 8, "child"};
    CreateCondition child_w_change {{{10000, w->p2tr(0, 0, 7) }}, 0, 0, true,  true, false, 0, 8, "child_w_change"};
    CreateCondition child_w_fee {{{ child_amount + (43 * fee_rate / 1000) + 1000, w->p2tr(0, 0, 8) }}, 1000, 0, false, true, false, 0, 8, "child_w_fee"};
    CreateCondition child_w_change_fee {{{10000, w->p2tr(0, 0, 9) }}, 1000, 0, true, true, false, 0, 8, "child_w_change_fee"};
    CreateCondition child_w_change_fixed_change {{{10000, w->p2tr(0, 0, 10) }}, 0, 5000, true, true, false, 0, 9, "child_w_change_fixed_change"};

    CreateCondition segwit_child {{{ segwit_child_amount, w->p2wpkh(0,0,11) }}, 0, 0, false, true, false, 0, 8, "segwit_child"};
    CreateCondition segwit_child_w_change {{{10000, w->p2wpkh(0,0,12) }}, 0, 0, true, true, false, 0, 8, "segwit_child_w_change"};
    CreateCondition segwit_child_w_fee {{{ segwit_child_amount + (43 * fee_rate / 1000) + 1000, w->p2wpkh(0,0,13) }}, 1000, 0, false, true, false, 0, 8, "segwit_child_w_fee"};
    CreateCondition segwit_child_w_change_fee {{{15000, w->p2wpkh(0,0,14) }}, 1000, 0, true, true, false, 0, 8, "segwit_child_w_change_fee"};

    // CreateCondition child_extfee_1 {{{20000, w->p2tr(0,0,15) }}, 0, 0, false, true, false, 11, 1, "child_extfee_1"};
    CreateCondition child_w_change_extfee_1 {{{20000, w->p2tr(0,0,16) }}, 0, 0, true, true, false, 1, 11, "child_w_change_extfee_1"};
    // CreateCondition child_w_fee_extfee_1 {{{ child_amount + (43 * fee_rate / 1000) + 1000, w->p2tr(0,0,17) }}, 1000, 0, false, true, false, 11, 1, "child_w_fee_extfee_1"};
    CreateCondition child_w_change_fee_extfee_1 {{{20000, w->p2tr(0,0,18) }}, 1000, 0, true, true, false, 1, 11, "child_w_change_fee_extfee_1"};
    CreateCondition child_w_change_fixed_change_extfee_2 {{{20000, w->p2tr(0,0,19) }}, 0, 5000, true, true, false, 2, 11, "child_w_change_fixed_change_extfee_2"};

    auto version = GENERATE(8,9,10,11);
    auto condition = GENERATE_COPY(inscription,
                                   inscription_w_change, inscription_w_fee, inscription_w_change_fee, inscription_w_fix_change,
                                   child, child_w_change, child_w_fee, child_w_change_fee, child_w_change_fixed_change,
                                   segwit_child, segwit_child_w_change, segwit_child_w_fee, segwit_child_w_change_fee,
                                   /*child_extfee_1,*/ child_w_change_extfee_1, /*child_w_fee_extfee_1,*/ child_w_change_fee_extfee_1, child_w_change_fixed_change_extfee_2
    );

    if (condition.min_version <= version) {
        stringvector rawtxs;
        std::vector<CTxOut> spends;
        bool check_result = false;
        bool lazy = false;

        SECTION("Self inscribe") {
            std::clog << "Self inscribe v." << version << ": " << condition.comment << " ====================================================" << std::endl;

            check_result = true;

            CreateInscriptionBuilder builder_terms(w->chain(), INSCRIPTION);
            CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));
            for (uint32_t i = 0; i < condition.ext_fee_count; ++i) {
                CHECK_NOTHROW(builder_terms.AddCustomFee(1000, extra_fee_addr));
            }

            std::string market_terms;
            REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(version, MARKET_TERMS));

            //std::clog << "MARKET_TERMS:\n" << market_terms << std::endl;

            CreateInscriptionBuilder builder(w->chain(), INSCRIPTION);
            REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

            CHECK_NOTHROW(builder.OrdOutput(546, destination_addr));
            CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
            if (get<1>(content).empty()) {
                if (delegate_id.empty()) {
                    REQUIRE_NOTHROW(builder.Data(get<0>(simple_html), get<1>(simple_html)));
                }
                else {
                    REQUIRE_NOTHROW(builder.Delegate(delegate_id));
                }
            }
            else {
                REQUIRE_NOTHROW(builder.Data(get<0>(content), get<1>(content)));
            }
            CHECK_NOTHROW(builder.InscribeScriptPubKey(w->derive(86,3,0,1).GetSchnorrKeyPair().GetPubKey()));
            CHECK_NOTHROW(builder.InscribeInternalPubKey(w->derive(86,4,0,0).GetSchnorrKeyPair().GetPubKey()));
            if (condition.fixed_change != 0) {
                CHECK_NOTHROW(builder.FixedChange(condition.fixed_change, destination_addr));
            }
            CHECK_NOTHROW(builder.ChangeAddress(destination_addr));

            for (const auto &utxo: condition.utxo) {
                CHECK_NOTHROW(builder.AddInput(w->fund(get<0>(utxo), get<1>(utxo))));
            }

            if (condition.has_parent) {
                CHECK_NOTHROW(builder.AddCollectionUTXO(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, collection_utxo.m_amount,
                                                      collection_utxo.m_addr));
            }

            CHECK_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
            CHECK_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));
            if (condition.has_parent) {
                CHECK_NOTHROW(builder.SignCollection(w->keyreg(), "ord"));
            }

            std::string contract;
            REQUIRE_NOTHROW(contract = builder.Serialize(version, INSCRIPTION_SIGNATURE));
            // std::clog << "INSCRIPTION_SIGNATURE:\n" << contract << std::endl;

            CreateInscriptionBuilder fin_contract(w->chain(), INSCRIPTION);
            REQUIRE_NOTHROW(fin_contract.Deserialize(contract, INSCRIPTION_SIGNATURE));

            REQUIRE_NOTHROW(rawtxs = fin_contract.RawTransactions());


            spends = fin_contract.GetGenesisTxSpends();
//        CMutableTransaction tx;
//        REQUIRE(DecodeHexTx(tx, rawtxs[0]));
//
//        std::clog << "TX ============================================================" << '\n';
//        LogTx(w->chain(), tx);
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
                std::clog << "Lazy inscribe v." << version << ": " << condition.comment << " ====================================================" << std::endl;

                check_result = true;
                lazy = true;

                CreateInscriptionBuilder builder_terms(w->chain(), LAZY_INSCRIPTION);
                CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, market_fee_addr));
                CHECK_NOTHROW(builder_terms.AuthorFee(1000, author_fee_addr));
                for (uint32_t i = 0; i < condition.ext_fee_count; ++i) {
                    CHECK_NOTHROW(builder_terms.AddCustomFee(1000, extra_fee_addr));
                }
                CHECK_NOTHROW(builder_terms.MarketInscribeScriptPubKey(w->derive(86, 3, 0, 0).GetSchnorrKeyPair().GetPubKey()));
                CHECK_NOTHROW(builder_terms.Collection(collection_id, collection_utxo.m_amount, collection_utxo.m_addr));
                std::string market_terms;
                REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(version, LAZY_INSCRIPTION_MARKET_TERMS));

                // std::clog << "{LASY_INSCRIPTION_MARKET_TERMS:\n" << market_terms << "\n}" << std::endl;

                CreateInscriptionBuilder builder(w->chain(), LAZY_INSCRIPTION);
                REQUIRE_NOTHROW(builder.Deserialize(market_terms, LAZY_INSCRIPTION_MARKET_TERMS));

                if (get<1>(content).empty()) {
                    if (delegate_id.empty()) {
                        REQUIRE_NOTHROW(builder.Data(get<0>(simple_html), get<1>(simple_html)));
                    }
                    else {
                        REQUIRE_NOTHROW(builder.Delegate(delegate_id));
                    }
                }
                else {
                    REQUIRE_NOTHROW(builder.Data(get<0>(content), get<1>(content)));
                }
                CHECK_NOTHROW(builder.OrdOutput(546, destination_addr));
                CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
                CHECK_NOTHROW(builder.InscribeInternalPubKey(w->derive(86,4,0,0).GetSchnorrKeyPair().GetPubKey()));
                CHECK_NOTHROW(builder.InscribeScriptPubKey(w->derive(86,3,0,1).GetSchnorrKeyPair().GetPubKey()));
                CHECK_NOTHROW(builder.FundMiningFeeInternalPubKey(w->derive(86,4,0,1).GetSchnorrKeyPair().GetPubKey()));
                if (condition.fixed_change != 0) {
                    CHECK_NOTHROW(builder.FixedChange(condition.fixed_change, destination_addr));
                }
                CHECK_NOTHROW(builder.ChangeAddress(destination_addr));

                get<0>(condition.utxo.back()) += lazy_add_amount;
                for (const auto &utxo: condition.utxo) {
                    CHECK_NOTHROW(builder.AddInput(w->fund(get<0>(utxo), get<1>(utxo))));
                }

                uint32_t txcount = builder.TransactionCount(LAZY_INSCRIPTION_SIGNATURE);
                for (uint32_t i = 0; i < txcount; ++i) {
                    std::string rawtx;
                    CHECK_NOTHROW(rawtx = builder.RawTransaction(LAZY_INSCRIPTION_SIGNATURE, i));

                    CMutableTransaction tx;
                    CHECK(DecodeHexTx(tx, rawtx));

                    //LogTx(w->chain(), tx);
                }

                CHECK_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
                CHECK_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));

                std::string contract;
                REQUIRE_NOTHROW(contract = builder.Serialize(version, LAZY_INSCRIPTION_SIGNATURE));
                // std::clog << "{LASY_INSCRIPTION_SIGNATURE:" << contract << "\n}" << std::endl;

                CreateInscriptionBuilder fin_builder(w->chain(), LAZY_INSCRIPTION);
                REQUIRE_NOTHROW(fin_builder.Deserialize(contract, LAZY_INSCRIPTION_SIGNATURE));

                CHECK_NOTHROW(fin_builder.AddCollectionUTXO(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, collection_utxo.m_amount, collection_utxo.m_addr));
                if (condition.return_collection) {
                    CHECK_NOTHROW(fin_builder.OverrideCollectionAddress(return_addr));
                }
                CHECK_NOTHROW(fin_builder.MarketSignInscription(w->keyreg(), "inscribe"));
                CHECK_NOTHROW(fin_builder.SignCollection(w->keyreg(), "ord"));

                REQUIRE_NOTHROW(rawtxs = fin_builder.RawTransactions());

                spends = fin_builder.GetGenesisTxSpends();

//                CMutableTransaction tx;
//                REQUIRE(DecodeHexTx(tx, rawtxs[1]));
//
//                //size_t nin = 2;
//
//                for (size_t nin = 0; nin < spends.size(); ++nin) {
//                    CAmount amount = spends[nin].nValue;
//
//                    PrecomputedTransactionData txdata;
//                    txdata.Init(tx, fin_builder.GetGenesisTxSpends(), /* force=*/ true);
//
//                    MutableTransactionSignatureChecker TxChecker(&tx, nin, amount, txdata, MissingDataBehavior::FAIL);
//                    bool ok = VerifyScript(CScript(), spends[nin].scriptPubKey, &tx.vin[nin].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxChecker);
//                    REQUIRE(ok);
//                }
            }
        }

        if (check_result) {

            CMutableTransaction commitTx, revealTx;

            REQUIRE(rawtxs.size() == 2);
            REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
            REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

            std::clog << condition.comment << " ^^^" << '\n';
            std::clog << "Funding TX min fee: " << CalculateTxFee(fee_rate, commitTx) << " ============================================================" << '\n';
            LogTx(w->chain(), commitTx);
            std::clog << "Genesis TX min fee: " << CalculateTxFee(fee_rate, revealTx) << " ============================================================" << '\n';
            LogTx(w->chain(), revealTx);
            std::clog << "=======================================================================" << '\n';

            if (condition.has_parent) {
                CHECK(revealTx.vin.size() == 3);
                CHECK(revealTx.vout[1].nValue == 546);
            }

            size_t vout_size = 1 + condition.ext_fee_count;
            if (condition.has_parent) vout_size += 1;
            if (condition.market_fee > 0) vout_size += 1;
            if (lazy) vout_size += 1;

            CHECK(revealTx.vout.size() == vout_size);

            CHECK(revealTx.vout[0].nValue == 546);

            if (condition.market_fee) {
                CHECK(revealTx.vout.back().nValue == condition.market_fee);
            }

            REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
//            REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

            try {
                w->btc().SpendTx(CTransaction(revealTx));
            }
            catch(...) {
                bool ok = true;

                PrecomputedTransactionData txdata;
                txdata.Init(revealTx, move(spends), /* force=*/ true);

                for (size_t nin = 0; nin < txdata.m_spent_outputs.size(); ++nin) {
                    std::clog << "Input " << nin << " signature ... ";
                    CAmount amount = txdata.m_spent_outputs[nin].nValue;

                    MutableTransactionSignatureChecker txChecker(&revealTx, nin, amount, txdata, MissingDataBehavior::FAIL);
                    ScriptError err;
                    ok &= VerifyScript(CScript(), txdata.m_spent_outputs[nin].scriptPubKey, &revealTx.vin[nin].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, txChecker, &err);

                    if (!ok) {
                        std::clog << " FAIL: " << ScriptErrorString(err) << std::endl;
                        FAIL("Genesis TX error on input " + std::to_string(nin));
                    }
                    else {
                        std::clog << " ok" << std::endl;
                    }
                }
                if (ok) {
                    std::clog << w->btc().TestTxSequence({revealTx}) << std::endl;

                    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));
                }
            }

            if (condition.has_parent) {
                collection_utxo.m_txid = revealTx.GetHash().GetHex();
                collection_utxo.m_nout = 1;
                collection_utxo.m_amount = revealTx.vout[1].nValue;
            }

            if (delegate_id.empty()) {
                delegate_id = collection_id;
            }

            w->confirm(1, revealTx.GetHash().GetHex());
        }
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
    string addr = w->p2tr(0,0,1);

    fee_rate = 1000;

    std::string content_type = "image/svg+xml";

    const char* svg =
R"(<?xml version="1.0" encoding="utf-8"?><svg version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 175 175">
<path style="fill:#777777;" d="M129,151l-11.7-6.6l-7.2-12l-2.8-5.3l-12.4-13l-0.15,11.7l0.5,2.3l1.2,11.3l-8.3,11l-10.9-3.7l-5.9-9.6l-13-27.8l-3.3-3.9l-4.5,3.6l-0.4,4.6l-0.1,3.1l-5,7l-6.8-3.1l-2.3-6.7l-5.3-28.1l-2.4-6.4l-3.1-1.5l-2,0.7l-1.4,4.4l-2.8,5.9l-5.6-1.5
l-2.3-4.8l-2.8-15.8l7.6-14.2l15.1-0.4l8.6,4.2l4.6,2.4l12.5,0.7l3.5-10.5l0.1-1.5l0.1-1.2l0.6-4l13.7-13.3l15.7,12.1l0.3,0.5l11.4,11l16.5-5.5l4-3.6l11.2-8.4l18.3,0.1l11.8,14l-3.7,11.6l-8.4-0.6l-5.1-1.2l-5.6,2.9l-2.5,9.7l9.7,15.2l8.2,14
l-3.4,9.3l-6.6,2.3l-7.4-4.9l-2-2l-6.2-2.1l-2.2,1.4l0.6,5.9l6.8,17.2l2.9,11.6l-5.9,10.5zM94,112l0.3,0l14.3,14.2l2.8,5.4l7,11.7l13.1,5.2l4.7-8.7l-2.8-10.8l-6.8-17.2l0.4-7.6l3.6-2.4l7.8,2.6l2.1,2l6.3,4.3l4.9-1.7l2.8-7.7l-7.6-13l-10.2-16.2
l3-11.3l7.2-3.6l5.8,1.3l6.9,0.7l2.6-9.3l-10.7-12.7l-16.6-0.1l-10.65,8l-4,3.7l-18.4,5.9l-12.4-11.9l-0.3-0.5l-14-11.2l-11.8,11.8l-0.6,3.6l-0.1,1.2l-0.1,1.5l-4.3,12l-14.6-0.5l-4.7-2.5l-8.3-4l-13.4,0.2l-6.6,12.5l2.8,15.1l1.9,4.1l3.3,1
l1.7-4.3l2-5.6l3.5-1.2l4.6,2.2l2.9,7.3l5.3,28l2,6l4.7,2.3l3.4-5.7l0.1-3l0.5-5l7-5l4.6,5l13,27.8l5.5,9l9,3l6.8-9.3l-1-10.7l-0.5-2.3l0.7-13.8zM81,42c1.4,1,2,2.5,1.3,3.4c-0.65,1-2.3,1-3.7-0.07c-1.4-1-2-2.5-1.3-3.4zM73,47c-1,2.2-3.2,3.5-4.7,2.7
c-1.5-0.7-1.8-3-0.7-5.4c1-2.2,3.2-3.5,4.7-2.7z"/></svg>)";

    auto content = bytevector(svg, svg + strlen(svg));

    const auto& condition = GENERATE_REF(short_metadata, exact_520_metadata, long_metadata);

    std::string destination_addr = condition.save_as_parent ? w->p2tr(2, 0, 0) : w->btc().GetNewAddress();

    CreateInscriptionBuilder builder_terms(w->chain(), INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee(0, destination_addr));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(9, MARKET_TERMS));

    CreateInscriptionBuilder builder(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    REQUIRE_NOTHROW(builder.OrdOutput(546, destination_addr));
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.Data(content_type, content));
    REQUIRE_NOTHROW(builder.MetaData(condition.metadata));
    CHECK_NOTHROW(builder.AuthorFee(0, destination_addr));
    CHECK_NOTHROW(builder.InscribeInternalPubKey(w->derive(86, 4, 0, 0).GetSchnorrKeyPair().GetPubKey()));
    CHECK_NOTHROW(builder.InscribeScriptPubKey(w->derive(86, 3, 0, 0).GetSchnorrKeyPair().GetPubKey()));

    CAmount min_fund = builder.GetMinFundingAmount(condition.has_parent ? "collection" : "");
    REQUIRE_NOTHROW(builder.AddInput(w->fund(min_fund, addr)));

    if (condition.has_parent) {
        CHECK_NOTHROW(builder.AddCollectionUTXO(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, collection_utxo.m_amount, collection_utxo.m_addr));
    }

    stringvector rawtxs0;
    CHECK_NOTHROW(rawtxs0 = builder.RawTransactions());

    CMutableTransaction commitTx0, revealTx0;

    REQUIRE(DecodeHexTx(commitTx0, rawtxs0[0]));
    REQUIRE(DecodeHexTx(revealTx0, rawtxs0[1]));

    std::clog << "Commit0: ========================" << std::endl;
    LogTx(w->chain(), commitTx0);
    std::clog << "Genesis0: ========================" << std::endl;
    LogTx(w->chain(), revealTx0);
    std::clog << "========================" << std::endl;

    REQUIRE_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
    REQUIRE_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));
    if (condition.has_parent) {
        CHECK_NOTHROW(builder.SignCollection(w->keyreg(), "ord"));
    }

    std::string contract = builder.Serialize(9, INSCRIPTION_SIGNATURE);
    std::clog << "Contract JSON: " << contract << std::endl;

    CreateInscriptionBuilder builder2(w->chain(), INSCRIPTION);
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

    CHECK(revealTx.vout[0].nValue == 546);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    if (condition.save_as_parent) {
        collection_id = revealTx.GetHash().GetHex() + "i0";
        collection_utxo = {revealTx.GetHash().GetHex(), 0, 546, w->p2tr(2, 0, 0)};
    }

    w->confirm(1, revealTx.GetHash().GetHex());
}

TEST_CASE("legacy_addr_out")
{
    string addr = w->p2tr(0,0,1);

    fee_rate = 1000;

    std::string content_type = "image/svg+xml";

    const char* svg =
R"(<?xml version="1.0" encoding="utf-8"?><svg version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 175 175">
<path style="fill:#777777;" d="M129,151l-11.7-6.6l-7.2-12l-2.8-5.3l-12.4-13l-0.15,11.7l0.5,2.3l1.2,11.3l-8.3,11l-10.9-3.7l-5.9-9.6l-13-27.8l-3.3-3.9l-4.5,3.6l-0.4,4.6l-0.1,3.1l-5,7l-6.8-3.1l-2.3-6.7l-5.3-28.1l-2.4-6.4l-3.1-1.5l-2,0.7l-1.4,4.4l-2.8,5.9l-5.6-1.5
l-2.3-4.8l-2.8-15.8l7.6-14.2l15.1-0.4l8.6,4.2l4.6,2.4l12.5,0.7l3.5-10.5l0.1-1.5l0.1-1.2l0.6-4l13.7-13.3l15.7,12.1l0.3,0.5l11.4,11l16.5-5.5l4-3.6l11.2-8.4l18.3,0.1l11.8,14l-3.7,11.6l-8.4-0.6l-5.1-1.2l-5.6,2.9l-2.5,9.7l9.7,15.2l8.2,14
l-3.4,9.3l-6.6,2.3l-7.4-4.9l-2-2l-6.2-2.1l-2.2,1.4l0.6,5.9l6.8,17.2l2.9,11.6l-5.9,10.5zM94,112l0.3,0l14.3,14.2l2.8,5.4l7,11.7l13.1,5.2l4.7-8.7l-2.8-10.8l-6.8-17.2l0.4-7.6l3.6-2.4l7.8,2.6l2.1,2l6.3,4.3l4.9-1.7l2.8-7.7l-7.6-13l-10.2-16.2
l3-11.3l7.2-3.6l5.8,1.3l6.9,0.7l2.6-9.3l-10.7-12.7l-16.6-0.1l-10.65,8l-4,3.7l-18.4,5.9l-12.4-11.9l-0.3-0.5l-14-11.2l-11.8,11.8l-0.6,3.6l-0.1,1.2l-0.1,1.5l-4.3,12l-14.6-0.5l-4.7-2.5l-8.3-4l-13.4,0.2l-6.6,12.5l2.8,15.1l1.9,4.1l3.3,1
l1.7-4.3l2-5.6l3.5-1.2l4.6,2.2l2.9,7.3l5.3,28l2,6l4.7,2.3l3.4-5.7l0.1-3l0.5-5l7-5l4.6,5l13,27.8l5.5,9l9,3l6.8-9.3l-1-10.7l-0.5-2.3l0.7-13.8zM81,42c1.4,1,2,2.5,1.3,3.4c-0.65,1-2.3,1-3.7-0.07c-1.4-1-2-2.5-1.3-3.4zM73,47c-1,2.2-3.2,3.5-4.7,2.7
c-1.5-0.7-1.8-3-0.7-5.4c1-2.2,3.2-3.5,4.7-2.7z"/></svg>)";

    auto content = bytevector(svg, svg + strlen(svg));

    std::string destination_addr = w->btc().GetNewAddress("", "p2sh-segwit");

    CreateInscriptionBuilder builder_terms(w->chain(), INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee(1000, destination_addr));
    CHECK_NOTHROW(builder_terms.AddCustomFee(1000, destination_addr));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(12, MARKET_TERMS));

    CreateInscriptionBuilder builder(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    REQUIRE_NOTHROW(builder.OrdOutput(546, destination_addr));
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.Data(content_type, content));
    CHECK_NOTHROW(builder.AuthorFee(1000, destination_addr));
    CHECK_NOTHROW(builder.ChangeAddress(destination_addr));
    CHECK_NOTHROW(builder.InscribeInternalPubKey(w->derive(86, 4, 0, 0).GetSchnorrKeyPair().GetPubKey()));
    CHECK_NOTHROW(builder.InscribeScriptPubKey(w->derive(86, 3, 0, 0).GetSchnorrKeyPair().GetPubKey()));

    CAmount min_fund = builder.GetMinFundingAmount("");
    REQUIRE_NOTHROW(builder.AddInput(w->fund(/*min_fund*/10000, addr)));

    stringvector rawtxs0;
    CHECK_NOTHROW(rawtxs0 = builder.RawTransactions());

    CMutableTransaction commitTx0, revealTx0;

    REQUIRE(DecodeHexTx(commitTx0, rawtxs0[0]));
    REQUIRE(DecodeHexTx(revealTx0, rawtxs0[1]));

    std::clog << "Commit0: ========================" << std::endl;
    LogTx(w->chain(), commitTx0);
    std::clog << "Genesis0: ========================" << std::endl;
    LogTx(w->chain(), revealTx0);
    std::clog << "========================" << std::endl;

    REQUIRE_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
    REQUIRE_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));

    std::string contract = builder.Serialize(12, INSCRIPTION_SIGNATURE);
    std::clog << "Contract JSON: " << contract << std::endl;

    CreateInscriptionBuilder builder2(w->chain(), INSCRIPTION);
    builder2.Deserialize(contract, INSCRIPTION_SIGNATURE);

    stringvector rawtxs;
    CHECK_NOTHROW(rawtxs = builder2.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);

    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    CHECK(revealTx.vout[0].nValue == 546);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    w->confirm(1, revealTx.GetHash().GetHex());
}

struct EtchParams
{
    uint128_t amount_per_mint;
    uint128_t pre_mint;
    uint128_t transfer;
    uint128_t burn;
};

TEST_CASE("etch")
{
    std::string market_fee_addr = w->btc().GetNewAddress();
    std::string author_fee_addr = w->btc().GetNewAddress();
    std::string return_addr = w->btc().GetNewAddress();

    fee_rate = 3000;

    auto content = std::tuple<std::string, bytevector>("image/avif", unhex<bytevector>("0000001c667479706d696631000000006d696631617669666d696166000000f16d657461000000000000002168646c72000000000000000070696374000000000000000000000000000000000e7069746d0000000000010000001e696c6f630000000004400001000100000000011500010000001e0000002869696e660000000000010000001a696e6665020000000001000061763031496d616765000000007069707270000000516970636f0000001469737065000000000000000100000001000000107061737000000001000000010000001561763143812000000a073800069010d002000000107069786900000000030808080000001769706d61000000000000000100010401028384000000266d6461740a073800069010d0023213164000004800000c066e6b60fb175753a17aa0"));

    const char* svg_text = "<svg width=\"440\" height=\"101\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xml:space=\"preserve\" overflow=\"hidden\"><g transform=\"translate(-82 -206)\"><g><text fill=\"#777777\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 191.984 275)\">sample collection</text></g></g></svg>";
    auto svg = std::tuple<std::string, bytevector>("image/svg+xml", bytevector(svg_text, svg_text + strlen(svg_text)));

     auto condition = GENERATE(
         EtchParams{65535, 0, 65535, 0},
         EtchParams{65535, 65535, 60000, 5535},
         EtchParams{32767, 65535, 5000, 2767}
     );

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, std::numeric_limits<uint16_t>::max());
    uint128_t suffix_rune = dist(rng);

    std::string spaced_name = "UTXORD TEST " + DecodeRune(suffix_rune);

    std::clog << "====" << spaced_name << "====" << std::endl;

    Rune rune(spaced_name, " ", 0x2204);
    rune.AmountPerMint() = condition.amount_per_mint;
    rune.MintCap() = 1;

    RuneStone runestone = condition.pre_mint ? rune.EtchAndMint(condition.pre_mint, 0) : rune.Etch();

    CreateInscriptionBuilder test_inscription(w->chain(), INSCRIPTION);

    REQUIRE_NOTHROW(test_inscription.OrdOutput(10000, w->p2tr(2, 0, 1)));
    REQUIRE_NOTHROW(test_inscription.MarketFee(0, market_fee_addr));
    REQUIRE_NOTHROW(test_inscription.AuthorFee(0, author_fee_addr));
    REQUIRE_NOTHROW(test_inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(test_inscription.Data(get<0>(content), get<1>(content)));
    REQUIRE_NOTHROW(test_inscription.Rune(std::make_shared<RuneStoneDestination>(w->chain(), runestone)));

    CreateInscriptionBuilder builder_terms(w->chain(), INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee(0, market_fee_addr));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(10, MARKET_TERMS));

    //std::clog << "MARKET_TERMS:\n" << market_terms << std::endl;

    CreateInscriptionBuilder builder(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    CHECK_NOTHROW(builder.OrdOutput(10000, w->p2tr(2, 0, 1)));
    CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
    CHECK_NOTHROW(builder.AuthorFee(0, author_fee_addr));
    CHECK_NOTHROW(builder.Data(get<0>(content), get<1>(content)));
    CHECK_NOTHROW(builder.InscribeScriptPubKey(w->derive(86, 3, 0, 1).GetSchnorrKeyPair().GetPubKey()));
    CHECK_NOTHROW(builder.InscribeInternalPubKey(w->derive(86, 4, 0, 0).GetSchnorrKeyPair().GetPubKey()));
    CHECK_NOTHROW(builder.ChangeAddress(w->btc().GetNewAddress()));
    CHECK_NOTHROW(builder.Rune(std::make_shared<RuneStoneDestination>(w->chain(), runestone)));

    CHECK_NOTHROW(builder.AddInput(w->fund(test_inscription.GetMinFundingAmount(""), w->p2tr(0, 0, 0))));

    CHECK_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
    CHECK_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));

    std::string contract;
     REQUIRE_NOTHROW(contract = builder.Serialize(10, INSCRIPTION_SIGNATURE));
    std::clog << "INSCRIPTION_SIGNATURE:\n" << contract << std::endl;

    CreateInscriptionBuilder fin_builder(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(fin_builder.Deserialize(contract, INSCRIPTION_SIGNATURE));

    stringvector rawtxs;
    REQUIRE_NOTHROW(rawtxs = fin_builder.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);
    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));

    w->btc().GenerateToAddress(return_addr, "4");

    LogTx(w->chain(), revealTx);

    REQUIRE_THROWS(w->btc().SpendTx(CTransaction(revealTx)));

    w->confirm(5, commitTx.GetHash().GetHex());

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    LogTx(w->chain(), revealTx);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    auto confirm = w->confirm(1, revealTx.GetHash().GetHex());

    auto rune_obj = w->rune(rune.RuneText(""));

    std::clog << rune_obj << std::endl;

    CHECK(condition.pre_mint == rune_obj["supply"].get<uint64_t>());

    rune.RuneId(get<0>(confirm), get<1>(confirm));

    SECTION("mint")
    {
            SimpleTransaction mintBuilder(w->chain());
            mintBuilder.MiningFeeRate(fee_rate);

            REQUIRE_NOTHROW(mintBuilder.AddInput(w->fund(10000, w->p2tr(0, 0, 0))));

            RuneStone mintrunestone = rune.Mint(1);

            REQUIRE_NOTHROW(mintBuilder.AddOutputDestination(std::make_shared<RuneStoneDestination>(w->chain(), move(mintrunestone))));
            REQUIRE_NOTHROW(mintBuilder.AddChangeOutput(w->p2tr(0, 1, 1)));
            REQUIRE_NOTHROW(mintBuilder.Sign(w->keyreg(), "fund"));

            std::string mint_contract;
            REQUIRE_NOTHROW(mint_contract = mintBuilder.Serialize(2, TX_SIGNATURE));

            std::clog << "Mint contract:\n" << mint_contract << std::endl;

            SimpleTransaction mintBuilder1(w->chain());
            REQUIRE_NOTHROW(mintBuilder1.Deserialize(mint_contract, TX_SIGNATURE));

            auto mint_raw_tx = mintBuilder1.RawTransactions();

            CHECK(mint_raw_tx.size() == 1);

            CMutableTransaction mintTx;
            REQUIRE(DecodeHexTx(mintTx, mint_raw_tx[0]));

            LogTx(w->chain(), mintTx);

            REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(mintTx)));

        w->confirm(1, mintTx.GetHash().GetHex(), 1);

        auto rune_json = w->rune(rune.RuneText(""));
        std::clog << rune_json << std::endl;

        uint128_t supply = condition.pre_mint + condition.amount_per_mint;

        CHECK(supply == rune_json["supply"].get<uint64_t>());

        SECTION("transfer")
        {
            SimpleTransaction transferBuilder(w->chain());
            transferBuilder.MiningFeeRate(fee_rate);

            REQUIRE_NOTHROW(transferBuilder.AddRuneInput(mintBuilder.ChangeOutput(), *rune.RuneId(), condition.amount_per_mint));
            REQUIRE_NOTHROW(transferBuilder.AddInput(w->fund(10000, w->p2tr(0, 0, 1))));
            REQUIRE_NOTHROW(transferBuilder.AddRuneOutput(546, w->btc().GetNewAddress(), *rune.RuneId(), condition.transfer));
            if ((condition.transfer + condition.burn) < condition.amount_per_mint) {
                REQUIRE_NOTHROW(transferBuilder.AddRuneOutput(546, w->btc().GetNewAddress(), *rune.RuneId(), condition.amount_per_mint - condition.transfer - condition.burn));
            }
            if (condition.burn > 0) {
                REQUIRE_NOTHROW(transferBuilder.BurnRune(*rune.RuneId(), condition.burn));
            }
            REQUIRE_NOTHROW(transferBuilder.AddChangeOutput(w->btc().GetNewAddress()));

            REQUIRE_NOTHROW(transferBuilder.PartialSign(w->keyreg(), "fund", 0));

            std::string transfer_contract;
            REQUIRE_NOTHROW(transfer_contract = transferBuilder.Serialize(3, TX_TERMS));

            std::clog << "Rune transfer terms:\n" << transfer_contract << std::endl;

            SimpleTransaction transferBuilder1(w->chain());

            transferBuilder1.Deserialize(transfer_contract, TX_TERMS);

            REQUIRE_NOTHROW(transferBuilder1.PartialSign(w->keyreg(), "fund", 1));

            auto transfer_raw_tx = transferBuilder1.RawTransactions();
            CMutableTransaction transferTx;
            REQUIRE(DecodeHexTx(transferTx, transfer_raw_tx[0]));

            LogTx(w->chain(), transferTx);

            REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(transferTx)));

            w->confirm(1, transferTx.GetHash().GetHex());

            auto rune_balance_json = w->rune_balances(rune.RuneText("•"));
            std::clog << rune_balance_json << std::endl;

            CHECK(condition.transfer == (rune_balance_json[transferTx.GetHash().GetHex() + ":0"]["amount"].get<uint64_t>()));
            if ((condition.transfer + condition.burn) < condition.amount_per_mint) {
                CHECK((condition.amount_per_mint - condition.transfer - condition.burn) == (rune_balance_json[transferTx.GetHash().GetHex() + ":2"]["amount"].get<uint64_t>()));
            }

            auto rune_json = w->rune(rune.RuneText(""));

            CHECK(condition.burn == rune_json["burned"].get<uint64_t>());
        }
    }
}

struct AvatarCondition
{
    std::string content;
    std::string content_type;
    bool is_parent;
    bool skip_for_avatar;
};

static const std::string avatar_html = R"(<!DOCTYPE html>
<html><head>
<script>
function cbor(raw){
let a=new Uint8Array(raw.match(/[\da-f]{2}/gi).map(function(h){return parseInt(h,16)}))
let d=a.buffer.slice(a.byteOffset,a.byteLength+a.byteOffset)
var dv=new DataView(d)
var off=0
function cr(l,v){off+=l;return v}
function ru8(){return cr(1,dv.getUint8(off))}
function rb(){if(dv.getUint8(off) !== 0xff)return false;off+=1;return true}
function rl(f){
if(f<24)return f
if(f===24)return ru8()
if(f===25)return cr(2,dv.getUint16(off))
if(f===26)return cr(4,dv.getUint32(off))
if(f===27)return cr(4,dv.getUint32(off))*0x0100000000+cr(4,dv.getUint32(off))
if(f===31)return-1
throw"len"}
function risl(t){
let ib=ru8()
if(ib===0xff)return-1
let l=rl(ib&0x1f)
if(l<0||(ib>>5)!==t)throw"len"
return l}
function item(){
let ib=ru8(),t=ib>>5,inf=ib&0x1f,i,l
if(t===7){
switch(inf){case 25:case 26:case 27:throw"float"}}
l=rl(inf)
if(l<0&&(t<2||6<t))throw"len"
switch(t){
case 0:return l
case 1:return-1-l
case 2:
if(l<0){
let el=[],al=0
while((l=risl(t))>=0){al += l;el.push(cr(l, new Uint8Array(d, off, l)))}
let fr=new Uint8Array(al),ff=0
for(i=0;i<el.length;++i){fr.set(el[i],ff);ff+=el[i].length}
return fr}
return cr(l,new Uint8Array(d,off,l))
case 3:
for(i=0,s='';i<l;i++){var x=ru8().toString(16);if(x.length<2) x=`0${x}`;s+=`%${x}`}
return decodeURIComponent(s)
case 5:
let r={}
for(i=0;i<l||l<0&&!rb();++i){let k=item();r[k]=item()}
return r
case 6:return item()
case 7:
switch(l){
case 20:return false
case 21:return true
case 22:return null
default:return undefined}}}
try{
let r=item()
if(off !== d.byteLength) return{}
return r
}catch(e){return{}}}
(async()=>{
let h={method:'GET',headers:{'Accept':'application/json'}}
async function a(){
let s=(await(await fetch('/r/children/----',h)).json()).ids[0]
let c=await(await fetch(`/r/children/${s}`,h)).json()
for(let i=0;c.more;++i){
let c1=await fetch(`/r/children/${s}/${i}`,h).json()
c.ids=c.concat(c1.ids)
c.more=c1.more}
do{let d=await fetch(`/r/metadata/${c.ids.at(-1)}`)
if(d.status!==200)break
let m=cbor(await d.json())
if(m.avatar===false) c.ids.pop();else break
}while(c.ids.length>0)
if(c.ids.length>0){
let i=document.createElement('img')
i.src=`/content/${c.ids.at(-1)}`
document.body.appendChild(i)}}
window.addEventListener("load",async()=>{await a()})})()
</script>
</head><body/></html>)";

static const std::string svg_1 = R"lit(<svg width="440" height="101" xmlns="http://www.w3.org/2000/svg">
<g transform="translate(-82 -206)">
<text fill="#777777" font-family="Arial,Arial_MSFontService,sans-serif" font-variant="normal" font-weight="400" font-size="37" transform="matrix(1 0 0 1 191.984 275)">s1</text>
</g></svg>)lit";

static const std::string svg_2 = R"lit(<svg width="440" height="101" xmlns="http://www.w3.org/2000/svg">
<g transform="translate(-82 -206)"><text fill="#777777" font-family="Arial,Arial_MSFontService,sans-serif" font-style="normal" font-variant="normal" font-weight="400" font-stretch="normal" font-size="37" transform="matrix(1 0 0 1 191.984 275)">s2</text></g></svg>)lit";
static const std::string svg_3 = R"lit(<svg width="440" height="101" xmlns="http://www.w3.org/2000/svg"><g transform="translate(-82 -206)"><text fill="#777777" font-family="Arial,Arial_MSFontService,sans-serif" font-style="normal" font-variant="normal" font-weight="400" font-stretch="normal" font-size="37" transform="matrix(1 0 0 1 191.984 275)">s3</text></g></svg>)lit";


std::optional<Transfer> avatar_collection_utxo;

TEST_CASE("avatar")
{
    fee_rate = 3000;

    auto &condition = GENERATE_REF(
                                    AvatarCondition{"", "text/ascii", true, false},
                                    AvatarCondition{avatar_html, "text/html", true, false},
                                    AvatarCondition{svg_1, "image/svg+xml", false, false},
                                    AvatarCondition{svg_2, "image/svg+xml", false, false},
                                    AvatarCondition{svg_3, "image/svg+xml", false, true});

    CreateInscriptionBuilder builder(w->chain(), INSCRIPTION);
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.MarketFee(0, ""));
    REQUIRE_NOTHROW(builder.AuthorFee(0, ""));
    REQUIRE_NOTHROW(builder.OrdOutput(546, w->p2tr(0, 0, 0)));

    std::string content = condition.content;
    if (avatar_collection_utxo) {
        auto pos = content.find("----");
        if (pos != content.npos) {
            content.replace(pos, 4, collection_id);
            std::clog << "Avatar size: " << content.length() << std::endl;
        }
    }

    REQUIRE_NOTHROW(builder.Data(condition.content_type, bytevector(content.begin(), content.end())));
    if (condition.skip_for_avatar) {
        REQUIRE_NOTHROW(builder.MetaData(nlohmann::json::to_cbor(nlohmann::json::parse("{\"avatar\":false,\"title\":\"test inscription\"}"))));
    }
    else {
        REQUIRE_NOTHROW(builder.MetaData(nlohmann::json::to_cbor(nlohmann::json::parse("{\"title\":\"test inscription\"}"))));

    }
    if (avatar_collection_utxo) {
        REQUIRE_NOTHROW(builder.AddCollectionUTXO(collection_id, avatar_collection_utxo->m_txid, avatar_collection_utxo->m_nout, avatar_collection_utxo->m_amount, avatar_collection_utxo->m_addr));
    }

    REQUIRE_NOTHROW(builder.AddInput(w->fund(builder.GetMinFundingAmount(""), w->p2tr(0, 0, 0))));

    REQUIRE_NOTHROW(builder.InscribeScriptPubKey(w->derive(86, 3, 1, 0).GetSchnorrKeyPair().GetPubKey()));
    REQUIRE_NOTHROW(builder.InscribeInternalPubKey(w->derive(86, 4, 0, 1).GetSchnorrKeyPair().GetPubKey()));

    CHECK_NOTHROW(builder.SignCommit(w->keyreg(), "fund"));
    CHECK_NOTHROW(builder.SignInscription(w->keyreg(), "inscribe"));
    if (avatar_collection_utxo) {
        CHECK_NOTHROW(builder.SignCollection(w->keyreg(), "fund"));
    }

    std::string terms = builder.Serialize(8, INSCRIPTION_SIGNATURE);

    CreateInscriptionBuilder contract(w->chain(), INSCRIPTION);
    contract.Deserialize(terms, INSCRIPTION_SIGNATURE);

    stringvector rawtxs;
    REQUIRE_NOTHROW(rawtxs = contract.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);
    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    w->confirm(1, revealTx.GetHash().GetHex());

    if (condition.is_parent) {
        collection_id = revealTx.GetHash().GetHex() + "i0";
        avatar_collection_utxo = Transfer{revealTx.GetHash().GetHex(), 0, 546, w->p2tr(0, 0, 0)};
    }
    else {
        avatar_collection_utxo = Transfer{revealTx.GetHash().GetHex(), 1, 546, avatar_collection_utxo->m_addr};
    }

}
