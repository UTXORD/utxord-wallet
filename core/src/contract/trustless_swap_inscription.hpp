#pragma once

#include <string>
#include <optional>
#include <list>

#include "script_merkle_tree.hpp"

#include "contract_builder.hpp"
#include "simple_transaction.hpp"

namespace utxord {

enum TrustlessSwapPhase {
    TRUSTLESS_ORD_TERMS,
    TRUSTLESS_ORD_SWAP_SIG,
    TRUSTLESS_FUNDS_TERMS,
    TRUSTLESS_FUNDS_COMMIT_SIG,
    TRUSTLESS_FUNDS_SWAP_TERMS,
    TRUSTLESS_FUNDS_SWAP_SIG,
};

class TrustlessSwapInscriptionBuilder : public ContractBuilder<TrustlessSwapPhase>
{
    static const CAmount TX_SWAP_BASE_VSIZE = 413;

    static const uint32_t s_protocol_version;
    static const char* s_versions;

    std::optional<xonly_pubkey> m_market_script_pk;
    std::optional<xonly_pubkey> m_ord_script_pk;
    std::optional<xonly_pubkey> m_ord_int_pk;

    std::optional<CAmount> m_ord_price;

    std::optional<std::string> m_funds_payoff_addr;

    std::optional<std::string> m_ord_payoff_addr;

    std::shared_ptr<SimpleTransaction> mOrdCommitBuilder;
    std::shared_ptr<SimpleTransaction> mCommitBuilder;

    std::vector<TxInput> m_swap_inputs;

    mutable std::optional<CMutableTransaction> mSwapTpl;
    mutable std::optional<CMutableTransaction> mOrdCommitTx;
    mutable std::optional<CMutableTransaction> mFundsCommitTx;
    mutable std::optional<CMutableTransaction> mSwapTx;

    static CScript MakeMultiSigScript(const xonly_pubkey& pk1, const xonly_pubkey& pk2);
    CScript OrdSwapScript() const;
    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> OrdSwapTapRoot() const;

    CAmount CalculateSwapTxFee(bool change) const;

    void BuildOrdCommit();

    CMutableTransaction MakeSwapTx() const;

    void CheckOrdSwapSig() const;
    void CheckFundsCommitSig() const;

public:
    const CMutableTransaction& GetOrdCommitTx() const;

    //CMutableTransaction GetFundsCommitTxTemplate() const;
    const CMutableTransaction& GetFundsCommitTx() const;

    const CMutableTransaction& GetSwapTx() const;

    static const std::string name_ord_price;
    static const std::string name_ord_commit;

    static const std::string name_market_script_pk;
    static const std::string name_ord_script_pk;
    static const std::string name_ord_int_pk;

    static const std::string name_ord_payoff_addr;
    static const std::string name_funds_payoff_addr;

    static const std::string name_funds;
    static const std::string name_swap_inputs;

    explicit TrustlessSwapInscriptionBuilder(ChainMode mode) : ContractBuilder(mode) {}

    TrustlessSwapInscriptionBuilder(const TrustlessSwapInscriptionBuilder&) = default;
    TrustlessSwapInscriptionBuilder(TrustlessSwapInscriptionBuilder&&) noexcept = default;

    TrustlessSwapInscriptionBuilder& operator=(const TrustlessSwapInscriptionBuilder& ) = default;
    TrustlessSwapInscriptionBuilder& operator=(TrustlessSwapInscriptionBuilder&& ) noexcept = default;

    const std::string& GetContractName() const override;
    UniValue MakeJson(uint32_t version, TrustlessSwapPhase phase) const override;
    void ReadJson(const UniValue& json, TrustlessSwapPhase phase) override;

    static const char* SupportedVersions() { return s_versions; }

    void OrdPrice(const std::string& price)
    { m_ord_price = l15::ParseAmount(price); }

    void MarketScriptPubKey(const std::string& pk)
    { m_market_script_pk = unhex<xonly_pubkey>(pk); }

    void OrdScriptPubKey(const std::string& pk);

    void OrdIntPubKey(const std::string& pk);

    void CommitOrdinal(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr);
    void FundCommitOrdinal(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr, const std::string& change_addr);
    void CommitFunds(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& addr);
    void Brick1SwapUTXO(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& addr);
    void Brick2SwapUTXO(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& addr);
    void AddMainSwapUTXO(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr);

    void OrdPayoffAddress(const std::string& addr)
    {
        bech32().Decode(addr);
        m_ord_payoff_addr = addr;
    }

    void FundsPayoffAddress(const std::string& addr)
    {
        bech32().Decode(addr);
        m_funds_payoff_addr = addr;
    }

    void SignOrdSwap(const KeyRegistry &masterKey, const std::string& key_filter);
    void SignMarketSwap(const KeyRegistry &masterKey, const std::string& key_filter);
    void SignOrdCommitment(const KeyRegistry &master_key, const std::string& key_filter);
    void SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter);
    void SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter);

    void CheckContractTerms(TrustlessSwapPhase phase) const override;

    std::string OrdCommitRawTransaction() const;
    std::string FundsCommitRawTransaction() const;
    std::string OrdSwapRawTransaction() const;

    uint32_t TransactionCount(TrustlessSwapPhase phase) const;
    std::string RawTransaction(TrustlessSwapPhase phase, uint32_t n);

    CAmount CalculateWholeFee(const std::string& params) const override;
    std::string GetMinFundingAmount(const std::string& params) const override;
    std::string GetMinSwapFundingAmount() const;
};

} // namespace l15::utxord
