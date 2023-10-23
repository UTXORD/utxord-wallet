#pragma once

#include <functional>
#include <list>

#include "common.hpp"
#include "channel_keys.hpp"
#include "ecdsa.hpp"
#include "address.hpp"
#include "master_key.hpp"

namespace utxord {

class KeyPair
{
    const secp256k1_context* m_ctx;
    l15::seckey m_sk;
public:

    KeyPair(const secp256k1_context* ctx, l15::seckey sk) : m_ctx(ctx), m_sk(move(sk)) {}
    explicit KeyPair(l15::seckey sk) : KeyPair(l15::core::ChannelKeys::GetStaticSecp256k1Context(), move(sk)) {}
    KeyPair() : m_ctx(l15::core::ChannelKeys::GetStaticSecp256k1Context()), m_sk(l15::core::ChannelKeys::GetStrongRandomKey(m_ctx)) {}

    KeyPair(const KeyPair&) = default;
    KeyPair(KeyPair&&) noexcept = default;

    KeyPair& operator=(const KeyPair& ) = default;
    KeyPair& operator=(KeyPair&&) noexcept = default;

    const l15::seckey& PrivKey() const
    { return m_sk; }

    l15::xonly_pubkey PubKey() const;

    std::string GetP2TRAddress(Bech32 bech) const;
    std::string GetP2WPKHAddress(Bech32 bech) const;

    l15::signature SignSchnorr(const char *m) const;
};

struct KeyLookupHint
{
    enum Type {DEFAULT, TAPROOT, TAPSCRIPT};

    bool look_cache;
    Type type;
    std::vector<uint32_t> accounts;
};

class KeyRegistry
{
    const secp256k1_context* m_ctx;
    Bech32 mBech;
    l15::core::MasterKey mMasterKey;
    std::list<l15::seckey> m_keys_cache;

public:
    KeyRegistry(Bech32 bech, const l15::bytevector& seed): m_ctx(l15::core::ChannelKeys::GetStaticSecp256k1Context()), mBech(bech), mMasterKey(m_ctx, seed) {}
    KeyRegistry(const secp256k1_context* ctx, Bech32 bech, const l15::bytevector& seed): m_ctx(ctx), mBech(bech), mMasterKey(m_ctx, seed) {}

    const secp256k1_context* Secp256k1Context() const
    { return m_ctx; }

    void AddKeyToCache(l15::seckey sk)
    { m_keys_cache.emplace_back(move(sk)); }

    void RemoveKeyFromCache(l15::seckey sk)
    { m_keys_cache.remove_if([&](const auto& el){ return el == sk; }); }

    KeyPair Derive(const char *path, bool for_script) const;

    KeyPair Lookup(const l15::bytevector& keyid, KeyLookupHint hint, std::function<bool(const l15::core::ChannelKeys&, const l15::bytevector&)>) const;
    KeyPair Lookup(const l15::xonly_pubkey& pk, KeyLookupHint hint) const;
    KeyPair Lookup(const std::string& addr, KeyLookupHint hint) const;
};

} // utxord

