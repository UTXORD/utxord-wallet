#include <iostream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <thread>


#define CATCH_CONFIG_RUNNER
#include <core_io.h>

#include "catch/catch.hpp"

#include "util/translation.h"

#include "test_case_wrapper.hpp"
#include "runes.hpp"
using namespace l15;
using namespace l15::core;
using namespace utxord;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

std::unique_ptr<TestcaseWrapper> w;

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

    if(configpath.empty())
    {
        std::cerr << "Bitcoin config is not passed!" << std::endl;
        return 1;
    }

    std::filesystem::path p(configpath);
    if(p.is_relative())
    {
        configpath = (std::filesystem::current_path() / p).string();
    }

    w = std::make_unique<TestcaseWrapper>(configpath);

    return session.run();
}

static const bytevector seed = unhex<bytevector>(
        "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

struct EtchParams
{
    std::optional<uint128_t> limit;
    uint128_t amount;
    uint128_t supply;
    uint128_t mint;
};

TEST_CASE("etch")
{
    std::string fee_rate;
    try {
        fee_rate = w->btc().EstimateSmartFee("1");
    }
    catch(...) {
        fee_rate = "0.00001";
    }

    KeyRegistry master_key(w->chain(), hex(seed));
    master_key.AddKeyType("funds", R"({"look_cache":true, "key_type":"DEFAULT", "accounts":["0'","1'"], "change":["0","1"], "index_range":"0-256"})");

    KeyPair utxo_key = master_key.Derive("m/86'/1'/0'/0/0", false);
    std::string utxo_addr = utxo_key.GetP2TRAddress(Bech32(w->chain()));

    string funds_txid = w->btc().SendToAddress(utxo_addr, FormatAmount(10000));
    auto prevout = w->btc().CheckOutput(funds_txid, utxo_addr);

    std::string destination_addr = w->btc().GetNewAddress();

    auto condition = GENERATE(
        EtchParams{{}, 65535, 65535, 32767},
        EtchParams{{}, 0, std::numeric_limits<uint128_t>::max()},
        EtchParams{{}, std::numeric_limits<uint128_t>::max(), std::numeric_limits<uint128_t>::max()},
        EtchParams{{65535}, 65535, 65535, 32767},
        EtchParams{{65535}, 32767, 32767, 32767},
        EtchParams{{65535}, 0, 65535, 32767}
        );

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, std::numeric_limits<uint16_t>::max());
    uint128_t suffix_rune = dist(rng);

    std::string spaced_name = "UTXORD TEST " + DecodeRune(suffix_rune);

    std::clog << "====" << spaced_name << "====" << std::endl;

    Rune rune(spaced_name, " ", 0x2204);
    if (condition.limit)
        rune.LimitPerMint(*condition.limit);

    RuneStone runestone = rune.EtchAndMint(condition.amount, 1);

    SimpleTransaction contract(w->chain());
    contract.MiningFeeRate(fee_rate);

    REQUIRE_NOTHROW(contract.AddInput(std::make_shared<UTXO>(w->chain(), funds_txid, get<0>(prevout).n, 10000, utxo_addr)));

    REQUIRE_NOTHROW(contract.AddOutput(std::make_shared<RuneStoneDestination>(w->chain(), move(runestone))));
    REQUIRE_NOTHROW(contract.AddChangeOutput(destination_addr));
    REQUIRE_NOTHROW(contract.Sign(master_key, "funds"));

    stringvector raw_tx;;
    REQUIRE_NOTHROW(raw_tx = contract.RawTransactions());

    CMutableTransaction tx;
    REQUIRE(DecodeHexTx(tx, raw_tx[0]));

    REQUIRE_NOTHROW(w->btc().SpendTx(CTransaction(tx)));

    w->btc().GenerateToAddress(destination_addr, "1");

    std::string runes_json = w->GetRunes();
    std::clog << runes_json << std::endl;

    UniValue resp;
    resp.read(runes_json);

    UniValue rune_obj = resp["runes"][rune.RuneText("")];

    std::string supply_text = rune_obj["supply"].getValStr();

    CHECK(condition.supply.str() == supply_text);

    if (condition.mint) {

        SECTION("mint")
        {
            string funds_txid = w->btc().SendToAddress(utxo_addr, FormatAmount(10000));
            auto prevout = w->btc().CheckOutput(funds_txid, utxo_addr);

            std::string destination_addr = w->btc().GetNewAddress();

            SimpleTransaction contract(w->chain());
            contract.MiningFeeRate(fee_rate);

            REQUIRE_NOTHROW(contract.AddInput(std::make_shared<UTXO>(w->chain(), funds_txid, get<0>(prevout).n, 10000, utxo_addr)));

            RuneStone runestone = rune.Mint(condition.mint, 1);

            REQUIRE_NOTHROW(contract.AddOutput(std::make_shared<RuneStoneDestination>(w->chain(), move(runestone))));
            REQUIRE_NOTHROW(contract.AddChangeOutput(destination_addr));
            REQUIRE_NOTHROW(contract.Sign(master_key, "funds"));

            w->btc().GenerateToAddress(destination_addr, "1");

            std::string runes_json = w->GetRunes();
            std::clog << runes_json << std::endl;

            UniValue resp;
            resp.read(runes_json);

            UniValue rune_obj = resp["runes"][rune.RuneText("")];

            std::string supply_text = rune_obj["supply"].getValStr();

            uint128_t final_supply = condition.supply + condition.mint;

            CHECK(final_supply.str() == supply_text);
        }

    }
}


static const std::string rune_tx1_hex = "020000000001029118498545933a2f552da54b4618fa91d43fb3445292aef914c04e557a59b2920100000000fdffffff0706c4a6cd4d4e512fb091c8c75f81297be042e95f7be7cc0add9aee0f67bafd0100000000fdffffff020000000000000000216a0952554e455f544553541502010482adbbcc96805c0321054200008980dd40012202000000000000225120a4196188ce6315674399fdb654e0f0db3b10fe19937afcb9b80aa1c37bdc39ae0140538d3b8fb0890bb6eb5dfe5721859af60d1c4939a437cff6d44de98466842359dd1ed12c8d2ba0c0d297295e938509ceb2fe8d0e039d4bd1ebe908015470b30f0140889ef86c09848108ee08ef27448da3f108ede4ea1dddaf46bc1ef83f00b2c905107f2a500626cc762b0d17867766784fb5b1019d4a530dabe16f21bd4c75f4c700000000";
static const std::string rune_tx2_hex = "02000000000101a3c570ae0d6444c8348d2bb53b925dfbbf8c8b272754362dadeb2b836a0a76b10100000000ffffffff020000000000000000226a0952554e455f544553541602010490c8a7dff58edac2eda838054c0000858c2001901a000000000000225120e3ccea79132e7d0952eb3a462f12032c456283943dc0f706deec0be684ca33aa014052a94eff755bbd9599938ecf3f7f01c141b160888be198e6b82746771946db7ad3e8eb30fdc0301cddcba2089b63dbe57566405c8f8e1fa6e74b67a132cb597900000000";
static const std::string rune_tx3_hex = "010000000001018088f37eaba3154c616ed937b70fd581c1fee75fc7acf85c03dfee4bdf8e71b80100000000fdffffff030000000000000000236a0952554e455f5445535417020104b9ca99a18ab5834901020558000086e8ace9000110270000000000002251203ad2ca0f315bde3b2e4cd762f2a650f3cbf031104d772efd896d76e65874a3083025250000000000225120da871895ee8922f3db43b0f53a3a96c489b5e723f34d518a084065d8d4b8c343014012e32015393285195fe36266607723473c0a01bcccd165bb2a5a2ef1da0e2769cb04de38fcf9e322c81ad58d6105c5615e9c602167837eb49ff586bef17ec80b00000000";
static const std::string rune_tx4_hex = "020000000001016be555a9f45acc7ad2bc217039dcde3203eb46e78766ef01da4ecfbb22942f8b000000000000000000030000000000000000286a0952554e455f544553541c0201048085c3c98ef5d8a984d484a5bb9c16010a05520000858c200110270000000000002251206670cb4abf4f6b66ac1034f85a0875543b28240ab16880a814b44b2882541621fe100000000000002251206670cb4abf4f6b66ac1034f85a0875543b28240ab16880a814b44b28825416210141c16275a08199b0737ab48049ae0f0609cd22c9bc42e6310787dd94adaeb3074dc3993ff3a8494ad025ced29f820c46f180e782e03fba58555701ad5682f74e190300000000";
static const std::string rune_tx5_hex = "02000000000101be1c42afcf3992797b3c0731f23252b643dd66661d98bead4347f9b71dc9803c0000000000ffffffff0200000000000000002a6a0952554e455f544553541e02000485a4b9c19797eae9f7b89f5d03832005c3040682fe7f089f000c006e26000000000000225120534796de27a84141f95034e688351c0250911afd803d8757c6d55ad59e51b2d40140cc02473155492adcd5027380d195d8271be94dc7b10c5532b81441452762376df4a12d346d8185f4c74cc1c3c9396c4807c95291f7debd330b2d022361ec78eb00000000";

TEST_CASE("parse")
{
    auto rune_tx_hex = GENERATE(rune_tx1_hex, rune_tx2_hex, rune_tx3_hex, rune_tx4_hex, rune_tx5_hex);

    std::optional<RuneStone> runeStone = ParseRuneStone(rune_tx_hex, w->chain());

    REQUIRE(runeStone.has_value());

    std::string spaced_rune_text;
    REQUIRE_NOTHROW(spaced_rune_text = AddSpaces(DecodeRune(runeStone->rune.value_or(uint128_t())), runeStone->spacers.value_or(0), " "));

    std::clog << spaced_rune_text << std::endl;

    uint32_t spacers;
    std::string rune_text;

    REQUIRE_NOTHROW(rune_text = ExtractSpaces(spaced_rune_text, spacers, " "));

    CHECK(EncodeRune(rune_text) == runeStone->rune.value_or(uint128_t()));
    CHECK(spacers == *runeStone->spacers);
}


struct readrunetest
{
    uint128_t rune;
    const char* text;
};

TEST_CASE("encode")
{
    auto testval = GENERATE(
            readrunetest{uint128_t(0), "A"},
            readrunetest{uint128_t(1), "B"},
            readrunetest{uint128_t(25), "Z"},
            readrunetest{uint128_t(26), "AA"},
            readrunetest{uint128_t(51), "AZ"},
            readrunetest{uint128_t(52), "BA"},
            readrunetest{uint128_t(702), "AAA"},
            readrunetest{std::numeric_limits<uint128_t>::max(), "BCGDENLQRQWDSLRUGSNLBTMFIJAV"}
        );

    SECTION("Decode") {
        std::string rune_text = DecodeRune(testval.rune);
        CHECK(rune_text == testval.text);
    }

    SECTION("Encode") {
        uint128_t rune = EncodeRune(testval.text);
        CHECK(rune == testval.rune);
    }
}

struct varinttest
{
    uint128_t value;
    bytevector varint;
};

TEST_CASE("varint")
{
    auto testval = GENERATE(
            varinttest{0, {0x0u}},
            varinttest{1, {0x1u}},
            varinttest{127, {0x7fu}},
            varinttest{128, {0x80u, 0x00u}},
            varinttest{255, {0x80u, 0x7fu}},
            varinttest{256, {0x81u, 0x00u}},
            varinttest{65535, {0x82u, 0xfeu, 0x7fu}},
            varinttest{uint128_t(1u) << 32, {0x8Eu, 0xFEu, 0xFEu, 0xFFu, 0x00u}}
        );

    SECTION("read") {
        bytevector::const_iterator i = testval.varint.begin();
        uint128_t res = read_varint(i, testval.varint.cend());

        CHECK(res == testval.value);
    }

    SECTION("write") {
        bytevector res = write_varint(testval.value);
        CHECK(res == testval.varint);
    }
}
