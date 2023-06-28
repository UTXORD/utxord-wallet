
#include "random.h"

#include "common.hpp"
#include "channel_keys.hpp"
#include "master_key.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"


namespace {

secp256k1_context * CreateSecp256k1() {
    RandomInit();
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    std::vector<unsigned char, secure_allocator<unsigned char>> vseed(32);
    GetRandBytes(vseed);
    int ret = secp256k1_context_randomize(ctx, vseed.data());
    assert(ret);
    return ctx;
}

const secp256k1_context * GetSecp256k1()
{
    static secp256k1_context *ctx = CreateSecp256k1();
    return ctx;
}

}

namespace l15::utxord {

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
    explicit MasterKey(const char* seed) : l15::core::MasterKey(GetSecp256k1(), unhex<std::vector<std::byte>>(seed)) {}
    ChannelKeys* Derive(const char* path, bool for_script) const
    { return new ChannelKeys(l15::core::MasterKey::Derive(std::string(path), for_script)); }
};

enum NetworkMode {REGTEST, TESTNET, MAINNET};

class Bech32
{
    std::unique_ptr<IBech32Coder> mBech32;
public:
    explicit Bech32(NetworkMode mode) : mBech32(mode == REGTEST ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::REGTEST>())
                                                       : (mode == TESTNET ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::TESTNET>())
                                                                          : mode == MAINNET ? std::unique_ptr<IBech32Coder>(new Bech32Coder<IBech32Coder::BTC, IBech32Coder::MAINNET>()) : std::unique_ptr<IBech32Coder>() ))
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

}

using namespace l15;
using namespace l15::utxord;

#include "contract.cpp"
