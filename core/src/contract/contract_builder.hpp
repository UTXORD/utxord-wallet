#pragma once

#include <string>
#include <optional>
#include <vector>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <list>

#include "safe_ptr.hpp"

#include "univalue.h"

#include "utils.hpp"
#include "contract_error.hpp"
#include "univalue.h"
#include "keypair.hpp"

#include "address.hpp"
#include "ecdsa.hpp"
#include "common.hpp"

namespace utxord {

using std::move;
using std::get;

using l15::hex;
using l15::unhex;

using l15::bytevector;
using l15::seckey;
using l15::xonly_pubkey;
using l15::signature;

enum OutputType {
    P2WPKH_DEFAULT, // m/84'/0'/0'/0/*
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

class ISigner
{
public:
    virtual std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const = 0;
};

class P2WPKHSigner: public ISigner
{
    EcdsaKeypair m_keypair;
public:
    explicit P2WPKHSigner(EcdsaKeypair keypair) : m_keypair(move(keypair)) {}
    P2WPKHSigner(const P2WPKHSigner&) = default;
    P2WPKHSigner(P2WPKHSigner&&) noexcept = default;
    P2WPKHSigner& operator=(const P2WPKHSigner&) = default;
    P2WPKHSigner& operator=(P2WPKHSigner&&) noexcept = default;

    std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const override;
};

class TaprootSigner: public ISigner
{
    l15::core::ChannelKeys m_keypair;
public:
    explicit TaprootSigner(l15::core::ChannelKeys keypair) : m_keypair(move(keypair)) {}
    TaprootSigner(const TaprootSigner&) = default;
    TaprootSigner(TaprootSigner&&) noexcept = default;
    TaprootSigner& operator=(const TaprootSigner&) = default;
    TaprootSigner& operator=(TaprootSigner&&) noexcept = default;

    std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const override;
};

class IContractDestination: public IJsonSerializable
{
public:
    virtual CAmount Amount() const = 0;
    virtual void Amount(CAmount amount) = 0;
    virtual std::string Address() const = 0;
    virtual CScript PubKeyScript() const = 0;
    virtual std::vector<bytevector> DummyWitness() const = 0;
    virtual std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const = 0;

    static std::shared_ptr<IContractDestination> ReadJson(Bech32 bech, const UniValue& json, bool allow_zero_destination = false);
};

class ZeroDestination: public IContractDestination
{
public:
    ZeroDestination() = default;
    explicit ZeroDestination(const UniValue &json);
    CAmount Amount() const override { return 0; }
    void Amount(CAmount amount) override { if (amount) throw std::invalid_argument("Non zero amount cannot be assigned to zero destination"); }
    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override { throw std::domain_error("zero destination cannot has a witness"); }
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("zero destination cannot has a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("zero destination cannot provide a signer"); }
    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;
};

class P2Witness: public IContractDestination
{
public:
    static const char* type;

protected:
    Bech32 mBech;
    CAmount m_amount = 0;
    std::string m_addr;
private:
    friend IContractDestination;
    explicit P2Witness(Bech32 bech) : mBech(bech) {}
public:
    P2Witness() = delete;//default;
    P2Witness(const P2Witness&) = default;
    P2Witness(P2Witness&&) noexcept = default;

    P2Witness(Bech32 bech, CAmount amount, std::string addr) : mBech(bech), m_amount(amount), m_addr(move(addr)) {}

    explicit P2Witness(Bech32 bech, const UniValue& json);

    ~P2Witness() override = default;

    CAmount Amount() const final
    { return m_amount; }

    void Amount(CAmount amount) override
    { m_amount = amount; }

    std::string Address() const override
    { return m_addr; }

    CScript PubKeyScript() const override
    { return mBech.PubKeyScript(m_addr); }

    std::vector<bytevector> DummyWitness() const override
    { throw std::logic_error("generic winness structure is unknown"); } // Should never be called directly

    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw ContractTermMissing("key"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;

    static std::shared_ptr<IContractDestination> Construct(Bech32 bech, CAmount amount, std::string addr);
};

class P2WPKH: public P2Witness
{
public:
    P2WPKH() = delete;
    P2WPKH(const P2WPKH&) = default;
    P2WPKH(P2WPKH&&) noexcept = default;
    P2WPKH(ChainMode m, CAmount amount, std::string addr) : P2Witness(Bech32(m), amount, move(addr)) {}
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override;
    std::vector<bytevector> DummyWitness() const override
    { return { bytevector(72), bytevector(33) }; }
};

class P2TR: public P2Witness
{
public:
    P2TR() = delete;
    P2TR(const P2TR &) = default;
    P2TR(P2TR &&) noexcept = default;
    P2TR(ChainMode m, CAmount amount, std::string addr) : P2Witness(Bech32(m), amount, move(addr)) {}
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override;
    std::vector<bytevector> DummyWitness() const override { return { signature() }; }
};
/*--------------------------------------------------------------------------------------------------------------------*/

class IContractMultiOutput {
public:
    virtual std::string TxID() const = 0;
    virtual std::vector<std::shared_ptr<IContractDestination>> Destinations() const = 0;
};

class IContractOutput {
public:
    virtual std::string TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual const std::shared_ptr<IContractDestination> Destination() const = 0;
    virtual std::shared_ptr<IContractDestination> Destination() = 0;
};

class ContractOutput : public IContractOutput {
    std::shared_ptr<IContractMultiOutput> m_contract;
    uint32_t m_nout;
public:
    ContractOutput(std::shared_ptr<IContractMultiOutput> contract, uint32_t nout) : m_contract(move(contract)), m_nout(nout) {}
    ContractOutput(const ContractOutput&) = default;
    ContractOutput(ContractOutput&&) noexcept = default;

    std::string TxID() const override { return m_contract->TxID(); }
    uint32_t NOut() const override { return m_nout; }
    const std::shared_ptr<IContractDestination> Destination() const override { return m_contract->Destinations()[m_nout]; }
    std::shared_ptr<IContractDestination> Destination() override { return m_contract->Destinations()[m_nout]; }
};

class UTXO: public IContractOutput, public IJsonSerializable
{
public:
    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_destination;

    static const char* type;
private:
    Bech32 mBech;
    std::string m_txid;
    uint32_t m_nout = 0;
    std::shared_ptr<IContractDestination> m_destination;
public:
    //UTXO() = default;
    UTXO(Bech32 bech, std::string txid, uint32_t nout, CAmount amount, std::string addr)
        : mBech(bech), m_txid(move(txid)), m_nout(nout), m_destination(P2Witness::Construct(bech, amount, move(addr))) {}

    UTXO(Bech32 bech, std::string txid, uint32_t nout, std::shared_ptr<IContractDestination> destination)
        : mBech(bech), m_txid(txid), m_nout(nout), m_destination(move(destination)) {}

    UTXO(Bech32 bech, const IContractOutput& out)
        : mBech(bech), m_txid(out.TxID()), m_nout(out.NOut()), m_destination(out.Destination()) {}

    UTXO(Bech32 bech, const IContractMultiOutput& out, uint32_t nout)
        : mBech(bech), m_txid(out.TxID()), m_nout(nout), m_destination(out.Destinations()[nout]) {}

    UTXO(ChainMode m, std::string txid, uint32_t nout, CAmount amount, std::string addr)
        : UTXO(Bech32(m), move(txid), nout, amount, move(addr)) {}

    UTXO(ChainMode m, const IContractOutput& out)
        : UTXO (Bech32(m), out) {}

    explicit UTXO(Bech32 bech, const UniValue& json) : mBech(bech)
    { UTXO::ReadJson(json); }

    std::string TxID() const final
    { return m_txid; }

    uint32_t NOut() const final
    { return  m_nout; }

    const std::shared_ptr<IContractDestination> Destination() const final
    { return m_destination; }
    std::shared_ptr<IContractDestination> Destination() final
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
    operator bool() const
    { return !m_stack.empty(); }
};


struct TxInput : public IJsonSerializable
{
    static const std::string name_witness;

    Bech32 bech32;
    uint32_t nin;
    std::shared_ptr<IContractOutput> output;
    WitnessStack witness;

    explicit TxInput(Bech32 bech, uint32_t n, std::shared_ptr<IContractOutput> prevout) : bech32(bech), nin(n), output(move(prevout)) {}
    explicit TxInput(Bech32 bech, uint32_t n, const UniValue& json) : bech32(bech), nin(n)
    { TxInput::ReadJson(json); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& data) override;

    bool operator<(const TxInput& r) const { return nin < r.nin; }
};

/*--------------------------------------------------------------------------------------------------------------------*/

class ContractBuilder
{
public:
    static const std::string name_contract_type;
    static const std::string name_params;
    static const std::string name_version;
    static const std::string name_mining_fee_rate;
    static const std::string name_market_fee;
    static const std::string name_market_fee_addr;

    static const char* name_utxo;
    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_amount;
    static const std::string name_pk;
    static const std::string name_addr;
    static const std::string name_sig;

    static const std::string name_change_addr;
protected:
    static const CAmount TX_BASE_VSIZE = 10;
    static const CAmount TAPROOT_VOUT_VSIZE = 43;
    static const CAmount TAPROOT_KEYSPEND_VIN_VSIZE = 58;
    static const CAmount P2WPKH_VIN_VSIZE = 69;
    static const CAmount MIN_TAPROOT_TX_VSIZE = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE;

    static const std::string FEE_OPT_HAS_CHANGE;
    static const std::string FEE_OPT_HAS_COLLECTION;
    static const std::string FEE_OPT_HAS_XTRA_UTXO;
    static const std::string FEE_OPT_HAS_P2WPKH_INPUT;

    Bech32 mBech;

    std::optional<CAmount> m_mining_fee_rate;
    std::shared_ptr<IContractDestination> m_market_fee;
    std::optional<std::string> m_change_addr;

public:
    //ContractBuilder() = default;
    ContractBuilder(const ContractBuilder&) = default;
    ContractBuilder(ContractBuilder&& ) noexcept = default;

    explicit ContractBuilder(Bech32 bech) : mBech(bech) {}

    virtual ~ContractBuilder() = default;

    ContractBuilder& operator=(const ContractBuilder& ) = default;
    ContractBuilder& operator=(ContractBuilder&& ) noexcept = default;

    const Bech32& bech32() const
    { return mBech; }

    void MarketFee(const std::string& amount, const std::string& addr)
    {
        if (l15::ParseAmount(amount) > 0) {
            m_market_fee = P2Witness::Construct(bech32(), l15::ParseAmount(amount), addr);
        }
        else {
            m_market_fee = std::make_shared<ZeroDestination>();
        }
    }

    void MiningFeeRate(const std::string& rate)
    { m_mining_fee_rate = l15::ParseAmount(rate); }

    void ChangeAddress(const std::string& addr)
    {
        bech32().Decode(addr);
        m_change_addr = addr;
    }

    virtual CAmount CalculateWholeFee(const std::string& params) const = 0;
    virtual std::string GetMinFundingAmount(const std::string& params) const = 0;

    std::string GetNewInputMiningFee();
    std::string GetNewOutputMiningFee();

    std::string GetMiningFeeRate() const { return l15::FormatAmount(m_mining_fee_rate.value()); }
    void SetMiningFeeRate(const std::string& v) { m_mining_fee_rate = l15::ParseAmount(v); }

    static void VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& spend_script);
    void VerifyTxSignature(const std::string& addr, const std::vector<bytevector>& witness, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs) const;

    static void DeserializeContractAmount(const UniValue& val, std::optional<CAmount> &target, std::function<std::string()> lazy_name);
    static void DeserializeContractString(const UniValue& val, std::optional<std::string> &target, std::function<std::string()> lazy_name);
    void DeserializeContractTaprootPubkey(const UniValue& val, std::optional<std::string> &addr, std::function<std::string()> lazy_name);
    void DeserializeContractScriptPubkey(const UniValue& val, std::optional<xonly_pubkey> &pk, std::function<std::string()> lazy_name);

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

