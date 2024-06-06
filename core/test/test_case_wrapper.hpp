#pragma once

#include "common.hpp"
#include "chain_api.hpp"
#include "config.hpp"
#include "exechelper.hpp"
#include "keypair.hpp"
#include "nodehelper.hpp"

#include "address.hpp"
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>


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
    ChainMode m_chain;
    l15::core::ChainApi mBtc;
    l15::ExecHelper mCli;
    l15::ExecHelper mBtcd;
    l15::ExecHelper mOrd;

    explicit TestcaseWrapper(const std::string& configpath) :
            mConfFactory(configpath),
            mMode(mConfFactory.conf[l15::config::option::CHAINMODE].as<std::string>()),
            m_chain((mMode == "mainnet") ? MAINNET : ((mMode == "testnet") ? TESTNET : REGTEST)),
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
        if (m_chain == ChainMode::REGTEST) {
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

    std::string GetRunes()
    {
        mOrd.Arguments() = {"--data-dir", mConfFactory.GetBitcoinDataDir() + "/../ord", "--bitcoin-data-dir", mConfFactory.GetBitcoinDataDir(), "--index-runes"};

        if (chain() == REGTEST) {
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

        return mOrd.Run();
    }

    l15::Config& conf()
    { return mConfFactory.conf; }

    l15::core::ChainApi& btc()
    { return mBtc; }

    ChainMode chain() const
    { return m_chain; }

    void ResetRegtestMemPool()
    {
        StartRegtestBitcoinNode();

        std::filesystem::remove(mConfFactory.GetBitcoinDataDir() + "/regtest/mempool.dat");

        StopRegtestBitcoinNode();
    }

    void WaitForConfirmations(uint32_t n)
    {
        if (chain() == REGTEST) {
            btc().GenerateToAddress(btc().GetNewAddress(), std::to_string(n));
        }
        else {
            uint32_t target_height = btc().GetChainHeight() + n;
            do {
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
            while (target_height < btc().GetChainHeight());
        }
    }

    std::string DerivationPath(uint32_t purpose, uint32_t account, uint32_t change, uint32_t index) const
    {
        char buf[32];
        sprintf(buf, "m/%d'/%d'/%d'/%d/%d", purpose, chain() == MAINNET ? 0 : 1, account, change, index);
        return {buf};
    }
};

}