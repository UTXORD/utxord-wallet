#pragma once

#include <string>
#include <optional>
#include <vector>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <list>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>

#include "univalue.h"
#include "base58.hpp"

#include "utils.hpp"
#include "contract_error.hpp"
#include "univalue.h"
#include "keyregistry.hpp"

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

using l15::ChainMode;
using l15::BTC;
using l15::MAINNET;
using l15::TESTNET;
using l15::REGTEST;
using l15::Bech32;

using l15::core::EcdsaKeyPair;
using l15::core::KeyPair;
using l15::core::KeyRegistry;

#ifndef DEBUG
using boost::multiprecision::uint128_t;
#else
namespace bmp = boost::multiprecision;
using uint128_t = bmp::number<bmp::debug_adaptor<bmp::cpp_int_backend<128, 128, bmp::unsigned_magnitude, bmp::unchecked, void> >>;
#endif

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

    virtual ~IJsonSerializable() = default;
    virtual UniValue MakeJson() const = 0;
    virtual void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) = 0;
};

class ISigner
{
public:
    virtual std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const = 0;
};

class P2WPKHSigner: public ISigner
{
    EcdsaKeyPair m_keypair;
public:
    explicit P2WPKHSigner(EcdsaKeyPair keypair) : m_keypair(move(keypair)) {}
    P2WPKHSigner(const P2WPKHSigner&) = default;
    P2WPKHSigner(P2WPKHSigner&&) noexcept = default;
    P2WPKHSigner& operator=(const P2WPKHSigner&) = default;
    P2WPKHSigner& operator=(P2WPKHSigner&&) noexcept = default;

    std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const override;
};

class TaprootSigner: public ISigner
{
    l15::core::SchnorrKeyPair m_keypair;
public:
    explicit TaprootSigner(l15::core::SchnorrKeyPair keypair) : m_keypair(move(keypair)) {}
    TaprootSigner(const TaprootSigner&) = default;
    TaprootSigner(TaprootSigner&&) noexcept = default;
    TaprootSigner& operator=(const TaprootSigner&) = default;
    TaprootSigner& operator=(TaprootSigner&&) noexcept = default;

    std::vector<bytevector> Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const override;
};

struct IContractDestination: IJsonSerializable
{
    static const std::string name_amount;
    static const std::string name_addr;

    virtual const char* Type() const = 0;
    virtual CAmount Amount() const = 0;
    virtual void Amount(CAmount amount) = 0;
    virtual std::string Address() const = 0;
    virtual CScript PubKeyScript() const = 0;
    virtual std::vector<bytevector> DummyWitness() const = 0;
    virtual std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const = 0;
};



struct ZeroDestination: IContractDestination
{
    ZeroDestination() = default;
    explicit ZeroDestination(const UniValue &json, const std::function<std::string()>& lazy_name);
    const char* Type() const override { return ""; }
    CAmount Amount() const override { return 0; }
    void Amount(CAmount amount) override { if (amount) throw l15::IllegalArgument("Non zero amount cannot be assigned to zero destination"); }
    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override { throw std::domain_error("zero destination cannot has a witness"); }
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("zero destination cannot has a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("zero destination cannot provide a signer"); }
    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;
};


class P2Address : public IContractDestination
{
protected:
    ChainMode m_chain;
    CAmount m_amount = 0;
    std::string m_addr;

private:
    P2Address(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);

public:
    P2Address() = delete;
    P2Address(const P2Address&) = default;
    P2Address(P2Address&&) noexcept = default;
    P2Address(ChainMode m, CAmount amount, std::string addr) : m_chain(m), m_amount(amount), m_addr(move(addr)) {}

    const char* Type() const override { throw std::logic_error("P2Address abstract call"); };

    CAmount Amount() const final
    { return m_amount; }

    void Amount(CAmount amount) override
    { m_amount = amount; }

    std::string Address() const override
    { return m_addr; }

    CScript PubKeyScript() const override { throw std::logic_error("P2Address abstract call"); }
    std::vector<bytevector> DummyWitness() const override { throw std::logic_error("P2Address abstract call"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::logic_error("P2Address abstract call"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);
    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, CAmount amount, std::string addr);
};

class P2Witness: public P2Address
{
public:
    static const char* type;

private:
    friend IContractDestination;
    //explicit P2Witness(ChainMode chain) : P2Address(chain) {}
public:
    P2Witness() = delete;//default;
    P2Witness(const P2Witness&) = default;
    P2Witness(P2Witness&&) noexcept = default;

    P2Witness(ChainMode chain, CAmount amount, std::string addr): P2Address(chain, amount, move(addr)) {}

    ~P2Witness() override = default;

    Bech32 Bech() const { return Bech32(BTC, m_chain); }

    const char* Type() const override
    { return type; }

    CScript PubKeyScript() const override
    { return Bech32(BTC, m_chain).PubKeyScript(m_addr); }

    std::vector<bytevector> DummyWitness() const override
    { throw std::logic_error("generic winness structure is unknown"); } // Should never be called directly

    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw ContractTermMissing("key"); }

};

class P2WPKH: public P2Witness
{
public:
    P2WPKH() = delete;
    P2WPKH(const P2WPKH&) = default;
    P2WPKH(P2WPKH&&) noexcept = default;
    P2WPKH(ChainMode m, CAmount amount, std::string addr) : P2WPKH(Bech32(BTC, m), amount, move(addr)) {}
    P2WPKH(Bech32 bech, CAmount amount, std::string addr) : P2Witness(bech.GetChainMode(), amount, move(addr)) {}
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
    P2TR(ChainMode m, CAmount amount, std::string addr) : P2TR(Bech32(BTC, m), amount, move(addr)) {}
    P2TR(Bech32 bech, CAmount amount, std::string addr) : P2Witness(bech.GetChainMode(), amount, move(addr)) {}
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override;
    std::vector<bytevector> DummyWitness() const override { return { signature() }; }
};

class P2PKH: public P2Address
{
public:
    static const char* type;

public:
    P2PKH() = delete;
    P2PKH(const P2PKH&) = default;
    P2PKH(P2PKH&&) noexcept = default;
    P2PKH(ChainMode chain, CAmount amount, std::string addr) : P2Address(chain, amount, move(addr)) {}

    const char* Type() const override { return type; }

    CScript PubKeyScript() const override
    {
        auto [addrtype, keyhash] = l15::Base58(m_chain).Decode(m_addr);
        if (addrtype != l15::PUB_KEY_HASH) throw ContractTermWrongValue("Not P2PKH: " + m_addr);
        return CScript() << OP_DUP << OP_HASH160 << keyhash << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    std::vector<bytevector> DummyWitness() const override
    { throw std::logic_error("P2PKH has no segregated witness"); }

    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::logic_error("P2PKH signing is not implemented yet"); }
};

class P2SH: public P2Address
{

};

/*--------------------------------------------------------------------------------------------------------------------*/

class OpReturnDestination final: public IContractDestination
{
    CAmount m_amount = 0;
    bytevector m_data {};
public:
    static const char* type;

    static const char* name_data;

    OpReturnDestination() = default;
    OpReturnDestination(const OpReturnDestination&) = default;
    OpReturnDestination(OpReturnDestination&&) noexcept = default;

    explicit OpReturnDestination(bytevector data, CAmount amount=0) : m_amount(amount), m_data(move(data)) {}

    explicit OpReturnDestination(const UniValue& json, const std::function<std::string()>& lazy_name);

    const char* Type() const override { return type; }
    void Amount(CAmount amount) override { m_amount = amount; }
    CAmount Amount() const final { return m_amount; }

    void Data(bytevector data);
    const bytevector& Data() const { return m_data; }

    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override;
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("OP_RETURN destination cannot have a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("OP_RETURN destination cannot provide a signer"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);

};

/*--------------------------------------------------------------------------------------------------------------------*/

class IContractMultiOutput {
public:
    virtual std::string TxID() const = 0;
    virtual const std::vector<std::shared_ptr<IContractDestination>>& Destinations() const = 0;
    virtual uint32_t CountDestinations() const = 0;
};
/*--------------------------------------------------------------------------------------------------------------------*/

class IContractOutput {
public:
    virtual std::string TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual CAmount Amount() const { return Destination()->Amount(); }
    virtual std::string Address() const { return Destination()->Address(); }
    virtual const std::shared_ptr<IContractDestination> & Destination() const = 0;
    virtual std::shared_ptr<IContractDestination> Destination() = 0;
};
/*--------------------------------------------------------------------------------------------------------------------*/

class ContractOutput : public IContractOutput {
    std::shared_ptr<IContractMultiOutput> m_contract;
    uint32_t m_nout;
public:
    ContractOutput(std::shared_ptr<IContractMultiOutput> contract, uint32_t nout) : m_contract(move(contract)), m_nout(nout) {}
    ContractOutput(const ContractOutput&) = default;
    ContractOutput(ContractOutput&&) noexcept = default;

    std::string TxID() const override { return m_contract->TxID(); }
    uint32_t NOut() const override { return m_nout; }
    const std::shared_ptr<IContractDestination> & Destination() const override { return m_contract->Destinations()[m_nout]; }
    std::shared_ptr<IContractDestination> Destination() override { return m_contract->Destinations()[m_nout]; }
};
/*--------------------------------------------------------------------------------------------------------------------*/

class UTXO: public IContractOutput, public IJsonSerializable
{
public:
    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_destination;

    static const char* type;
private:
    ChainMode m_chain;
    std::string m_txid;
    uint32_t m_nout = 0;
    std::shared_ptr<IContractDestination> m_destination;
public:
    //UTXO() = default;
    UTXO(ChainMode chain, std::string txid, uint32_t nout, CAmount amount, std::string addr)
        : m_chain(chain), m_txid(move(txid)), m_nout(nout), m_destination(P2Address::Construct(chain, amount, move(addr))) {}

    UTXO(ChainMode chain, std::string txid, uint32_t nout, std::shared_ptr<IContractDestination> destination)
        : m_chain(chain), m_txid(move(txid)), m_nout(nout), m_destination(move(destination)) {}

    UTXO(ChainMode chain, const IContractOutput& out)
        : m_chain(chain), m_txid(out.TxID()), m_nout(out.NOut()), m_destination(out.Destination()) {}

    UTXO(ChainMode chain, const IContractMultiOutput& out, uint32_t nout)
        : m_chain(chain), m_txid(out.TxID()), m_nout(nout), m_destination(out.Destinations()[nout]) {}

    explicit UTXO(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name) : m_chain(chain)
    { UTXO::ReadJson(json, lazy_name); }

    std::string TxID() const final
    { return m_txid; }

    uint32_t NOut() const final
    { return  m_nout; }

    const std::shared_ptr<IContractDestination> & Destination() const final
    { return m_destination; }
    std::shared_ptr<IContractDestination> Destination() final
    { return m_destination; }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()>& lazy_name) override;
};

class WitnessStack: public IJsonSerializable
{
    std::vector<bytevector> m_stack;
public:
    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;
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

    ChainMode chain;
    uint32_t nin;
    std::shared_ptr<IContractOutput> output;
    WitnessStack witness;

    explicit TxInput(Bech32 bech, uint32_t n, std::shared_ptr<IContractOutput> prevout) : chain(bech.GetChainMode()), nin(n), output(move(prevout)) {}
    explicit TxInput(ChainMode ch, uint32_t n, const UniValue& json, const std::function<std::string()>& lazy_name) : chain(ch), nin(n)
    { TxInput::ReadJson(json, lazy_name); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& data, const std::function<std::string()> &lazy_name) override;

    bool operator<(const TxInput& r) const { return nin < r.nin; }
};

/*--------------------------------------------------------------------------------------------------------------------*/

class IContractBuilder
{
public:
    static const std::string name_contract_type;
    static const std::string name_params;
    static const std::string name_version;
    static const std::string name_mining_fee_rate;
    static const std::string name_market_fee;
    static const std::string name_custom_fee;

    static const std::string name_utxo;
    static const std::string name_txid;
    static const std::string name_nout;
    static const std::string name_pk;
    static const std::string name_sig;

    static const std::string name_change_addr;
protected:
    static const CAmount TX_BASE_VSIZE = 10;
    static const CAmount TAPROOT_VOUT_VSIZE = 43;
    static const CAmount TAPROOT_KEYSPEND_VIN_VSIZE = 58;
    static const CAmount TAPROOT_MULTISIG_VIN_VSIZE = 100;
    static const CAmount P2WPKH_VIN_VSIZE = 69;
    static const CAmount MIN_TAPROOT_TX_VSIZE = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE;

    static const std::string FEE_OPT_HAS_CHANGE;
    static const std::string FEE_OPT_HAS_COLLECTION;
    static const std::string FEE_OPT_HAS_XTRA_UTXO;
    static const std::string FEE_OPT_HAS_P2WPKH_INPUT;

    ChainMode m_chain;

    std::optional<CAmount> m_mining_fee_rate;
    std::shared_ptr<IContractDestination> m_market_fee;
    std::list<std::shared_ptr<IContractDestination>> m_custom_fees;
    std::optional<std::string> m_change_addr;

    virtual CAmount CalculateWholeFee(const std::string &params) const;

    ///deprecated
    virtual std::vector<std::pair<CAmount,CMutableTransaction>> GetTransactions() const { return {}; };

public:
    static CScript MakeMultiSigScript(const xonly_pubkey& pk1, const xonly_pubkey& pk2);

    //ContractBuilder() = default;
    IContractBuilder(const IContractBuilder&) = default;
    IContractBuilder(IContractBuilder&& ) noexcept = default;

    explicit IContractBuilder(ChainMode chain) : m_chain(chain) {}

    virtual ~IContractBuilder() = default;

    IContractBuilder &operator=(const IContractBuilder& ) = default;

    IContractBuilder &operator=(IContractBuilder&& ) noexcept = default;

    ChainMode chain() const
    { return m_chain; }

    Bech32 bech32() const
    { return Bech32(BTC, m_chain); }

    void MarketFee(CAmount amount, std::string addr)
    {
        if (amount > 0) {
            m_market_fee = P2Address::Construct(chain(), amount, move(addr));
        }
        else {
            m_market_fee = std::make_shared<ZeroDestination>();
        }
    }

    void AddCustomFee(CAmount amount, std::string addr)
    { m_custom_fees.emplace_back(P2Address::Construct(chain(), amount, move(addr))); }

    void MiningFeeRate(CAmount rate)
    { m_mining_fee_rate = rate; }

    void ChangeAddress(std::string addr)
    {
        bech32().Decode(addr);
        m_change_addr = move(addr);
    }

    CAmount GetTotalMiningFee(const std::string& params) const
    { return CalculateWholeFee(params); }

    virtual CAmount GetMinFundingAmount(const std::string& params) const = 0;

    CAmount GetNewInputMiningFee();
    CAmount GetNewOutputMiningFee();

    CAmount GetMiningFeeRate() const { return m_mining_fee_rate.value(); }

    static void VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& spend_script);
    void VerifyTxSignature(const std::string& addr, const std::vector<bytevector>& witness, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs) const;

    static void DeserializeContractAmount(const UniValue& val, std::optional<CAmount> &target, const std::function<std::string()> &lazy_name);
    static void DeserializeContractString(const UniValue& val, std::optional<std::string> &target, const std::function<std::string()> &lazy_name);
    static void DeserializeContractScriptPubkey(const UniValue &val, std::optional<xonly_pubkey> &pk, const std::function<std::string()> &lazy_name);

    template<typename HEX>
    static void DeserializeContractHexData(const UniValue &val, std::optional<HEX> &target, const std::function<std::string()>& lazy_name)
    {
        if (!val.isNull()) {
            HEX hexdata;
            try {
                hexdata = unhex<HEX>(val.get_str());
            }
            catch (...) {
                std::throw_with_nested(ContractTermWrongValue(lazy_name() + ": " + val.getValStr()));
            }

            if (target) {
                if (*target != hexdata) throw ContractTermMismatch(lazy_name() + " is already set to " + hex(*target));
            }
            else target = hexdata;
        }
    }

};

template <typename PHASE>
class ContractBuilder : public IContractBuilder {
public:
    ContractBuilder(const ContractBuilder &) = default;
    ContractBuilder(ContractBuilder && ) noexcept = default;

    explicit ContractBuilder(ChainMode chain) : IContractBuilder(chain) {}

    ContractBuilder& operator=(const ContractBuilder&) = default;
    ContractBuilder& operator=(ContractBuilder&&) = default;

    std::string Serialize(uint32_t version, PHASE phase)
    {
        CheckContractTerms(phase);

        UniValue dataRoot(UniValue::VOBJ);
        dataRoot.pushKV(name_contract_type, GetContractName());
        dataRoot.pushKV(name_params, MakeJson(version, phase));

        return dataRoot.write();
    }

    void Deserialize(const std::string& data, PHASE phase)
    {
        UniValue root;
        root.read(data);

        if (!root.isObject() || !root[name_contract_type].isStr() || !root[name_params].isObject())
            throw ContractProtocolError("JSON is not " + GetContractName() + " contract");

        if (root[name_contract_type].get_str() != GetContractName())
            throw ContractProtocolError(GetContractName() + " contract does not match " + root[name_contract_type].getValStr());

        ReadJson(root[name_params], phase);

        CheckContractTerms(phase);
    }

    virtual const std::string& GetContractName() const = 0;
    virtual void CheckContractTerms(PHASE phase) const = 0;
    virtual UniValue MakeJson(uint32_t version, PHASE phase) const = 0;
    virtual void ReadJson(const UniValue& json, PHASE phase) = 0;

};

} // utxord

