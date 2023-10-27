#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "nlohmann/json.hpp"

#include "util/translation.h"

#include "inscription_common.hpp"
#include "inscription.hpp"

#include "test_case_wrapper.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace utxord;

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

    w = std::make_unique<TestcaseWrapper>(configpath, "bitcoin-cli");

    return session.run();
}

static const std::string txid_text = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";

TEST_CASE("listtags")
{
    std::string txid = "c080b1cfac56310be20a3b13237192a8403f7615a0cbd92be65e0efe6243aa80";

    CTransaction tx = w->btc().GetTx(txid);

    if (tx.vin[0].scriptWitness.stack.size() < 3) throw InscriptionFormatError("no witness script");

    const auto &witness_stack = tx.vin[0].scriptWitness.stack;
    CScript script(witness_stack[witness_stack.size() - 2].begin(), witness_stack[witness_stack.size() - 2].end());

    auto tagged_data = ParseEnvelopeScript(script);

    bool text = false;

    bytevector metadata;

    std::clog << "Inscription: " << txid << " =====================================\n";
    for (const auto &tag_value: tagged_data) {

        if (tag_value.first == CONTENT_TYPE_TAG) {
            std::string val = std::string(tag_value.second.begin(), tag_value.second.end());
            text = val.find("text") != std::string::npos;
        }

        if (tag_value.first == CONTENT_TAG && !text) {
            std::clog << hex(tag_value.first) << " -- " << hex(tag_value.second) << "\n";
        }
        else if (tag_value.first == METADATA_TAG) {
            metadata.insert(metadata.end(), tag_value.second.begin(), tag_value.second.end());
        }
        else {
            std::clog << hex(tag_value.first) << " -- " << hex(tag_value.second) <<
                      " [[" << std::string(tag_value.second.begin(), tag_value.second.end()) << "]]\n";
        }

    }

    if (!metadata.empty()) {
        auto metadataobj = nlohmann::json::from_cbor(metadata);
        std::clog << "metadata:\n" << metadataobj.dump() << std::endl;

    }

    std::clog << std::endl;
}

TEST_CASE("parsebatch")
{
    const std::string txid = "9842f9ab1adf3870da7ebd695082c208364cd784490d09399a366dfef7498338";

    CTransaction tx = w->btc().GetTx(txid);

        for (uint32_t i = 0; i<64; ++i) {
        Inscription inscr(tx, i);

        std::clog << inscr.GetIscriptionId() << std::endl;

        CHECK(inscr.GetIscriptionId() == (txid + 'i' + std::to_string(i)));

    }
}

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
