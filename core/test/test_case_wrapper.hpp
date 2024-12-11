#pragma once

#include "utils.hpp"
#include "chain_api.hpp"
#include "config.hpp"
#include "exechelper.hpp"
#include "keyregistry.hpp"
#include "nodehelper.hpp"

#include "contract_builder.hpp"

#include "nlohmann/json.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <optional>


namespace utxord {

struct TestConfigFactory
{
    l15::Config conf;

    explicit TestConfigFactory(const std::string &confpath)
    {
        conf.ProcessConfig({"--conf=" + confpath});
    }

    std::string GetBitcoinDataDir() const
    {
        auto datadir_opt = conf.Subcommand(l15::config::BITCOIND).get_option(l15::config::option::DATADIR);
        if (!datadir_opt->empty())
            return datadir_opt->as<std::string>();
        else
            return std::string();
    }
};

struct TestcaseWrapper
{
    TestConfigFactory mConfFactory;
    std::string mMode;
    std::optional<l15::core::KeyRegistry> mKeyRegistry;
    l15::ChainMode m_chain;
    l15::core::ChainApi mBtc;
    l15::ExecHelper mCli;
    l15::ExecHelper mBtcd;
    l15::ExecHelper mOrd;

    explicit TestcaseWrapper(const std::string& configpath) :
            mConfFactory(configpath),
            mMode(mConfFactory.conf[l15::config::option::CHAINMODE].as<std::string>()),
            m_chain((mMode == "mainnet") ? l15::MAINNET : ((mMode == "testnet") ? l15::TESTNET : l15::REGTEST)),
            mBtc(std::move(mConfFactory.conf.ChainValues(l15::config::BITCOIN))),
            mCli("bitcoin-cli", false),
            mBtcd("bitcoind", false),
            mOrd("ord", false)
    {
        bool is_connected = true;
        try {
            btc().CheckConnection();
        }
        catch (...) {
            is_connected = false;
        }

        if (!is_connected && mMode == "regtest") {
            StartRegtestBitcoinNode();
        }

        try {
            btc().GetWalletInfo();
        }
        catch (...) {
            if (mMode == "regtest") {
                btc().CreateWallet("testwallet");
            }
        }
        if (mMode == "regtest") {
            btc().GenerateToAddress(btc().GetNewAddress(), "101");
        }
        //        else if (mMode == "testnet") {
        //            btc().WalletPassPhrase("********", "30");
        //        }
    }

    virtual ~TestcaseWrapper()
    {
        if (m_chain == l15::REGTEST) {
            StopRegtestBitcoinNode();
            std::filesystem::remove_all(mConfFactory.GetBitcoinDataDir() + "/regtest");
            std::filesystem::remove_all(mConfFactory.GetBitcoinDataDir() + "/../ord/regtest");
        }
    }

    void StartRegtestBitcoinNode()
    {
        StartNode(l15::NodeChainMode::MODE_REGTEST, mBtcd, conf().Subcommand(l15::config::BITCOIND));
    }

    void StopRegtestBitcoinNode()
    {
        StopNode(l15::NodeChainMode::MODE_REGTEST, mCli, conf().Subcommand(l15::config::BITCOIN));
    }

    nlohmann::json rune(std::string_view rune_text)
    {
        mOrd.Arguments() = {"--data-dir", mConfFactory.GetBitcoinDataDir() + "/../ord", "--bitcoin-data-dir", mConfFactory.GetBitcoinDataDir(), "--index-runes"};

        if (chain() == l15::REGTEST) {
            mOrd.Arguments().emplace_back("--regtest");
        }

        std::string host, port;

        for(const auto opt: conf().Subcommand(l15::config::BITCOIN).get_options(
                    [](const CLI::Option* o){ return o && o->check_name(l15::config::option::RPCHOST); }))
            host = opt->as<std::string>();
        for(const auto opt: conf().Subcommand(l15::config::BITCOIN).get_options(
                    [](const CLI::Option* o){ return o && o->check_name(l15::config::option::RPCPORT); }))
            port = opt->as<std::string>();

        mOrd.Arguments().emplace_back("--bitcoin-rpc-url");
        mOrd.Arguments().emplace_back(host + ":" + port);

        mOrd.Arguments().emplace_back("runes");

        nlohmann::json output_json = nlohmann::json::parse(mOrd.Run());
        return output_json["runes"][rune_text];
    }

    nlohmann::json rune_balances(std::string_view rune_text) {
        mOrd.Arguments() = {"--data-dir", mConfFactory.GetBitcoinDataDir() + "/../ord", "--bitcoin-data-dir", mConfFactory.GetBitcoinDataDir(), "--index-runes"};

        if (chain() == l15::REGTEST) {
            mOrd.Arguments().emplace_back("--regtest");
        }

        std::string host, port;

        for(const auto opt: conf().Subcommand(l15::config::BITCOIN).get_options(
                    [](const CLI::Option* o){ return o && o->check_name(l15::config::option::RPCHOST); }))
            host = opt->as<std::string>();
        for(const auto opt: conf().Subcommand(l15::config::BITCOIN).get_options(
                    [](const CLI::Option* o){ return o && o->check_name(l15::config::option::RPCPORT); }))
            port = opt->as<std::string>();

        mOrd.Arguments().emplace_back("--bitcoin-rpc-url");
        mOrd.Arguments().emplace_back(host + ":" + port);

        mOrd.Arguments().emplace_back("balances");

        nlohmann::json output_json = nlohmann::json::parse(mOrd.Run());
        return output_json["runes"][rune_text];
    }

    l15::Config& conf()
    { return mConfFactory.conf; }

    l15::core::ChainApi& btc()
    { return mBtc; }

    l15::ChainMode chain() const
    { return m_chain; }

    l15::core::KeyRegistry& keyreg()
    { return *mKeyRegistry; }

    void ResetRegtestMemPool()
    {
        StartRegtestBitcoinNode();

        std::filesystem::remove(mConfFactory.GetBitcoinDataDir() + "/regtest/mempool.dat");

        StopRegtestBitcoinNode();
    }

    std::tuple<uint64_t, uint32_t> confirm(uint32_t n, const std::string& txid, uint32_t nout = 0)
    {
        uint32_t confirmations = 0;
        uint32_t height = 0;
        std::string blockhash;
        do {
            if (chain() == l15::REGTEST) {
                btc().GenerateToAddress(btc().GetNewAddress(), "1");
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(45));
            }

            std::string strTx = btc().GetTxOut(txid, std::to_string(nout));
            if(strTx.empty()) continue;

            nlohmann::json tx_json = nlohmann::json::parse(strTx);
            confirmations = tx_json["confirmations"].get<uint32_t>();
            blockhash = tx_json["bestblock"].get<std::string>();

        }
        while (confirmations < n);

        nlohmann::json block_json;
        do {
            std::string strBlock = btc().GetBlock(blockhash, "1");
            block_json = nlohmann::json::parse(strBlock);
            height = block_json["height"].get<uint32_t>();
            blockhash = block_json["previousblockhash"].get<std::string>();
            --confirmations;
        }
        while (confirmations > 0);

        const auto& txs = block_json["tx"];
        uint32_t i;
        uint32_t cnt = block_json["nTx"].get<uint32_t>();
        for (i = 0; i < cnt; ++i) {
            std::string tx = txs[i].get<std::string>();
            if (tx == txid) return {height, i};
        }
        throw std::runtime_error("Tx is not found: " + txid);
    }

    void InitKeyRegistry(const std::string& seedhex)
    { mKeyRegistry.emplace(chain(), seedhex); }

    std::string keypath(uint32_t purpose, uint32_t account, uint32_t change, uint32_t index) const
    {
        char buf[32];
        sprintf(buf, "m/%d'/%d'/%d'/%d/%d", purpose, chain() == l15::MAINNET ? 0 : 1, account, change, index);
        return {buf};
    }

    l15::core::KeyPair derive(uint32_t purpose, uint32_t account, uint32_t change, uint32_t index, bool for_script = true)
    { return keyreg().Derive(keypath(purpose, account, change, index), for_script); }

    xonly_pubkey pubkey(uint32_t account, uint32_t change, uint32_t index)
    { return keyreg().Derive(keypath(86, account, change, index), true).GetSchnorrKeyPair().GetPubKey(); }

    std::string p2tr(uint32_t account, uint32_t change, uint32_t index)
    { return keyreg().Derive(keypath(86, account, change, index), false).GetP2TRAddress(l15::Bech32(l15::BTC, chain())); }

    std::string p2wpkh(uint32_t account, uint32_t change, uint32_t index)
    { return keyreg().Derive(keypath(84, account, change, index), false).GetP2WPKHAddress(l15::Bech32(l15::BTC, chain())); }

    std::string p2pkh(uint32_t account, uint32_t change, uint32_t index)
    { return keyreg().Derive(keypath(44, account, change, index), false).GetP2PKHAddress(chain()); }

    std::shared_ptr<IContractOutput> fund(CAmount amount, std::string addr)
    {
        std::string txid = btc().SendToAddress(addr, l15::FormatAmount(amount));
        auto prevout = btc().CheckOutput(txid, addr);

        return std::make_shared<UTXO>(chain(), txid, get<0>(prevout).n, amount, move(addr));
    }
};

}