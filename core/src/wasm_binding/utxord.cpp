
#include "random.h"

#include "common.hpp"
#include "channel_keys.hpp"
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

    std::string GetLocalPrivKey() const
    { return l15::hex(l15::core::ChannelKeys::GetLocalPrivKey()); }

    std::string GetLocalPubKey() const
    { return l15::hex(l15::core::ChannelKeys::GetLocalPubKey()); }

    std::string SignSchnorr(const char* m) const
    { return l15::hex(l15::core::ChannelKeys::SignSchnorr(uint256S(m))); }
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
