#pragma once

#include <functional>
#include <list>
#include <ranges>
#include <unordered_map>

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

struct KeyLookupFilter
{
    enum Type {DEFAULT, TAPROOT, TAPSCRIPT};

    bool look_cache;
    Type type;
    std::vector<uint32_t> accounts;
    std::vector<uint32_t> change;
    std::ranges::iota_view<uint32_t, uint32_t> index_range;
};

class KeyRegistry
{
    const secp256k1_context* m_ctx;
    Bech32 mBech;

    std::unordered_map<std::string, KeyLookupFilter> m_key_type_filters;

    l15::core::MasterKey mMasterKey;
    std::list<l15::seckey> m_keys_cache;

public:
    KeyRegistry(const secp256k1_context* ctx, Bech32 bech, const l15::bytevector& seed): m_ctx(ctx), mBech(bech), m_key_type_filters(10), mMasterKey(m_ctx, seed) {}
    KeyRegistry(ChainMode bech, const std::string& seedhex): KeyRegistry(l15::core::ChannelKeys::GetStaticSecp256k1Context(), Bech32(bech), l15::unhex<l15::bytevector>(seedhex)) {}

    const secp256k1_context* Secp256k1Context() const
    { return m_ctx; }

    void AddKeyType(std::string name, KeyLookupFilter filter)
    { m_key_type_filters.emplace(move(name), std::move(filter)); }
    void AddKeyType(std::string name, const std::string& filter_json);
    void RemoveKeyType(const std::string& name)
    { m_key_type_filters.erase(name); }

    void AddKeyToCache(l15::seckey sk)
    { m_keys_cache.emplace_back(move(sk)); }

    void AddKeyToCache(const KeyPair& key)
    { m_keys_cache.emplace_back(key.PrivKey()); }

    void AddKeyToCache(const std::string& key)
    { m_keys_cache.emplace_back(l15::unhex<l15::seckey>(key)); }

    void RemoveKeyFromCache(const std::string& addr);

    void RemoveKeyFromCache(l15::seckey sk)
    { m_keys_cache.remove_if([&](const auto& el){ return el == sk; }); }

    KeyPair Derive(const std::string& path, bool for_script) const;

    KeyPair Lookup(const l15::bytevector& keyid, const KeyLookupFilter& hint, std::function<bool(const l15::core::ChannelKeys&, const l15::bytevector&)>) const;
    KeyPair Lookup(const l15::xonly_pubkey& pk, const KeyLookupFilter& hint) const;
    KeyPair Lookup(const l15::xonly_pubkey& pk, const std::string& hint_json) const;
    KeyPair Lookup(const std::string& addr, const KeyLookupFilter& hint) const;
    KeyPair Lookup(const std::string& addr, const std::string& hint_json) const;
};

class ExtPubKey
{
    ChainMode m_chainmode;
    const secp256k1_context* m_ctx;
    l15::core::ext_pubkey m_extpk;

public:
    ExtPubKey(ChainMode chainmode, const secp256k1_context* ctx, l15::core::ext_pubkey extpk): m_chainmode(chainmode), m_ctx(l15::core::ChannelKeys::GetStaticSecp256k1Context()), m_extpk(move(extpk)) {}
    ExtPubKey(ChainMode chainmode, const std::string& extpk);

    ExtPubKey(const ExtPubKey&) = default;
    ExtPubKey& operator=(const ExtPubKey&) = default;
    ExtPubKey& operator=(ExtPubKey&&) noexcept = default;

    uint256 GetChainCode() const
    { return uint256(Span<const uint8_t >(m_extpk.data(), 32)); }
    l15::xonly_pubkey GetPubKey() const
    { return l15::xonly_pubkey(m_extpk.begin() + 33, m_extpk.end()); }


    ExtPubKey Derive(uint32_t index) const
    { return ExtPubKey(m_chainmode, m_ctx, l15::core::MasterKey::Derive(m_ctx, m_extpk, index)); }

    std::string DeriveAddress(const std::string& path) const;
};


} // utxord

