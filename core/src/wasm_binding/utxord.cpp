#include <memory>

#include "random.h"

#include "common.hpp"
#include "address.hpp"
#include "keypair.hpp"
#include "master_key.hpp"
#include "contract_builder.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"
#include "simple_transaction.hpp"


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

class SimpleTransaction;

namespace wasm {

using l15::FormatAmount;
using l15::ParseAmount;
using l15::hex;
using l15::unhex;

enum Bech32Encoding
{
    BECH32,
    BECH32M
};

class Bech32 : public utxord::Bech32
{
public:
    explicit Bech32(ChainMode mode) : utxord::Bech32(mode) {}
    Bech32(const Bech32& another) = default;
    Bech32& operator=(const Bech32& another) = default;
    const char* Encode(std::string val, Bech32Encoding encoding)
    {
        static std::string cache;
        cache = utxord::Bech32::Encode(unhex<l15::bytevector>(val), encoding == BECH32M ? bech32::Encoding::BECH32M : bech32::Encoding::BECH32);
        return cache.c_str();
    }
};


class KeyPair : private utxord::KeyPair
{
public:
    static void InitSecp256k1()
    { GetSecp256k1(); }

    explicit KeyPair(const char* sk) : utxord::KeyPair(GetSecp256k1(), unhex<l15::seckey>(sk)) {}

    KeyPair(const KeyPair& ) = default;
    KeyPair(KeyPair&& ) noexcept = default;

    KeyPair(const utxord::KeyPair& keypair) : utxord::KeyPair(keypair) {}
    KeyPair(utxord::KeyPair&& keypair) noexcept : utxord::KeyPair(std::move(keypair)) {}

    const char* PrivKey() const
    {
        static std::string cache;
        cache = l15::hex(utxord::KeyPair::PrivKey());
        return cache.c_str();
    }

    const char* PubKey() const
    {
        static std::string cache;
        cache = l15::hex(utxord::KeyPair::PubKey());
        return cache.c_str();
    }

    const char* SignSchnorr(const char* m)
    {
        static std::string cache;
        cache = l15::hex(utxord::KeyPair::SignSchnorr(m));
        return cache.c_str();
    }

    const char* GetP2TRAddress(ChainMode mode)
    {
        static std::string cache;
        cache = utxord::KeyPair::GetP2TRAddress(utxord::Bech32(mode));
        return cache.c_str();
    }

    const char* GetP2WPKHAddress(ChainMode mode)
    {
        static std::string cache;
        cache = utxord::KeyPair::GetP2WPKHAddress(utxord::Bech32(mode));
        return cache.c_str();
    }
};


class KeyRegistry : private utxord::KeyRegistry
{
public:
    explicit KeyRegistry(ChainMode mode, const char *seed) : utxord::KeyRegistry(GetSecp256k1(), utxord::Bech32(mode), unhex<bytevector>(seed))
    {}

    void AddKeyType(const char* name, const char* filter_json)
    { utxord::KeyRegistry::AddKeyType(name, filter_json); }

    void RemoveKeyType(const char* name)
    { utxord::KeyRegistry::RemoveKeyType(name); }

    using utxord::KeyRegistry::AddKeyToCache;

    void RemoveKeyFromCache(const char* sk)
    { utxord::KeyRegistry::RemoveKeyFromCache(unhex<l15::seckey>(sk)); }

    void RemoveKeyFromCacheByAddress(const char* address)
    { utxord::KeyRegistry::RemoveKeyFromCache(address); }

    KeyPair* Derive(const char *path, bool for_script) const
    { return new KeyPair(utxord::KeyRegistry::Derive(path, for_script)); }

    KeyPair* LookupPubKey(const char* pk, const char* key_lookup_opt_json) const
    { return new KeyPair(utxord::KeyRegistry::Lookup(unhex<l15::xonly_pubkey>(pk), {true, utxord::KeyLookupFilter::DEFAULT, {0, 1}})); }

    KeyPair* LookupAddress(const std::string& addr, const char* key_lookup_opt_json) const
    { return new KeyPair(utxord::KeyRegistry::Lookup(addr, {true, utxord::KeyLookupFilter::DEFAULT, {0, 1}})); }

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

    virtual std::string Amount() const = 0;

    virtual std::string Address() const = 0;

    virtual std::shared_ptr<utxord::IContractDestination> &Share() = 0;
};

class ContractDestinationWrapper : public IContractDestination
{
    std::shared_ptr<utxord::IContractDestination> m_ptr;
public:
    ContractDestinationWrapper(std::shared_ptr<utxord::IContractDestination> ptr) : m_ptr(move(ptr))
    {}

    void SetAmount(const std::string& amount) override
    { m_ptr->Amount(ParseAmount(amount)); }

    std::string Amount() const override
    { return FormatAmount(m_ptr->Amount()); }

    std::string Address() const override
    { return m_ptr->Address(); }

    std::shared_ptr<utxord::IContractDestination> &Share() final
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


struct IContractOutput
{
    virtual ~IContractOutput() = default;

    virtual std::string TxID() const = 0;

    virtual uint32_t NOut() const = 0;

    virtual IContractDestination *Destination() = 0;

    virtual std::shared_ptr<utxord::IContractOutput> Share() = 0;
};

struct IContractMultiOutput
{
    virtual ~IContractMultiOutput() = default;

    virtual std::string TxID() const = 0;

    virtual uint32_t CountDestinations() const = 0;

    virtual IContractDestination *Destination(uint32_t n) = 0;

    virtual std::shared_ptr<utxord::IContractMultiOutput> Share() = 0;
};

class ContractOutputWrapper : public IContractOutput
{
    std::shared_ptr<utxord::IContractOutput> m_ptr;
public:
    explicit ContractOutputWrapper(std::shared_ptr<utxord::IContractOutput> ptr) : m_ptr(move(ptr))
    {}

    std::string TxID() const final
    { return m_ptr->TxID(); }

    uint32_t NOut() const final
    { return m_ptr->NOut(); }

    IContractDestination *Destination() final
    { return new ContractDestinationWrapper(m_ptr->Destination()); }

    std::shared_ptr<utxord::IContractOutput> Share() final
    { return m_ptr; }

};

class ContractMultiOutputWrapper : public IContractMultiOutput
{
    std::shared_ptr<utxord::IContractMultiOutput> m_ptr;
public:
    explicit ContractMultiOutputWrapper(std::shared_ptr<utxord::IContractMultiOutput> ptr) : m_ptr(move(ptr))
    {}

    std::string TxID() const final
    { return m_ptr->TxID(); }

    uint32_t CountDestinations() const final
    { return m_ptr->CountDestinations(); }

    IContractDestination *Destination(uint32_t n) final
    { return new ContractDestinationWrapper(m_ptr->Destinations()[n]); }

    std::shared_ptr<utxord::IContractMultiOutput> Share() final
    { return m_ptr; }

};

class UTXO : public ContractOutputWrapper
{
public:
    UTXO(ChainMode mode, std::string txid, uint32_t nout, const std::string &amount, const char *addr)
            : ContractOutputWrapper(std::make_shared<utxord::UTXO>(mode, move(txid), nout, ParseAmount(amount), std::string(addr)))
    {}
};

class SimpleTransaction : public IContractMultiOutput
{
    std::shared_ptr<utxord::SimpleTransaction> m_ptr;
public:
    SimpleTransaction(ChainMode mode) : m_ptr(std::make_shared<utxord::SimpleTransaction>(utxord::Bech32(mode)))
    {}

    std::string TxID() const final
    { return m_ptr->TxID(); }

    uint32_t CountDestinations() const final
    { return m_ptr->CountDestinations(); }

    IContractDestination *Destination(uint32_t n) final
    { return new ContractDestinationWrapper(m_ptr->Destinations()[n]); }

    void MiningFeeRate(const std::string &rate)
    { m_ptr->MiningFeeRate(rate); }

    const char* GetMinFundingAmount(const std::string& params) const
    {
        static std::string cache;
        cache = m_ptr->GetMinFundingAmount(params);
        return cache.c_str();
    }

    const char* GetNewInputMiningFee()
    {
        static std::string cache;
        cache = m_ptr->GetNewInputMiningFee();
        return cache.c_str();
    }

    const char* GetNewOutputMiningFee()
    {
        static std::string cache;
        cache = m_ptr->GetNewInputMiningFee();
        return cache.c_str();
    }

    void AddInput(IContractOutput *prevout)
    { m_ptr->AddInput(prevout->Share()); }

    void AddOutput(IContractDestination *out)
    { m_ptr->AddOutput(out->Share()); }

    void AddChangeOutput(const std::string &pk)
    { m_ptr->AddChangeOutput(pk); }

    void Sign(const KeyRegistry *master, const std::string key_filter_tag)
    { m_ptr->Sign(*reinterpret_cast<const utxord::KeyRegistry *>(master), key_filter_tag); }

    const char *Serialize()
    { return m_ptr->Serialize(); }

    void Deserialize(const std::string &data)
    { m_ptr->Deserialize(data); }

    std::shared_ptr<utxord::IContractMultiOutput> Share() final
    { return m_ptr; }
};

class CreateInscriptionBuilder : public utxord::CreateInscriptionBuilder
{
public:
    CreateInscriptionBuilder(ChainMode mode, InscribeType type) : utxord::CreateInscriptionBuilder(Bech32(mode), type) {}

    void SignCommit(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::CreateInscriptionBuilder::SignCommit(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignInscription(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::CreateInscriptionBuilder::SignInscription(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignCollection(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::CreateInscriptionBuilder::SignCollection(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }
};

class SwapInscriptionBuilder : public utxord::SwapInscriptionBuilder
{
public:
    SwapInscriptionBuilder(ChainMode mode) : utxord::SwapInscriptionBuilder(Bech32(mode)) {}

    void SignOrdSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::SwapInscriptionBuilder::SignOrdSwap(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignMarketSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::SwapInscriptionBuilder::SignMarketSwap(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignOrdCommitment(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::SwapInscriptionBuilder::SignOrdCommitment(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsCommitment(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::SwapInscriptionBuilder::SignFundsCommitment(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }

    void SignFundsSwap(const KeyRegistry* keyRegistry, const std::string& key_filter)
    { utxord::SwapInscriptionBuilder::SignFundsSwap(*reinterpret_cast<const utxord::KeyRegistry *>(keyRegistry), key_filter); }
};

} // wasm

} // utxord

using namespace l15;
using namespace utxord;
using namespace utxord::wasm;

#include "contract.cpp"
