#pragma once

#include <string>
#include <optional>
#include <list>

#include "script_merkle_tree.hpp"

#include "contract_builder.hpp"

namespace l15::utxord {

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

class SwapInscriptionBuilder : public ContractBuilder
{
    CAmount m_whole_fee = 0;
    CAmount m_last_fee_rate = 0;

    static const uint32_t m_protocol_version;

    CAmount m_ord_price;
    std::optional<CAmount> m_market_fee;

    std::optional<CAmount> m_ord_mining_fee_rate;

    std::optional<xonly_pubkey> m_swap_script_pk_A;
    std::optional<xonly_pubkey> m_swap_script_pk_B;
    std::optional<xonly_pubkey> m_swap_script_pk_M;

    std::optional<std::string> m_ord_txid;
    std::optional<uint32_t> m_ord_nout;
    std::optional<CAmount> m_ord_amount;
    std::optional<xonly_pubkey> m_ord_pk;

    std::list<Transfer> m_funds;

    std::optional<seckey> m_funds_unspendable_key_factor;

    std::optional<signature> m_ord_swap_sig_A;

    std::optional<signature> m_funds_swap_sig_B;
    std::optional<signature> m_funds_swap_sig_M;

    std::optional<signature> m_ordpayoff_sig;

    mutable std::optional<CMutableTransaction> mFundsCommitTpl;
    mutable std::optional<CMutableTransaction> mFundsPaybackTpl;

    mutable std::optional<CMutableTransaction> mSwapTpl;
    mutable std::optional<CMutableTransaction> mOrdPayoffTpl;

    mutable std::optional<CMutableTransaction> mFundsCommitTx;
    mutable std::optional<CMutableTransaction> mFundsPaybackTx;

    mutable std::optional<CMutableTransaction> mSwapTx;
    mutable std::optional<CMutableTransaction> mOrdPayoffTx;

    std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> FundsCommitTapRoot() const;

    CMutableTransaction MakeSwapTx(bool with_funds_in) const;

    void CheckOrdSwapSig() const;
    void CheckFundsSwapSig() const;

    void CheckOrdPayoffSig() const;

    std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> FundsCommitTemplateTapRoot() const;
protected:
    std::vector<std::pair<CAmount,CMutableTransaction>> GetTransactions() const override;

public:
    CMutableTransaction CreatePayoffTxTemplate() const;
    CMutableTransaction GetSwapTxTemplate() const;

    CMutableTransaction& GetFundsCommitTxTemplate() const;
    CMutableTransaction MakeFundsCommitTx() const;
    const CMutableTransaction& GetFundsCommitTx() const;

    const CMutableTransaction& GetSwapTx() const;
    const CMutableTransaction& GetPayoffTx() const;

    static const std::string name_ord_price;
    static const std::string name_market_fee;

    static const std::string name_ord_mining_fee_rate;

    static const std::string name_swap_script_pk_A;
    static const std::string name_swap_script_pk_B;
    static const std::string name_swap_script_pk_M;

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
    static const std::string name_ordpayoff_sig;

    explicit SwapInscriptionBuilder(): m_ord_price(0), m_market_fee(0) {}

    SwapInscriptionBuilder(const SwapInscriptionBuilder&) = default;
    SwapInscriptionBuilder(SwapInscriptionBuilder&&) noexcept = default;

    explicit SwapInscriptionBuilder(const std::string& ord_price, const std::string& market_fee);

    SwapInscriptionBuilder& operator=(const SwapInscriptionBuilder& ) = default;
    SwapInscriptionBuilder& operator=(SwapInscriptionBuilder&& ) noexcept = default;

    uint32_t GetProtocolVersion() const override { return m_protocol_version; }

    SwapInscriptionBuilder& MiningFeeRate(const std::string& fee_rate) { SetMiningFeeRate(fee_rate); return *this; }
    SwapInscriptionBuilder& OrdUTXO(const std::string& txid, uint32_t nout, const std::string& amount);
    SwapInscriptionBuilder& AddFundsUTXO(const std::string& txid, uint32_t nout, const std::string& amount, const std::string& pk);

    SwapInscriptionBuilder& SwapScriptPubKeyA(const std::string& v) { m_swap_script_pk_A = unhex<xonly_pubkey>(v); return *this; }
    SwapInscriptionBuilder& SwapScriptPubKeyB(const std::string& v) { m_swap_script_pk_B = unhex<xonly_pubkey>(v); return *this; }

    std::string GetSwapScriptPubKeyA() const { return hex(m_swap_script_pk_A.value()); }
    std::string GetSwapScriptPubKeyB() const { return hex(m_swap_script_pk_B.value()); }

    void SetOrdMiningFeeRate(const std::string& fee_rate) { m_ord_mining_fee_rate = ParseAmount(fee_rate); }

    std::string GetSwapScriptPubKeyM() const { return hex(m_swap_script_pk_M.value()); }
    void SetSwapScriptPubKeyM(const std::string& v) { m_swap_script_pk_M = unhex<xonly_pubkey>(v); }

    void SignOrdSwap(const std::string& sk);

    void SignFundsCommitment(uint32_t n, const std::string& sk);
    void SignFundsSwap(const std::string& sk);
    void SignFundsPayBack(const std::string& sk);

    void MarketSignOrdPayoffTx(const std::string& sk);
    void MarketSignSwap(const std::string& sk);

    void CheckContractTerms(SwapPhase phase) const;
    std::string Serialize(SwapPhase phase);
    void Deserialize(const std::string& data);

    std::string FundsCommitRawTransaction() const;
    std::string FundsPayBackRawTransaction();

    std::string OrdSwapRawTransaction() const;
    std::string OrdPayoffRawTransaction() const;

    std::string GetMinFundingAmount(const std::string& params) const override;
};

} // namespace l15::utxord
