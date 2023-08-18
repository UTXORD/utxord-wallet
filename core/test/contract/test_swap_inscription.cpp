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


struct SwapCondition
{
    std::vector<CAmount> funds;
    //CAmount funds;
    bool has_change;
};

TEST_CASE("Swap")
{
    const std::string ORD_AMOUNT = "0.00000546";
    const std::string ORD_PRICE = "0.0001";
    const std::string MARKET_FEE = "0.00001";

    ChannelKeys swap_script_key_A;
    ChannelKeys swap_script_key_B;
    ChannelKeys swap_script_key_M;
    //get key pair
    ChannelKeys ord_utxo_key;
    ChannelKeys funds_utxo_key;

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = FormatAmount(1000);
    }

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(ORD_PRICE, MARKET_FEE);
    builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2));
    builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.GetLocalPubKey()));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(ORD_TERMS));

    SwapInscriptionBuilder builderOrdSeller(ORD_PRICE, MARKET_FEE);
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions));

    REQUIRE_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));

    //Create ord utxo
    string ord_addr = w->bech32().Encode(ord_utxo_key.GetLocalPubKey());
    string ord_txid = w->btc().SendToAddress(ord_addr, ORD_AMOUNT);
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, FormatAmount(get<1>(ord_prevout).nValue));
    builderOrdSeller.SwapScriptPubKeyA(hex(swap_script_key_A.GetLocalPubKey()));

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(hex(ord_utxo_key.GetLocalPrivKey())));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms= builderOrdSeller.Serialize(ORD_SWAP_SIG));


    // FUNDS side terms
    //--------------------------------------------------------------------------
    builderMarket.SetMiningFeeRate(fee_rate);
    string marketFundsConditions;
    REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(FUNDS_TERMS));

    SwapInscriptionBuilder builderOrdBuyer(ORD_PRICE, MARKET_FEE);
    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions));

    CAmount min_funding = ParseAmount(builderOrdBuyer.GetMinFundingAmount(""));
    CAmount dust = Dust(3000);

    const SwapCondition fund_min_cond = {{min_funding}, false};
    const SwapCondition fund_min_3000_cond = {{min_funding + 3000}, true};
    const SwapCondition fund_min_dust_cond = {{min_funding+dust}, false};
    const SwapCondition fund_min_dust_1_cond = {{min_funding + dust + 1}, true};
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

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.GetLocalPubKey()));

    //Fund commitment
    string funds_addr = w->bech32().Encode(funds_utxo_key.GetLocalPubKey());
    std::vector<CTxOut> spent_outs;

    for (CAmount amount: condition.funds) {
        const std::string funds_amount = FormatAmount(amount);

        string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
        auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

        builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, hex(funds_utxo_key.GetLocalPubKey()));
        spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.GetLocalPubKey());
    }

    for (size_t n = 0; n < condition.funds.size(); ++n) {
        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(n, hex(funds_utxo_key.GetLocalPrivKey())));
    }

    auto commit_tx = builderOrdBuyer.GetFundsCommitTx();
    PrecomputedTransactionData txdata;
    txdata.Init(commit_tx, move(spent_outs), true);

    for (uint32_t n = 0; n < commit_tx.vin.size(); ++n) {
        const CTxIn &in = commit_tx.vin.at(n);
        MutableTransactionSignatureChecker txChecker(&commit_tx, n, txdata.m_spent_outputs[n].nValue, txdata, MissingDataBehavior::FAIL);
        bool res = VerifyScript(in.scriptSig, txdata.m_spent_outputs[n].scriptPubKey, &in.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, txChecker);
        REQUIRE(res);
    }


    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(builderOrdBuyer.GetFundsCommitTx())));

    string ordBuyerTerms;
    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(FUNDS_COMMIT_SIG));


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

        REQUIRE_NOTHROW(builderMarket.MarketSignOrdPayoffTx(hex(swap_script_key_M.GetLocalPrivKey())));
        string ordMarketTerms;
        REQUIRE_NOTHROW(ordMarketTerms = builderMarket.Serialize(MARKET_PAYOFF_SIG));


        // BUYER sign swap
        //--------------------------------------------------------------------------

        REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordMarketTerms));
        REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(MARKET_PAYOFF_SIG));

        REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(hex(swap_script_key_B.GetLocalPrivKey())));

        string ordFundsSignature;
        REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(FUNDS_SWAP_SIG));


        // MARKET sign swap
        //--------------------------------------------------------------------------

        REQUIRE_NOTHROW(builderMarket.Deserialize(ordFundsSignature));
        REQUIRE_NOTHROW(builderMarket.MarketSignSwap(hex(swap_script_key_M.GetLocalPrivKey())));

        string ord_swap_raw_tx = builderMarket.OrdSwapRawTransaction();
        string ord_transfer_raw_tx = builderMarket.OrdPayoffRawTransaction();

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

            xonly_pubkey change_pk = w->bech32().Decode(w->btc().GetNewAddress());
            CScript buyer_change_pubkey_script = CScript() << 1 << change_pk;

            CMutableTransaction ord_change_spend_tx;
            ord_change_spend_tx.vin = {CTxIn(funds_commit_tx.GetHash(), 1)};
            ord_change_spend_tx.vin.front().scriptWitness.stack.emplace_back(64);
            ord_change_spend_tx.vout = {CTxOut(funds_commit_tx.vout[1].nValue, buyer_change_pubkey_script)};


            if (CalculateTxFee(ParseAmount(fee_rate), ord_change_spend_tx) + Dust(3000) < funds_commit_tx.vout[1].nValue) {
                ord_change_spend_tx.vout.front().nValue = CalculateOutputAmount(funds_commit_tx.vout[1].nValue, ParseAmount(fee_rate), ord_change_spend_tx);

                REQUIRE_NOTHROW(ord_change_spend_tx.vin.front().scriptWitness.stack[0] = swap_script_key_B.SignTaprootTx(ord_change_spend_tx, 0, {funds_commit_tx.vout[1]}, {}));
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

    ChannelKeys swap_script_key_A;
    ChannelKeys swap_script_key_B;
    ChannelKeys swap_script_key_M;
    //get key pair
    ChannelKeys ord_utxo_key;
    ChannelKeys funds_utxo_key;

    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = FormatAmount(1000);
    }

    // ORD side terms
    //--------------------------------------------------------------------------

    SwapInscriptionBuilder builderMarket(ORD_PRICE, MARKET_FEE);
    builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2));
    builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.GetLocalPubKey()));

    string marketOrdConditions;
    REQUIRE_NOTHROW(marketOrdConditions = builderMarket.Serialize(ORD_TERMS));

    SwapInscriptionBuilder builderOrdSeller(ORD_PRICE, MARKET_FEE);
    REQUIRE_NOTHROW(builderOrdSeller.Deserialize(marketOrdConditions));

    REQUIRE_NOTHROW(builderOrdSeller.CheckContractTerms(ORD_TERMS));

    //Create ord utxo
    string ord_addr = w->bech32().Encode(ord_utxo_key.GetLocalPubKey());
    string ord_txid = w->btc().SendToAddress(ord_addr, ORD_AMOUNT);
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, FormatAmount(get<1>(ord_prevout).nValue));
    builderOrdSeller.SwapScriptPubKeyA(hex(swap_script_key_A.GetLocalPubKey()));

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(hex(ord_utxo_key.GetLocalPrivKey())));

    string ordSellerTerms;
    REQUIRE_NOTHROW(ordSellerTerms= builderOrdSeller.Serialize(ORD_SWAP_SIG));


    // FUNDS side terms
    //--------------------------------------------------------------------------
    builderMarket.SetMiningFeeRate(fee_rate);
    string marketFundsConditions;
    REQUIRE_NOTHROW(marketFundsConditions = builderMarket.Serialize(FUNDS_TERMS));

    SwapInscriptionBuilder builderOrdBuyer(ORD_PRICE, MARKET_FEE);
    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(marketFundsConditions));

    CAmount min_funding = ParseAmount(builderOrdBuyer.GetMinFundingAmount(""));
    CAmount dust = Dust(3000);

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.GetLocalPubKey()));

    //Fund commitment
    string funds_addr = w->bech32().Encode(funds_utxo_key.GetLocalPubKey());
    std::vector<CTxOut> spent_outs;


    const std::string funds_amount = FormatAmount(min_funding);

    string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);
    auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

    builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, hex(funds_utxo_key.GetLocalPubKey()));
    spent_outs.emplace_back(ParseAmount(funds_amount), CScript() << 1 << funds_utxo_key.GetLocalPubKey());
    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(0, hex(funds_utxo_key.GetLocalPrivKey())));


    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(builderOrdBuyer.GetFundsCommitTx())));

    string ordBuyerTerms;
    REQUIRE_NOTHROW(ordBuyerTerms = builderOrdBuyer.Serialize(FUNDS_COMMIT_SIG));


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

    REQUIRE_NOTHROW(builderMarket.MarketSignOrdPayoffTx(hex(swap_script_key_M.GetLocalPrivKey())));
    string ordMarketTerms;
    REQUIRE_NOTHROW(ordMarketTerms = builderMarket.Serialize(MARKET_PAYOFF_SIG));


    // BUYER sign swap
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderOrdBuyer.Deserialize(ordMarketTerms));
    REQUIRE_NOTHROW(builderOrdBuyer.CheckContractTerms(MARKET_PAYOFF_SIG));

    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsSwap(hex(swap_script_key_B.GetLocalPrivKey())));

    string ordFundsSignature;
    REQUIRE_NOTHROW(ordFundsSignature = builderOrdBuyer.Serialize(FUNDS_SWAP_SIG));


    // MARKET sign swap
    //--------------------------------------------------------------------------

    REQUIRE_NOTHROW(builderMarket.Deserialize(ordFundsSignature));
    REQUIRE_NOTHROW(builderMarket.MarketSignSwap(hex(swap_script_key_M.GetLocalPrivKey())));

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

    ChannelKeys swap_script_key_A;
    ChannelKeys swap_script_key_B;
    ChannelKeys swap_script_key_M;
    seckey preimage = ChannelKeys::GetStrongRandomKey();
    bytevector swap_hash(32);
    CHash256().Write(preimage).Finalize(swap_hash);
    //get key pair
    ChannelKeys ord_utxo_key;
    ChannelKeys funds_utxo_key;

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

    SwapInscriptionBuilder builderMarket(ORD_PRICE, MARKET_FEE);
    builderMarket.SetMiningFeeRate(fee_rate);
    builderMarket.SetSwapScriptPubKeyM(hex(swap_script_key_M.GetLocalPubKey()));
    builderMarket.SetOrdMiningFeeRate(FormatAmount(ParseAmount(fee_rate) * 2));

    string marketOrdConditions = builderMarket.Serialize(ORD_TERMS);

    SwapInscriptionBuilder builderOrdSeller(ORD_PRICE, MARKET_FEE);
    builderOrdSeller.Deserialize(marketOrdConditions);

    builderOrdSeller.CheckContractTerms(ORD_TERMS);

    //Create ord utxo
    string ord_addr = w->bech32().Encode(ord_utxo_key.GetLocalPubKey());
    string ord_txid = w->btc().SendToAddress(ord_addr, "0.0001");
    auto ord_prevout = w->btc().CheckOutput(ord_txid, ord_addr);

    builderOrdSeller.SwapScriptPubKeyA(hex(swap_script_key_A.GetLocalPubKey()));
    builderOrdSeller.OrdUTXO(get<0>(ord_prevout).hash.GetHex(), get<0>(ord_prevout).n, "0.0001");

    REQUIRE_NOTHROW(builderOrdSeller.SignOrdSwap(hex(swap_script_key_A.GetLocalPrivKey())));

    string ordSellerTerms = builderOrdSeller.Serialize(ORD_SWAP_SIG);


    // FUNDS side terms
    //--------------------------------------------------------------------------

    //builderMarket.SetMiningFeeRate(fee_rate);
    string marketFundsConditions = builderMarket.Serialize(FUNDS_TERMS);

    SwapInscriptionBuilder builderOrdBuyer(ORD_PRICE, MARKET_FEE);
    builderOrdBuyer.Deserialize(marketFundsConditions);

    //Create insufficient funds utxo
    std::string funds_amount = FormatAmount(ParseAmount(builderOrdBuyer.GetMinFundingAmount("")) - Dust(3000));
    std::string funds_addr = w->bech32().Encode(funds_utxo_key.GetLocalPubKey());
    std::string funds_txid = w->btc().SendToAddress(funds_addr, funds_amount);

    auto funds_prevout = w->btc().CheckOutput(funds_txid, funds_addr);

    builderOrdBuyer.SwapScriptPubKeyB(hex(swap_script_key_B.GetLocalPubKey()));
    builderOrdBuyer.AddFundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount, hex(funds_utxo_key.GetLocalPubKey()));
    REQUIRE_THROWS_AS(builderOrdBuyer.SignFundsCommitment(0, hex(funds_utxo_key.GetLocalPrivKey())), l15::TransactionError);

    //Create funds utxo
//    funds_amount = builderOrdBuyer.GetMinFundingAmount();
//
//    builderOrdBuyer.FundsUTXO(get<0>(funds_prevout).hash.GetHex(), get<0>(funds_prevout).n, funds_amount);
//    REQUIRE_NOTHROW(builderOrdBuyer.SignFundsCommitment(hex(funds_utxo_key.GetLocalPrivKey())));
}
