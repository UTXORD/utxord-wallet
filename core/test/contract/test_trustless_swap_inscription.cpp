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
#include "schnorr.hpp"
#include "exechelper.hpp"
#include "trustless_swap_inscription.hpp"
#include "core_io.h"

#include "policy/policy.h"

#include "test_case_wrapper.hpp"
//#include "testutils.hpp"

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

struct SwapCondition
{
    std::vector<CAmount> funds;
    bool has_change;
};


TEST_CASE("Swap")
{
    const CAmount DUST_AMOUNT = Dust(3000);
    const CAmount ORD_AMOUNT = 546;
    const CAmount ORD_PRICE = 10000;

    KeyRegistry master_key(w->chain(), hex(seed));
    master_key.AddKeyType("fund", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("ord+balance", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","2'"], "change":["0","1"], "index_range":"0-256"})");
    master_key.AddKeyType("script", R"({"look_cache":false, "key_type":"TAPSCRIPT", "accounts":["3'"], "change":["0"], "index_range":"0-256"})");
    master_key.AddKeyType("ordint", R"({"look_cache":false, "key_type":"TAPSCRIPT", "accounts":["4'"], "change":["0"], "index_range":"0-256"})");

    //get key pair
    KeyPair funds_utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
    KeyPair funds_payoff_key = master_key.Derive("m/86'/1'/0'/0/1", false);
    KeyPair ord_utxo_key = master_key.Derive("m/86'/1'/2'/0/0", false);
    KeyPair free_balance_key = master_key.Derive("m/86'/1'/0'/1/1", false);
    KeyPair ord_payoff_key = master_key.Derive("m/86'/1'/2'/0/10", false);
    KeyPair change_key = master_key.Derive("m/86'/1'/0'/1/0", false);
    KeyPair market_script_key = master_key.Derive("m/86'/1'/3'/0/0", true);
    KeyPair ord_script_key = master_key.Derive("m/86'/1'/3'/0/1", true);
    KeyPair ord_int_key = master_key.Derive("m/86'/1'/4'/0/1", true);

    std::string market_fee_addr = w->btc().GetNewAddress();

    CAmount fee_rate;
//    try {
//        fee_rate = w->btc().EstimateSmartFee("1");
//    }
//    catch(...) {
        fee_rate = 1000;
//    }

    std::clog << "Mining fee rate: " << fee_rate << std::endl;

    std::string destination_addr = w->btc().GetNewAddress();

    // ORD side terms
    //--------------------------------------------------------------------------

    TrustlessSwapInscriptionBuilder builderMarket(w->chain());

    CHECK_NOTHROW(builderMarket.MiningFeeRate(fee_rate));
    CHECK_NOTHROW(builderMarket.OrdPrice(ORD_PRICE));
    CHECK_NOTHROW(builderMarket.MarketScriptPubKey(market_script_key.PubKey()));

    //Create ord utxo
    string ord_addr = ord_utxo_key.GetP2TRAddress(Bech32(BTC, w->chain()));
    string ord_txid = w->btc().SendToAddress(ord_addr, FormatAmount(ORD_AMOUNT));
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    CHECK_NOTHROW(builderMarket.CommitOrdinal(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, get<1>(ord_prevout).nValue, ord_addr));
    CHECK_NOTHROW(builderMarket.FundsPayoffAddress(funds_payoff_key.GetP2TRAddress(Bech32(BTC, w->chain()))));

    //Create ord balance
    string free_balance_addr = free_balance_key.GetP2TRAddress(Bech32(BTC, w->chain()));
    string balance_txid = w->btc().SendToAddress(free_balance_addr, FormatAmount(30000));
    auto balance_prevout = w->btc().CheckOutput(balance_txid, free_balance_addr);

    CHECK_NOTHROW(builderMarket.FundCommitOrdinal(get<0>(balance_prevout).hash.GetHex(), get<0>(balance_prevout).n, get<1>(balance_prevout).nValue, free_balance_addr, destination_addr));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(6, TRUSTLESS_ORD_TERMS));

    std::clog << "TRUSTLESS_ORD_TERMS: ===========================================\n"
              << marketOrdConditions << std::endl;

    TrustlessSwapInscriptionBuilder builderOrdSeller(w->chain());
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions, TRUSTLESS_ORD_TERMS));

    REQUIRE_NOTHROW(builderOrdSeller.OrdIntPubKey(ord_int_key.PubKey()));
    REQUIRE_NOTHROW(builderOrdSeller.OrdScriptPubKey(ord_script_key.PubKey()));

    CHECK(builderOrdSeller.TransactionCount(TRUSTLESS_ORD_TERMS) == 2);
    std::string ordCommitHex, ordSwapHex;
    REQUIRE_NOTHROW(ordCommitHex = builderOrdSeller.RawTransaction(TRUSTLESS_ORD_TERMS, 0));
    REQUIRE_NOTHROW(ordSwapHex = builderOrdSeller.RawTransaction(TRUSTLESS_ORD_TERMS, 1));

    CMutableTransaction ordCommit, ordSwap;
    REQUIRE(DecodeHexTx(ordCommit, ordCommitHex));
    REQUIRE(DecodeHexTx(ordSwap, ordSwapHex));
//    LogTx(ordCommit);
//    LogTx(ordSwap);

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdCommitment(master_key, "ord+balance"));
    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(master_key, "script"));

    CHECK(builderOrdSeller.TransactionCount(TRUSTLESS_ORD_SWAP_SIG) == 2);

    CMutableTransaction ordCommitTx;
    REQUIRE_NOTHROW(DecodeHexTx(ordCommitTx, builderOrdSeller.RawTransaction(TRUSTLESS_ORD_SWAP_SIG, 0)));
//    LogTx(ordCommitTx);

    CMutableTransaction ordSwapTx;
    REQUIRE_NOTHROW(DecodeHexTx(ordSwapTx, builderOrdSeller.RawTransaction(TRUSTLESS_ORD_SWAP_SIG, 1)));

    ordSwapTx.vin[0].scriptWitness.stack[1] = signature();

//    LogTx(ordSwapTx);

    REQUIRE_NOTHROW(w->btc().TestTxSequence({ordCommitTx}));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms = builderOrdSeller.Serialize(6, TRUSTLESS_ORD_SWAP_SIG));

    std::clog << "TRUSTLESS_ORD_SWAP_SIG: ===========================================\n"
              << ordSellerTerms << std::endl;

    REQUIRE_NOTHROW(builderMarket.Deserialize(ordSellerTerms, TRUSTLESS_ORD_SWAP_SIG));


    CMutableTransaction ordCommitTx1;
    REQUIRE(DecodeHexTx(ordCommitTx1, builderMarket.OrdCommitRawTransaction()));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ordCommitTx1)));
    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

    // FUNDS side terms
    //--------------------------------------------------------------------------

    CAmount market_fee = GENERATE(1000, 0, 600);
    //CAmount market_fee = GENERATE(1000);

    REQUIRE_NOTHROW(builderMarket.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builderMarket.MarketFee(market_fee, market_fee_addr));
    REQUIRE_NOTHROW(builderMarket.OrdPayoffAddress(ord_payoff_key.GetP2TRAddress(Bech32(BTC, w->chain()))));
    REQUIRE_NOTHROW(builderMarket.ChangeAddress(change_key.GetP2TRAddress(Bech32(BTC, w->chain()))));

    TrustlessSwapInscriptionBuilder builderMarket1(w->chain());

    bool complete_swap = false;
    CAmount swapFundsIn = ordCommit.vout.front().nValue;

    SECTION("Commit Funds") {
        std::clog << "Commit Funds ================================================================" << std::endl;
        complete_swap = true;

        CAmount min_funding = builderMarket.GetMinFundingAmount("");
        CAmount min_funding_change = builderMarket.GetMinFundingAmount("change");

        const SwapCondition fund_min_cond = {{min_funding}, false};
        const SwapCondition fund_min_3000_cond = {{min_funding + 3000}, true};
        const SwapCondition fund_min_dust_cond = {{min_funding_change + DUST_AMOUNT - 1}, false};
        const SwapCondition fund_min_dust_1_cond = {{min_funding_change + DUST_AMOUNT}, true};
        const SwapCondition fund_min_dust_100_cond = {{min_funding + DUST_AMOUNT - 100}, false};
        const SwapCondition fund_min_50_cond = {{min_funding + 50}, false};

        const SwapCondition fund_min_3000_cond_2 = {{min_funding, 3000}, true};

        auto condition = GENERATE_REF(
                fund_min_cond,
                fund_min_3000_cond,
                fund_min_dust_cond,
                fund_min_dust_1_cond,
                fund_min_dust_100_cond,
                fund_min_50_cond,
                fund_min_3000_cond_2
        );

        //Fund commitment
        string funds_addr = funds_utxo_key.GetP2TRAddress(Bech32(BTC, w->chain()));
        std::vector<CTxOut> spent_outs;

        for (CAmount amount: condition.funds) {
            const std::string funds_amount = FormatAmount(amount);

            string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
            auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

            REQUIRE_NOTHROW(builderMarket.CommitFunds(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, amount, funds_addr));
            spent_outs.emplace_back(amount, CScript() << 1 << funds_utxo_key.PubKey());
        }

        string marketFundsConditions;
        REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(6, TRUSTLESS_FUNDS_TERMS));

        std::clog << "TRUSTLESS_FUNDS_TERMS: ===========================================\n"
                  << marketFundsConditions << std::endl;

        TrustlessSwapInscriptionBuilder builderOrdBuyer(w->chain());
        REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions, TRUSTLESS_FUNDS_TERMS));

        CHECK(builderOrdBuyer.TransactionCount(TRUSTLESS_FUNDS_COMMIT_SIG) == 1);

        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(master_key, "fund"));

        //auto commitTx = builderOrdBuyer.GetFundsCommitTx();
        //LogTx(commitTx);

//    PrecomputedTransactionData txdata;
//    txdata.Init(commitTx, move(spent_outs), true);
//
//    for (uint32_t n = 0; n < commitTx.vin.size(); ++n) {
//        const CTxIn &in = commitTx.vin.at(n);
//        MutableTransactionSignatureChecker txChecker(&commitTx, n, txdata.m_spent_outputs[n].nValue, txdata, MissingDataBehavior::FAIL);
//        bool res = VerifyScript(in.scriptSig, txdata.m_spent_outputs[n].scriptPubKey, &in.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, txChecker);
//        REQUIRE(res);
//    }

        string ordBuyerTerms;
        REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(6, TRUSTLESS_FUNDS_COMMIT_SIG));

        std::clog << "TRUSTLESS_FUNDS_COMMIT_SIG: ===========================================\n"
                  << ordBuyerTerms << std::endl;

        REQUIRE_NOTHROW(builderMarket1.Deserialize(ordBuyerTerms, TRUSTLESS_FUNDS_COMMIT_SIG));

        CMutableTransaction fundCommitTx;
        REQUIRE(DecodeHexTx(fundCommitTx, builderMarket1.FundsCommitRawTransaction()));

        for (const auto& out: fundCommitTx.vout) swapFundsIn += out.nValue;

        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(fundCommitTx)));
        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
    }

    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordSellerTerms, TRUSTLESS_ORD_SWAP_SIG));

    SECTION("Ready Funds") {
        std::clog << "Ready Funds ================================================================" << std::endl;
        complete_swap = true;

        REQUIRE_NOTHROW(builderMarket1.MiningFeeRate(fee_rate));
        REQUIRE_NOTHROW(builderMarket1.MarketFee(market_fee, market_fee_addr));
        REQUIRE_NOTHROW(builderMarket1.OrdPayoffAddress(ord_payoff_key.GetP2TRAddress(Bech32(BTC, w->chain()))));
        REQUIRE_NOTHROW(builderMarket1.ChangeAddress(change_key.GetP2TRAddress(Bech32(BTC, w->chain()))));

        //Brick1 = Dust
        //----
        //Brick2 = (market_fee >= Dust * 2) ? (market_fee - DUST_AMOUNT) : any
        //----
        //Ord
        //----
        //GetMinSwapFundingAmount()

        string funds_addr = funds_utxo_key.GetP2TRAddress(Bech32(BTC, w->chain()));

        const CAmount brick1_amount = DUST_AMOUNT;
        const CAmount brick2_amount = (market_fee >= DUST_AMOUNT * 2) ? (market_fee - DUST_AMOUNT) : 1000;
        const CAmount min_amount = builderMarket1.GetMinSwapFundingAmount();

        const SwapCondition fund_min_cond = {{min_amount}, false};
        const SwapCondition fund_min_3000_cond = {{min_amount + 3000}, true};
        const SwapCondition fund_min_dust_cond = {{min_amount + DUST_AMOUNT - 1 + builderMarket1.GetNewOutputMiningFee()}, false};
        const SwapCondition fund_min_dust_1_cond = {{min_amount + DUST_AMOUNT + builderMarket1.GetNewOutputMiningFee()}, true};
        const SwapCondition fund_min_dust_100_cond = {{min_amount + DUST_AMOUNT - 100}, false};

        const SwapCondition fund_min_3000_cond_2 = {{min_amount, 3000}, true};

        auto condition = GENERATE_REF(
                fund_min_cond,
                fund_min_3000_cond,
                fund_min_dust_cond,
                fund_min_dust_1_cond,
                fund_min_dust_100_cond,
                fund_min_3000_cond_2
        );

        string brick1_txid = w->btc().SendToAddress(funds_addr, FormatAmount(brick1_amount));
        auto brick1_prevout = w->btc().CheckOutput(brick1_txid, funds_addr);

        string brick2_txid = w->btc().SendToAddress(funds_addr, FormatAmount(brick2_amount));
        auto brick2_prevout = w->btc().CheckOutput(brick2_txid, funds_addr);

        builderMarket1.Brick1SwapUTXO(brick1_txid, get<0>(brick1_prevout).n, brick1_amount, funds_addr);
        builderMarket1.Brick2SwapUTXO(brick2_txid, get<0>(brick2_prevout).n, brick2_amount, funds_addr);

        //std::vector<CTxOut> spent_outs;

        for (CAmount amount: condition.funds) {
            string funds_txid = w->btc().SendToAddress(funds_addr, FormatAmount(amount));
            auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

            REQUIRE_NOTHROW(builderMarket1.AddMainSwapUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, amount, funds_addr));

            swapFundsIn += amount;
        }
    }

    // MARKET confirm terms
    //--------------------------------------------------------------------------

    if (complete_swap) {
        REQUIRE_NOTHROW(builderMarket1.SignMarketSwap(master_key, "script"));

        string ordSwapTerms;
        REQUIRE_NOTHROW(ordSwapTerms = builderMarket1.Serialize(6, TRUSTLESS_FUNDS_SWAP_TERMS));

        std::clog << "TRUSTLESS_FUNDS_SWAP_TERMS: ===========================================\n"
                  << ordSwapTerms << std::endl;

        TrustlessSwapInscriptionBuilder builderOrdBuyer(w->chain());
        REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordSwapTerms, TRUSTLESS_FUNDS_SWAP_TERMS));

        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(master_key, "fund"));

        string ordFundsSignature;
        REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(6, TRUSTLESS_FUNDS_SWAP_SIG));

        REQUIRE_NOTHROW(builderMarket1.Deserialize(ordFundsSignature, TRUSTLESS_FUNDS_SWAP_SIG));

        string swap_raw_tx = builderMarket1.OrdSwapRawTransaction();

        CMutableTransaction swapTx;
        REQUIRE(DecodeHexTx(swapTx, swap_raw_tx));
//
//        LogTx(swapTx);
//
//        std::clog << "Swap TX mining fee:  " << l15::CalculateTxFee(1000, swapTx) << std::endl;

        REQUIRE(swapTx.vout.size() >= 3);
        CHECK(swapTx.vout[1].nValue == ORD_AMOUNT);
        CHECK(swapTx.vout[2].nValue == ORD_PRICE);

//        if (market_fee >= l15::Dust(DUST_RELAY_TX_FEE) * 2) {
//            //CHECK(swapTx.vout.size() == (condition.has_change ? 4 : 3));
//            CHECK(swapTx.vout[0].nValue == market_fee);
//        }
//        else {
//            CHECK(swapTx.vout.size() == (market_fee ? 4 : 3));
//            if (market_fee) {
//                CHECK(swapTx.vout[3].nValue == market_fee);
//            }
//        }

//    PrecomputedTransactionData txdata;
//    txdata.Init(ordSwapTx, {ord_commit_tx.vout[0], funds_commit_tx.vout[0]}, / * force=* / true);
//
//    const CTxIn& ordTxin = ordSwapTx.vin.at(0);
//    MutableTransactionSignatureChecker TxOrdChecker(&ordSwapTx, 0, ord_commit_tx.vout[0].nValue, txdata, MissingDataBehavior::FAIL);
//    bool ordPath = VerifyScript(ordTxin.scriptSig, ord_commit_tx.vout[0].scriptPubKey, &ordTxin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
//    REQUIRE(ordPath);
//
//    const CTxIn& txin = ordSwapTx.vin.at(1);
//    MutableTransactionSignatureChecker tx_checker(&ordSwapTx, 1, funds_commit_tx.vout[0].nValue, txdata, MissingDataBehavior::FAIL);
//    bool fundsPath = VerifyScript(txin.scriptSig, funds_commit_tx.vout[0].scriptPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, tx_checker);
//    REQUIRE(fundsPath);

//        CHECK(CheckMiningFee(swapFundsIn, swapTx, 1000) < l15::Dust() + 43);

        REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(swapTx)));

        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
    }
}


