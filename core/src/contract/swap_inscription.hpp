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
    static const uint32_t s_protocol_version;
    static const char* s_versions;

    std::optional<CAmount> m_ord_price;

    std::optional<CAmount> m_ord_mining_fee_rate;

    std::optional<xonly_pubkey> m_swap_script_pk_B;
    std::optional<xonly_pubkey> m_swap_script_pk_M;

    std::optional<TxInput> m_ord_input;
    std::optional<std::string> m_funds_payoff_addr;

    std::vector<TxInput> m_fund_inputs;
    std::optional<std::string> m_ord_payoff_addr;

    std::optional<seckey> m_funds_unspendable_key_factor;

    std::optional<signature> m_funds_swap_sig_B;
    std::optional<signature> m_funds_swap_sig_M;

    std::optional<signature> m_ord_payoff_sig;

    //mutable std::optional<CMutableTransaction> mFundsCommitTpl;
    mutable std::optional<CMutableTransaction> mFundsPaybackTpl;

    mutable std::optional<CMutableTransaction> mSwapTpl;
    mutable std::optional<CMutableTransaction> mOrdPayoffTpl;

    mutable std::optional<CMutableTransaction> mFundsCommitTx;
    mutable std::optional<CMutableTransaction> mFundsPaybackTx;

    mutable std::optional<CMutableTransaction> mSwapTx;
    mutable std::optional<CMutableTransaction> mOrdPayoffTx;

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
    const CMutableTransaction& GetFundsCommitTx() const;

    const CMutableTransaction& GetSwapTx() const;
    const CMutableTransaction& GetPayoffTx() const;

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
    void ReadJson(const UniValue& json, SwapPhase phase) override;

    static const char* SupportedVersions() { return s_versions; }

    void OrdPrice(CAmount price)
    { m_ord_price = price; }

    void OrdUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
    void AddFundsUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);

    void OrdPayoffAddress(std::string addr)
    {
        bech32().Decode(addr);
        m_ord_payoff_addr = move(addr);
    }

    void FundsPayoffAddress(std::string addr)
    {
        bech32().Decode(addr);
        m_funds_payoff_addr = move(addr);
    }

    void SwapScriptPubKeyB(xonly_pubkey v) { m_swap_script_pk_B = move(v); }

    const xonly_pubkey& GetSwapScriptPubKeyB() const { return m_swap_script_pk_B.value(); }

    void SetOrdMiningFeeRate(CAmount fee_rate) { m_ord_mining_fee_rate = fee_rate; }

    const xonly_pubkey& GetSwapScriptPubKeyM() const { return m_swap_script_pk_M.value(); }
    void SetSwapScriptPubKeyM(xonly_pubkey v) { m_swap_script_pk_M = move(v); }

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
    std::string RawTransaction(SwapPhase phase, uint32_t n) const;

    CAmount GetMinFundingAmount(const std::string& params) const override;

    std::shared_ptr<IContractOutput> InscriptionOutput() const;
    std::shared_ptr<IContractOutput> FundsOutput() const;
    std::shared_ptr<IContractOutput> ChangeOutput() const;
};

} // namespace l15::utxord
