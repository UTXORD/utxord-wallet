
#include <string>

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "util/translation.h"

#include "channel_keys.hpp"
#include "key.h"
#include "util/spanparsing.h"
#include "utils.hpp"
#include "keypair.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace utxord;

static const std::vector<std::byte> seed = unhex<std::vector<std::byte>>(
"b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

static const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;
static const std::string derive_path = "m/86'/2'/0'/0/0";


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

    std::clog << "Derived addr: " << derived.GetP2TRAddress(Bech32(utxord::Hrp<TESTNET>())) << std::endl;

    KeyPair keypair;
    REQUIRE_NOTHROW(keypair = master.Lookup("tb1p673hxdtlaa07z46wc4pz2kewz0l37dta7j367dep3ytgk6nlxq6st04x8m", "fund"));

    KeyPair derived2 = master.Derive("m/86'/1'/3'/0/0", true);

    KeyPair keypair2;
    REQUIRE_NOTHROW(keypair2 = master.Lookup(derived2.PubKey(), "uns"));
}


TEST_CASE("Derive")
{
    ECC_Start();

    auto bech = Bech32Coder<IBech32Coder::BTC, IBech32Coder::TESTNET>();

    std::clog << "Seed: " << hex(seed) << std::endl;

    CExtKey key;
    key.SetSeed(seed);

    auto branches = spanparsing::Split(derive_path, '/');

//    std::clog << "Derivation branches: " << std::endl;
//    for (const auto& branch: branches) {
//        std::clog << std::string_view(branch.data(), branch.size()) << std::endl;
//    }

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
                throw std::invalid_argument("Wrong hex string");
            }
            index += BIP32_HARDENED_KEY_LIMIT;
        }
        else {
            // non hardened
            auto conv_res = std::from_chars(branch.begin(), branch.end(), index);
            if (conv_res.ec == std::errc::invalid_argument) {
                throw std::invalid_argument("Wrong hex string");
            }
        }
        if (!key.Derive(key, index)) {
            throw std::runtime_error("Derivation error");
        }

        sk.assign(key.key.begin(), key.key.end());

        core::ChannelKeys derived_key(sk);
        std::clog << "Addr: " << bech.Encode(derived_key.GetLocalPubKey()) << std::endl;
    }

    core::ChannelKeys derived_key(sk);
    derived_key.AddTapTweak();

    std::clog << "Addr: " << bech.Encode(derived_key.GetLocalPubKey()) << std::endl;

    CHECK(bech.Encode(derived_key.GetLocalPubKey()) == "tb1ptnn4tufj4yr8ql0e8w8tye7juxzsndnxgnlehfk2p0skftzks20sncm2dz");

    ECC_Stop();
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
