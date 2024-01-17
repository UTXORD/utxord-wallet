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
    std::vector<CAmount> funds;
    bool has_change;
};


TEST_CASE("Swap")
{
    const std::string ORD_AMOUNT = "0.00000546";
    const std::string ORD_PRICE = "0.0001";

    KeyRegistry master_key(bech->GetChainMode(), hex(seed));
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

    std::string fee_rate;
//    try {
//        fee_rate = w->btc().EstimateSmartFee("1");
//    }
//    catch(...) {
        fee_rate = FormatAmount(1000);
//    }

    std::clog << "Mining fee rate: " << fee_rate << std::endl;

    std::string destination_addr = w->btc().GetNewAddress();

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(*bech);

    CHECK_NOTHROW(builderMarket.MiningFeeRate(fee_rate));
    CHECK_NOTHROW(builderMarket.OrdPrice(ORD_PRICE));
    CHECK_NOTHROW(builderMarket.MarketScriptPubKey(hex(market_script_key.PubKey())));

    //Create ord utxo
    string ord_addr = ord_utxo_key.GetP2TRAddress(*bech);
    string ord_txid = w->btc().SendToAddress(ord_addr, ORD_AMOUNT);
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    CHECK_NOTHROW(builderMarket.CommitOrdinal(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, FormatAmount(get<1>(ord_prevout).nValue), ord_addr));
    CHECK_NOTHROW(builderMarket.FundsPayoffAddress(funds_payoff_key.GetP2TRAddress(*bech)));

    //Create ord balance
    string free_balance_addr = free_balance_key.GetP2TRAddress(*bech);
    string balance_txid = w->btc().SendToAddress(free_balance_addr, FormatAmount(30000));
    auto balance_prevout = w->btc().CheckOutput(balance_txid, free_balance_addr);

    CHECK_NOTHROW(builderMarket.FundCommitOrdinal(get<0>(balance_prevout).hash.GetHex(), get<0>(balance_prevout).n, FormatAmount(get<1>(balance_prevout).nValue), free_balance_addr, destination_addr));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(6, ORD_TERMS));

    std::clog << "ORD_TERMS: ===========================================\n"
              << marketOrdConditions << std::endl;

    SwapInscriptionBuilder builderOrdSeller(*bech);
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions));
    CHECK_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));

    REQUIRE_NOTHROW(builderOrdSeller.OrdIntPubKey(hex(ord_int_key.PubKey())));
    REQUIRE_NOTHROW(builderOrdSeller.OrdScriptPubKey(hex(ord_script_key.PubKey())));

    CHECK(builderOrdSeller.TransactionCount(ORD_TERMS) == 2);
    std::string ordCommitHex, ordSwapHex;
    REQUIRE_NOTHROW(ordCommitHex = builderOrdSeller.RawTransaction(ORD_TERMS, 0));
    REQUIRE_NOTHROW(ordSwapHex = builderOrdSeller.RawTransaction(ORD_TERMS, 1));

    CMutableTransaction ordCommit, ordSwap;
    REQUIRE(DecodeHexTx(ordCommit, ordCommitHex));
    REQUIRE(DecodeHexTx(ordSwap, ordSwapHex));
    LogTx(ordCommit);
    LogTx(ordSwap);

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdCommitment(master_key, "ord+balance"));
    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(master_key, "script"));

    CHECK(builderOrdSeller.TransactionCount(ORD_SWAP_SIG) == 2);

    CMutableTransaction ordCommitTx;
    REQUIRE_NOTHROW(DecodeHexTx(ordCommitTx, builderOrdSeller.RawTransaction(ORD_SWAP_SIG, 0)));
//    LogTx(ordCommitTx);

    CMutableTransaction ordSwapTx;
    REQUIRE_NOTHROW(DecodeHexTx(ordSwapTx, builderOrdSeller.RawTransaction(ORD_SWAP_SIG, 1)));

    ordSwapTx.vin[0].scriptWitness.stack[1] = signature();

//    LogTx(ordSwapTx);

    REQUIRE_NOTHROW(w->btc().TestTxSequence({ordCommitTx}));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms = builderOrdSeller.Serialize(6, ORD_SWAP_SIG));

    std::clog << "ORD_SWAP_SIG: ===========================================\n"
              << ordSellerTerms << std::endl;

    builderMarket.Deserialize(ordSellerTerms);
    REQUIRE_NOTHROW(builderMarket.CheckContractTerms(ORD_SWAP_SIG));


    CMutableTransaction ordCommitTx1;
    REQUIRE(DecodeHexTx(ordCommitTx1, builderMarket.OrdCommitRawTransaction()));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ordCommitTx1)));
    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

    // FUNDS side terms
    //--------------------------------------------------------------------------

    CAmount market_fee = GENERATE(1000, 0, 600);

    REQUIRE_NOTHROW(builderMarket.MiningFeeRate(fee_rate));
    REQUIRE_NOTHROW(builderMarket.MarketFee(FormatAmount(market_fee), market_fee_addr));
    REQUIRE_NOTHROW(builderMarket.OrdPayoffAddress(ord_payoff_key.GetP2TRAddress(*bech)));
    REQUIRE_NOTHROW(builderMarket.ChangeAddress(change_key.GetP2TRAddress(*bech)));

    CAmount min_funding = ParseAmount(builderMarket.GetMinFundingAmount(""));
    CAmount min_funding_change = ParseAmount(builderMarket.GetMinFundingAmount("change"));
    CAmount dust = Dust(3000);

    const SwapCondition fund_min_cond = {{min_funding}, false};
    const SwapCondition fund_min_3000_cond = {{min_funding + 3000}, true};
    const SwapCondition fund_min_dust_cond = {{min_funding_change + dust - 1}, false};
    const SwapCondition fund_min_dust_1_cond = {{min_funding_change + dust}, true};
    const SwapCondition fund_min_dust_100_cond = {{min_funding + dust - 100}, false};
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
    string funds_addr = funds_utxo_key.GetP2TRAddress(*bech);
    std::vector<CTxOut> spent_outs;

    for (CAmount amount: condition.funds) {
        const std::string funds_amount = FormatAmount(amount);

        string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
        auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

        REQUIRE_NOTHROW(builderMarket.CommitFunds(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, funds_addr));
        spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.PubKey());
    }

    string marketFundsConditions;
    REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(6, FUNDS_TERMS));

    std::clog << "FUNDS_TERMS: ===========================================\n"
              << marketFundsConditions << std::endl;

    SwapInscriptionBuilder builderOrdBuyer(*bech);
    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions));

    CHECK(builderOrdBuyer.TransactionCount(FUNDS_COMMIT_SIG) == 1);

    CMutableTransaction fundsCommitTx;
    REQUIRE_NOTHROW(DecodeHexTx(fundsCommitTx, builderOrdBuyer.RawTransaction(FUNDS_COMMIT_SIG, 0)));
    LogTx(fundsCommitTx);

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
    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(6, FUNDS_COMMIT_SIG));

    std::clog << "FUNDS_COMMIT_SIG: ===========================================\n"
              << ordBuyerTerms << std::endl;

    SwapInscriptionBuilder builderMarket1(*bech);
    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordBuyerTerms));
    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(FUNDS_COMMIT_SIG));

    CMutableTransaction fundCommitTx;
    REQUIRE(DecodeHexTx(fundCommitTx, builderMarket1.FundsCommitRawTransaction()));
    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(fundCommitTx)));
    w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

    // MARKET confirm terms
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordSellerTerms));
    REQUIRE_NOTHROW(builderMarket1.CheckContractTerms(ORD_SWAP_SIG));

    REQUIRE_NOTHROW(builderMarket1.SignMarketSwap(master_key, "script"));

    string ordSwapTerms;
    REQUIRE_NOTHROW(ordSwapTerms = builderMarket1.Serialize(6, FUNDS_SWAP_TERMS));

    std::clog << "FUNDS_SWAP_TERMS: ===========================================\n"
              << ordSwapTerms << std::endl;

    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordSwapTerms));
    REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(FUNDS_SWAP_TERMS));

    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(master_key, "fund"));

    string ordFundsSignature;
    REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(6, FUNDS_SWAP_SIG));

    REQUIRE_NOTHROW(builderMarket1.Deserialize(ordFundsSignature));

    string ord_swap_raw_tx = builderMarket1.OrdSwapRawTransaction();

    CMutableTransaction swapTx;
    REQUIRE(DecodeHexTx(swapTx, ord_swap_raw_tx));

    LogTx(swapTx);

    REQUIRE(swapTx.vout.size() >= 3);
    CHECK(swapTx.vout[1].nValue == ParseAmount(ORD_AMOUNT));
    CHECK(swapTx.vout[2].nValue == ParseAmount(ORD_PRICE));

    if (market_fee >= l15::Dust(DUST_RELAY_TX_FEE) * 2) {
        CHECK(swapTx.vout.size() == (condition.has_change ? 4 : 3));
        CHECK(swapTx.vout[0].nValue == market_fee);
    }
    else {
        CHECK(swapTx.vout.size() == (market_fee ? 4 : 3));
        if (market_fee) {
            CHECK(swapTx.vout[3].nValue == market_fee);
        }
    }

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

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(swapTx)));

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
//
//        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");

//        if (condition.has_change) {
//            // BUYER spends his change
//            //--------------------------------------------------------------------------
//
//            CMutableTransaction ord_change_spend_tx;
//            ord_change_spend_tx.vin = {CTxIn(funds_commit_tx.GetHash(), 1)};
//            ord_change_spend_tx.vin.front().scriptWitness.stack.emplace_back(64);
//            ord_change_spend_tx.vout = {CTxOut(funds_commit_tx.vout[1].nValue, bech->PubKeyScript(change_key.GetP2TRAddress(*bech)))};
//
//
//            if (CalculateTxFee(ParseAmount(fee_rate), ord_change_spend_tx) + Dust(3000) < funds_commit_tx.vout[1].nValue) {
//                ord_change_spend_tx.vout.front().nValue = CalculateOutputAmount(funds_commit_tx.vout[1].nValue, ParseAmount(fee_rate), ord_change_spend_tx);
//
//                ChannelKeys changeKey(change_key.PrivKey());
//                REQUIRE_NOTHROW(ord_change_spend_tx.vin.front().scriptWitness.stack[0] = changeKey.SignTaprootTx(ord_change_spend_tx, 0, {funds_commit_tx.vout[1]}, {}));
//                REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(ord_change_spend_tx)));
//            }
//
//        }
//
//        w->btc().GenerateToAddress(w->btc().GetNewAddress(), "1");
}


