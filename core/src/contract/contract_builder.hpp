#pragma once

#include <string>
#include <optional>
#include <vector>
#include <stdexcept>

#include "univalue.h"

#include "utils.hpp"
#include "contract_error.hpp"
#include "master_key.hpp"


namespace utxord {

using namespace l15;

enum OutputType {
    TAPROOT_DEFAULT, // m/86'/0'/0'/0/* or m/86'/0'/0'/1/*
    TAPROOT_DEFAULT_SCRIPT, // m/86'/0'/0'/0/* w/o tweak
    TAPROOT_OUTPUT, // m/86'/1'/0'/0/*
    TAPROOT_SCRIPT, // m/86'/1'/0'/0/* w/o tweak
    INSCRIPTION_OUTPUT // m/86'/2'/0'/0/*
};

struct IJsonSerializable
{
    static const std::string name_type;

    virtual ~IJsonSerializable() = default;
    virtual UniValue MakeJson() const = 0;
    virtual void ReadJson(const UniValue& json) = 0;

    virtual std::string Serialize() const
    { return MakeJson().write(); }

    virtual void Deserialize(const std::string& jsonStr)
    {
        UniValue json;
        if (json.read(jsonStr)) {
            ReadJson(json);
        }
        else {
            throw ContractError("parse json");
        }
    }
};

class WitnessStack: public IJsonSerializable
{
    std::vector<bytevector> m_stack;
public:
    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;
    size_t size() const
    { return m_stack.size(); }
    const bytevector& operator[](size_t i) const
    { return m_stack[i]; }
    void Set(size_t i, bytevector data)
    {
        if (i >= m_stack.size())
            m_stack.resize(i+1);

        m_stack[i] = move(data);
    }
    operator const std::vector<bytevector>&() const
    { return m_stack; }
};

class IContractDestination: public IJsonSerializable
{
public:
    static const std::string name_witness;
private:
    WitnessStack m_witness;
public:
    virtual CAmount Amount() const = 0;
    virtual xonly_pubkey DestinationPK() const = 0;
    virtual CScript DestinationPubKeyScript() const = 0;
    virtual core::ChannelKeys LookupKeyPair(const core::MasterKey& masterKey, l15::utxord::OutputType outType) const = 0;
    virtual const WitnessStack& Witness() const
    { return m_witness; }
    virtual WitnessStack& Witness()
    { return m_witness; }
};

class TapRootKeyPath: public IContractDestination
{
public:
    static const std::string name_amount;
    static const std::string name_pk;

    static const char* type;

private:
    CAmount m_amount;
    xonly_pubkey m_pk;

public:
    TapRootKeyPath() = default;
    TapRootKeyPath(const TapRootKeyPath&) = default;
    TapRootKeyPath(TapRootKeyPath&&) noexcept = default;

    explicit TapRootKeyPath(CAmount amount, const xonly_pubkey& pk) : m_amount(amount), m_pk(pk) {}
    explicit TapRootKeyPath(CAmount amount, xonly_pubkey&& pk) : m_amount(amount), m_pk(move(pk)) {}

    ~TapRootKeyPath() override = default;

    CAmount Amount() const final
    { return m_amount; }

    xonly_pubkey DestinationPK() const override
    { return m_pk; }

    CScript DestinationPubKeyScript() const override
    { return CScript() << 1 << m_pk; }

    core::ChannelKeys LookupKeyPair(const core::MasterKey& masterKey, l15::utxord::OutputType outType) const override;

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;
};

/*--------------------------------------------------------------------------------------------------------------------*/

class IContractOutput: public IJsonSerializable{
public:
    virtual std::string TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual const IContractDestination& Destination() const = 0;
    virtual IContractDestination& Destination() = 0;
};

class UTXO: public IContractOutput
{
public:
    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_destination;

    static const char* type;
private:
    std::string m_txid;
    uint32_t m_nout = 0;
    std::shared_ptr<IContractDestination> m_destination;
public:
    UTXO() = default;
    UTXO(std::string txid, uint32_t nout, CAmount amount, const xonly_pubkey& pk)
        : m_txid(move(txid)), m_nout(nout), m_destination(std::make_shared<TapRootKeyPath>(amount, pk))
    { if (!m_destination) throw std::invalid_argument("null destination"); }

    std::string TxID() const final
    { return m_txid; }

    uint32_t NOut() const final
    { return  m_nout; }

    const IContractDestination& Destination() const final
    { return *m_destination; }
    IContractDestination& Destination() final
    { return *m_destination; }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;
};

/*--------------------------------------------------------------------------------------------------------------------*/

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

    static const char* name_utxo;
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

    virtual ~ContractBuilder() = default;

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

