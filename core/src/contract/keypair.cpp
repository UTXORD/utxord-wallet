
#include <execution>

#include "nlohmann/json.hpp"

#include "keypair.hpp"
#include "utils.hpp"
#include "chainmode_spec.hpp"
#include "base58.h"

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


namespace {

uint32_t derivation_index(const std::string& val)
{
    uint32_t res;
    if (val.back() == '\'') {
        std::from_chars(val.data(), val.data() + val.size() - 1, res);
        res += l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT;
    }
    else {
        std::from_chars(val.data(), val.data() + val.size(), res);
    }
    return res;
}

KeyLookupFilter ParseKeyLookupFilter(const nlohmann::json& json)
{
    try {
        std::string key_type_str = json["key_type"];
        KeyLookupFilter::Type key_type = KeyLookupFilter::DEFAULT;
        if (key_type_str != "DEFAULT") {
            if (key_type_str == "TAPROOT") key_type = KeyLookupFilter::TAPROOT;
            else if (key_type_str == "TAPSCRIPT") key_type = KeyLookupFilter::TAPSCRIPT;
            else throw std::invalid_argument(std::string("key_type: ") + key_type_str);
        }

        if (!json["accounts"].is_array()) throw std::invalid_argument("accounts is missed or not an array");
        if (!json["change"].is_array()) throw std::invalid_argument("change is missed or not an array");
        if (!json["index_range"].is_string()) throw std::invalid_argument("index_range is missed or not a string");

        std::vector<uint32_t> accounts(json["accounts"].size());
        std::transform(json["accounts"].begin(), json["accounts"].end(), accounts.begin(), &derivation_index);
        std::vector<uint32_t> change(json["change"].size());
        std::transform(json["change"].begin(), json["change"].end(), change.begin(), &derivation_index);

        const std::string &index_range_str = json["index_range"];
        size_t split_pos = index_range_str.find('-');

        if (split_pos == std::string::npos) throw std::invalid_argument(std::string("index_range: ") + index_range_str);

        uint32_t index_begin = 0, index_end = 0;
        std::string_view beg_str{index_range_str.c_str(), split_pos};
        std::string end_str = index_range_str.substr(split_pos + 1);

        if (beg_str.back() == '\'') {
            if (end_str.back() != '\'') throw std::invalid_argument(std::string("index_range: ") + index_range_str + ", end bound must be hardened too");

            std::from_chars(beg_str.data(), beg_str.data() + beg_str.size() - 1, index_begin);
            index_begin += l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT;
        }
        else {
            std::from_chars(beg_str.data(), beg_str.data() + beg_str.size(), index_begin);
        }

        if (end_str.back() == '\'') {
            if (beg_str.back() != '\'') throw std::invalid_argument(std::string("index_range: ") + index_range_str + ", end bound must not be hardened too");

            std::from_chars(end_str.data(), end_str.data() + end_str.size() - 1, index_end);
            index_end += l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT;
        }
        else {
            std::from_chars(end_str.data(), end_str.data() + end_str.size(), index_end);
        }


        return {json["look_cache"], key_type, move(accounts), move(change), std::ranges::iota_view{index_begin, index_end}};
    }
    catch(std::exception& ex) {
        std::throw_with_nested(std::invalid_argument("key filter json"));
    }
}

}

KeyPair KeyRegistry::Derive(const char *path, bool for_script) const
{
    l15::core::ChannelKeys keypair = mMasterKey.Derive(path, for_script);
    return KeyPair(m_ctx, keypair.GetLocalPrivKey());
}

KeyPair KeyRegistry::Lookup(const l15::bytevector &keyid, const KeyLookupFilter& hint, std::function<bool(const l15::core::ChannelKeys&, const l15::bytevector&)> compare) const
{
    if (hint.look_cache) {
        for (const auto &sk: m_keys_cache) {
            l15::core::ChannelKeys keypair(m_ctx, sk);
            if (compare(keypair, keyid)) {
                return KeyPair(m_ctx, keypair.GetLocalPrivKey());
            }
        }
    }

    l15::core::MasterKey masterCopy(mMasterKey);
    if (hint.type == KeyLookupFilter::TAPROOT || hint.type == KeyLookupFilter::TAPSCRIPT) {
        masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + l15::core::MasterKey::BIP86_TAPROOT);
    }
    else {
        masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + l15::core::MasterKey::BIP84_P2WPKH);
    }

    switch (mBech.GetChainMode()) {
    case MAINNET:
        masterCopy.DeriveSelf(l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT);
        break;
    case TESTNET:
    case REGTEST:
        masterCopy.DeriveSelf(1 + l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT);
        break;
    }

    std::vector<l15::core::MasterKey> accountKeys;
    accountKeys.reserve(hint.accounts.size());

    for (uint32_t acc: hint.accounts) {
        l15::core::MasterKey account(masterCopy);
        account.DeriveSelf(/*l15::core::MasterKey::BIP32_HARDENED_KEY_LIMIT + */acc);

        for (uint32_t ch: hint.change) {
            l15::core::MasterKey change(account);
            change.DeriveSelf(ch);
            accountKeys.emplace_back(std::move(change));
        }
    }

#ifndef WASM
    std::atomic<std::shared_ptr<l15::core::ChannelKeys>> res;
    const uint32_t step = 64;
    uint32_t indexes[step];
    for (uint32_t key_index = *hint.index_range.begin(); key_index < *hint.index_range.end(); key_index += step) {
        std::iota(indexes, indexes + step, key_index);
        std::for_each(std::execution::par_unseq, indexes, indexes + step, [&](const auto &k) {
            for (const l15::core::MasterKey &account: accountKeys) {

                // For TAPROOT case lets look for both tweaked and untweaked keys just to provide more robustness

                l15::core::ChannelKeys keypair = account.Derive(std::vector<uint32_t>{k}, l15::core::SUPPRESS);
                if (compare(keypair, keyid)) {
                    res = std::make_shared<l15::core::ChannelKeys>(std::move(keypair));
                }
                else if (hint.type == KeyLookupFilter::TAPROOT) {
                    keypair.AddTapTweak();
                    if (compare(keypair, keyid)) {
                        res = std::make_shared<l15::core::ChannelKeys>(std::move(keypair));
                    }
                }
            }
        });
        std::shared_ptr<l15::core::ChannelKeys> keypair = res.load();
        if (keypair) {
            return KeyPair(m_ctx, keypair->GetLocalPrivKey());
        }
    }
#else
    for (uint32_t key_index: hint.index_range) {
        for (const l15::core::MasterKey& account: accountKeys) {

            // For TAPROOT case lets look for both tweaked and untweaked keys just to provide more robustness

            l15::core::ChannelKeys keypair = account.Derive(std::vector<uint32_t>{key_index}, l15::core::SUPPRESS);
            if (compare(keypair, keyid)) {
                return KeyPair(m_ctx, keypair.GetLocalPrivKey());
            }
            else if (hint.type == KeyLookupFilter::TAPROOT){
                keypair.AddTapTweak();
                if (compare(keypair, keyid)) {
                    return KeyPair(m_ctx, keypair.GetLocalPrivKey());
                }
            }
        }
    }
#endif
    throw l15::KeyError("derivation lookup");
}


KeyPair KeyRegistry::Lookup(const l15::xonly_pubkey &pk, const KeyLookupFilter& hint) const
{
    std::cout << "lookup for pk: " << hex(pk) << std::endl;

    KeyLookupFilter taproot_hint = hint;
    if (taproot_hint.type == KeyLookupFilter::DEFAULT) {
        taproot_hint.type = KeyLookupFilter::TAPROOT;
    }
    return Lookup(pk.get_vector(), taproot_hint, [](const l15::core::ChannelKeys& key, const l15::bytevector& id) { return key.GetLocalPubKey() == id; });
}

KeyPair KeyRegistry::Lookup(const l15::xonly_pubkey &pk, const std::string& hint_json) const
{
    try {
        auto json = nlohmann::json::parse(hint_json);
        return Lookup(pk.get_vector(), ParseKeyLookupFilter(json),
                      [](const l15::core::ChannelKeys &key, const l15::bytevector &id) { return key.GetLocalPubKey() == id; });
    }
    catch(const nlohmann::json::parse_error& e) {
        if (!m_key_type_filters.contains(hint_json)) std::throw_with_nested(std::invalid_argument("key filter is unknown: " + hint_json));
        return Lookup(pk.get_vector(), m_key_type_filters.at(hint_json),
                      [](const l15::core::ChannelKeys &key, const l15::bytevector &id) { return key.GetLocalPubKey() == id; });
    }
}

KeyPair KeyRegistry::Lookup(const std::string& addr, const KeyLookupFilter& hint) const
{
    unsigned witver;
    l15::bytevector keyid;
    std::tie(witver, keyid) = mBech.Decode(addr);

    if (witver == 0) {
        return Lookup(keyid, hint, [&](const l15::core::ChannelKeys &k, const l15::bytevector &id) {
            EcdsaKeypair keypair(m_ctx, k.GetLocalPrivKey());
            return l15::Hash160(keypair.GetPubKey().as_vector()) == id;
        });
    }

    KeyLookupFilter taproot_hint = hint;
    if (taproot_hint.type == KeyLookupFilter::DEFAULT) {
        taproot_hint.type = KeyLookupFilter::TAPROOT;
    }
    return Lookup(keyid, taproot_hint, [](const l15::core::ChannelKeys &k, const l15::bytevector &id) { return k.GetLocalPubKey() == id; });
}

KeyPair KeyRegistry::Lookup(const std::string& addr, const std::string& hint_json) const
{
    try {
        auto json = nlohmann::json::parse(hint_json);
        return Lookup(addr, ParseKeyLookupFilter(json));
    }
    catch(const nlohmann::json::parse_error& e) {
        if (!m_key_type_filters.contains(hint_json)) std::throw_with_nested(std::invalid_argument("key filter is unknown: " + hint_json));
        return Lookup(addr, m_key_type_filters.at(hint_json));
    }
}

void KeyRegistry::AddKeyType(std::string name, const string &filter_json)
{
    auto json = nlohmann::json::parse(filter_json);
    AddKeyType(move(name), ParseKeyLookupFilter(json));
}

void KeyRegistry::RemoveKeyFromCache(const string &addr)
{
    uint32_t witver;
    l15::bytevector keyid;
    std::tie(witver, keyid) = mBech.Decode(addr);

    if (witver == 1)
        m_keys_cache.remove_if([&](const auto& el){ return KeyPair(m_ctx, el).GetP2TRAddress(mBech) == addr; });
    else if (witver == 0)
        m_keys_cache.remove_if([&](const auto& el){ return KeyPair(m_ctx, el).GetP2WPKHAddress(mBech) == addr; });
    else
        throw std::invalid_argument("address: " + addr);
}

ExtPubKey::ExtPubKey(ChainMode chainmode, const std::string &extpk) : m_chainmode(chainmode)
{
    l15::bytevector data;
    if (!DecodeBase58Check(extpk, data, 78))
        throw l15::KeyError("Bad base58chech encoding: " + extpk);

    const uint8_t* prefix = chainmode == ChainMode::MAINNET ? XPubPrefix<MAINNET>::value : XPubPrefix<TESTNET>::value;
    if (!std::equal(prefix, prefix+4, data.begin()))
        throw l15::KeyError("Wrong extpubkey prefix: " + extpk);

    if (data.size() != 78)
        throw l15::KeyError("Wrong extpubkey data size: " + std::to_string(data.size()));

    std::clog << "Raw ExtPubKey data: " << l15::hex(data) << std::endl;

    std::copy(data.begin() + 13, data.end(), m_extpk.begin());
}

std::string ExtPubKey::DeriveAddress(const string &path) const
{
    auto branches = spanparsing::Split(path, '/');
    std::vector<uint32_t> uint_branches;
    uint_branches.reserve(branches.size());

    for (const auto& branch: branches) {
        uint32_t index;
        if (branch[branch.size() - 1] == '\'') throw l15::KeyError("Pubkey derivation cannot use hardened algo");

            // non hardened
        auto conv_res = std::from_chars(branch.begin(), branch.end(), index);
        if (conv_res.ec == std::errc::invalid_argument) {
            throw std::invalid_argument("Wrong hex string");
        }
        uint_branches.push_back(index);
    }

    ExtPubKey extPk = *this;
    for (uint32_t b: uint_branches) {
        extPk = extPk.Derive(b);
    }

    auto tweaked_pk = l15::core::ChannelKeys::AddTapTweak(extPk.GetPubKey());

    Bech32 bech(m_chainmode);

    return bech.Encode(std::get<0>(tweaked_pk));
}

} // utxord