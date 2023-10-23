
#include <execution>

#include "keypair.hpp"
#include "utils.hpp"

namespace utxord {

l15::xonly_pubkey KeyPair::PubKey() const
{
    l15::core::ChannelKeys keypair(m_ctx, m_sk);
    return keypair.GetLocalPubKey();
}

std::string KeyPair::GetP2TRAddress(Bech32 bech) const
{
    return bech.Encode(PubKey(), bech32::Encoding::BECH32M);
}

std::string KeyPair::GetP2WPKHAddress(Bech32 bech) const
{
    EcdsaKeypair keypair(m_ctx, m_sk);
    return bech.Encode(l15::Hash160(keypair.GetPubKey().as_vector()), bech32::Encoding::BECH32);
}

l15::signature KeyPair::SignSchnorr(const char *m) const
{
    l15::core::ChannelKeys keypair(m_ctx, m_sk);
    return keypair.SignSchnorr(uint256S(m));
}


KeyPair KeyRegistry::Derive(const char *path, bool for_script) const
{
    l15::core::ChannelKeys keypair = mMasterKey.Derive(path, for_script);
    return KeyPair(m_ctx, keypair.GetLocalPrivKey());
}

KeyPair KeyRegistry::Lookup(const l15::bytevector &keyid, KeyLookupHint hint, std::function<bool(const l15::core::ChannelKeys&, const l15::bytevector&)> compare) const
{
    l15::core::MasterKey masterCopy(mMasterKey);
    if (hint.type == KeyLookupHint::TAPROOT || hint.type == KeyLookupHint::TAPSCRIPT) {
        masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + l15::core::MasterKey::BIP86_TAPROOT);
    }
    else {
        masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + l15::core::MasterKey::BIP84_P2WPKH);
    }

    masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT);

    std::vector<l15::core::MasterKey> accountKeys;
    accountKeys.reserve(hint.accounts.size());

    for (uint32_t acc: hint.accounts) {
        l15::core::MasterKey account(masterCopy);
        account.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + acc);

        l15::core::MasterKey change(account);
        change.DeriveSelf(1);
        accountKeys.emplace_back(std::move(change));

        account.DeriveSelf(0);
        accountKeys.emplace_back(std::move(account));
    }

#ifndef WASM
    std::atomic<std::shared_ptr<l15::core::ChannelKeys>> res;
    const uint32_t step = 64;
    uint32_t indexes[step];
    for (uint32_t key_index = 0; key_index < 65536/*MasterKey::BIP32_HARDENED_KEY_LIMIT*/; key_index += step) {
        std::iota(indexes, indexes + step, key_index);
        std::for_each(std::execution::par_unseq, indexes, indexes + step, [&](const auto &k) {
            for (const l15::core::MasterKey &account: accountKeys) {
                l15::core::ChannelKeys keypair = account.Derive(std::vector<uint32_t>{k},
                                                     (hint.type != KeyLookupHint::TAPROOT) ? l15::core::SUPPRESS : l15::core::FORCE);
                if (compare(keypair, keyid)) {
                    res = std::make_shared<l15::core::ChannelKeys>(std::move(keypair));
                }
            }
        });
        std::shared_ptr<l15::core::ChannelKeys> keypair = res.load();
        if (keypair) {
            return KeyPair(m_ctx, keypair->GetLocalPrivKey());
        }
    }
#else
    for (uint32_t key_index = 0; key_index < 65536/*core::MasterKey::BIP32_HARDENED_KEY_LIMIT*/; ++key_index) {
        for (const l15::core::MasterKey& account: accountKeys) {
            l15::core::ChannelKeys keypair = account.Derive(std::vector<uint32_t>{key_index}, (hint.type != KeyLookupHint::TAPROOT) ? l15::core::SUPPRESS : l15::core::FORCE);
            if (compare(keypair, keyid)) {
                return KeyPair(m_ctx, keypair.GetLocalPrivKey());
            }
        }
    }
#endif
    throw l15::KeyError("derivation lookup");
}


KeyPair KeyRegistry::Lookup(const l15::xonly_pubkey &pk, KeyLookupHint hint) const
{
    if (hint.look_cache) {
        for (const auto &sk: m_keys_cache) {
            KeyPair keypair(m_ctx, sk);
            if (keypair.PubKey() == pk) {
                return keypair;
            }
        }
    }

    return Lookup(pk.get_vector(), hint, [](const l15::core::ChannelKeys& key, const l15::bytevector& id) { return key.GetLocalPubKey() == id; });
}

KeyPair KeyRegistry::Lookup(const std::string& addr, KeyLookupHint hint) const
{
    unsigned witver;
    l15::bytevector keyid;
    std::tie(witver, keyid) = mBech.Decode(addr);

    std::function<bool(const l15::core::ChannelKeys&, const l15::bytevector&)> compare;
    if (witver == 0) {
        compare = [&](const l15::core::ChannelKeys &k, const l15::bytevector &id) {
            EcdsaKeypair keypair(m_ctx, k.GetLocalPrivKey());
            return l15::Hash160(keypair.GetPubKey().as_vector()) == id;
        };
    }
    else {
        if (hint.type == KeyLookupHint::DEFAULT) {
            hint.type = KeyLookupHint::TAPROOT;
        }
        compare = [](const l15::core::ChannelKeys &k, const l15::bytevector &id) { return k.GetLocalPubKey() == id; };
    }

    if (hint.look_cache) {
        for (const auto &sk: m_keys_cache) {
            l15::core::ChannelKeys keypair(m_ctx, sk);
            if (compare(keypair, keyid)) {
                return KeyPair(m_ctx, keypair.GetLocalPrivKey());
            }
        }
    }
    return Lookup(keyid, hint, compare);
}

} // utxord