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
using l15::compressed_pubkey;
using l15::signature;

using l15::ChainMode;
using l15::BTC;
using l15::MAINNET;
using l15::TESTNET;
using l15::REGTEST;
using l15::Bech32;

using l15::core::EcdsaKeyPair;
using l15::core::SchnorrKeyPair;
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

class ISigner;
struct TxInput;

struct IContractDestination: IJsonSerializable
{
    static const std::string name_amount;
    static const std::string name_addr;

    virtual const char* Type() const = 0;
    virtual CAmount Amount() const = 0;
    virtual void Amount(CAmount amount) = 0;
    virtual std::string Address() const = 0;
    virtual CScript PubKeyScript() const = 0;
    virtual CScript ScriptSig() const = 0;
    CTxOut TxOutput() const
    { return {Amount(), PubKeyScript()}; };
    virtual CScript DummyScriptSig() const = 0;
    virtual std::vector<bytevector> DummyWitness() const = 0;
    virtual std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const = 0;
    virtual void SetSignature(TxInput& input, bytevector pk, bytevector sig) = 0;
};

struct ZeroDestination: IContractDestination
{
    ZeroDestination() = default;
    explicit ZeroDestination(const UniValue &json, const std::function<std::string()>& lazy_name);
    const char* Type() const override { return ""; }
    CAmount Amount() const override { return 0; }
    void Amount(CAmount amount) override { if (amount) throw l15::IllegalArgument("Non zero amount cannot be assigned to zero destination"); }
    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override { throw std::domain_error("zero destination cannot have a PubKeyScript"); }
    CScript ScriptSig() const override { throw std::domain_error("zero destination cannot have a scriptSig"); }
    CScript DummyScriptSig() const override { throw std::domain_error("zero destination cannot have a scriptSig"); }
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("zero destination cannot have a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("zero destination cannot provide a signer"); }
    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;
    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override {throw std::domain_error("zero destination cannot have a signature"); }
};

class P2Address : public IContractDestination
{
public:
    static const char* type;
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

    const char* Type() const override
    { return type; };

    CAmount Amount() const final
    { return m_amount; }

    void Amount(CAmount amount) override
    { throw std::logic_error("P2Address::Amount(amount) abstract call"); }

    std::string Address() const override
    { return m_addr; }

    CScript PubKeyScript() const override { throw std::logic_error("P2Address::PubKeyScript() abstract call"); }
    CScript ScriptSig() const override { throw std::logic_error("P2Address::ScriptSig() abstract call"); }
    CScript DummyScriptSig() const override { throw std::logic_error("P2Address::DummyScriptSig() abstract call"); }
    std::vector<bytevector> DummyWitness() const override { throw std::logic_error("P2Address::DummyWitness() abstract call"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::logic_error("P2Address::LookupKey(...) abstract call"); }
    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override
    { throw std::logic_error("P2Address::SetSignature(...) abstract call"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);
    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, std::optional<CAmount> amount, std::string addr);
};

class P2Witness: public P2Address
{
public:
    static const char* type;

private:
    friend IContractDestination;

protected:
    void CheckDust() const;
public:
    P2Witness() = delete;//default;
    P2Witness(const P2Witness&) = default;
    P2Witness(P2Witness&&) noexcept = default;

    P2Witness(ChainMode chain, CAmount amount, std::string addr): P2Address(chain, amount, move(addr))
    { CheckDust(); }

    ~P2Witness() override = default;

    Bech32 Bech() const { return Bech32(BTC, m_chain); }

    const char* Type() const override
    { return type; }

    void Amount(CAmount amount) override
    {
        m_amount = amount;
        if (amount != 0) CheckDust();
    }

    CScript PubKeyScript() const override
    { return Bech32(BTC, m_chain).PubKeyScript(m_addr); }

    CScript ScriptSig() const override
    { return {};}

    CScript DummyScriptSig() const override
    { return {};}

    std::vector<bytevector> DummyWitness() const override
    { throw std::logic_error("generic winness structure is unknown"); } // Should never be called directly
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
    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override;
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
    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override;
};


class P2Legacy: public P2Address
{
protected:
    std::optional<compressed_pubkey> m_pubkey;
    void CheckDust() const;
public:
    P2Legacy() = delete;
    P2Legacy(const P2Legacy&) = default;
    P2Legacy(P2Legacy&&) noexcept = default;
    P2Legacy(ChainMode chain, CAmount amount, std::string addr, std::optional<compressed_pubkey> pk = {})
        : P2Address(chain, amount, move(addr))
        , m_pubkey(move(pk))
    { }

    void Amount(CAmount amount) override
    {
        m_amount = amount;
        if (amount != 0) CheckDust();
    }
    std::vector<bytevector> DummyWitness() const override
    { return {}; }

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, std::optional<CAmount> amount, std::string addr, compressed_pubkey pubkey);
};

class P2PKH: public P2Legacy
{
    bytevector DecodePubKeyHash() const;
    void CheckPubKey() const;
public:
    P2PKH() = delete;
    P2PKH(const P2PKH&) = default;
    P2PKH(P2PKH&&) noexcept = default;
    P2PKH(ChainMode chain, CAmount amount, std::string addr, std::optional<compressed_pubkey> pk = {})
        : P2Legacy(chain, amount, move(addr), move(pk))
    {
        CheckDust();
        CheckPubKey();
    }

    CScript PubKeyScript() const override;

    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override;

    CScript ScriptSig() const override
    { return CScript() << signature() << m_pubkey.value_or(compressed_pubkey()).as_vector(); }

    CScript DummyScriptSig() const override
    { return CScript() << signature() << compressed_pubkey().as_vector();}

    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override;
};

class P2SH: public P2Legacy
{
    bytevector DecodeScriptHash() const;
    void CheckPubKey() const;
public:
    P2SH() = delete;
    P2SH(const P2SH&) = default;
    P2SH(P2SH&&) noexcept = default;
    P2SH(ChainMode chain, CAmount amount, std::string addr, std::optional<compressed_pubkey> pk = {})
        : P2Legacy(chain, amount, move(addr), move(pk))
    {
        CheckDust();
        if (m_pubkey) CheckPubKey();
    }

    CScript PubKeyScript() const override
    { return CScript() << OP_HASH160 << DecodeScriptHash() << OP_EQUAL; }

    CScript ScriptSig() const override;

    std::vector<bytevector> DummyWitness() const override
    { return { bytevector(72), bytevector(33) }; }

    CScript DummyScriptSig() const override
    { return CScript() << bytevector(22); }

    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override;

    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override;
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
    CScript ScriptSig() const override { return {}; }
    CScript DummyScriptSig() const override { return {}; }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("OP_RETURN destination cannot provide a signer"); }
    void SetSignature(TxInput &input, bytevector pk, bytevector sig) override
    { throw std::domain_error("OP_RETURN destination cannot have a signature"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);

};

/*--------------------------------------------------------------------------------------------------------------------*/

class IContractMultiOutput {
public:
    virtual ~IContractMultiOutput() = default;
    virtual std::string TxID() const = 0;
    virtual const std::vector<std::shared_ptr<IContractDestination>>& Destinations() const = 0;
    virtual uint32_t CountDestinations() const = 0;
};
/*--------------------------------------------------------------------------------------------------------------------*/

class IContractOutput {
public:
    virtual ~IContractOutput() = default;
    virtual std::string TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual CAmount Amount() const { return Destination()->Amount(); }
    virtual std::string Address() const { return Destination()->Address(); }
    virtual const std::shared_ptr<IContractDestination> & Destination() const = 0;
    virtual std::shared_ptr<IContractDestination> Destination() = 0;
};

/*--------------------------------------------------------------------------------------------------------------------*/

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
    static const std::string name_scriptsig;

    ChainMode chain;
    uint32_t nin;
    std::shared_ptr<IContractOutput> output;
    CScript scriptSig;
    WitnessStack witness;

    explicit TxInput(ChainMode ch, uint32_t n, std::shared_ptr<IContractOutput> prevout) : chain(ch), nin(n), output(move(prevout)) {}
    explicit TxInput(ChainMode ch, uint32_t n, const UniValue& json, const std::function<std::string()>& lazy_name) : chain(ch), nin(n)
    { TxInput::ReadJson(json, lazy_name); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& data, const std::function<std::string()> &lazy_name) override;

    bool operator<(const TxInput& r) const { return nin < r.nin; }
};

/*--------------------------------------------------------------------------------------------------------------------*/

class ISigner
{
public:
    virtual void SignInput(TxInput& input, const CMutableTransaction &tx, std::vector<CTxOut> spent_outputs, int hashtype) const = 0;
};

class P2PKHSigner : public ISigner
{
    EcdsaKeyPair m_keypair;
public:
    explicit P2PKHSigner(EcdsaKeyPair keypair) : m_keypair(move(keypair)) {}
    P2PKHSigner(const P2PKHSigner&) = default;
    P2PKHSigner(P2PKHSigner&&) noexcept = default;
    P2PKHSigner& operator=(const P2PKHSigner&) = default;
    P2PKHSigner& operator=(P2PKHSigner&&) noexcept = default;

    void SignInput(TxInput &input, const CMutableTransaction &tx, std::vector<CTxOut> spent_outputs,
                   int hashtype) const override;
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

    void SignInput(TxInput &input, const CMutableTransaction &tx, std::vector<CTxOut> spent_outputs,
                   int hashtype) const override;
};

class TaprootSigner: public ISigner
{
    SchnorrKeyPair m_keypair;
public:
    explicit TaprootSigner(SchnorrKeyPair keypair) : m_keypair(move(keypair)) {}
    TaprootSigner(const TaprootSigner&) = default;
    TaprootSigner(TaprootSigner&&) noexcept = default;
    TaprootSigner& operator=(const TaprootSigner&) = default;
    TaprootSigner& operator=(TaprootSigner&&) noexcept = default;

    void SignInput(TxInput &input, const CMutableTransaction &tx, std::vector<CTxOut> spent_outputs,
                   int hashtype) const override;
};

class P2WPKH_P2SHSigner: public ISigner
{
    EcdsaKeyPair m_keypair;
public:
    explicit P2WPKH_P2SHSigner(EcdsaKeyPair keypair) : m_keypair(move(keypair)) {}
    P2WPKH_P2SHSigner(const P2WPKH_P2SHSigner&) = default;
    P2WPKH_P2SHSigner(P2WPKH_P2SHSigner&&) noexcept = default;
    P2WPKH_P2SHSigner& operator=(const P2WPKH_P2SHSigner&) = default;
    P2WPKH_P2SHSigner& operator=(P2WPKH_P2SHSigner&&) noexcept = default;

    void SignInput(TxInput &input, const CMutableTransaction &tx, std::vector<CTxOut> spent_outputs,
                   int hashtype) const override;
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

/*--------------------------------------------------------------------------------------------------------------------*/

class IContractBuilder
{
public:
    static const std::string name_contract_type;
    static const std::string name_contract_phase;
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

    virtual void ChangeAddress(std::string addr)
    { m_change_addr = move(addr); }

    CAmount GetTotalMiningFee(const std::string& params) const
    { return CalculateWholeFee(params); }

    virtual CAmount GetMinFundingAmount(const std::string& params) const = 0;

    CAmount GetNewInputMiningFee();
    CAmount GetNewOutputMiningFee();

    CAmount GetMiningFeeRate() const { return m_mining_fee_rate.value(); }

    static void VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& spend_script);
    static void VerifyTxSignature(ChainMode chain, const std::string& addr, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs);

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

    std::string Serialize(uint32_t version, PHASE phase) const
    {
        CheckContractTerms(version, phase);

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

        CheckContractTerms(GetVersion(), phase);
    }

    virtual const std::string& GetContractName() const = 0;
    virtual uint32_t GetVersion() const = 0;
    virtual void CheckContractTerms(uint32_t version, PHASE phase) const = 0;
    virtual UniValue MakeJson(uint32_t version, PHASE phase) const = 0;
    virtual void ReadJson(const UniValue& json, PHASE phase) = 0;

};

} // utxord

