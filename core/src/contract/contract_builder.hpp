#pragma once

#include <string>
#include <optional>
#include <vector>
#include <stdexcept>

#include "univalue.h"

#include "utils.hpp"
#include "contract_error.hpp"
#include "master_key.hpp"


namespace l15::utxord {

enum OutputType {
    TAPROOT_DEFAULT, // m/86'/0'/0'/0/* or m/86'/0'/0'/1/*
    TAPROOT_DEFAULT_SCRIPT, // m/86'/0'/0'/0/* w/o tweak
    TAPROOT_OUTPUT, // m/86'/0'/1'/0/*
    TAPROOT_SCRIPT, // m/86'/0'/1'/0/* w/o tweak
    INSCRIPTION_OUTPUT // m/86'/0'/1'/0/*
};

struct IJsonSerializable
{
    static const std::string name_type;

    mutable std::string buf;

    virtual ~IJsonSerializable() = default;
    virtual UniValue MakeJson() const = 0;
    virtual void ReadJson(const UniValue& json) = 0;

    const char* Serialize() const
    {
        buf = MakeJson().write();
        return buf.c_str();
    }

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


class IContractDestination: public IJsonSerializable
{
public:
    virtual CAmount Amount() const = 0;
    virtual void Amount(CAmount amount) = 0;
    virtual std::string DestinationPK() const = 0;
    virtual CScript DestinationPubKeyScript() const = 0;
    virtual core::ChannelKeys LookupKeyPair(const core::MasterKey& masterKey, l15::utxord::OutputType outType) const = 0;
};

class P2TR: public IContractDestination
{
public:
    static const std::string name_amount;
    static const std::string name_pk;

    static const char* type;

private:
    CAmount m_amount = 0;
    xonly_pubkey m_pk;

public:
    P2TR() = default;
    P2TR(const P2TR&) = default;
    P2TR(P2TR&&) noexcept = default;

    P2TR(CAmount amount, const std::string& pk) : m_amount(amount), m_pk(unhex<xonly_pubkey>(pk)) {}

    explicit P2TR(const UniValue& json)
    { P2TR::ReadJson(json); }

    ~P2TR() override = default;

    CAmount Amount() const final
    { return m_amount; }

    void Amount(CAmount amount) override
    { m_amount = amount; }

    std::string DestinationPK() const override
    { return hex(m_pk); }

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
    virtual const std::shared_ptr<IContractDestination>& Destination() const = 0;
    virtual std::shared_ptr<IContractDestination>& Destination() = 0;
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
    UTXO(std::string txid, uint32_t nout, CAmount amount, const std::string& pk)
        : m_txid(move(txid)), m_nout(nout), m_destination(std::make_shared<P2TR>(amount, pk))
    {}

    explicit UTXO(const IContractOutput& out)
        : m_txid(out.TxID()), m_nout(out.NOut()), m_destination(out.Destination()) {}

    explicit UTXO(const UniValue& json)
    { UTXO::ReadJson(json); }

    std::string TxID() const final
    { return m_txid; }

    uint32_t NOut() const final
    { return  m_nout; }

    const std::shared_ptr<IContractDestination>& Destination() const final
    { return m_destination; }
    std::shared_ptr<IContractDestination>& Destination() final
    { return m_destination; }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;
};


class WitnessStack
{
    std::vector<bytevector> m_stack;
public:
    UniValue MakeJson() const;
    void ReadJson(const UniValue& json);
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


struct ContractInput : public IJsonSerializable
{
    static const std::string name_witness;

    std::shared_ptr<IContractOutput> output;
    WitnessStack witness;

    explicit ContractInput(std::shared_ptr<IContractOutput> prevout) : output(move(prevout)) {}
    explicit ContractInput(const UniValue& json)
    { ContractInput::ReadJson(json); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& data) override;
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

    std::shared_ptr<IContractDestination> ReadContractDestination(const UniValue& ) const;

};

} // utxord

