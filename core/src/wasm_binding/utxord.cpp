#include <memory>

#include "nlohmann/json.hpp"

#include "random.h"
#include "core_io.h"

#include "mnemonic.hpp"
#include "bech32.hpp"
#include "schnorr.hpp"
#include "keypair.hpp"
#include "keyregistry.hpp"
#include "contract_builder.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"
#include "trustless_swap_inscription.hpp"
#include "simple_transaction.hpp"
#include "runes.hpp"


namespace {

secp256k1_context * CreateSecp256k1() {
    uint8_t vseed[32];
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    try {
        RandomInit();
        GetRandBytes(vseed);
        if (!secp256k1_context_randomize(ctx, vseed)) {
            throw std::runtime_error("Secp256k1 context");
        }
    }
    catch(...) {
        memory_cleanse(vseed, sizeof(vseed));
        std::rethrow_exception(std::current_exception());
    }
    return ctx;
}

const secp256k1_context * GetSecp256k1()
{
    static secp256k1_context *ctx = CreateSecp256k1();
    return ctx;
}

}

namespace utxord {
namespace wasm {

using l15::FormatAmount;
using l15::ParseAmount;
using l15::hex;
using l15::unhex;

class MnemonicParser : private l15::core::MnemonicParser<nlohmann::json>
{
    static nlohmann::json ConvertJsonList(std::string json_list)
    {
        auto word_list = nlohmann::json::parse(move(json_list));
        if (!word_list.is_array()) throw l15::core::MnemonicDictionaryError("dictionary JSON is not an array");
        if (word_list.size() != 2048) throw l15::core::MnemonicDictionaryError("wrong size: " + std::to_string(word_list.size()));

        return word_list;
    }

    static l15::sensitive_stringvector to_vec(const l15::sensitive_string& str)
    {
        l15::sensitive_string curword;
        l15::sensitive_stringvector vec;
        vec.reserve(24);

        std::basic_istringstream<char, std::char_traits<char>, secure_allocator<char>> buf(str);
        while (getline (buf, curword, ' ')) {
            vec.emplace_back(move(curword));
        }
        return vec;
    }
public:
    explicit MnemonicParser(std::string word_list_json) : l15::core::MnemonicParser<nlohmann::json>(ConvertJsonList(move(word_list_json))) {}

    const char* DecodeEntropy(const l15::sensitive_string& phrase) const
    {
        static std::string cache;

        cache = hex(l15::core::MnemonicParser<nlohmann::json>::DecodeEntropy(to_vec(phrase)));
        return cache.c_str();
    }

    const char* EncodeEntropy(const char* entropy_hex) const
    {
        static l15::sensitive_string cache;

        auto phrase_vec = l15::core::MnemonicParser<nlohmann::json>::EncodeEntropy(unhex<l15::sensitive_bytevector>(entropy_hex));

        std::ostringstream buf;
        bool insert_space = false;
        for (const auto& w: phrase_vec) {
            if (insert_space)
                buf << ' ';
            else
                insert_space = true;
            buf << w;
        }

        cache = buf.str();
        return cache.c_str();
    }

    const char* MakeSeed(const l15::sensitive_string& phrase, const l15::sensitive_string& passphrase) const
    {
        static l15::sensitive_string cache;

        cache = hex(l15::core::MnemonicParser<nlohmann::json>::MakeSeed(to_vec(phrase), passphrase));
        return cache.c_str();
    }
};


enum Bech32Encoding
{
    BECH32,
    BECH32M
};

class Bech32 : public l15::Bech32
{
public:
    explicit Bech32(ChainMode mode) : utxord::Bech32(l15::BTC, mode) {}
    Bech32(const Bech32& another) = default;
    Bech32& operator=(const Bech32& another) = default;
    const char* Encode(std::string val, Bech32Encoding encoding)
    {
        static std::string cache;
        cache = utxord::Bech32::Encode(unhex<l15::bytevector>(val), encoding == BECH32M ? bech32::Encoding::BECH32M : bech32::Encoding::BECH32);
        return cache.c_str();
    }
};

class Util {
public:
    static const char* LogTx(ChainMode chain, const std::string hex)
    {
        static std::string cache;
        CMutableTransaction tx;
        if (!DecodeHexTx(tx, hex)) {
            throw l15::IllegalArgument("Wrong Tx hex");
        }
        cache = l15::JsonTx<nlohmann::ordered_json>(chain, tx).dump();
        return cache.c_str();
    }
};

class KeyPair : private l15::core::KeyPair
{
public:
    static void InitSecp256k1()
    { GetSecp256k1(); }

    explicit KeyPair(const char* sk) : l15::core::KeyPair(GetSecp256k1(), unhex<l15::seckey>(sk)) {}

    KeyPair(const KeyPair& ) = default;
    KeyPair(KeyPair&& ) noexcept = default;

    KeyPair(const l15::core::KeyPair& keypair) : l15::core::KeyPair(keypair) {}
    KeyPair(l15::core::KeyPair&& keypair) noexcept : l15::core::KeyPair(std::move(keypair)) {}

    const char* PrivKey() const
    {
        static std::string cache;
        cache = l15::hex(l15::core::KeyPair::PrivKey());
        return cache.c_str();
    }

    const char* PubKey() const
    {
        static std::string cache;
        cache = l15::hex(GetSchnorrKeyPair().GetPubKey());
        return cache.c_str();
    }

    const char* SignSchnorr(const char* m)
    {
        static std::string cache;
        cache = l15::hex(GetSchnorrKeyPair().SignSchnorr(uint256S(m)));
        return cache.c_str();
    }

    const char* GetP2TRAddress(ChainMode mode)
    {
        static std::string cache;
        cache = l15::core::KeyPair::GetP2TRAddress(l15::Bech32(l15::BTC, mode));
        return cache.c_str();
    }

    const char* GetP2WPKHAddress(ChainMode mode)
    {
        static std::string cache;
        cache = l15::core::KeyPair::GetP2WPKHAddress(l15::Bech32(l15::BTC, mode));
        return cache.c_str();
    }
};


class KeyRegistry : private l15::core::KeyRegistry
{
public:
    explicit KeyRegistry(ChainMode mode, const char *seed) : l15::core::KeyRegistry(GetSecp256k1(), l15::Bech32(l15::BTC, mode), unhex<l15::sensitive_bytevector>(seed))
    {}

    void AddKeyType(const char* name, const char* filter_json)
    { l15::core::KeyRegistry::AddKeyType(name, filter_json); }

    void RemoveKeyType(const char* name)
    { l15::core::KeyRegistry::RemoveKeyType(name); }

    using l15::core::KeyRegistry::AddKeyToCache;

    void RemoveKeyFromCache(const char* sk)
    { l15::core::KeyRegistry::RemoveKeyFromCache(unhex<l15::seckey>(sk)); }

    void RemoveKeyFromCacheByAddress(const char* address)
    { l15::core::KeyRegistry::RemoveKeyFromCache(address); }

    KeyPair* Derive(const char *path, bool for_script) const
    { return new KeyPair(l15::core::KeyRegistry::Derive(path, for_script)); }

    KeyPair* LookupPubKey(const char* pk, const char* key_lookup_opt_json) const
    { return new KeyPair(l15::core::KeyRegistry::Lookup(unhex<l15::xonly_pubkey>(pk), std::string(key_lookup_opt_json))); }

    KeyPair* LookupAddress(const std::string& addr, const char* key_lookup_opt_json) const
    { return new KeyPair(l15::core::KeyRegistry::Lookup(addr, std::string(key_lookup_opt_json))); }

};

struct Exception
{
    static const char* getMessage(void *exceptionPtr)
    {
        static std::string cache;
        std::exception *e = reinterpret_cast<std::exception *>(exceptionPtr);
        if (l15::Error * l15err = dynamic_cast<l15::Error *>(e)) {
            cache =  std::string(l15err->what()) + ": " + l15err->details();
            return cache.c_str();
        }
        return e->what();
    }
};

struct IContractDestination
{
    virtual ~IContractDestination() = default;

    virtual void SetAmount(const std::string& amount) = 0;

    virtual const char* Amount() const = 0;

    virtual const char* Address() const = 0;

    virtual std::shared_ptr<utxord::IContractDestination> Share() const = 0;
};

class ContractDestinationWrapper : public IContractDestination
{
    std::shared_ptr<utxord::IContractDestination> m_ptr;
public:
    ContractDestinationWrapper(std::shared_ptr<utxord::IContractDestination> ptr) : m_ptr(move(ptr))
    {}

    ContractDestinationWrapper() = default;
    ContractDestinationWrapper(const ContractDestinationWrapper& ) = default;
    ContractDestinationWrapper(ContractDestinationWrapper&& ) = default;

    ContractDestinationWrapper& operator=(const ContractDestinationWrapper& ) = default;
    ContractDestinationWrapper& operator=(ContractDestinationWrapper&& ) = default;

    void SetAmount(const std::string& amount) final
    { m_ptr->Amount(ParseAmount(amount)); }

    const char* Amount() const final
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->Amount());
        return cache.c_str();
    }

    const char* Address() const final
    {
        static std::string cache;
        cache =  m_ptr->Address();
        return cache.c_str();
    }

    std::shared_ptr<utxord::IContractDestination> Share() const final
    { return m_ptr; }
};

class RuneStoneDestination : public IContractDestination
{
    std::shared_ptr<utxord::RuneStoneDestination> m_ptr;
public:
    RuneStoneDestination(std::shared_ptr<utxord::RuneStoneDestination> ptr) : m_ptr(move(ptr))
    {}

    RuneStoneDestination() = default;
    RuneStoneDestination(const RuneStoneDestination& ) = default;
    RuneStoneDestination(RuneStoneDestination&& ) = default;

    RuneStoneDestination& operator=(const RuneStoneDestination& ) = default;
    RuneStoneDestination& operator=(RuneStoneDestination&& ) = default;

    void SetAmount(const std::string& amount) final
    { m_ptr->Amount(ParseAmount(amount)); }

    const char* Amount() const final
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->Amount());
        return cache.c_str();
    }

    const char* Address() const final
    {
        static std::string cache;
        cache =  m_ptr->Address();
        return cache.c_str();
    }

    std::shared_ptr<utxord::RuneStoneDestination> ShareRuneStone() const
    { return m_ptr; }

    std::shared_ptr<utxord::IContractDestination> Share() const final
    { return m_ptr; }
};


class P2WPKH : public ContractDestinationWrapper
{
public:
    P2WPKH(ChainMode mode, const std::string &amount, const std::string &addr) : ContractDestinationWrapper(std::make_shared<utxord::P2WPKH>(mode, ParseAmount(amount), addr))
    {}
};

class P2TR : public ContractDestinationWrapper
{
public:
    P2TR(ChainMode mode, const std::string &amount, const std::string &addr) : ContractDestinationWrapper(std::make_shared<utxord::P2TR>(mode, ParseAmount(amount), addr))
    {}
};


class Rune: private utxord::Rune
{
public:
    Rune(const std::string &rune_text, const std::string &space, unsigned unicode_symbol = 0)
    : utxord::Rune(rune_text, space, unicode_symbol ? std::optional<wchar_t>(unicode_symbol) : std::optional<wchar_t>())
    {}

    void SetMintCap(const char* v)
    {
        uint128_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::MintCap().emplace(numval);
    }

    void SetAmountPerMint(const char* v)
    {
        uint128_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::AmountPerMint().emplace(numval);
    }

    void SetMintHeightStart(const char* v)
    {
        uint64_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::MintHeightStart().emplace(numval);
    }

    void SetMintHeightEnd(const char* v)
    {
        uint64_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::MintHeightEnd().emplace(numval);
    }

    void SetMintHeightOffsetStart(const char* v)
    {
        uint64_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::MintHeightOffsetStart().emplace(numval);
    }

    void SetMintHeightOffsetEnd(const char* v)
    {
        uint64_t numval;
        std::istringstream valstream(v);
        valstream >> numval;
        utxord::Rune::MintHeightOffsetEnd().emplace(numval);
    }

    const RuneStoneDestination* Etch(ChainMode mode) const
    {
        static RuneStoneDestination cache;

        std::shared_ptr<utxord::RuneStoneDestination> ptr =
                std::make_shared<utxord::RuneStoneDestination>(mode, utxord::Rune::Etch());
        cache = RuneStoneDestination(move(ptr));
        return &cache;
    }

    const RuneStoneDestination* EtchAndMint(ChainMode mode, const char* amount, uint32_t nout) const
    {
        static RuneStoneDestination cache;

        uint128_t numamount;
        std::istringstream amountstream(amount);
        amountstream >> numamount;

        std::shared_ptr<utxord::RuneStoneDestination> ptr =
                std::make_shared<utxord::RuneStoneDestination>(mode, utxord::Rune::EtchAndMint(numamount, nout));
        cache = RuneStoneDestination(move(ptr));
        return &cache;
    }

    const RuneStoneDestination* Mint(ChainMode mode, uint32_t nout) const
    {
        static RuneStoneDestination cache;

        std::shared_ptr<utxord::RuneStoneDestination> ptr =
                std::make_shared<utxord::RuneStoneDestination>(mode, utxord::Rune::Mint(nout));
        cache = RuneStoneDestination(move(ptr));
        return &cache;
    }

};


struct IContractOutput
{
    virtual ~IContractOutput() = default;

    virtual const char* TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual const char* Amount() const = 0;
    virtual const char* Address() const = 0;

    virtual const IContractDestination* Destination() const = 0;

    virtual const std::shared_ptr<utxord::IContractOutput> Share() const = 0;
};

//struct IContractMultiOutput
//{
//    virtual ~IContractMultiOutput() = default;
//
//    virtual const char* TxID() const = 0;
//
//    virtual uint32_t CountDestinations() const = 0;
//
//    virtual const IContractDestination* Destination(uint32_t n) const = 0;
//
//    virtual const std::shared_ptr<utxord::IContractMultiOutput> Share() const = 0;
//};

class ContractOutputWrapper : public IContractOutput
{
    std::shared_ptr<utxord::IContractOutput> m_ptr;
public:
    explicit ContractOutputWrapper(std::shared_ptr<utxord::IContractOutput> ptr) : m_ptr(move(ptr))
    {}

    const char* TxID() const final
    {
        static std::string cache;
        cache = m_ptr->TxID();
        return cache.c_str();
    }

    uint32_t NOut() const final
    { return m_ptr->NOut(); }

    virtual const char* Amount() const final
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->Amount());
        return cache.c_str();

    }

    virtual const char* Address() const final
    {
        static std::string cache;
        cache = m_ptr->Address();
        return cache.c_str();
    }

    const IContractDestination* Destination() const final
    { return new ContractDestinationWrapper(m_ptr->Destination()); }

    const std::shared_ptr<utxord::IContractOutput> Share() const final
    { return m_ptr; }

};

//class ContractMultiOutputWrapper : public IContractMultiOutput
//{
//    std::shared_ptr<utxord::IContractMultiOutput> m_ptr;
//public:
//    explicit ContractMultiOutputWrapper(std::shared_ptr<utxord::IContractMultiOutput> ptr) : m_ptr(move(ptr))
//    {}
//
//    const char* TxID() const final
//    {
//        static std::string cache;
//        cache = m_ptr->TxID();
//        return cache.c_str();
//    }
//
//    uint32_t CountDestinations() const final
//    { return m_ptr->CountDestinations(); }
//
//    const IContractDestination *Destination(uint32_t n) const final
//    { return new ContractDestinationWrapper(m_ptr->Destinations()[n]); }
//
//    const std::shared_ptr<utxord::IContractMultiOutput> Share() const final
//    { return m_ptr; }
//
//};

class UTXO : public ContractOutputWrapper
{
public:
    UTXO(ChainMode mode, std::string txid, uint32_t nout, const std::string &amount, const char *addr)
            : ContractOutputWrapper(std::make_shared<utxord::UTXO>(mode, move(txid), nout, ParseAmount(amount), std::string(addr)))
    {}
};

template <class BUILDER>
class ContractBuilder
{
protected:
    std::shared_ptr<BUILDER> m_ptr;

    ContractBuilder(std::shared_ptr<BUILDER> ptr) : m_ptr(move(ptr))
    {}

public:
    void MiningFeeRate(const std::string& rate)
    { m_ptr->MiningFeeRate(ParseAmount(rate)); }

    void MarketFee(const std::string& amount, std::string addr)
    { m_ptr->MarketFee(ParseAmount(amount), move(addr)); }

    void ChangeAddress(std::string addr)
    { m_ptr->ChangeAddress(move(addr)); }

    const char* GetTotalMiningFee(const std::string& params) const
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->GetTotalMiningFee(params));
        return cache.c_str();
    }

    const char* GetMinFundingAmount(const std::string& params) const
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->GetMinFundingAmount(params));
        return cache.c_str();
    }

    const char* GetNewInputMiningFee() const
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->GetNewInputMiningFee());
        return cache.c_str();
    }

    const char* GetNewOutputMiningFee() const
    {
        static std::string cache;
        cache = FormatAmount(m_ptr->GetNewOutputMiningFee());
        return cache.c_str();
    }

    std::shared_ptr<BUILDER> Share() const
    { return m_ptr; }
};

class SimpleTransaction : public ContractBuilder<utxord::SimpleTransaction>
{

public:
    SimpleTransaction(ChainMode mode) : ContractBuilder(std::make_shared<utxord::SimpleTransaction>(mode))
    {}

    const char* TxID() const
    {
        static std::string cache;
        cache = m_ptr->TxID();
        return cache.c_str();
    }

    uint32_t CountOutputs() const
    { return m_ptr->CountDestinations(); }

    const IContractOutput* Output(uint32_t n) const
    { return new ContractOutputWrapper(std::make_shared<utxord::ContractOutput>(m_ptr, n)); }

    void AddInput(const IContractOutput *prevout)
    { m_ptr->AddInput(prevout->Share()); }

    void AddUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->AddUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void AddRuneInput(const IContractOutput *prevout, const std::string& rune_id_json, const std::string rune_amount)
    {
        UniValue runeIdVal;
        if (!runeIdVal.read(rune_id_json)) throw std::invalid_argument("Wrong RuneId JSON");
        RuneId runeid;
        runeid.ReadJson(runeIdVal, []{ return "rune_id_json"; });

        uint128_t amount;
        std::istringstream buf;
        buf.str(rune_amount);
        buf >> amount;

        m_ptr->AddRuneInput(prevout->Share(), move(runeid), move(amount));
    }

    void AddRuneUTXO(std::string txid, uint32_t nout, const std::string& btc_amount, std::string addr, const std::string& rune_id_json, const std::string rune_amount)
    {
        UniValue runeIdVal;
        if (!runeIdVal.read(rune_id_json)) throw std::invalid_argument("Wrong RuneId JSON");
        RuneId runeid;
        runeid.ReadJson(runeIdVal, []{ return "rune_id_json"; });

        uint128_t amount;
        std::istringstream buf;
        buf.str(rune_amount);
        buf >> amount;

        m_ptr->AddRuneUTXO(move(txid), nout, ParseAmount(btc_amount), move(addr), move(runeid), move(amount));
    }

    void AddOutput(const std::string& amount, std::string addr)
    { m_ptr->AddOutput(ParseAmount(amount), move(addr)); }

    void AddOutputDestination(const IContractDestination *out)
    { m_ptr->AddOutputDestination(out->Share()); }

    void AddRuneOutput(const std::string& btc_amount, std::string addr, const std::string& rune_id_json, const std::string rune_amount)
    {
        UniValue runeIdVal;
        if (!runeIdVal.read(rune_id_json)) throw std::invalid_argument("Wrong RuneId JSON");
        RuneId runeid;
        runeid.ReadJson(runeIdVal, []{ return "rune_id_json"; });
        
        uint128_t amount;
        std::istringstream buf;
        buf.str(rune_amount);
        buf >> amount;
        
        m_ptr->AddRuneOutput(ParseAmount(btc_amount), move(addr), move(runeid), move(amount));
    }
    
    void AddRuneOutputDestination(const IContractDestination *out, const std::string& rune_id_json, const std::string& rune_amount)
    {
        UniValue runeIdVal;
        if (!runeIdVal.read(rune_id_json)) throw std::invalid_argument("Wrong RuneId JSON");
        RuneId runeid;
        runeid.ReadJson(runeIdVal, []{ return "rune_id_json"; });

        uint128_t amount;
        std::istringstream buf;
        buf.str(rune_amount);
        buf >> amount;
        
        m_ptr->AddRuneOutputDestination(out->Share(), move(runeid), move(amount));
    }

    void BurnRune(const std::string& rune_id_json, const std::string rune_amount)
    {
        UniValue runeIdVal;
        if (!runeIdVal.read(rune_id_json)) throw std::invalid_argument("Wrong RuneId JSON");
        RuneId runeid;
        runeid.ReadJson(runeIdVal, []{ return "rune_id_json"; });

        uint128_t amount;
        std::istringstream buf;
        buf.str(rune_amount);
        buf >> amount;

        m_ptr->BurnRune(move(runeid), move(amount));
    }

    void AddChangeOutput(const std::string &pk)
    { m_ptr->AddChangeOutput(pk); }

    void Sign(const KeyRegistry *master, const std::string key_filter_tag)
    { m_ptr->Sign(*reinterpret_cast<const l15::core::KeyRegistry *>(master), key_filter_tag); }

    void PartialSign(const KeyRegistry *master, const std::string key_filter_tag, uint32_t nin)
    { m_ptr->PartialSign(*reinterpret_cast<const l15::core::KeyRegistry *>(master), key_filter_tag, nin); }

    const char *Serialize(uint32_t version, TxPhase phase)
    {
        static std::string cache;
        cache = m_ptr->Serialize(version, phase);
        return cache.c_str();
    }

    void Deserialize(const std::string &data, TxPhase phase)
    { m_ptr->Deserialize(data, phase); }

    uint32_t TransactionCount(TxPhase phase) const
    { return 1; }

    const char* RawTransaction(TxPhase phase, uint32_t n) const
    {
        static std::string cache;
        if (n == 0) {
            cache = m_ptr->RawTransactions()[0];
            return cache.c_str();
        }
        else return {};
    }

    static const char* SupportedVersions()
    { return utxord::SimpleTransaction::SupportedVersions(); }

    const IContractOutput* ChangeOutput() const
    {
        auto out = m_ptr->ChangeOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }

    const IContractOutput* RuneStoneOutput() const
    {
        auto out = m_ptr->RuneStoneOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }
};

class CreateInscriptionBuilder : public ContractBuilder<utxord::CreateInscriptionBuilder>
{
public:
    CreateInscriptionBuilder(ChainMode mode, InscribeType type)
    : ContractBuilder(std::make_shared<utxord::CreateInscriptionBuilder>(mode, type))
    {}

    void OrdOutput(const std::string& amount, std::string addr)
    { m_ptr->OrdOutput(ParseAmount(amount), move(addr)); }

    void OrdOutputDestination(const IContractDestination *out)
    { m_ptr->OrdOutputDestination(out->Share()); }

    void AddUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->AddUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void AddInput(const IContractOutput* prevout)
    { m_ptr->AddInput(prevout->Share()); }

    void Data(std::string content_type, const std::string& hex_data)
    { m_ptr->Data(move(content_type), unhex<bytevector>(hex_data)); }

    void Delegate(std::string inscription_id)
    { m_ptr->Delegate(move(inscription_id)); }

    void MetaData(const std::string& metadata)
    { m_ptr->MetaData(unhex<bytevector>(metadata)); }

    void Rune(const RuneStoneDestination* runeStone)
    { m_ptr->Rune(runeStone->ShareRuneStone()); }

    void InscribeScriptPubKey(const std::string& pk)
    { m_ptr->InscribeScriptPubKey(unhex<xonly_pubkey>(pk)); }

    void MarketInscribeScriptPubKey(const std::string& pk)
    { m_ptr->MarketInscribeScriptPubKey(unhex<xonly_pubkey>(pk)); }

    void InscribeInternalPubKey(const std::string& pk)
    { m_ptr->InscribeInternalPubKey(unhex<xonly_pubkey>(pk)); }

    void FundMiningFeeInternalPubKey(const std::string& pk)
    { m_ptr->FundMiningFeeInternalPubKey(unhex<xonly_pubkey>(pk)); }

    void AddCollectionUTXO(std::string collection_id, std::string txid, uint32_t nout, const std::string& amount, std::string collection_addr)
    { m_ptr->AddCollectionUTXO(move(collection_id), move(txid), nout, ParseAmount(amount), move(collection_addr)); }

    void AddCollectionInput(std::string collection_id, const IContractOutput* prevout)
    { m_ptr->AddCollectionInput(move(collection_id), prevout->Share()); }

    void SignCommit(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignCommit(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignInscription(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignInscription(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignCollection(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignCollection(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    const char* Serialize(uint32_t version, InscribePhase phase) const
    {
        static std::string cache;
        cache = m_ptr->Serialize(version, phase);
        return cache.c_str();
    }

    void Deserialize(const std::string &data, InscribePhase phase)
    { m_ptr->Deserialize(data, phase); }

    uint32_t TransactionCount(InscribePhase phase) const
    { return m_ptr->TransactionCount(phase); }

    const char* RawTransaction(InscribePhase phase, uint32_t n) const
    {
        static std::string cache;
        cache = m_ptr->RawTransaction(phase, n);
        return cache.c_str();
    }

    static const char* SupportedVersions()
    { return utxord::CreateInscriptionBuilder::SupportedVersions(); }

    const char* MakeInscriptionId() const
    {
        static std::string cache;
        cache = m_ptr->MakeInscriptionId();
        return cache.c_str();
    }

    const IContractOutput* InscriptionOutput() const
    { return new ContractOutputWrapper(m_ptr->InscriptionOutput()) ; }

    const IContractOutput* CollectionOutput() const
    {
        auto out = m_ptr->CollectionOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }

    const IContractOutput* ChangeOutput() const
    {
        auto out = m_ptr->ChangeOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }

    const IContractOutput* FixedChangeOutput() const
    {
        auto out = m_ptr->FixedChangeOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }

};

class SwapInscriptionBuilder : public ContractBuilder<utxord::SwapInscriptionBuilder>
{
public:
    SwapInscriptionBuilder(ChainMode mode)
    : ContractBuilder(std::make_shared<utxord::SwapInscriptionBuilder>(mode))
    {}

    void OrdPrice(const std::string& price)
    { m_ptr->OrdPrice(ParseAmount(price)); }

    void OrdUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->OrdUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void AddFundsUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->AddFundsUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void OrdPayoffAddress(std::string addr)
    { m_ptr->OrdPayoffAddress(move(addr)); }

    void FundsPayoffAddress(std::string addr)
    { m_ptr->FundsPayoffAddress(move(addr)); }

    void SwapScriptPubKeyB(const std::string pk)
    { m_ptr->SwapScriptPubKeyB(unhex<xonly_pubkey>(pk)); }

    void SignOrdSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignOrdSwap(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsCommitment(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignFundsCommitment(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignFundsSwap(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsPayBack(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignFundsPayBack(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    const char* Serialize(uint32_t version, SwapPhase phase) const
    {
        static std::string cache;
        cache = m_ptr->Serialize(version, phase);
        return cache.c_str();
    }

    void Deserialize(const std::string &data, SwapPhase phase)
    { m_ptr->Deserialize(data, phase); }

    uint32_t TransactionCount(SwapPhase phase) const
    { return m_ptr->TransactionCount(phase); }

    const char* RawTransaction(SwapPhase phase, uint32_t n) const
    {
        static std::string cache;
        cache = m_ptr->RawTransaction(phase, n);
        return cache.c_str();
    }

    static const char* SupportedVersions()
    { return utxord::SwapInscriptionBuilder::SupportedVersions(); }

    const IContractOutput* InscriptionOutput() const
    { return new ContractOutputWrapper(m_ptr->InscriptionOutput()) ; }

    const IContractOutput* FundsOutput() const
    { return new ContractOutputWrapper(m_ptr->FundsOutput()); }

    const IContractOutput* ChangeOutput() const
    {
        auto out = m_ptr->ChangeOutput();
        return out ? new ContractOutputWrapper(out) : nullptr;
    }
};

class TrustlessSwapInscriptionBuilder : public ContractBuilder<utxord::TrustlessSwapInscriptionBuilder>
{
public:
    TrustlessSwapInscriptionBuilder(ChainMode mode)
    : ContractBuilder(std::make_shared<utxord::TrustlessSwapInscriptionBuilder>(mode))
    {}

    void OrdPrice(const std::string& price)
    { m_ptr->OrdPrice(ParseAmount(price)); }

    void CommitOrdinal(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->CommitOrdinal(move(txid), nout, ParseAmount(amount), move(addr)); }

    void FundCommitOrdinal(std::string txid, uint32_t nout, const std::string& amount, std::string addr, std::string change_addr)
    { m_ptr->FundCommitOrdinal(move(txid), nout, ParseAmount(amount), move(addr), move(change_addr)); }

    void CommitFunds(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->CommitFunds(move(txid), nout, ParseAmount(amount), move(addr)); }

    void Brick1SwapUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->Brick1SwapUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void Brick2SwapUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->Brick2SwapUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void AddMainSwapUTXO(std::string txid, uint32_t nout, const std::string& amount, std::string addr)
    { m_ptr->AddMainSwapUTXO(move(txid), nout, ParseAmount(amount), move(addr)); }

    void OrdPayoffAddress(std::string addr)
    { m_ptr->OrdPayoffAddress(move(addr)); }

    void FundsPayoffAddress(std::string addr)
    { m_ptr->FundsPayoffAddress(move(addr)); }

    void OrdScriptPubKey(const std::string pk)
    { m_ptr->OrdScriptPubKey(unhex<xonly_pubkey>(pk)); }

    void OrdIntPubKey(const std::string pk)
    { m_ptr->OrdIntPubKey(unhex<xonly_pubkey>(pk)); }

    void SignOrdSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignOrdSwap(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignOrdCommitment(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignOrdCommitment(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsCommitment(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignFundsCommitment(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { m_ptr->SignFundsSwap(*reinterpret_cast<const l15::core::KeyRegistry *>(keyRegistry), key_filter); }

    const char* Serialize(uint32_t version, TrustlessSwapPhase phase) const
    {
        static std::string cache;
        cache = m_ptr->Serialize(version, phase);
        return cache.c_str();
    }

    void Deserialize(const std::string &data, TrustlessSwapPhase phase)
    { m_ptr->Deserialize(data, phase); }

    uint32_t TransactionCount(TrustlessSwapPhase phase) const
    { return m_ptr->TransactionCount(phase); }

    const char* RawTransaction(TrustlessSwapPhase phase, uint32_t n) const
    {
        static std::string cache;
        cache = m_ptr->RawTransaction(phase, n);
        return cache.c_str();
    }

    static const char* SupportedVersions()
    { return utxord::CreateInscriptionBuilder::SupportedVersions(); }

    const char* GetMinSwapFundingAmount()
    {
        static std::string cache;
        cache = m_ptr->GetMinSwapFundingAmount();
        return cache.c_str();
    }

};

} // wasm

} // utxord

using namespace l15;
using namespace utxord;
using namespace utxord::wasm;

#include "contract.cpp"
