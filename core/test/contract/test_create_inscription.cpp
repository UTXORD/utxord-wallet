#include <iostream>
#include <filesystem>
#include <algorithm>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"
#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "wallet_api.hpp"
#include "channel_keys.hpp"
#include "exechelper.hpp"
#include "create_inscription.hpp"
#include "inscription.hpp"
#include "core_io.h"

#include "test_case_wrapper.hpp"

using namespace l15;
using namespace l15::core;
using namespace utxord;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;


std::unique_ptr<TestcaseWrapper> w;

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

    return session.run();
}

struct CreateCondition
{
    std::vector<std::string> funds;
    std::string market_fee;
    bool has_change;
    bool is_parent;
    bool has_parent;
};

std::string collection_id;
seckey collection_sk;
//xonly_pubkey collection_int_pk;
Transfer collection_utxo;


TEST_CASE("inscribe")
{
    ChannelKeys utxo_key;
    ChannelKeys script_key, inscribe_key;
    ChannelKeys collection_key;//, collection_int_key;

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());

    xonly_pubkey destination_pk = w->bech32().Decode(w->btc().GetNewAddress());
    xonly_pubkey market_fee_pk = w->bech32().Decode(w->btc().GetNewAddress());

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

    CreateInscriptionBuilder test_inscription(INSCRIPTION);

    REQUIRE_NOTHROW(test_inscription.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(test_inscription.MarketFee("0", hex(market_fee_pk)));
    REQUIRE_NOTHROW(test_inscription.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(test_inscription.Data(content_type, content));
    std::string inscription_amount = test_inscription.GetMinFundingAmount("");
    std::string child_amount = test_inscription.GetMinFundingAmount("collection");

    std::clog << "Min funding: " << inscription_amount << std::endl;

    CreateCondition inscription {{inscription_amount}, "0", false, true, false};
    CreateCondition inscription_w_change {{"0.0001"}, "0", true, false, false};
    CreateCondition inscription_w_fee {{FormatAmount(ParseAmount(inscription_amount) + 43 + 1000)}, "0.00001", false, false, false};
    CreateCondition inscription_w_change_fee {{"0.0001"}, "0.00001", true, false, false};

    CreateCondition child {{child_amount}, "0", false, false, true};
    CreateCondition child_w_change {{"0.0001"}, "0", true, false, true};
    CreateCondition child_w_fee {{FormatAmount(ParseAmount(child_amount) + 43 + 1000)}, "0.00001", false, false, true};
    CreateCondition child_w_change_fee {{"0.0001"}, "0.00001", true, false, true};

    std::clog << "Inscription ord amount: " << inscription_amount << '\n';

    auto condition = GENERATE_COPY(inscription, inscription_w_change, inscription_w_fee, inscription_w_change_fee, child, child_w_change, child_w_change_fee);

    string funds_txid = w->btc().SendToAddress(addr, condition.funds[0]);
    auto prevout = w->btc().CheckOutput(funds_txid, addr);


    stringvector rawtxs;
    bool check_result = false;

    SECTION("Self inscribe") {
        check_result = true;

        CreateInscriptionBuilder builder_terms(INSCRIPTION);
        CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, hex(market_fee_pk)));

        std::string market_terms;
        REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(MARKET_TERMS));

        CreateInscriptionBuilder builder(INSCRIPTION);
        REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

        CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
        CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
        CHECK_NOTHROW(builder.Data(content_type, content));
        CHECK_NOTHROW(builder.InscribePubKey(hex(condition.is_parent ? collection_key.GetLocalPubKey() : destination_pk)));
        CHECK_NOTHROW(builder.ChangePubKey(hex(destination_pk)));
        CHECK_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, condition.funds[0], hex(utxo_key.GetLocalPubKey())));
        if (condition.has_parent) {
            CHECK_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount),
                                                  hex(*collection_utxo.m_pubkey)));
        }

        CHECK_NOTHROW(builder.SignCommit(0, hex(utxo_key.GetLocalPrivKey()), hex(script_key.GetLocalPubKey())));
        CHECK_NOTHROW(builder.SignInscription(hex(script_key.GetLocalPrivKey())));
        if (condition.has_parent) {
            CHECK_NOTHROW(builder.SignCollection(hex(collection_sk)));
        }

        std::string contract;
        REQUIRE_NOTHROW(contract = builder.Serialize(INSCRIPTION_SIGNATURE));
        std::clog << contract << std::endl;

        CreateInscriptionBuilder fin_contract(INSCRIPTION);
        REQUIRE_NOTHROW(fin_contract.Deserialize(contract, INSCRIPTION_SIGNATURE));

        REQUIRE_NOTHROW(rawtxs = fin_contract.RawTransactions());
    }
    SECTION("lazy inscribe") {
        if (condition.has_parent) {
            check_result = true;

            CreateInscriptionBuilder builder_terms(LASY_INSCRIPTION);
            CHECK_NOTHROW(builder_terms.MarketFee(condition.market_fee, hex(market_fee_pk)));
            CHECK_NOTHROW(builder_terms.Data(content_type, content));
            CHECK_NOTHROW(builder_terms.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount),
                                                  hex(*collection_utxo.m_pubkey)));
            std::string market_terms;
            REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(LASY_COLLECTION_MARKET_TERMS));

            CreateInscriptionBuilder builder(LASY_INSCRIPTION);
            REQUIRE_NOTHROW(builder.Deserialize(market_terms, LASY_COLLECTION_MARKET_TERMS));

            CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
            CHECK_NOTHROW(builder.MiningFeeRate(fee_rate));
            CHECK_NOTHROW(builder.InscribePubKey(hex(condition.is_parent ? collection_key.GetLocalPubKey() : destination_pk)));
            CHECK_NOTHROW(builder.ChangePubKey(hex(destination_pk)));
            CHECK_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, condition.funds[0], hex(utxo_key.GetLocalPubKey())));

            CHECK_NOTHROW(builder.SignCommit(0, hex(utxo_key.GetLocalPrivKey()), hex(script_key.GetLocalPubKey())));
            CHECK_NOTHROW(builder.SignInscription(hex(script_key.GetLocalPrivKey())));

            std::string contract;
            REQUIRE_NOTHROW(contract = builder.Serialize(LASY_COLLECTION_INSCRIPTION_SIGNATURE));
            std::clog << contract << std::endl;

            CreateInscriptionBuilder fin_contract(LASY_INSCRIPTION);
            REQUIRE_NOTHROW(fin_contract.Deserialize(contract, LASY_COLLECTION_INSCRIPTION_SIGNATURE));

            CHECK_NOTHROW(fin_contract.SignCollection(hex(collection_sk)));

            REQUIRE_NOTHROW(rawtxs = fin_contract.RawTransactions());
        }
    }

    if (check_result) {

        CMutableTransaction commitTx, revealTx;

        REQUIRE(rawtxs.size() == 2);
        REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
        REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

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
            collection_sk = collection_key.GetLocalPrivKey();
            collection_utxo = {revealTx.GetHash().GetHex(), 0, revealTx.vout[0].nValue, collection_key.GetLocalPubKey()};
        } else if (condition.has_parent) {
            collection_utxo.m_txid = revealTx.GetHash().GetHex();
            collection_utxo.m_nout = 1;
            collection_utxo.m_amount = revealTx.vout[1].nValue;
        }
    }
}


//TEST_CASE("inscribe")
//{
//    ChannelKeys utxo_key;
//    ChannelKeys script_key, inscribe_key;
//    ChannelKeys collection_key(collection_sk);
//    ChannelKeys new_collection_key;//, new_collection_int_key;
//
//    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());
//
//    xonly_pubkey destination_pk = w->bech32().Decode(w->btc().GetNewAddress());
//
//    std::string fee_rate;
//    try {
//        fee_rate = w->btc().EstimateSmartFee("1");
//    }
//    catch(...) {
//        fee_rate = "0.00001";
//    }
//
//    std::clog << "Fee rate: " << fee_rate << std::endl;
//
//    std::string content_type = "text/ascii";
//    auto content = hex(GenRandomString(2048));
//
//    CreateInscriptionBuilder test_builder(INSCRIPTION, "0.00000546");
//    REQUIRE_NOTHROW(test_builder.MiningFeeRate(fee_rate).Data(content_type, content));
//
//    std::clog << ">>>>> Estimate mining fee <<<<<" << std::endl;
//
//    std::string exact_amount = test_builder.GetMinFundingAmount("");
//    std::string exact_amount_w_collection = test_builder.GetMinFundingAmount("collection");
//
//    CreateInscriptionBuilder test_collection(INSCRIPTION, "0.00000546");
//    REQUIRE_NOTHROW(test_collection.MiningFeeRate(fee_rate).Data(content_type, content));
//    std::string exact_collection_root_amount = test_collection.GetMinFundingAmount("collection");
//
//    std::clog << "Amount for collection: " << exact_amount_w_collection << std::endl;
//
//
//    CAmount vin_cost = ParseAmount(test_builder.GetNewInputMiningFee());
//
//    const CreateCondition parent = {{ParseAmount(exact_collection_root_amount)}, COLLECTION, false, true};
//    const CreateCondition fund = {{ParseAmount(exact_amount_w_collection)}, INSCRIPTION, false, true};
//    const CreateCondition multi_fund = {{ParseAmount(exact_amount_w_collection) - 800, 800 + vin_cost}, INSCRIPTION, false, true};
//    const CreateCondition fund_change = {{10000}, INSCRIPTION, true, true};
//
//    auto condition = GENERATE_REF(parent, fund, multi_fund, fund_change);
//
//    CreateInscriptionBuilder builder(/*condition.type*/INSCRIPTION, "0.00000546");
//    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate).Data(content_type, content));
//    REQUIRE_NOTHROW(builder.InscribePubKey(hex(condition.type == COLLECTION ? new_collection_key.GetLocalPubKey() : destination_pk)));
//    REQUIRE_NOTHROW(builder.ChangePubKey(hex(destination_pk)));
//
//    for (CAmount amount: condition.funds) {
//        const std::string funds_amount = FormatAmount(amount);
//
//        string funds_txid = w->btc().SendToAddress(addr, funds_amount);
//        auto prevout = w->btc().CheckOutput(funds_txid, addr);
//
//        REQUIRE_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, funds_amount, hex(utxo_key.GetLocalPubKey())));
//    }
//
////    if (condition.type == COLLECTION) {
////        REQUIRE_NOTHROW(builder.CollectionCommitPubKeys(hex(new_collection_script_key.GetLocalPubKey()), hex(new_collection_int_key.GetLocalPubKey())));
////    }
//
//    if (condition.has_parent) {
////        std::string collection_out_pk = Collection::GetCollectionTapRootPubKey(collection_id, hex(new_collection_script_key.GetLocalPrivKey()), hex(new_collection_int_key.GetLocalPrivKey()));
//
//        REQUIRE_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount),
//                                                hex(collection_key.GetLocalPubKey())/*, hex(collection_int_pk),
//                                                collection_out_pk*/));
//    }
//
//    for (uint32_t n = 0; n < condition.funds.size(); ++n) {
//        std::clog << ">>>>> Sign commit <<<<<" << std::endl;
//        REQUIRE_NOTHROW(builder.SignCommit(n, hex(utxo_key.GetLocalPrivKey()), hex(script_key.GetLocalPubKey())));
//    }
//    if (condition.has_parent) {
//        std::clog << ">>>>> Sign collection <<<<<" << std::endl;
//        REQUIRE_NOTHROW(builder.SignCollection(hex(collection_sk)));
////        CHECK_NOTHROW(builder.SignFundMiningFee(0, hex(extra_key.GetLocalPrivKey())));
//    }
//    std::clog << ">>>>> Sign inscription <<<<<" << std::endl;
//    REQUIRE_NOTHROW(builder.SignInscription(hex(script_key.GetLocalPrivKey())));
////    if (condition.type == COLLECTION) {
////        REQUIRE_NOTHROW(builder.SignCollectionRootCommit(hex(inscribe_key.GetLocalPrivKey())));
////    }
//
//    ChannelKeys rollback_key(unhex<seckey>(builder.getIntermediateTaprootSK()));
//
//    std::string inscription_id = builder.MakeInscriptionId();
//
//    std::string ser_data;
//    REQUIRE_NOTHROW(ser_data = builder.Serialize());
//
//    std::clog << ser_data << std::endl;
//
//    CreateInscriptionBuilder builder2(condition.type, "0.00000546");
//
//    std::clog << ">>>>> Deserialize <<<<<" << std::endl;
//    REQUIRE_NOTHROW(builder2.Deserialize(ser_data));
//
//    stringvector rawtx;
//    REQUIRE_NOTHROW(rawtx = builder2.RawTransactions());
//
//    CMutableTransaction funding_tx, genesis_tx;//, collection_commit_tx;
//    REQUIRE(DecodeHexTx(funding_tx, rawtx.front()));
//    std::clog << "Funding TX ============================================================" << '\n';
//    LogTx(funding_tx);
//    CHECK(funding_tx.vout.size() == (1 + (condition.has_parent ? 1 : 0) + (condition.has_change ? 1 : 0)));
//
//    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(funding_tx)));
//
//    SECTION("Inscribe")
//    {
//        REQUIRE(DecodeHexTx(genesis_tx, rawtx[1]));
//        CHECK(inscription_id == (genesis_tx.GetHash().GetHex() + "i0"));
//        std::clog << "Reveal TX ============================================================" << '\n';
//        LogTx(genesis_tx);
////        if (condition.type == COLLECTION) {
////            REQUIRE(DecodeHexTx(collection_commit_tx, rawtx[2]));
////            std::clog << "Collection commit TX ============================================================" << '\n';
////            LogTx(collection_commit_tx);
////        }
//        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(genesis_tx)));
//
////        if (condition.type == COLLECTION) {
////            REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(collection_commit_tx)));
////            CHECK(collection_commit_tx.vout[0].nValue == 546);
////        }
////        else {
//            CHECK(genesis_tx.vout[0].nValue == 546);
////        }
//
//        std::optional<Inscription> inscription;
//        CHECK_NOTHROW(inscription = Inscription(rawtx[1]));
//        CHECK(inscription->GetIscriptionId() == genesis_tx.GetHash().GetHex() + "i" + std::to_string(0));
//        CHECK(inscription->GetContentType() == content_type);
//        CHECK(hex(inscription->GetContent()) == content);
//
//        if (condition.has_parent) {
//            CHECK(inscription->GetCollectionId() == collection_id);
//        }
//
//        if (condition.type == COLLECTION) {
//            collection_id = genesis_tx.GetHash().GetHex() + "i0";
//            collection_utxo = {genesis_tx.GetHash().GetHex(), 0, genesis_tx.vout.front().nValue};
//            collection_sk = new_collection_key.GetLocalPrivKey();
//        }
//        else if (condition.has_parent){
//            collection_utxo = {genesis_tx.GetHash().GetHex(), 1, genesis_tx.vout[1].nValue};
//        }
//
//        if (condition.has_parent) {
//            if (condition.type == INSCRIPTION) {
//                CHECK(funding_tx.vout[0].nValue == 546);
//            }
//
//            CHECK(genesis_tx.vout[1].nValue == 546);
//
//            //collection_int_pk = new_collection_int_key.GetLocalPubKey();
//        }
//    }
//
//    SECTION("Payback")
//    {
//        CScript rollbackpubkeyscript;
//        rollbackpubkeyscript << 1;
//        rollbackpubkeyscript << rollback_key.GetLocalPubKey();
//
//        CMutableTransaction rollback_tx;
//        rollback_tx.vin.emplace_back(COutPoint(funding_tx.GetHash(), 0));
//        rollback_tx.vout.emplace_back(0, rollbackpubkeyscript);
//
//        rollback_tx.vin.front().scriptWitness.stack.emplace_back(64);
//
//        rollback_tx.vout.front().nValue = CalculateOutputAmount(funding_tx.vout.front().nValue, ParseAmount(fee_rate), rollback_tx);
//
//        signature rollback_sig = rollback_key.SignTaprootTx(rollback_tx, 0, {funding_tx.vout.front()}, {});
//        rollback_tx.vin.front().scriptWitness.stack.front() = static_cast<bytevector&>(rollback_sig);
//
//        CHECK_NOTHROW(w->btc().SpendTx(CTransaction(rollback_tx)));
//    }
//
//    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
//}

struct InscribeWithMetadataCondition {
    std::string metadata;
    bool save_as_parent;
    bool has_parent;
};

const InscribeWithMetadataCondition short_metadata = {R"({"name":"sample inscription 1"})", true, false};
const InscribeWithMetadataCondition exact_520_metadata = {
        R"({"description":"very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong description","name":"sample inscription 2"})",
        false, true};
const InscribeWithMetadataCondition long_metadata = {
        "{\"name\":\"sample inscription 3\","
        "\"description\":\"very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
        "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong description\"}",
        false, false};

TEST_CASE("metadata") {
    ChannelKeys utxo_key;
    ChannelKeys script_key, inscribe_key;

    string addr = w->bech32().Encode(utxo_key.GetLocalPubKey());

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    std::clog << "Fee rate: " << fee_rate << std::endl;

    std::string content_type = "image/svg+xml";

    const string svg = "<svg width=\"440\" height=\"101\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xml:space=\"preserve\" overflow=\"hidden\"><g transform=\"translate(-82 -206)\"><g><text fill=\"#777777\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 118.078 275)\">Svg</text><text fill=\"#FFFFFF\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 191.984 275)\">svg</text><text fill=\"#000000\" fill-opacity=\"1\" font-family=\"Arial,Arial_MSFontService,sans-serif\" font-style=\"normal\" font-variant=\"normal\" font-weight=\"400\" font-stretch=\"normal\" font-size=\"37\" text-anchor=\"start\" direction=\"ltr\" writing-mode=\"lr-tb\" unicode-bidi=\"normal\" text-decoration=\"none\" transform=\"matrix(1 0 0 1 259.589 275)\">svg</text></g></g></svg>";
    auto content = hex(svg);

    const auto& condition = GENERATE_REF(short_metadata, exact_520_metadata, long_metadata);

    xonly_pubkey destination_pk = condition.save_as_parent ? inscribe_key.GetLocalPubKey() : w->bech32().Decode(w->btc().GetNewAddress());

    CreateInscriptionBuilder builder_terms(INSCRIPTION);
    CHECK_NOTHROW(builder_terms.MarketFee("0", hex(destination_pk)));

    std::string market_terms;
    REQUIRE_NOTHROW(market_terms = builder_terms.Serialize(MARKET_TERMS));

    CreateInscriptionBuilder builder(INSCRIPTION);
    REQUIRE_NOTHROW(builder.Deserialize(market_terms, MARKET_TERMS));

    CHECK_NOTHROW(builder.OrdAmount("0.00000546"));
    REQUIRE_NOTHROW(builder.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builder.Data(content_type, content));
    REQUIRE_NOTHROW(builder.MetaData(condition.metadata));

    std::string min_fund = builder.GetMinFundingAmount(condition.has_parent ? "collection" : "");
    string funds_txid = w->btc().SendToAddress(addr, min_fund);
    auto prevout = w->btc().CheckOutput(funds_txid, addr);

    REQUIRE_NOTHROW(builder.InscribePubKey(hex(destination_pk)));
    REQUIRE_NOTHROW(builder.AddUTXO(get<0>(prevout).hash.GetHex(), get<0>(prevout).n, min_fund, hex(utxo_key.GetLocalPubKey())));

    if (condition.has_parent) {
        REQUIRE_NOTHROW(builder.AddToCollection(collection_id, collection_utxo.m_txid, collection_utxo.m_nout, FormatAmount(collection_utxo.m_amount), hex(*collection_utxo.m_pubkey)));
    }

    REQUIRE_NOTHROW(builder.SignCommit(0, hex(utxo_key.GetLocalPrivKey()), hex(script_key.GetLocalPubKey())));
    REQUIRE_NOTHROW(builder.SignInscription(hex(script_key.GetLocalPrivKey())));
    if (condition.has_parent) {
        REQUIRE_NOTHROW(builder.SignCollection(hex(collection_sk)));
    }

//    stringvector rawtxs0;
//    CHECK_NOTHROW(rawtxs0 = builder.RawTransactions());

    std::string contract = builder.Serialize(INSCRIPTION_SIGNATURE);
    std::clog << "Contract JSON: " << contract << std::endl;

    CreateInscriptionBuilder builder2(INSCRIPTION);
    builder2.Deserialize(contract, INSCRIPTION_SIGNATURE);

    stringvector rawtxs;
    CHECK_NOTHROW(rawtxs = builder2.RawTransactions());

    CMutableTransaction commitTx, revealTx;

    REQUIRE(rawtxs.size() == 2);

    Inscription inscr(rawtxs[1]);
    const auto& result_metadata = inscr.GetMetadata();

    CHECK(result_metadata == condition.metadata);

    REQUIRE(DecodeHexTx(commitTx, rawtxs[0]));
    REQUIRE(DecodeHexTx(revealTx, rawtxs[1]));

    CHECK(revealTx.vout[0].nValue == 546);

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(commitTx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(revealTx)));

    if (condition.save_as_parent) {
        collection_sk = inscribe_key.GetLocalPrivKey();
        collection_id = revealTx.GetHash().GetHex() + "i0";
        collection_utxo = {revealTx.GetHash().GetHex(), 0, 546, inscribe_key.GetLocalPubKey()};
    }

    std::clog << "Commit: ========================" << std::endl;
    LogTx(commitTx);
    std::clog << "Genesis: ========================" << std::endl;
    LogTx(revealTx);
    std::clog << "========================" << std::endl;

    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
}