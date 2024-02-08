#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"
#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "wallet_api.hpp"
#include "channel_keys.hpp"
#include "exechelper.hpp"
#include "swap_inscription.hpp"
#include "core_io.h"

#include "policy/policy.h"

#include "test_case_wrapper.hpp"

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

struct SwapCondition
{
    std::vector<std::tuple<CAmount, std::string>> funds;
    bool has_change;
};

TEST_CASE("Swap")
{
    const std::string ORD_AMOUNT = "0.00000546";
    const std::string ORD_PRICE = "0.0001";
    const std::string MARKET_FEE = "0.00001";

    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("swap", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");

    KeyPair swap_script_key_A = master_key.Derive("m/86'/1'/0'/0/0", false);
    KeyPair swap_script_key_B = master_key.Derive("m/86'/1'/3'/0/1", true);
    KeyPair swap_script_key_M = master_key.Derive("m/86'/1'/3'/0/2", true);
    //get key pair
    KeyPair funds_utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
    KeyPair funds_utxo_segwit_key = master_key.Derive("m/84'/1'/0'/0/0", false);
    KeyPair ord_utxo_key = master_key.Derive("m/86'/1'/2'/0/0", false);
    KeyPair ord_payoff_key = master_key.Derive("m/86'/1'/2'/0/10", false);
    KeyPair change_key = master_key.Derive("m/86'/1'/3'/1/0", false);

    std::string fee_rate;
//    try {
//        fee_rate = w->btc().EstimateSmartFee("1");
//    }
//    catch(...) {
        fee_rate = FormatAmount(1000);
//    }

    std::string destination_addr = w->btc().GetNewAddress();

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(*bech);
    CHECK_NOTHROW(builderMarket.OrdPrice(ORD_PRICE));
    CHECK_NOTHROW(builderMarket.MarketFee(MARKET_FEE, destination_addr));
    CHECK_NOTHROW(builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2)));
    CHECK_NOTHROW(builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.PubKey())));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(5, ORD_TERMS));

//    std::clog << "ORD_TERMS: ===========================================\n"
//              << marketOrdConditions << std::endl;

    SwapInscriptionBuilder builderOrdSeller(*bech);
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions));
    CHECK_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));

    //Create ord utxo
    string ord_addr = ord_utxo_key.GetP2TRAddress(*bech);
    string ord_txid = w->btc().SendToAddress(ord_addr, ORD_AMOUNT);
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    //CHECK_NOTHROW(builderOrdSeller.OrdPrice(ORD_PRICE));
    CHECK_NOTHROW(builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, FormatAmount(get<1>(ord_prevout).nValue), ord_addr));
    CHECK_NOTHROW(builderOrdSeller.FundsPayoffAddress(swap_script_key_A.GetP2TRAddress(*bech)));

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(master_key, "ord"));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms = builderOrdSeller.Serialize(5, ORD_SWAP_SIG));

//    std::clog << "ORD_SWAP_SIG: ===========================================\n"
//              << ordSellerTerms << std::endl;

    builderMarket.Deserialize(ordSellerTerms);
    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(ORD_SWAP_SIG));

    // FUNDS side terms
    //--------------------------------------------------------------------------
    builderMarket.MiningFeeRate(fee_rate);
    string marketFundsConditions;
    REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(5, FUNDS_TERMS));

//    std::clog << "FUNDS_TERMS: ===========================================\n"
//              << marketFundsConditions << std::endl;

    SwapInscriptionBuilder builderOrdBuyer(*bech);
    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions));

    CAmount min_funding = ParseAmount(builderOrdBuyer.GetMinFundingAmount(""));
    CAmount min_segwit_funding = ParseAmount(builderOrdBuyer.GetMinFundingAmount("p2wpkh_utxo"));
    CAmount dust = Dust(3000);

    const SwapCondition fund_min_cond = {{{min_funding, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, false};
    const SwapCondition fund_min_3000_cond = {{{min_funding + 3000, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, true};
    const SwapCondition fund_min_dust_cond = {{{min_funding+dust, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, false};
    const SwapCondition fund_min_dust_1_cond = {{{min_funding + dust + 43, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, true};
    const SwapCondition fund_min_dust_100_cond = {{{min_funding + dust - 100, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, false};
    const SwapCondition fund_min_50_cond = {{{min_funding + 50, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, false};

    const SwapCondition fund_min_segwit_cond = {{{min_segwit_funding, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, false};
    const SwapCondition fund_min_3000_segwit_cond = {{{min_segwit_funding + 3000, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, true};
    const SwapCondition fund_min_dust_segwit_cond = {{{min_segwit_funding+dust, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, false};
    const SwapCondition fund_min_dust_1_segwit_cond = {{{min_segwit_funding + dust + 1, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, true};
    const SwapCondition fund_min_dust_100_segwit_cond = {{{min_segwit_funding + dust - 100, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, false};
    const SwapCondition fund_min_50_segwit_cond = {{{min_segwit_funding + 50, std::string(funds_utxo_segwit_key.GetP2WPKHAddress(*bech))}}, false};

    const SwapCondition fund_min_3000_cond_2 = {{{min_funding, std::string(funds_utxo_key.GetP2TRAddress(*bech))}, {3000, std::string(funds_utxo_key.GetP2TRAddress(*bech))}}, true};

    auto condition = GENERATE_REF(
                fund_min_cond,
                fund_min_3000_cond,
//                fund_min_dust_cond
                fund_min_dust_1_cond,
                fund_min_dust_100_cond,
                fund_min_50_cond,
                fund_min_segwit_cond,
                fund_min_3000_segwit_cond,
                fund_min_dust_segwit_cond,
                fund_min_dust_1_segwit_cond,
                fund_min_dust_100_segwit_cond,
                fund_min_50_segwit_cond,
                fund_min_3000_cond_2
            );

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.PubKey()));
    builderOrdBuyer.ChangeAddress(change_key.GetP2TRAddress(*bech));

    //Fund commitment
//    string funds_addr = funds_utxo_key.GetP2TRAddress(*bech);
    std::vector<CTxOut> spent_outs;

    for (const auto& fund: condition.funds) {
        const std::string funds_amount = FormatAmount(get<0>(fund));
        const std::string funds_addr = get<1>(fund);

        string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
        auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

        builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, funds_addr);
        spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.PubKey());
    }

    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(master_key, "fund"));

//    auto commit_tx = builderOrdBuyer.GetFundsCommitTx();
//    PrecomputedTransactionData txdata;
//    txdata.Init(commit_tx, move(spent_outs), true);
//
//    for (uint32_t n = 0; n < commit_tx.vin.size(); ++n) {
//        const CTxIn &in = commit_tx.vin.at(n);
//        MutableTransactionSignatureChecker txChecker(&commit_tx, n, txdata.m_spent_outputs[n].nValue, txdata, MissingDataBehavior::FAIL);
//        bool res = VerifyScript(in.scriptSig, txdata.m_spent_outputs[n].scriptPubKey, &in.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, txChecker);
//        REQUIRE(res);
//    }


    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(builderOrdBuyer.GetFundsCommitTx())));

    builderOrdBuyer.OrdPayoffAddress(ord_payoff_key.GetP2TRAddress(*bech));

    string ordBuyerTerms;
    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(5, FUNDS_COMMIT_SIG));

//    std::clog << "FUNDS_COMMIT_SIG: ===========================================\n"
//              << ordBuyerTerms << std::endl;

    // MARKET confirm terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket1(*bech);
    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordSellerTerms));
    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(ORD_SWAP_SIG));

    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordBuyerTerms));
    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(FUNDS_COMMIT_SIG));

    string funds_commit_raw_tx;
    REQUIRE_NOTHROW(funds_commit_raw_tx = builderMarket1.FundsCommitRawTransaction());

    CMutableTransaction funds_commit_tx;
    REQUIRE(DecodeHexTx(funds_commit_tx, funds_commit_raw_tx));

    std::clog << "FundsCommitTx: ====================================" << std::endl;
    LogTx(funds_commit_tx);

    CHECK(funds_commit_tx.vout.size() == (condition.has_change ? 2 : 1));

    //REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(funds_commit_tx)));

//    SECTION("Funds PayBack") {
//        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsPayBack(hex(swap_script_key_B.GetLocalPrivKey())));
//        std::string funds_payback_raw_tx;
//        REQUIRE_NOTHROW(funds_payback_raw_tx = builderOrdBuyer.FundsPayBackRawTransaction());
//
//        CMutableTransaction funds_payback_tx;
//        REQUIRE(DecodeHexTx(funds_payback_tx, funds_payback_raw_tx));
//
//        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "11");
//        REQUIRE_THROWS(w->btc().SpendTx(CTransaction(funds_payback_tx)));
//        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
//        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(funds_payback_tx)));
//
//    }
    SECTION("Execute Swap") {
        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

        REQUIRE_NOTHROW(builderMarket1.MarketSignOrdPayoffTx(master_key, "swap"));
        string ordMarketTerms;
        REQUIRE_NOTHROW(ordMarketTerms = builderMarket1.Serialize(5, MARKET_PAYOFF_SIG));

//        std::clog << "MARKET_PAYOFF_SIG: ===========================================\n"
//                  << ordMarketTerms << std::endl;

        // BUYER sign swap
        //--------------------------------------------------------------------------

        REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordMarketTerms));
        REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(MARKET_PAYOFF_SIG));

        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(master_key, "swap"));

        string ordFundsSignature;
        REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(5, FUNDS_SWAP_SIG));

//        std::clog << "FUNDS_SWAP_SIG: ===========================================\n"
//                  << ordFundsSignature << std::endl;

        // MARKET sign swap
        //--------------------------------------------------------------------------

        REQUIRE_NOTHROW(builderMarket1.Deserialize(ordFundsSignature));
        REQUIRE_NOTHROW(builderMarket1.MarketSignSwap(master_key, "swap"));

        string ord_swap_raw_tx = builderMarket1.OrdSwapRawTransaction();
        string ord_transfer_raw_tx = builderMarket1.OrdPayoffRawTransaction();

        CMutableTransaction ord_swap_tx, ord_transfer_tx;
        REQUIRE(DecodeHexTx(ord_swap_tx, ord_swap_raw_tx));
        REQUIRE(DecodeHexTx(ord_transfer_tx, ord_transfer_raw_tx));

        CHECK(ord_transfer_tx.vout[0].nValue == ParseAmount(ORD_AMOUNT));

//    PrecomputedTransactionData txdata;
//    txdata.Init(ord_swap_tx, {ord_commit_tx.vout[0], funds_commit_tx.vout[0]}, /* force=*/ true);
//
//    const CTxIn& ordTxin = ord_swap_tx.vin.at(0);
//    MutableTransactionSignatureChecker TxOrdChecker(&ord_swap_tx, 0, ord_commit_tx.vout[0].nValue, txdata, MissingDataBehavior::FAIL);
//    bool ordPath = VerifyScript(ordTxin.scriptSig, ord_commit_tx.vout[0].scriptPubKey, &ordTxin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
//    REQUIRE(ordPath);
//
//    const CTxIn& txin = ord_swap_tx.vin.at(1);
//    MutableTransactionSignatureChecker tx_checker(&ord_swap_tx, 1, funds_commit_tx.vout[0].nValue, txdata, MissingDataBehavior::FAIL);
//    bool fundsPath = VerifyScript(txin.scriptSig, funds_commit_tx.vout[0].scriptPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, tx_checker);
//    REQUIRE(fundsPath);

        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_swap_tx)));
        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_transfer_tx)));

        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");


        // BUYER spends his ord
        //--------------------------------------------------------------------------

//        xonly_pubkey payoff_pk = w->bech32().Decode(w->btc().GetNewAddress());
//        CScript buyer_pubkeyscript = CScript() << 1 << payoff_pk;
//
//        CMutableTransaction ord_payoff_tx;
//        ord_payoff_tx.vin = {CTxIn(ord_transfer_tx.GetHash(), 0)};
//        ord_payoff_tx.vin.front().scriptWitness.stack.emplace_back(64);
//        ord_payoff_tx.vout = {CTxOut(ord_transfer_tx.vout[0].nValue, buyer_pubkeyscript)};
//        ord_payoff_tx.vout.front().nValue = CalculateOutputAmount(ord_transfer_tx.vout[0].nValue, ParseAmount(fee_rate), ord_payoff_tx);
//
//        REQUIRE_NOTHROW(ord_payoff_tx.vin.front().scriptWitness.stack[0] = swap_script_key_B.SignTaprootTx(ord_payoff_tx, 0, {ord_transfer_tx.vout[0]}, {}));
//
//        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_payoff_tx)));

        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

        if (condition.has_change) {
            // BUYER spends his change
            //--------------------------------------------------------------------------

            CMutableTransaction ord_change_spend_tx;
            ord_change_spend_tx.vin = {CTxIn(funds_commit_tx.GetHash(), 1)};
            ord_change_spend_tx.vin.front().scriptWitness.stack.emplace_back(64);
            ord_change_spend_tx.vout = {CTxOut(funds_commit_tx.vout[1].nValue, bech->PubKeyScript(change_key.GetP2TRAddress(*bech)))};


            if (CalculateTxFee(ParseAmount(fee_rate), ord_change_spend_tx) + Dust(3000) < funds_commit_tx.vout[1].nValue) {
                ord_change_spend_tx.vout.front().nValue = CalculateOutputAmount(funds_commit_tx.vout[1].nValue, ParseAmount(fee_rate), ord_change_spend_tx);

                ChannelKeys changeKey(change_key.PrivKey());
                REQUIRE_NOTHROW(ord_change_spend_tx.vin.front().scriptWitness.stack[0] = changeKey.SignTaprootTx(ord_change_spend_tx, 0, {funds_commit_tx.vout[1]}, {}));
                REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_change_spend_tx)));
            }

        }

        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
    }
}

TEST_CASE("SwapNoFee")
{
    const std::string ORD_AMOUNT = "0.00000546";
    const std::string ORD_PRICE = "0.0001";
    const std::string MARKET_FEE = "0";

    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("swap", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");

    KeyPair swap_script_key_A = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair swap_script_key_B = master_key.Derive("m/86'/1'/3'/0/1", true);
    KeyPair swap_script_key_M = master_key.Derive("m/86'/1'/3'/0/2", true);
    KeyPair funds_utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
    KeyPair ord_utxo_key = master_key.Derive("m/86'/1'/2'/0/0", false);

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = FormatAmount(1000);
    }

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(*bech);
    builderMarket.OrdPrice(ORD_AMOUNT);
    builderMarket.MarketFee(MARKET_FEE, bech->Encode(xonly_pubkey()));
    builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2));
    builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.PubKey()));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(5, ORD_TERMS));

    SwapInscriptionBuilder builderOrdSeller(*bech);
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions));

    REQUIRE_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));

    //Create ord utxo
    string ord_addr = ord_utxo_key.GetP2TRAddress(*bech);
    string ord_txid = w->btc().SendToAddress(ord_addr, ORD_AMOUNT);
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, FormatAmount(get<1>(ord_prevout).nValue), ord_addr);
    CHECK_NOTHROW(builderOrdSeller.FundsPayoffAddress(w->btc().GetNewAddress()));

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(master_key, "ord"));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms= builderOrdSeller.Serialize(5, ORD_SWAP_SIG));


    // FUNDS side terms
    //--------------------------------------------------------------------------
    builderMarket.MiningFeeRate(fee_rate);
    string marketFundsConditions;
    REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(5, FUNDS_TERMS));

    SwapInscriptionBuilder builderOrdBuyer(*bech);
    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions));

    CAmount min_funding = ParseAmount(builderOrdBuyer.GetMinFundingAmount(""));

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.PubKey()));

    //Fund commitment
    string funds_addr = funds_utxo_key.GetP2TRAddress(*bech);
    std::vector<CTxOut> spent_outs;


    const std::string funds_amount = FormatAmount(min_funding);

    string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
    auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

    builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, funds_addr);
    spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.PubKey());
    REQUIRE_NOTHROW(builderOrdBuyer.OrdPayoffAddress(w->btc().GetNewAddress()));
    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(master_key, "fund"));


    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(builderOrdBuyer.GetFundsCommitTx())));

    string ordBuyerTerms;
    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(5, FUNDS_COMMIT_SIG));


    // MARKET confirm terms
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderMarket.Deserialize(ordSellerTerms));
    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(ORD_SWAP_SIG));

    REQUIRE_NOTHROW(builderMarket.Deserialize(ordBuyerTerms));
    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(FUNDS_COMMIT_SIG));

    string funds_commit_raw_tx;
    REQUIRE_NOTHROW(funds_commit_raw_tx = builderMarket.FundsCommitRawTransaction());

    CMutableTransaction funds_commit_tx;
    REQUIRE(DecodeHexTx(funds_commit_tx, funds_commit_raw_tx));

    CHECK(funds_commit_tx.vout.size() == 1);

    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

    REQUIRE_NOTHROW(builderMarket.MarketSignOrdPayoffTx(master_key, "swap"));
    string ordMarketTerms;
    REQUIRE_NOTHROW(ordMarketTerms = builderMarket.Serialize(5, MARKET_PAYOFF_SIG));


    // BUYER sign swap
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordMarketTerms));
    REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(MARKET_PAYOFF_SIG));

    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(master_key, "swap"));

    string ordFundsSignature;
    REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(5, FUNDS_SWAP_SIG));


    // MARKET sign swap
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderMarket.Deserialize(ordFundsSignature));
    REQUIRE_NOTHROW(builderMarket.MarketSignSwap(master_key, "swap"));

    string ord_swap_raw_tx = builderMarket.OrdSwapRawTransaction();
    string ord_transfer_raw_tx = builderMarket.OrdPayoffRawTransaction();

    CMutableTransaction ord_swap_tx, ord_transfer_tx;
    REQUIRE(DecodeHexTx(ord_swap_tx, ord_swap_raw_tx));
    REQUIRE(DecodeHexTx(ord_transfer_tx, ord_transfer_raw_tx));

    CHECK(ord_swap_tx.vout.size() == 2); // No market fee output

    CHECK(ord_transfer_tx.vout[0].nValue == ParseAmount(ORD_AMOUNT));

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_swap_tx)));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_transfer_tx)));

    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
}

TEST_CASE("FundsNotEnough")
{
    const std::string ORD_PRICE = "0.0001";
    const std::string MARKET_FEE = "0.00001";

    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("swap", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");

    KeyPair swap_script_key_A = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair swap_script_key_B = master_key.Derive("m/86'/1'/3'/0/1", true);
    KeyPair swap_script_key_M = master_key.Derive("m/86'/1'/3'/0/2", true);
    KeyPair funds_utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
    KeyPair ord_utxo_key = master_key.Derive("m/86'/1'/2'/0/0", false);

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = FormatAmount(1000);
    }
    std::clog << "Fee rate: " << fee_rate << std::endl;

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(*bech);
    builderMarket.MarketFee(MARKET_FEE, bech->Encode(swap_script_key_M.PubKey()));
    builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2));
    builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.PubKey()));

    string marketOrdConditions = builderMarket.Serialize(5, ORD_TERMS);

    SwapInscriptionBuilder builderOrdSeller(*bech);
    builderOrdSeller.Deserialize(marketOrdConditions);
    builderOrdSeller.CheckContractTerms(ORD_TERMS);

    //Create ord utxo
    string ord_addr = ord_utxo_key.GetP2TRAddress(*bech);
    string ord_txid = w->btc().SendToAddress(ord_addr, "0.0001");
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    builderOrdSeller.OrdPrice(ORD_PRICE);
    builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, "0.0001", ord_addr);
    CHECK_NOTHROW(builderOrdSeller.FundsPayoffAddress(swap_script_key_A.GetP2TRAddress(*bech)));

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(master_key, "ord"));

    string ordSellerTerms = builderOrdSeller.Serialize(5, ORD_SWAP_SIG);

    builderMarket.Deserialize(ordSellerTerms);
    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(ORD_SWAP_SIG));

    // FUNDS side terms
    //--------------------------------------------------------------------------

    builderMarket.MiningFeeRate(fee_rate);
    string marketFundsConditions = builderMarket.Serialize(5, FUNDS_TERMS);

    SwapInscriptionBuilder builderOrdBuyer(*bech);
    builderOrdBuyer.Deserialize(marketFundsConditions);

    //Create insufficient funds utxo
    std::string funds_amount = FormatAmount(ParseAmount(builderOrdBuyer.GetMinFundingAmount("")) - Dust(3000));
    std::string funds_addr = funds_utxo_key.GetP2TRAddress(*bech);
    std::string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);

    auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.PubKey()));
    builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, funds_addr);
    REQUIRE_THROWS_AS(builderOrdBuyer.SignFundsCommitment(master_key, "fund"), l15::TransactionError);

    //Create funds utxo
//    funds_amount = builderOrdBuyer.GetMinFundingAmount();
//
//    builderOrdBuyer.FundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount);
//    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(hex(funds_utxo_key.GetLocalPrivKey())));
}

//TEST_CASE("SwapV4Compatibility")
//{
//    const std::string ORD_AMOUNT = "0.00000546";
//    const std::string ORD_PRICE = "0.0001";
//    const std::string MARKET_FEE = "0.00001";
//
//    KeyRegistry master_key(*bech, seed);
//    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
//    master_key.AddKeyType("ord", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["2'"], "change":["0"], "index_range":"0-256"})");
//    master_key.AddKeyType("swap", R"({"look_cache":true, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");
//
//    KeyPair swap_script_key_A = master_key.Derive("m/86'/1'/3'/0/0", true);
//    KeyPair swap_script_key_B = master_key.Derive("m/86'/1'/3'/0/1", true);
//    KeyPair swap_script_key_M = master_key.Derive("m/86'/1'/3'/0/2", true);
//    KeyPair funds_utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
//    KeyPair ord_utxo_key = master_key.Derive("m/86'/1'/2'/0/0", false);
//
//    const std::string fund_min_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"12c1a744e78da3805bc3620f3ae157183ec69b4da67f1fadce0e4b9caf90e388","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"8f72194a76e61e31e93868bfe89a6fc826ba4a381ee8f118bb75354c9a1c990eed08e27b2e4602bd413a79536359575772e21b39bbc0f5035e9ec73b3a7cc7e981"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"b8a64acdaf9a480781d24dac626faba63ddcf559b7521d8b3b188398e5c5b1db","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"e99ae9a8c3e4314472d030586f21717f8cdd8fcab1d271e903c3289dd2a51cb6b3321d30b8b9706f7f865ace3fdaa163cfe2ebe6ed21ed361845fdd612de62b7"}],"funds_unspendable_key_factor":"d4064148449308f0d4fceef11d39e8fa9bbb34cf87b744dbeedd4652e9c098e3","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"12c1a744e78da3805bc3620f3ae157183ec69b4da67f1fadce0e4b9caf90e388","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"8f72194a76e61e31e93868bfe89a6fc826ba4a381ee8f118bb75354c9a1c990eed08e27b2e4602bd413a79536359575772e21b39bbc0f5035e9ec73b3a7cc7e981","mining_fee_rate":1000,"funds":[{"funds_txid":"b8a64acdaf9a480781d24dac626faba63ddcf559b7521d8b3b188398e5c5b1db","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"e99ae9a8c3e4314472d030586f21717f8cdd8fcab1d271e903c3289dd2a51cb6b3321d30b8b9706f7f865ace3fdaa163cfe2ebe6ed21ed361845fdd612de62b7"}],"funds_unspendable_key_factor":"d4064148449308f0d4fceef11d39e8fa9bbb34cf87b744dbeedd4652e9c098e3","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"df3cce372761c81d19786139787b2a803f9c70d9add9ffe9b61379cf3b01ed77ec661c049a85766a7bcb2b4e5629a6a1f3826845747c729ecec293afd9909b79"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"b8a64acdaf9a480781d24dac626faba63ddcf559b7521d8b3b188398e5c5b1db","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"e99ae9a8c3e4314472d030586f21717f8cdd8fcab1d271e903c3289dd2a51cb6b3321d30b8b9706f7f865ace3fdaa163cfe2ebe6ed21ed361845fdd612de62b7"}],"funds_unspendable_key_factor":"d4064148449308f0d4fceef11d39e8fa9bbb34cf87b744dbeedd4652e9c098e3","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"276b1eb0543703e7a6c32e0169f56f1d388b4c1d65c57e0b2ac676a2c80a01a0fe8c610825f4005e932fa5e02f81cf25125ec1f558f2914114e4f9a4bf4023ef"}})json",
//    };
//    const std::string fund_min_3000_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"1a7b1ef5ed66362b53c2269824479ca8bcf8010998195afac5e7f7190cbb8e1f","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"9a668bb0958a67ebb315bdf974473dd3c9bcaf0cfaddf601e2ba0695cc1dcffd0cb206e248b88b604ecf340da9ed79213d333ee2250befb6ef5b7916f0868a2781"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"5a48d87df2885cb7676e52973417cffef395b2bf7b9b5f9dc37a44d371b453a6","funds_nout":1,"funds_amount":14682,"funds_commit_sig":"9e7d251f6dad4ed8e78611e13b661ce21533233a5cb97086a60a87c47b9ccdcd5d4872ac1c0e8d77dc8abbb2d2ee8159d56e1bc37d4b5d2553682b089a09521a"}],"funds_unspendable_key_factor":"411d1a5e7c995819c1bad858efd29265c53e3df919cfb7fbe59a58b5e4f336a2","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"1a7b1ef5ed66362b53c2269824479ca8bcf8010998195afac5e7f7190cbb8e1f","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"9a668bb0958a67ebb315bdf974473dd3c9bcaf0cfaddf601e2ba0695cc1dcffd0cb206e248b88b604ecf340da9ed79213d333ee2250befb6ef5b7916f0868a2781","mining_fee_rate":1000,"funds":[{"funds_txid":"5a48d87df2885cb7676e52973417cffef395b2bf7b9b5f9dc37a44d371b453a6","funds_nout":1,"funds_amount":14682,"funds_commit_sig":"9e7d251f6dad4ed8e78611e13b661ce21533233a5cb97086a60a87c47b9ccdcd5d4872ac1c0e8d77dc8abbb2d2ee8159d56e1bc37d4b5d2553682b089a09521a"}],"funds_unspendable_key_factor":"411d1a5e7c995819c1bad858efd29265c53e3df919cfb7fbe59a58b5e4f336a2","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"5f4f3f02a9979e8c59f1121df610d3250d7ae1dd9964ce98982ec0b82d9d75ad15264271d37c4037c3b68fb4669d50da9c1ce31d63dc9b204825952c3b4ab0e7"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"5a48d87df2885cb7676e52973417cffef395b2bf7b9b5f9dc37a44d371b453a6","funds_nout":1,"funds_amount":14682,"funds_commit_sig":"9e7d251f6dad4ed8e78611e13b661ce21533233a5cb97086a60a87c47b9ccdcd5d4872ac1c0e8d77dc8abbb2d2ee8159d56e1bc37d4b5d2553682b089a09521a"}],"funds_unspendable_key_factor":"411d1a5e7c995819c1bad858efd29265c53e3df919cfb7fbe59a58b5e4f336a2","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"cca4f0a3747e3248d2ca241f3eee1b9a6f957e0c42856634866de934cea22eb2456677a01a75001011d3989b87282ac86ad09bffc9a3da355743f56836ea3d7d"}})json",
//    };
//    const std::string fund_min_dust_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"7d0c701ea8e572ceaa90bac1f31eee9c516bbebb279924b04520b2d7d73113bc","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"02a738c524c0b62fb72e3842ce5c6ed416c5b2de80e9238fc4498e4afa94dece91e0a22394dc3e5fbb1a38aaf84cb0b6006804141cd064ad877f0ce2dce913ca81"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"f5c2bccfc10c9ab7cfc222601370ba5970850058d3b9935cc7f23ac862d44774","funds_nout":1,"funds_amount":12012,"funds_commit_sig":"11894a981c7be0e220c45607571687a37f3c425656d99cdf85729fcaddbb16a00eaed0bffe9d7871a22fde90ed6dafbc102a6fa7e54b73faef76d9984928a39d"}],"funds_unspendable_key_factor":"746ab7fdc43f8e98455600bf9cc2a6464b24f9cff09a9acaa07d5380b9ca579e","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"7d0c701ea8e572ceaa90bac1f31eee9c516bbebb279924b04520b2d7d73113bc","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"02a738c524c0b62fb72e3842ce5c6ed416c5b2de80e9238fc4498e4afa94dece91e0a22394dc3e5fbb1a38aaf84cb0b6006804141cd064ad877f0ce2dce913ca81","mining_fee_rate":1000,"funds":[{"funds_txid":"f5c2bccfc10c9ab7cfc222601370ba5970850058d3b9935cc7f23ac862d44774","funds_nout":1,"funds_amount":12012,"funds_commit_sig":"11894a981c7be0e220c45607571687a37f3c425656d99cdf85729fcaddbb16a00eaed0bffe9d7871a22fde90ed6dafbc102a6fa7e54b73faef76d9984928a39d"}],"funds_unspendable_key_factor":"746ab7fdc43f8e98455600bf9cc2a6464b24f9cff09a9acaa07d5380b9ca579e","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"e3db9da253174fa8d4b016eae2f4c5a166f89866d2dfb12ad0f2d029f69225fb40fff1f173d8e6c683a63828291959f0a37e7532915f1f0bac752917e053770e"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"f5c2bccfc10c9ab7cfc222601370ba5970850058d3b9935cc7f23ac862d44774","funds_nout":1,"funds_amount":12012,"funds_commit_sig":"11894a981c7be0e220c45607571687a37f3c425656d99cdf85729fcaddbb16a00eaed0bffe9d7871a22fde90ed6dafbc102a6fa7e54b73faef76d9984928a39d"}],"funds_unspendable_key_factor":"746ab7fdc43f8e98455600bf9cc2a6464b24f9cff09a9acaa07d5380b9ca579e","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"78e3d302538017630a1b7625a796508ca12f0ded74d427f8d3a8faae1eecb4ac499b0aade54b1ca8eb247bff8e0bd639297c0a0a56a3b179c160dd455fee7d80"}})json",
//    };
//    const std::string fund_min_dust_1_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"b840963bc48471684e65791b13fac3a9948c1c0500887b0b2df4d0c811953b4f","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"3599a96b94ebc59a4f3a1db667c72cdc1f0e16b161f9e3c30d81544e3050bb5f30367941bf014a1543eab41b9e5cc50227162b7aebe8f90e25b187c7d74fb03081"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"e725e9f7569d54f1a4b6189b8e8cb8fb1542b957e7a5a0748596dc3b821822c3","funds_nout":0,"funds_amount":12013,"funds_commit_sig":"42f88ce2dcfc527e8bcad26c618c28afdc6de65e9f6a91d9b2b3a6a8ab6936eb243f3ffed15c58da1621f0b2bd5dc25b28f5a538ae57608518ea4c0b4b5baa69"}],"funds_unspendable_key_factor":"e3fd82a6452fcaad3bf3a440ba50844673fe26d564911321da13e7da4e85af1f","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"b840963bc48471684e65791b13fac3a9948c1c0500887b0b2df4d0c811953b4f","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"3599a96b94ebc59a4f3a1db667c72cdc1f0e16b161f9e3c30d81544e3050bb5f30367941bf014a1543eab41b9e5cc50227162b7aebe8f90e25b187c7d74fb03081","mining_fee_rate":1000,"funds":[{"funds_txid":"e725e9f7569d54f1a4b6189b8e8cb8fb1542b957e7a5a0748596dc3b821822c3","funds_nout":0,"funds_amount":12013,"funds_commit_sig":"42f88ce2dcfc527e8bcad26c618c28afdc6de65e9f6a91d9b2b3a6a8ab6936eb243f3ffed15c58da1621f0b2bd5dc25b28f5a538ae57608518ea4c0b4b5baa69"}],"funds_unspendable_key_factor":"e3fd82a6452fcaad3bf3a440ba50844673fe26d564911321da13e7da4e85af1f","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"bbd71733bfc75acf352c950c87d36cceafddc49b3009f94b50cf75cc91bb177dcf2296bc915a2e6553545229b5ee4c50ad4475f89e0c71b5b18f5192514276dd"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"e725e9f7569d54f1a4b6189b8e8cb8fb1542b957e7a5a0748596dc3b821822c3","funds_nout":0,"funds_amount":12013,"funds_commit_sig":"42f88ce2dcfc527e8bcad26c618c28afdc6de65e9f6a91d9b2b3a6a8ab6936eb243f3ffed15c58da1621f0b2bd5dc25b28f5a538ae57608518ea4c0b4b5baa69"}],"funds_unspendable_key_factor":"e3fd82a6452fcaad3bf3a440ba50844673fe26d564911321da13e7da4e85af1f","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"d3cd0b509c2df6366c8a2892cc93c2722db4b18595da45a41944d86d8b10d11f4d324d17e334d7be5a2c3b12caf9123ec996c0dfbddf83be7e0b64d473de4ce2"}})json",
//    };
//    const std::string fund_min_dust_100_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"f7f970675ef75f0bd04d89ddfdb9e618c3c7053e419f4a3c1bbaf958f636c63a","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"7a3d87c82da033b24cad3815120e2618da6689b5f332cb7f5e7adf52a13351299247b8a678316dc19284a71a6b0f8e554497003f12c184ec31fc798f0abc97f981"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"90a7e6ada471d33a5fb30412997d5a11b173ac4c8398a9784d502036559f0a3f","funds_nout":1,"funds_amount":11912,"funds_commit_sig":"3200c0662b23922b5a95bfedb5c81ece30d2d98950dd926e921611b27f8c4b6d8b9149f5211a8a4472ef2a46f9b0de9105dbcfc626eb4d85bdf3806a633ed71d"}],"funds_unspendable_key_factor":"014f82c6e0c87b799c624959091a81c70ea6f2fad0d0d3c8bd2eb41caa97bcf8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"f7f970675ef75f0bd04d89ddfdb9e618c3c7053e419f4a3c1bbaf958f636c63a","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"7a3d87c82da033b24cad3815120e2618da6689b5f332cb7f5e7adf52a13351299247b8a678316dc19284a71a6b0f8e554497003f12c184ec31fc798f0abc97f981","mining_fee_rate":1000,"funds":[{"funds_txid":"90a7e6ada471d33a5fb30412997d5a11b173ac4c8398a9784d502036559f0a3f","funds_nout":1,"funds_amount":11912,"funds_commit_sig":"3200c0662b23922b5a95bfedb5c81ece30d2d98950dd926e921611b27f8c4b6d8b9149f5211a8a4472ef2a46f9b0de9105dbcfc626eb4d85bdf3806a633ed71d"}],"funds_unspendable_key_factor":"014f82c6e0c87b799c624959091a81c70ea6f2fad0d0d3c8bd2eb41caa97bcf8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"eff3b145d28af05ec74a3f0fe0890b49313e0b52b14aa2a17dea853ebbf48c7d6f068cbf25e3da6c14d0165392e79bd036a9d223ef87e2a5c8c5bd390cf987f0"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"90a7e6ada471d33a5fb30412997d5a11b173ac4c8398a9784d502036559f0a3f","funds_nout":1,"funds_amount":11912,"funds_commit_sig":"3200c0662b23922b5a95bfedb5c81ece30d2d98950dd926e921611b27f8c4b6d8b9149f5211a8a4472ef2a46f9b0de9105dbcfc626eb4d85bdf3806a633ed71d"}],"funds_unspendable_key_factor":"014f82c6e0c87b799c624959091a81c70ea6f2fad0d0d3c8bd2eb41caa97bcf8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"33042dfb247de5485389a4ef2b7fd95100c5d579395531f79ba8415569cf1c0d06d7c5c3f4b4bd778346da9a8084a6a5ece47d7d10a398c4ecdd99ffdc6ef869"}})json",
//    };
//    const std::string fund_min_50_contracts[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"57b431a965b2a7bd9b12812098e8541bbd3ea757d62b9f4eb5515017fc1c8a46","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"2bdb79fd67d3d46f6915f423520fe7956f3d50f47773b4388947c08acecb881aac15c7a7dfd6438b081350a336ecc4695fc127d0943b7d45e8284ed91a2878fe81"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"8e2061f0f5a0b8392bde17ed330f87c7edf92c70506fdb9473ec6d5fe03298c6","funds_nout":0,"funds_amount":11732,"funds_commit_sig":"4b3cf1a8951bdfbd8f436afd92141073f2d570133d549cb4ccd8b204a98dacb4151ca325dfffe7f695f446a2a4de3451995bd775fac0e2d3a4ba20ae0bfb23c5"}],"funds_unspendable_key_factor":"d3ede0d8fff76298e33a71328732b2b2545dceac6ab2d0ba1d84f3811a6e77d8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"57b431a965b2a7bd9b12812098e8541bbd3ea757d62b9f4eb5515017fc1c8a46","ord_nout":1,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"2bdb79fd67d3d46f6915f423520fe7956f3d50f47773b4388947c08acecb881aac15c7a7dfd6438b081350a336ecc4695fc127d0943b7d45e8284ed91a2878fe81","mining_fee_rate":1000,"funds":[{"funds_txid":"8e2061f0f5a0b8392bde17ed330f87c7edf92c70506fdb9473ec6d5fe03298c6","funds_nout":0,"funds_amount":11732,"funds_commit_sig":"4b3cf1a8951bdfbd8f436afd92141073f2d570133d549cb4ccd8b204a98dacb4151ca325dfffe7f695f446a2a4de3451995bd775fac0e2d3a4ba20ae0bfb23c5"}],"funds_unspendable_key_factor":"d3ede0d8fff76298e33a71328732b2b2545dceac6ab2d0ba1d84f3811a6e77d8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"f75e09db78b99620f89929f3618f65af404fd4d7a3db06f3346c5f4c6852b0fb9bc76b3cf66c24da64556867cb91dc3c08b844cdf7366d566c558bb6cc25b56c"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"8e2061f0f5a0b8392bde17ed330f87c7edf92c70506fdb9473ec6d5fe03298c6","funds_nout":0,"funds_amount":11732,"funds_commit_sig":"4b3cf1a8951bdfbd8f436afd92141073f2d570133d549cb4ccd8b204a98dacb4151ca325dfffe7f695f446a2a4de3451995bd775fac0e2d3a4ba20ae0bfb23c5"}],"funds_unspendable_key_factor":"d3ede0d8fff76298e33a71328732b2b2545dceac6ab2d0ba1d84f3811a6e77d8","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"d6fbf3428ded4d4f9054235b29c20da6030e3ad0da8bf526f5f6854012335ab0f81a27f6d3e142efd05b0f5ec3697d881533694816b3a5cdeeedf723ac78c218"}})json",
//    };
//    const std::string fund_min_3000_contracts2[] = {
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"c1f46771e20dbefdc8777c68789601c21903e7925f4925d80cc31be7fedfbb64","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"039b129aace11bbacaead3325fe22aea6184da14daf4ad09738249fa79b3f6c4dcff8dc75f2dc6097cba44981622ffc7ce0e8b7e42fdc3837fa0abd082baf93481"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"d9b884f49193b4e16bc1811e364bbca404517b91a36eb7748aff6fcdd7a219ee","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"1e841b1698fa24c3392d00ccfc9550c8488056e6e8fd9ebb9d1311574a500019fed053527b62fdddc47285dccf290585352589d605afabc76387c0706c5badee"},{"funds_txid":"ee56b64319551f2d716b4b7d419fd56b1091e196960b1f895696a5b9daf8f4c6","funds_nout":1,"funds_amount":3000,"funds_commit_sig":"aed1122d4d3a1684f93ff67bd6963b0e05b7906d9d9fa97fb968ddf34edb5883c87070d5a14cc0463cf64d2ed9d7e922b7f7424b29c97245b13dca6010561900"}],"funds_unspendable_key_factor":"08606ba3b029d1c7545fcb3111eb59779a139550f4c192f68cd43d7b0d35b8f6","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"ord_txid":"c1f46771e20dbefdc8777c68789601c21903e7925f4925d80cc31be7fedfbb64","ord_nout":0,"ord_amount":546,"ord_pk":"569702e32f2140651994f934f467524d7de9d75c0cf4c8fda7ef60e48ad402bf","swap_script_pk_A":"5b6334756b300f7c11f4739a33a890fcfa481c761b33fc60a479a51cbfc734e0","ord_swap_sig_A":"039b129aace11bbacaead3325fe22aea6184da14daf4ad09738249fa79b3f6c4dcff8dc75f2dc6097cba44981622ffc7ce0e8b7e42fdc3837fa0abd082baf93481","mining_fee_rate":1000,"funds":[{"funds_txid":"d9b884f49193b4e16bc1811e364bbca404517b91a36eb7748aff6fcdd7a219ee","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"1e841b1698fa24c3392d00ccfc9550c8488056e6e8fd9ebb9d1311574a500019fed053527b62fdddc47285dccf290585352589d605afabc76387c0706c5badee"},{"funds_txid":"ee56b64319551f2d716b4b7d419fd56b1091e196960b1f895696a5b9daf8f4c6","funds_nout":1,"funds_amount":3000,"funds_commit_sig":"aed1122d4d3a1684f93ff67bd6963b0e05b7906d9d9fa97fb968ddf34edb5883c87070d5a14cc0463cf64d2ed9d7e922b7f7424b29c97245b13dca6010561900"}],"funds_unspendable_key_factor":"08606ba3b029d1c7545fcb3111eb59779a139550f4c192f68cd43d7b0d35b8f6","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","ordpayoff_sig":"adccd9387e3a006c2afa90b74b4acb41e9b828b0056132bdcda312a65afbf88c688d9b694b691e20aa13db7c60c0b1bbc0583914d0c532fda0f256e6cca8cca1"}})json",
//            R"json({"contract_type":"SwapInscription","params":{"protocol_version":4,"ord_price":10000,"market_fee":1000,"swap_script_pk_M":"54e3178cb4089b632982bbbc43d6eafc7e6e6ce36bf1f51d866fee49d9b211f2","ord_mining_fee_rate":2000,"mining_fee_rate":1000,"funds":[{"funds_txid":"d9b884f49193b4e16bc1811e364bbca404517b91a36eb7748aff6fcdd7a219ee","funds_nout":1,"funds_amount":11682,"funds_commit_sig":"1e841b1698fa24c3392d00ccfc9550c8488056e6e8fd9ebb9d1311574a500019fed053527b62fdddc47285dccf290585352589d605afabc76387c0706c5badee"},{"funds_txid":"ee56b64319551f2d716b4b7d419fd56b1091e196960b1f895696a5b9daf8f4c6","funds_nout":1,"funds_amount":3000,"funds_commit_sig":"aed1122d4d3a1684f93ff67bd6963b0e05b7906d9d9fa97fb968ddf34edb5883c87070d5a14cc0463cf64d2ed9d7e922b7f7424b29c97245b13dca6010561900"}],"funds_unspendable_key_factor":"08606ba3b029d1c7545fcb3111eb59779a139550f4c192f68cd43d7b0d35b8f6","swap_script_pk_B":"c649063e4090eba4d55825ff30f5caffc93ca5c9af223db8d0e1592009f67413","funds_swap_sig_B":"b052bf74a240d6571610dad165237ec790a16c0d60981b2bdb5d3ce708517a0cffc384cb0224bd363d47ebd932d1f6f003bb7af12a483a0ca9ce8be62d7ff3ea"}})json",
//    };
//
//    auto terms = GENERATE_REF(
//            fund_min_contracts,
//            fund_min_3000_contracts,
//            fund_min_dust_contracts,
//            fund_min_dust_1_contracts,
//            fund_min_dust_100_contracts,
//            fund_min_50_contracts,
//            fund_min_3000_contracts2
//    );
//    std::string fee_rate = FormatAmount(1000);
//
//    SwapInscriptionBuilder builderMarket(*bech);
//    CHECK_NOTHROW(builderMarket.MarketFee(MARKET_FEE, bech->Encode(swap_script_key_M.PubKey())));
//    CHECK_NOTHROW(builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2)));
//    CHECK_NOTHROW(builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.PubKey())));
//
////    std::clog << "ORD_TERMS: ===========================================\n"
////              << marketOrdConditions << std::endl;
//
//    SwapInscriptionBuilder builderOrdSeller(*bech);
//    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(terms[0]));
//    CHECK_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));
//
//    CHECK_NOTHROW(builderOrdSeller.OrdPrice(ORD_PRICE));
//    CHECK_NOTHROW(builderOrdSeller.OrdUTXO("8d5f91291826bdc6bff1e9e5fbd90d12e3764f2eb2e34a4bac166eeca6961865", 0, FormatAmount(546)));
//    CHECK_NOTHROW(builderOrdSeller.SwapScriptPubKeyA(hex(swap_script_key_A.GetLocalPubKey())));
//
//    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(hex(ord_utxo_key.GetLocalPrivKey())));
//
////    std::clog << "ORD_SWAP_SIG: ===========================================\n"
////              << ordSellerTerms << std::endl;
////
//    builderMarket.Deserialize(terms[1]);
//    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(ORD_SWAP_SIG));
//
////    std::clog << "FUNDS_TERMS: ===========================================\n"
////              << marketFundsConditions << std::endl;
//
//    SwapInscriptionBuilder builderOrdBuyer(*bech);
//    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(terms[2]));
//
//
////    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.GetLocalPubKey()));
////
////    //Fund commitment
////    string funds_addr = w->bech32().Encode(funds_utxo_key.GetLocalPubKey());
////    std::vector<CTxOut> spent_outs;
////
////    for (CAmount amount: condition.funds) {
////        const std::string funds_amount = FormatAmount(amount);
////
////        string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
////        auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);
////
////        builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, funds_addr);
////        spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.GetLocalPubKey());
////    }
////
////    for (size_t n = 0; n < condition.funds.size(); ++n) {
////        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(n, hex(funds_utxo_key.GetLocalPrivKey())));
////    }
////
////    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(builderOrdBuyer.GetFundsCommitTx())));
////
////    string ordBuyerTerms;
////    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(5, FUNDS_COMMIT_SIG));
////
////    std::clog << "FUNDS_COMMIT_SIG: ===========================================\n"
////              << ordBuyerTerms << std::endl;
//    SwapInscriptionBuilder builderMarket1(*bech);
//    REQUIRE_NOTHROW(builderMarket1.Deserialize(terms[1]));
//    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(ORD_SWAP_SIG));
//
//    REQUIRE_NOTHROW(builderMarket1.Deserialize(terms[3]));
//    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(FUNDS_COMMIT_SIG));
//
//    string funds_commit_raw_tx;
//    REQUIRE_NOTHROW(funds_commit_raw_tx = builderMarket1.FundsCommitRawTransaction());
//
//    CMutableTransaction funds_commit_tx;
//    REQUIRE(DecodeHexTx(funds_commit_tx, funds_commit_raw_tx));
//
//    REQUIRE_NOTHROW(builderMarket1.MarketSignOrdPayoffTx(hex(swap_script_key_M.GetLocalPrivKey())));
//
////        std::clog << "MARKET_PAYOFF_SIG: ===========================================\n"
////                  << ordMarketTerms << std::endl;
//        REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(terms[4]));
//        REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(MARKET_PAYOFF_SIG));
//
//        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(hex(swap_script_key_B.GetLocalPrivKey())));
//
////        std::clog << "FUNDS_SWAP_SIG: ===========================================\n"
////                  << ordFundsSignature << std::endl;
//        REQUIRE_NOTHROW(builderMarket1.Deserialize(terms[5]));
//        REQUIRE_NOTHROW(builderMarket1.MarketSignSwap(hex(swap_script_key_M.GetLocalPrivKey())));
//
//        string ord_swap_raw_tx = builderMarket1.OrdSwapRawTransaction();
//        string ord_transfer_raw_tx = builderMarket1.OrdPayoffRawTransaction();
//
//        CMutableTransaction ord_swap_tx, ord_transfer_tx;
//        REQUIRE(DecodeHexTx(ord_swap_tx, ord_swap_raw_tx));
//        REQUIRE(DecodeHexTx(ord_transfer_tx, ord_transfer_raw_tx));
//
//        CHECK(ord_transfer_tx.vout[0].nValue == ParseAmount(ORD_AMOUNT));
//}
