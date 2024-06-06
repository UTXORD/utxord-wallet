
#include <string>
#include <array>

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "util/translation.h"

#include "channel_keys.hpp"
#include "key.h"
#include "base58.h"
#include "util/spanparsing.h"
#include "utils.hpp"
#include "keypair.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace utxord;

//static const std::vector<std::byte> seed = unhex<std::vector<std::byte>>(
static const bytevector seed = unhex<bytevector>(
"d1dfff73303d878129dbe8e7a830135a85a0142498d669f234b40dabafc1fb4469377a968640418453a09cf84f715c408070a17f9c1b823fdce168dcd0866fa3");

static const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;
static const std::string derive_path = "m/86'/1'/1'/0/0";
static const std::string derive_path1 = "m/84'/1'/1'/0/0";


TEST_CASE("KeyFilter")
{
    KeyRegistry master(TESTNET, "f35c7006dd5a72d1023dff8b856fd9c90bc5f334650c5f933da8218a183e8a14d0ea9c8fb1694b34edb7f5b1674edb2a8f90fee52325cc3a7968062608c61cce");

    CHECK_NOTHROW(master.AddKeyType("oth", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["0'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("fund", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["1'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("ord", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["2'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("uns", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["3'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("intsk", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["4'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("scrsk", R"({"look_cache":true,"key_type":"TAPSCRIPT","accounts":["5'"],"change":["0"],"index_range":"0-16384"})"));
}

TEST_CASE("KeyLookup")
{
    KeyRegistry master(TESTNET, "f35c7006dd5a72d1023dff8b856fd9c90bc5f334650c5f933da8218a183e8a14d0ea9c8fb1694b34edb7f5b1674edb2a8f90fee52325cc3a7968062608c61cce");

    CHECK_NOTHROW(master.AddKeyType("oth", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["0'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("fund", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["1'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("ord", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["2'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("uns", R"({"look_cache":true,"key_type":"TAPSCRIPT","accounts":["3'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("intsk", R"({"look_cache":true,"key_type":"TAPSCRIPT","accounts":["4'"],"change":["0"],"index_range":"0-16384"})"));
    CHECK_NOTHROW(master.AddKeyType("scrsk", R"({"look_cache":true,"key_type":"TAPSCRIPT","accounts":["5'"],"change":["0"],"index_range":"0-16384"})"));

    KeyPair derived = master.Derive("m/86'/1'/1'/0/0", false);

    std::clog << "Derived addr: " << derived.GetP2TRAddress(Bech32(TESTNET)) << std::endl;

    KeyPair keypair;
    REQUIRE_NOTHROW(keypair = master.Lookup("tb1p673hxdtlaa07z46wc4pz2kewz0l37dta7j367dep3ytgk6nlxq6st04x8m", "fund"));

    KeyPair derived2 = master.Derive("m/86'/1'/3'/0/0", true);

    KeyPair keypair2;
    REQUIRE_NOTHROW(keypair2 = master.Lookup(derived2.PubKey(), "uns"));
}


TEST_CASE("Derive")
{
    ECC_Start();

    auto bech = Bech32(TESTNET);

    std::clog << "Seed: " << hex(seed) << std::endl;

    CExtKey key;
    key.SetSeed(unhex<std::vector<std::byte>>(hex(seed)));

    std::array<uint8_t, 74> extkeydata;

    key.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;

    auto keypath = GENERATE(derive_path, derive_path1);

    auto branches = spanparsing::Split(keypath, '/');

    if (branches.front()[0] == 'm') {
        branches.erase(branches.begin());
    }

    seckey sk;
    for (const auto& branch: branches) {
        uint32_t index;
        if (branch[branch.size()-1] == '\'') {
            //hardened
            auto conv_res = std::from_chars(branch.begin(), branch.end() - 1, index);
            if (conv_res.ec == std::errc::invalid_argument) {
                throw std::invalid_argument("Wrong num string");
            }
            index += BIP32_HARDENED_KEY_LIMIT;
        }
        else {
            // non hardened
            auto conv_res = std::from_chars(branch.begin(), branch.end(), index);
            if (conv_res.ec == std::errc::invalid_argument) {
                throw std::invalid_argument("Wrong num string");
            }
        }
        if (!key.Derive(key, index)) {
            throw std::runtime_error("Derivation error");
        }

        key.Encode(extkeydata.data());
        std::clog << "Key data: " << hex(extkeydata) << std::endl;

        sk.assign(key.key.begin(), key.key.end());

        //core::ChannelKeys derived_key(sk);
        //std::clog << "Addr: " << bech.Encode(derived_key.GetLocalPubKey()) << std::endl;
    }

    std::string btc_addr;
    if (keypath.starts_with("m/86")) {
        core::ChannelKeys derived_key(sk);
        std::clog << "+++seckey: " << hex(derived_key.GetLocalPrivKey()) << std::endl;
        derived_key.AddTapTweak();
        std::clog << "tweaked seckey: " << hex(derived_key.GetLocalPrivKey()) << std::endl;
        KeyPair keypair(derived_key.GetLocalPrivKey());
        btc_addr = keypair.GetP2TRAddress(bech);
        std::clog << "P2TR: " << btc_addr << std::endl;
        CHECK(btc_addr == "tb1plj0pw0aq70cal6j4p9z4qdrcts8ky6jhqk4lmlzmrqa9js88ha9sv4lclv");
    }
    else {
        KeyPair keypair(sk);
        btc_addr = keypair.GetP2WPKHAddress(bech);
        std::clog << "P2WPKH: " << btc_addr << std::endl;
        CHECK(btc_addr == "tb1qeqvy2au533z2q3tlw8v0xrckfwrsqak3dlw66g");
    }

    ECC_Stop();

    KeyRegistry keyRegistry(TESTNET, hex(seed));
    KeyPair k = keyRegistry.Derive(keypath.c_str(), false);

    if (keypath.starts_with("m/86")) {
        std::string addr = k.GetP2TRAddress(bech);
        std::clog << "KeyRegistry derived P2TR address: " << addr << std::endl;
        CHECK(btc_addr == addr);
    }
    else {
        std::string addr = k.GetP2WPKHAddress(bech);
        std::clog << "KeyRegistry derived P2WPKH address: " << addr << std::endl;
        CHECK(btc_addr == addr);
    }
}

TEST_CASE("VerifySignature")
{
    const std::string pk_hex = "0a15a355a35e3181990f161bda0849022ffe5f53d331b9f989525e43633c9f67";
    const std::string challenge_hex = "2a845aeebbe5ebe3fe621ef6b13430bf0e22528e4ad72dbfff539d6cdbed6659";
    const std::string signature_hex = "128bdf69c469a23792b78f492d3444da9e7a5cc3276f09acb0f29d8a2314e7b1e0985448ea440c84ccb348a1bd1999011469b04c868e139daf53d3497034947f";
//    const std::string pk_hex = "d505e969f72a85c1b484d859e38188d1af63872d743b58e8bb3bdefe1ccd5396";
//    const std::string challenge_hex = "f7e6cc0f9549b34a7225391d965f2ad6ed04e4a641c9e675c7a4e5a11eb49da7";
//    const std::string signature_hex = "485e41ef0e4e7c33be3010275a4fc1852dda058e2a55179374a80ca7abddb5131146cd6e2838ed016fd0dd2f28aeacdbe53238af4e402fc051cc02fa83be65b4";


    l15::xonly_pubkey pk = l15::unhex<l15::xonly_pubkey>(pk_hex);

    CHECK(pk.verify(l15::core::ChannelKeys::GetStaticSecp256k1Context(), unhex<signature>(signature_hex), uint256S(challenge_hex)));
}

TEST_CASE("MakeSignature")
{
    const std::string challenge_hex = "2a845aeebbe5ebe3fe621ef6b13430bf0e22528e4ad72dbfff539d6cdbed6659";
    const seckey sk = unhex<seckey>("76a2aa9d77b047b3b9822c6fabf90ccd0bbf511b3158184a6f13f90851cb76e2");
    core::ChannelKeys keypair(sk);

    signature sig = keypair.SignSchnorr(uint256S(challenge_hex));

    xonly_pubkey pk = keypair.GetLocalPubKey();

    CHECK(pk.verify(l15::core::ChannelKeys::GetStaticSecp256k1Context(), sig, uint256S(challenge_hex)));
}

TEST_CASE("PubKeyDerive")
{
    ECC_Start();
    CExtKey btcRoot;
    btcRoot.SetSeed(unhex<std::vector<std::byte>>(hex(seed)));

    std::array<uint8_t, 74> extkeydata;

    std::clog << "Seed: " << hex(seed) << std::endl;

    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;


    REQUIRE(btcRoot.Derive(btcRoot, 86 + BIP32_HARDENED_KEY_LIMIT));
    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;
    REQUIRE(btcRoot.Derive(btcRoot, 1 + BIP32_HARDENED_KEY_LIMIT));
    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;
    REQUIRE(btcRoot.Derive(btcRoot, 1 + BIP32_HARDENED_KEY_LIMIT));
    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;

    CExtPubKey btcExtPK = btcRoot.Neuter();
    std::clog << "btc pubkey: " << hex(btcExtPK.pubkey) << std::endl;

    REQUIRE(btcRoot.Derive(btcRoot, 0));
    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;
    REQUIRE(btcRoot.Derive(btcRoot, 0));
    btcRoot.Encode(extkeydata.data());
    std::clog << "Key data: " << hex(extkeydata) << std::endl;

    seckey sk0(btcRoot.key.begin(), btcRoot.key.end());

    l15::core::ChannelKeys keypair0(sk0);
    std::clog << "^^^seckey: " << hex(keypair0.GetLocalPrivKey()) << std::endl;
    keypair0.AddTapTweak();
    std::clog << "tweaked seckey: " << hex(keypair0.GetLocalPrivKey()) << std::endl;
    KeyPair trkeypair0(keypair0.GetLocalPrivKey());
    std::clog << "btc addr 0: " << trkeypair0.GetP2TRAddress(Bech32(TESTNET)) << std::endl;


    std::vector<unsigned char> data = {0x04, 0x35, 0x87, 0xcf};
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    btcExtPK.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);

    CHECK(ret == "tpubDCKwXGY6buZjvJFHitFttGMbM6WNwUfqvw26xsbkdUJ2wh6hQ64CLagJYjWYkzSyRVAYKaTDRWyNtDXUcpCqMT57LWVUvBbp1xZ85NzPovD");

    std::clog << "btc ext PK: " << ret << std::endl;

    CExtPubKey btcExtPK0;
    std::vector<unsigned char> data0;
    if (DecodeBase58Check(ret, data0, 78)) {
        const std::vector<unsigned char>& prefix = {0x04, 0x35, 0x87, 0xcf};
        if (data0.size() == BIP32_EXTKEY_SIZE + prefix.size() && std::equal(prefix.begin(), prefix.end(), data0.begin())) {
            btcExtPK0.Decode(data0.data() + prefix.size());
        }
    }

    std::clog << "BTC chain code 0: " << btcExtPK.chaincode.GetHex() << std::endl;
    std::clog << "BTC pubkey 0: " << hex(btcExtPK.pubkey) << std::endl;


    CExtPubKey btcExtPK1;
    REQUIRE(btcExtPK.Derive(btcExtPK1, 0));
    REQUIRE(btcExtPK1.Derive(btcExtPK1, 0));

    REQUIRE(btcExtPK1.pubkey.size() == 33);

    xonly_pubkey pk(btcExtPK1.pubkey.begin() + 1, btcExtPK1.pubkey.end());
    auto twpk1 = l15::core::ChannelKeys::AddTapTweak(pk);

    std::string btcpkaddr = Bech32(TESTNET).Encode(std::get<0>(twpk1));

    std::clog << "btc pk addr: " << btcpkaddr << std::endl;

    CHECK(btcpkaddr == "tb1plj0pw0aq70cal6j4p9z4qdrcts8ky6jhqk4lmlzmrqa9js88ha9sv4lclv");

    std::clog << "BTC chain code 1: " << btcExtPK.chaincode.GetHex() << std::endl;
    std::clog << "BTC pubkey 1: " << hex(btcExtPK.pubkey) << std::endl;

    // mnemonic:
    // motor chef under enroll hub skate magic tide forum figure alien slight
    KeyRegistry keyRegistry(TESTNET, hex(seed));
    REQUIRE_NOTHROW(keyRegistry.AddKeyType("key", R"({"look_cache":true,"key_type":"DEFAULT","accounts":["1'"],"change":["0"],"index_range":"0-256"})"));

    ExtPubKey extPubKey(TESTNET, "tpubDCKwXGY6buZjvJFHitFttGMbM6WNwUfqvw26xsbkdUJ2wh6hQ64CLagJYjWYkzSyRVAYKaTDRWyNtDXUcpCqMT57LWVUvBbp1xZ85NzPovD");
    std::clog << "Chain code: " << extPubKey.GetChainCode().GetHex() << std::endl;
    std::clog << "Pubkey: " << hex(extPubKey.GetPubKey()) << std::endl;

    std::string addr0, addr11;

    CHECK_NOTHROW(addr0 = extPubKey.DeriveAddress("0/0"));
    CHECK(addr0 == "tb1plj0pw0aq70cal6j4p9z4qdrcts8ky6jhqk4lmlzmrqa9js88ha9sv4lclv");
    CHECK_NOTHROW(addr11 = extPubKey.DeriveAddress("0/11"));

    std::clog << "Addr 0/0: " << addr0 << std::endl;

    CHECK_NOTHROW(keyRegistry.Lookup(addr0, "key"));
    CHECK_NOTHROW(keyRegistry.Lookup(addr11, "key"));

    ECC_Stop();
}
