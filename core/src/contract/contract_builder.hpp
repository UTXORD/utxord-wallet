#pragma once

#include <string>
#include <optional>
#include <vector>


#include "utils.hpp"
#include "contract_error.hpp"
#include "univalue.h"

namespace utxord {

using namespace l15;

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

    static const std::string name_market_fee;

protected:
    static const CAmount TX_BASE_VSIZE = 10;
    static const CAmount TAPROOT_VOUT_VSIZE = 43;
    static const CAmount TAPROOT_KEYSPEND_VIN_VSIZE = 58;
    static const CAmount MIN_TAPROOT_TX_VSIZE = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE;

    std::optional<CAmount> m_mining_fee_rate;
    std::optional<CAmount> m_market_fee;
    std::optional<xonly_pubkey> m_market_fee_pk;

    virtual CAmount CalculateWholeFee(const std::string& params) const;

    ///deprecated
    virtual std::vector<std::pair<CAmount,CMutableTransaction>> GetTransactions() const { return {}; };

public:
    ContractBuilder() = default;
    ContractBuilder(const ContractBuilder&) = default;
    ContractBuilder(ContractBuilder&& ) noexcept = default;

    ContractBuilder& operator=(const ContractBuilder& ) = default;
    ContractBuilder& operator=(ContractBuilder&& ) noexcept = default;

    void MarketFee(const std::string& amount, const std::string& pk)
    {
        CAmount market_fee = ParseAmount(amount);
        if (market_fee != 0 && market_fee < Dust(3000)) {
            throw ContractTermWrongValue(std::string(name_market_fee));
        }

        m_market_fee = market_fee;
        m_market_fee_pk = unhex<xonly_pubkey>(pk);
    }

    void MiningFeeRate(const std::string& rate)
    { m_mining_fee_rate = ParseAmount(rate); }

    virtual std::string GetMinFundingAmount(const std::string& params) const = 0;

    std::string GetNewInputMiningFee();
    std::string GetNewOutputMiningFee();

    virtual uint32_t GetProtocolVersion() const = 0;

    std::string GetMiningFeeRate() const { return FormatAmount(m_mining_fee_rate.value()); }

    static void VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut>&& spent_outputs, const CScript& spend_script);

    static void DeserializeContractAmount(const UniValue& val, std::optional<CAmount> &target, std::function<std::string()> lazy_name);
    static void DeserializeContractString(const UniValue& val, std::optional<std::string> &target, std::function<std::string()> lazy_name);
    static std::optional<Transfer> DeserializeContractTransfer(const UniValue& val, std::function<std::string()> lazy_name);

    template <typename HEX>
    static void DeserializeContractHexData(const UniValue& val, std::optional<HEX> &target, std::function<std::string()> lazy_name)
    {
        if (!val.isNull()) {
            HEX hexdata;
            try {
                hexdata = unhex<HEX>(val.get_str());
            } catch (...) {
                std::throw_with_nested(ContractTermWrongValue(lazy_name() + ": " + val.getValStr()));
            }

            if (target) {
                if (*target != hexdata) throw ContractTermMismatch(lazy_name() + " is already set to " + hex(*target));
            }
            else target = hexdata;
        }
    }

};

} // utxord

