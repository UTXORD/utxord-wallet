#pragma once

#include <string>
#include <optional>
#include <list>

#include "script_merkle_tree.hpp"

#include "contract_builder.hpp"

namespace utxord {

enum SwapPhase {
    ORD_TERMS,
    FUNDS_TERMS,
    FUNDS_COMMIT_SIG,
    MARKET_PAYOFF_TERMS,
    MARKET_PAYOFF_SIG,
    ORD_SWAP_SIG,
    FUNDS_SWAP_SIG,
    MARKET_SWAP_SIG,
};

class SwapInscriptionBuilder : public utxord::ContractBuilder<utxord::SwapPhase>
{
    CAmount m_whole_fee = 0;
    CAmount m_last_fee_rate = 0;

    static const uint32_t s_protocol_version;
    static const char* s_versions;
    static const uint32_t s_protocol_version_pubkey_v4;
    static const uint32_t s_protocol_version_old_v3;

    std::optional<CAmount> m_ord_price;

    std::optional<CAmount> m_ord_mining_fee_rate;

    std::optional<xonly_pubkey> m_swap_script_pk_B;
    std::optional<xonly_pubkey> m_swap_script_pk_M;

    std::optional<ContractInput> m_ord_input;
    std::optional<std::string> m_funds_payoff_addr;

    std::vector<ContractInput> m_fund_inputs;
    std::optional<std::string> m_ord_payoff_addr;

    std::optional<seckey> m_funds_unspendable_key_factor;

    std::optional<signature> m_funds_swap_sig_B;
    std::optional<signature> m_funds_swap_sig_M;

    std::optional<signature> m_ord_payoff_sig;

    //mutable std::optional<CMutableTransaction> mFundsCommitTpl;
    mutable std::optional<CMutableTransaction> mFundsPaybackTpl;

    mutable std::optional<CMutableTransaction> mSwapTpl;
    mutable std::optional<CMutableTransaction> mOrdPayoffTpl;

    mutable std::optional<CTransaction> mFundsCommitTx;
    mutable std::optional<CTransaction> mFundsPaybackTx;

    mutable std::optional<CTransaction> mSwapTx;
    mutable std::optional<CTransaction> mOrdPayoffTx;

    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> FundsCommitTapRoot() const;

    CMutableTransaction MakeSwapTx(bool with_funds_in) const;

    void CheckFundsCommitSig() const;

    void CheckOrdSwapSig() const;
    void CheckFundsSwapSig() const;
    void CheckMarketSwapSig() const;

    void CheckOrdPayoffSig() const;

    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> FundsCommitTemplateTapRoot() const;
protected:
    std::vector<std::pair<CAmount,CMutableTransaction>> GetTransactions() const override;
    CAmount CalculateWholeFee(const std::string& params) const override;

public:
    CMutableTransaction CreatePayoffTxTemplate() const;
    CMutableTransaction GetSwapTxTemplate() const;

    CMutableTransaction GetFundsCommitTxTemplate(bool segwit_in = true) const;
    CMutableTransaction MakeFundsCommitTx() const;
    const CTransaction& GetFundsCommitTx() const;

    const CTransaction& GetSwapTx() const;
    const CTransaction& GetPayoffTx() const;

    static const std::string name_ord_price;

    static const std::string name_ord_mining_fee_rate;

    static const std::string name_swap_script_pk_A;
    static const std::string name_swap_script_pk_B;
    static const std::string name_swap_script_pk_M;

    static const std::string name_ord_payoff_addr;
    static const std::string name_funds_payoff_addr;

    static const std::string name_ord_input;
    static const std::string name_ord_txid;
    static const std::string name_ord_nout;
    static const std::string name_ord_amount;
    static const std::string name_ord_pk;

    static const std::string name_funds;
    static const std::string name_funds_unspendable_key;
    static const std::string name_funds_txid;
    static const std::string name_funds_nout;
    static const std::string name_funds_amount;

    static const std::string name_funds_commit_sig;

    static const std::string name_ord_swap_sig_A;

    static const std::string name_funds_swap_sig_B;
    static const std::string name_funds_swap_sig_M;

    static const std::string name_ordpayoff_unspendable_key_factor;
    static const std::string name_ord_payoff_sig;

    //explicit SwapInscriptionBuilder(Bech32 bech) : ContractBuilder(bech) {}
    explicit SwapInscriptionBuilder(ChainMode mode) : ContractBuilder(mode) {}

    SwapInscriptionBuilder(const SwapInscriptionBuilder&) = default;
    SwapInscriptionBuilder(SwapInscriptionBuilder&&) noexcept = default;

    SwapInscriptionBuilder& operator=(const SwapInscriptionBuilder& ) = default;
    SwapInscriptionBuilder& operator=(SwapInscriptionBuilder&& ) noexcept = default;

    const std::string& GetContractName() const override;
    void CheckContractTerms(SwapPhase phase) const override;
    UniValue MakeJson(uint32_t version, SwapPhase phase) const override;
    void ReadJson_v4(const UniValue& json, SwapPhase phase);
    void ReadJson(const UniValue& json, SwapPhase phase) override;

    static const char* SupportedVersions() { return s_versions; }

    void OrdPrice(const std::string& price)
    { m_ord_price = l15::ParseAmount(price); }

    void OrdUTXO(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& addr);
    void AddFundsUTXO(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& addr);

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

    void SwapScriptPubKeyB(const std::string& v) { m_swap_script_pk_B = unhex<xonly_pubkey>(v); }

    std::string GetSwapScriptPubKeyB() const { return hex(m_swap_script_pk_B.value()); }

    void SetOrdMiningFeeRate(const std::string& fee_rate) { m_ord_mining_fee_rate = l15::ParseAmount(fee_rate); }

    std::string GetSwapScriptPubKeyM() const { return hex(m_swap_script_pk_M.value()); }
    void SetSwapScriptPubKeyM(const std::string& v) { m_swap_script_pk_M = unhex<xonly_pubkey>(v); }

    void SignOrdSwap(const KeyRegistry &master_key, const std::string& key_filter);

    void SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter);
    void SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter);
    void SignFundsPayBack(const KeyRegistry &master_key, const std::string& key_filter);

    void MarketSignOrdPayoffTx(const KeyRegistry &master_key, const std::string& key_filter);
    void MarketSignSwap(const KeyRegistry &master_key, const std::string& key_filter);

    std::string FundsCommitRawTransaction() const;
    std::string FundsPayBackRawTransaction() const;

    std::string OrdSwapRawTransaction() const;
    std::string OrdPayoffRawTransaction() const;

    uint32_t TransactionCount(SwapPhase phase) const;
    std::string RawTransaction(SwapPhase phase, uint32_t n);

    std::string GetMinFundingAmount(const std::string& params) const override;
};

} // namespace l15::utxord
