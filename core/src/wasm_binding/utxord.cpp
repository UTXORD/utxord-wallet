#include <memory>

#include "random.h"

#include "common.hpp"
#include "channel_keys.hpp"
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

class ChannelKeys : private l15::core::ChannelKeys
{
public:
    static void InitSecp256k1()
    { GetSecp256k1(); }

    ChannelKeys() : l15::core::ChannelKeys(GetSecp256k1())
    {}

    explicit ChannelKeys(const char* sk) : l15::core::ChannelKeys(GetSecp256k1(), l15::unhex<l15::seckey>(sk))
    {}

    ChannelKeys(l15::core::ChannelKeys&& key) : l15::core::ChannelKeys(move(key))
    {}

    std::string GetLocalPrivKey() const
    { return l15::hex(l15::core::ChannelKeys::GetLocalPrivKey()); }

    std::string GetLocalPubKey() const
    { return l15::hex(l15::core::ChannelKeys::GetLocalPubKey()); }

    std::string SignSchnorr(const char* m) const
    { return l15::hex(l15::core::ChannelKeys::SignSchnorr(uint256S(m))); }
};

class MasterKey : private l15::core::MasterKey
{
public:
    explicit MasterKey(const char* seed) : l15::core::MasterKey(GetSecp256k1(), unhex<bytevector>(seed)) {}
    ChannelKeys* Derive(const char* path, bool for_script) const
    { return new ChannelKeys(l15::core::MasterKey::Derive(std::string(path), for_script)); }
};

enum NetworkMode {REGTEST, TESTNET, MAINNET};

class Bech32
{
    std::unique_ptr<IBech32Coder> mBech32;
public:
    explicit Bech32(NetworkMode mode)
    : mBech32(mode == REGTEST ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::REGTEST>())
                              : (mode == TESTNET ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::TESTNET>())
                                                 : (mode == MAINNET ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::MAINNET>())
                                                                    : std::unique_ptr<IBech32Coder>() )))
    {}

    std::string Encode(const char* pubkey)
    { return mBech32->Encode(unhex<xonly_pubkey>(pubkey)); }

    std::string Decode(const char* addr)
    { return hex(mBech32->Decode(addr)); }

};

struct Exception
{
    static std::string getMessage(void* exceptionPtr)
    {
        std::exception *e = reinterpret_cast<std::exception *>(exceptionPtr);
        if (l15::Error *l15err = dynamic_cast<l15::Error *>(e)) {
            return std::string(l15err->what()) + ": " + l15err->details();
        }
        return std::string(e->what());
    }
};

struct IContractDestination
{
    virtual ~IContractDestination() = default;
    virtual std::string Amount() const = 0;
    virtual std::string DestinationPK() const = 0;
    virtual std::shared_ptr<utxord::IContractDestination>& Share() = 0;
};

class ContractDestinationWrapper : public IContractDestination
{
    std::shared_ptr<utxord::IContractDestination> m_ptr;
public:
    ContractDestinationWrapper(std::shared_ptr<utxord::IContractDestination> ptr) : m_ptr(move(ptr))
    {}

    std::string Amount() const override
    { return FormatAmount(m_ptr->Amount()); }
    std::string DestinationPK() const override
    { return m_ptr->DestinationPK(); }

    std::shared_ptr<utxord::IContractDestination>& Share() final
    { return m_ptr; }
};

class P2TR : public ContractDestinationWrapper
{
public:
    P2TR(const std::string& amount, const std::string& pk) : ContractDestinationWrapper(std::make_shared<utxord::P2TR>(ParseAmount(amount), pk))
    {}
};

struct IContractOutput
{
    virtual ~IContractOutput() = default;
    virtual std::string TxID() const = 0;
    virtual uint32_t NOut() const = 0;
    virtual IContractDestination* Destination() = 0;
    virtual std::shared_ptr<utxord::IContractOutput> Share() = 0;
};

class ContractOutputWrapper : public IContractOutput
{
    std::shared_ptr<utxord::IContractOutput> m_ptr;
public:
    ContractOutputWrapper(std::shared_ptr<utxord::IContractOutput> ptr) : m_ptr(move(ptr))
    {}
    std::string TxID() const final
    { return m_ptr->TxID(); }

    uint32_t NOut() const final
    { return  m_ptr->NOut(); }

    IContractDestination* Destination() final
    { return new ContractDestinationWrapper(m_ptr->Destination()); }

    std::shared_ptr<utxord::IContractOutput> Share() final
    { return m_ptr; }

};

class UTXO : public ContractOutputWrapper
{
public:
    UTXO(std::string txid, uint32_t nout, const std::string& amount, const char* pk)
        : ContractOutputWrapper(std::make_shared<utxord::UTXO>(move(txid), nout, ParseAmount(amount), pk))
    {}
};

class SimpleTransaction : public IContractOutput
{
    std::shared_ptr<utxord::SimpleTransaction> m_ptr;
public:
    SimpleTransaction() : m_ptr(std::make_shared<utxord::SimpleTransaction>())
    {}

    std::string TxID() const final
    { return m_ptr->TxID(); }
    uint32_t NOut() const final
    { return  m_ptr->NOut(); }
    IContractDestination* Destination() final
    { return new ContractDestinationWrapper(m_ptr->Destination()); }

    void MiningFeeRate(const std::string& rate)
    { m_ptr->MiningFeeRate(rate); }
    std::string GetMinFundingAmount() const
    { return m_ptr->GetMinFundingAmount(""); }

    void AddInput(IContractOutput* prevout)
    { m_ptr->AddInput(prevout->Share()); }
    void AddOutput(IContractDestination* out)
    { m_ptr->AddOutput(out->Share()); }
    void AddChangeOutput(std::string pk)
    { m_ptr->AddChangeOutput(pk); }

    void Sign(const MasterKey* master)
    { m_ptr->Sign(*reinterpret_cast<const core::MasterKey*>(master)); }

    std::string Serialize()
    { return m_ptr->Serialize(); }
    void Deserialize(std::string data)
    { m_ptr->Deserialize(move(data)); }

    std::shared_ptr<utxord::IContractOutput> Share() final
    { return m_ptr; }
};

}

using namespace l15;
using namespace utxord;

#include "contract.cpp"
