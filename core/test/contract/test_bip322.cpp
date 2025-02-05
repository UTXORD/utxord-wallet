#define CATCH_CONFIG_RUNNER

#include "base58.h"

#include "base64.hpp"

#include "catch/catch.hpp"

#include "util/translation.h"

#include "bip322.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace l15::core;
using namespace utxord;

int main(int argc, char* argv[])
{
    std::string configpath;
    Catch::Session session;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli
            = session.cli() // Get Catch's composite command line parser
              | Opt(configpath, "Config path") // bind variable to a new option, with a hint string
              ["--config"]    // the option names it will respond to
                      ("Path to L15 config");

    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if(returnCode != 0) // Indicates a command line error
        return returnCode;

//    if(configpath.empty())
//    {
//        std::cerr << "Bitcoin config is not passed!" << std::endl;
//        return 1;
//    }
//
//    std::filesystem::path p(configpath);
//    if(p.is_relative())
//    {
//        configpath = (std::filesystem::current_path() / p).string();
//    }

    //w = std::make_unique<TestcaseWrapper>(configpath);

    return session.run();
}

TEST_CASE("hash")
{
    CHECK(Bip322::Hash({}) == unhex<bytevector>("c90c269c4f8fcbe6880f72a721ddfbf1914268a794cbb21cfafee13770ae19f1"));

    std::string hello("Hello World");
    CHECK(Bip322::Hash(bytevector(hello.begin(), hello.end())) == unhex<bytevector>("f0eb03b1a75ac6d9847f55c624a99169b5dccba2a31f5b23bea77ba270de0a7a"));
}

TEST_CASE("tospendtxid")
{
    Bip322 s(MAINNET);

    CHECK(s.ToSpendTxID({}, "bc1q9vza2e8x573nczrlzms0wvx3gsqjx7vavgkx0l").GetHex() == "c5680aa69bb8d860bf82d4e9cd3504b55dde018de765a91bb566283c545a99a7");
}

const static std::string hello("Hello World");
const static bytevector hello_bytes(hello.begin(), hello.end());

TEST_CASE("verify")
{
    Bip322 s(MAINNET);

    auto [msg, sig] = GENERATE(std::make_pair(bytevector(),"AkcwRAIgM2gBAQqvZX15ZiysmKmQpDrG83avLIT492QBzLnQIxYCIBaTpOaD20qRlEylyxFSeEA2ba9YOixpX8z46TSDtS40ASECx/EgAxlkQpQ9hYjgGu6EBCPMVPwVIVJqO4XCsMvViHI="),
                               std::make_pair(bytevector(), "AkgwRQIhAPkJ1Q4oYS0htvyuSFHLxRQpFAY56b70UvE7Dxazen0ZAiAtZfFz1S6T6I23MWI2lK/pcNTWncuyL8UL+oMdydVgzAEhAsfxIAMZZEKUPYWI4BruhAQjzFT8FSFSajuFwrDL1Yhy"),
                               std::make_pair(hello_bytes, "AkcwRAIgZRfIY3p7/DoVTty6YZbWS71bc5Vct9p9Fia83eRmw2QCICK/ENGfwLtptFluMGs2KsqoNSk89pO7F29zJLUx9a/sASECx/EgAxlkQpQ9hYjgGu6EBCPMVPwVIVJqO4XCsMvViHI="),
                               std::make_pair(hello_bytes, "AkgwRQIhAOzyynlqt93lOKJr+wmmxIens//zPzl9tqIOua93wO6MAiBi5n5EyAcPScOjf1lAqIUIQtr3zKNeavYabHyR8eGhowEhAsfxIAMZZEKUPYWI4BruhAQjzFT8FSFSajuFwrDL1Yhy"));

    REQUIRE(s.Verify(l15::base64::decode<bytevector>(sig), "bc1q9vza2e8x573nczrlzms0wvx3gsqjx7vavgkx0l", msg));
}

TEST_CASE("xverse_verify")
{
    Bip322 s(MAINNET);

    xonly_pubkey pk = unhex<xonly_pubkey>("f5f18482cebf824d1ea115b8d90528454aa4ddc093c848f4b66943ca89ad3b8a");
    Bech32 bech(BTC, MAINNET);

    std::string addr = bech.Encode(pk);
    std::string challenge = "9838992211213ec345be7be79ae8af8ca0822a63b70aee224267d6145751fa99";

    REQUIRE(s.Verify(l15::base64::decode<bytevector>("AUB5OtTvOmgYBX3s8JBps4dQYKX79Bddzx1B9nvm273skV2YPB+pHSfTVEyzgzPVuXg2ioQZXkjcjFN97P1JLaed"), addr, {challenge.begin(), challenge.end()}));
}

TEST_CASE("sign_p2tr")
{
    Bip322 s(MAINNET);

    KeyRegistry k(MAINNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");
    k.AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");

    bytevector data;
    seckey sk;
    if (DecodeBase58Check("L3VFeEujGtevx9w18HD1fhRbCH67Az2dpCymeRE1SoPK6XQtaN2k", data, 34)) {
        const bytevector privkey_prefix = bytevector(1,128);
        if ((data.size() == 32 + privkey_prefix.size() || (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
            std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
                bool compressed = data.size() == 33 + privkey_prefix.size();
                std::copy(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32, sk.begin());
            }
    }

    k.AddKeyToCache(sk);

    auto sig = s.Sign(k, "funds", "bc1q9vza2e8x573nczrlzms0wvx3gsqjx7vavgkx0l", {});

    REQUIRE(s.Verify(sig, "bc1q9vza2e8x573nczrlzms0wvx3gsqjx7vavgkx0l", {}));
}
