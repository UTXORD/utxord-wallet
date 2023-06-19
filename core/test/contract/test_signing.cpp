
#include <string>

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "util/translation.h"

#include "channel_keys.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;

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
