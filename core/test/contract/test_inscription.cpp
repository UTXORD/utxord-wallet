#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "util/translation.h"

#include "inscription_common.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace l15::utxord;

static const std::string txid_text = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";

static const uint256 txid_sample = uint256S("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");

TEST_CASE("inscriptionid")
{
    auto test_in = GENERATE(0, 1, 5);

    std::string test_id = txid_text + 'i' + std::to_string(test_in);

    bytevector bin = SerializeInscriptionId(test_id);

    bytevector etalon = unhex<bytevector>("ffeeddccbbaa99887766554433221100ffeeddccbbaa99887766554433221100");
    if (test_in) {
        etalon.push_back(test_in);
    }
    CHECK(bin == etalon);

    std::string id = DeserializeInscriptionId(bin);

    CHECK(test_id == id);
}

TEST_CASE("textinscriptionid")
{
    auto test_id = GENERATE(txid_text+"i0", txid_text + "i1", txid_text + "i5");

    bytevector text_bin(test_id.begin(), test_id.end());

    std::string id = DeserializeInscriptionId(text_bin);

    CHECK(test_id == id);
}
