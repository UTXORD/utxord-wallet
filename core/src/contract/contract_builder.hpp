#pragma once

#include <string>
#include <optional>
#include <vector>


#include "utils.hpp"
#include "contract_error.hpp"

namespace l15::utxord {

struct Transfer
{
    std::string m_txid;
    uint32_t m_nout;
    CAmount m_amount;
    std::optional<xonly_pubkey> m_pubkey;
    std::optional<signature> m_sig;
};

class ContractBuilder
{
public:
    static const std::string name_contract_type;
    static const std::string name_params;
    static const std::string name_version;
    static const std::string name_mining_fee_rate;

    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_amount;
    static const std::string name_pk;
    static const std::string name_sig;

protected:
    static const CAmount TX_BASE_VSIZE = 10;
    static const CAmount TAPROOT_VOUT_VSIZE = 43;
    static const CAmount TAPROOT_KEYSPEND_VIN_VSIZE = 58;
    static const CAmount MIN_TAPROOT_TX_VSIZE = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE;

    std::optional<CAmount> m_mining_fee_rate;

    virtual CAmount CalculateWholeFee(const std::string& params) const;

    ///deprecated
    virtual std::vector<std::pair<CAmount,CMutableTransaction>> GetTransactions() const { return {}; };

public:
    ContractBuilder() = default;
    ContractBuilder(const ContractBuilder&) = default;
    ContractBuilder(ContractBuilder&& ) noexcept = default;

    ContractBuilder& operator=(const ContractBuilder& ) = default;
    ContractBuilder& operator=(ContractBuilder&& ) noexcept = default;
    virtual std::string GetMinFundingAmount(const std::string& params) const = 0;

    std::string GetNewInputMiningFee();
    std::string GetNewOutputMiningFee();

    virtual uint32_t GetProtocolVersion() const = 0;

    std::string GetMiningFeeRate() const { return FormatAmount(m_mining_fee_rate.value()); }
    void SetMiningFeeRate(const std::string& v) { m_mining_fee_rate = ParseAmount(v); }

    static void VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut>&& spent_outputs, const CScript& spend_script);

};

} // utxord

