#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <tuple>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"
#include "config.hpp"
#include "nodehelper.hpp"
#include "chain_api.hpp"
#include "exechelper.hpp"
#include "inscription.hpp"

#include "contract_builder.hpp"

#include "policy/policy.h"

//#include "test_case_wrapper.hpp"
#include "univalue.h"
#include "script_merkle_tree.hpp"

using namespace l15;
using namespace l15::core;
using namespace utxord;
using std::get;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;


//std::unique_ptr<TestcaseWrapper> w;


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
//
//    w = std::make_unique<TestcaseWrapper>(configpath);

    return session.run();
}

//TEST_CASE("svgscript")
//{
//    std::string raw_tx ="02000000000103d7c0da6cf9f8c338d9066502ba68826bca8a23a3db9b9dc2dd099cc22069195f0000000000ffffffffe85634756abcc7b86e05842a2558feb2b74e8c76f5647e6e55ea99a3a22b08070000000000ffffffffd7c0da6cf9f8c338d9066502ba68826bca8a23a3db9b9dc2dd099cc22069195f0100000000ffffffff02b80b000000000000225120710a783cda4379b04320faa434c4cd22b379f926ba2f3a9459c070c3e41f5570b00f000000000000225120b0400e3a141f65a94e16a3c5be17949e82fe37136b6d1505389a063b07a8fb5e034065089e2582831c5143cca03fd053f179792a3e7c0e9708d72f08389a8b60796d03e474b8671057806e322c5bb8370617e023ea04b805a63f693dfa81268a388ffddc0620c23bcccb3a1829c5a637ac505c02ba5f1d7b221d155f9942196b0ff74ded2bb8ac0063036f726401010d696d6167652f7376672b786d6c004d08023c7376672077696474683d22313822206865696768743d223136222076696577426f783d22302030203138203136222066696c6c3d226e6f6e652220786d6c6e733d22687474703a2f2f7777772e77332e6f72672f323030302f737667223e0a3c706174682066696c6c2d72756c653d226576656e6f64642220636c69702d72756c653d226576656e6f64642220643d224d352e303431363720312e373439363743332e333230393220312e373439363720312e393136363720332e313535323820312e393136363720342e393030373243312e393136363720362e3030393320322e353637303920372e313532363320332e353933333620382e333430323243352e32363534352031302e3237353120372e32303136332031322e3239373120392031332e393536394331302e373938342031322e323937312031322e373334352031302e323735312031342e3430363620382e33343032324331352e3433323920372e31353236332031362e3038333320362e303039332031362e3038333320342e39303037324331362e3038333320332e31353532382031342e3637393120312e37343936372031322e3935383320312e37343936374331322e3231343820312e37343936372031312e3533333520322e30313034362031302e3939363420322e34343738384331302e3432393820322e39303934332031302e31333220332e32343130334d080220392e393636343820332e343434323443392e393233373720332e343936363820392e383837373520332e353433313820392e383534373920332e353836323843392e383439303520332e353933373820392e383432383620332e3630313920392e383336333820332e3631303443392e383130353620332e363434323920392e373739393820332e363834343120392e373533373620332e373137303743392e373235343520332e373532333220392e363631343320332e383331353320392e353734393720332e393033373243392e343837383420332e393736343720392e323739363520342e3132353520382e393734333720342e3131393543382e363732373820342e313133353720382e343732333920332e3935393720382e333931343220332e383838363743382e333039323220332e383136353620382e32343820332e373338323520382e323230383220332e373033323943382e313737353620332e363437363620382e313534353220332e363136313320382e313332373720332e353836333943382e3130343820332e353438313320382e303738393820332e353132383120382e303135303720332e343332393143372e383534313220332e323331363920372e353632353720322e3930333220372e303033353720322e343437383843362e343636353420322e303130343620352e373835313620312e3734393637204d0802352e303431363720312e37343936375a4d302e323520342e393030373243302e323520322e323435313320322e333930313620302e3038333030373820352e303431363720302e3038333030373843362e313833343820302e3038333030373820372e323333343820302e34383535373520382e303536313320312e313535363443382e343639353820312e3439323420382e373731383320312e373831383920382e393937363420322e303233373243392e3232343920312e373833303520392e353238383120312e343933373220392e393433383720312e31353536344331302e3736363520302e3438353537352031312e3831363520302e303833303037382031322e3935383320302e303833303037384331352e3630393820302e303833303037382031372e373520322e32343531332031372e373520342e39303037324331372e373520362e36333639322031362e3735333320382e31373336362031352e3636373720392e34323939364331332e383730312031312e353130312031312e373730352031332e3639333120392e38333730382031352e3435303243392e33363231382031352e3838313920382e36333738322031352e3838313920382e31363239322031352e3435303243362e32323934382031332e3639333120342e31323938382031312e3531303120322e333332333220392e343239393643312e32343636373b20382e313733363620302e323520362e363336393220302e323520342e39303037325a222066696c6c3d22626c61636b222f3e0a3c2f7376673e0a0102423037303832626132613339396561353536653765363466353736386334656237623266653538323532613834303536656238633762633661373533343536653869306821c1b9a1c51c78599e5e91c8706b4fa6a0ac38a6cc94f1b9d965b0e6976352aef577014021810f8835d636ed2cb0dc686a25617994d1d765c5659538d978955a66ba7a574fcf0f66699b095b069d74b7e6aa2bd9f24a14c7cecb868bc4882415bc8be07501408b5fe2e36519eaa13207cf02a22737a92d25f7a6d559b4b701ae3cd0ea7ea59ba1258f133462a07e782093593046897bdd1462fc99356a5525101c5322b0034f00000000";
//
//    auto inscriptions = ParseInscriptions(raw_tx);
//
//    std::clog << hex(inscriptions.front().GetContent()) << std::endl;
//    std::clog << "===========================================================\n"
//              << inscriptions.front().GetCollectionId() << std::endl;
//}

TEST_CASE("parse")
{
    std::string raw_tx ="010000000001014f414ef59996cf68d0f7952a89241d7aebd99aa53c474cfb31d7b9d8350fcfc10000000000fdffffff01102700000000000022512032cc8b416c04cf857bc8eb891da2403816d3e12e26a0b51e7780ef362fb4c673034065eba88bc56b942a25f0d92fb59cda2fc378be37afb62a66273903a2459de93626466a50a454ff240f390814fb5872ca27c3f181aede6b6092949048ac5d0641fdef01204635b7b5fdc55b8c072892447bfbf9893f81d937f0c2d41e73f1d400cdfd38d6ac0063036f7264010109696d6167652f706e67004db60189504e470d0a1a0a0000000d494844520000002a0000002c0403000000e1b4377500000015504c5445ff9000c2a633edc348000000fbeeca9c7b33ffffff7024a5bc00000028655849664d4d002a000000080001010d00020000000e0000001a000000006464646f6f6f6767676765656500a8076e270000012849444154789c7dd3ed9184200c066067a8c0a18320ff95d7b500a5023d1bb8ebbf874bc2c7e2ea1dc3cc8e8f21b03174ddbfc300186f36f57d6faf6ec464d8960b32bf11fc585ea0ae9718a791fc6e6cd6db4334b89ac368d4ceefac971563ab0e088d6a824032546ddd2b2191ee39662d487eca2abf01a1519bd4ba9f6f72b86ba06519e27ad1c8610b8618fd9154369ba32665a5231d4275c32cf6a0f1a611f1496925653aa736365090c4e5cf6919220112bc5e94f2c96a21f4f3d6eaa8dabf34157d1f8893e6bcb90efdeb6b1b30f3ac0afdb638ce8327497950d40b39724459793b3e99733a45b5d70c3e3435e64d739b5e3437ab816adc78b296ce96ca388f174fe232d5f60fe4e9dc65baf725406912c6dad5c2ea6c0deab54ae37a899eb07bbe851fe31723a762c2083a61870000000049454e44ae4260826821c14635b7b5fdc55b8c072892447bfbf9893f81d937f0c2d41e73f1d400cdfd38d600000000";

    auto inscriptions = ParseInscriptions(raw_tx);

    std::clog << inscriptions.front().GetIscriptionId() << std::endl;
    std::clog << inscriptions.front().GetContentType() << std::endl;
    std::clog << hex(inscriptions.front().GetContent()) << std::endl;
    std::clog << "===========================================================\n"
              << "collection: " << inscriptions.front().GetCollectionId() << std::endl;
}


TEST_CASE("Fee")
{
    CMutableTransaction tx;

    CAmount base_fee = CalculateTxFee(1000, tx);

    std::clog << "Base tx vsize (no vin/vout): " << base_fee << std::endl;

    tx.vin.emplace_back(CTxIn());
    tx.vin.back().scriptWitness.stack.emplace_back(64);
    tx.vout.emplace_back(0, CScript() << 1 << xonly_pubkey());

    CAmount min_fee = CalculateTxFee(1000, tx);

    std::clog << "Mininal taproot tx vsize: " << min_fee << std::endl;

    tx.vin.emplace_back(CTxIn());
    tx.vin.back().scriptWitness.stack.emplace_back(64);

    CAmount double_vin_fee = CalculateTxFee(1000, tx);
    std::clog << "Key spend path taproot vin vsize: " << (double_vin_fee - min_fee) << std::endl;

    tx.vout.emplace_back(0, CScript() << 1 << xonly_pubkey());

    CAmount double_vout_fee = CalculateTxFee(1000, tx);
    std::clog << "Taproot vout vsize: " << (double_vout_fee - double_vin_fee) << std::endl;

    SchnorrKeyPair key;
    l15::ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED, { IContractBuilder::MakeMultiSigScript(xonly_pubkey(), xonly_pubkey()) });
    auto tr = core::SchnorrKeyPair::AddTapTweak(key.Secp256k1Context(), key.GetPubKey(), tap_tree.CalculateRoot());

    std::vector<uint256> scriptpath = tap_tree.CalculateScriptPath(tap_tree.GetScripts().front());
    bytevector control_block;
    control_block.reserve(1 + xonly_pubkey().size() + scriptpath.size() * uint256::size());
    control_block.emplace_back(static_cast<uint8_t>(0xc0 | get<1>(tr)));
    control_block.insert(control_block.end(), key.GetPubKey().begin(), key.GetPubKey().end());

    for (uint256 &branch_hash: scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());

    tx.vin.emplace_back(CTxIn());
    tx.vin.back().scriptWitness.stack.emplace_back(signature());
    tx.vin.back().scriptWitness.stack.emplace_back(signature());
    tx.vin.back().scriptWitness.stack.emplace_back(tap_tree.GetScripts().front().begin(), tap_tree.GetScripts().front().end());
    tx.vin.back().scriptWitness.stack.emplace_back(move(control_block));

    CAmount msig_vin_fee = CalculateTxFee(1000, tx);
    std::clog << "Taproot multi-sig vin vsize: " << (msig_vin_fee - double_vout_fee) << std::endl;


    CMutableTransaction p2wpkhtx;
    p2wpkhtx.vin.emplace_back(CTxIn());
    p2wpkhtx.vin.back().scriptWitness.stack.emplace_back(unhex<bytevector>("30440220744cd353daa4c84042229bfdb5f95f4e374fe32f43f16c984f82ab11f0cfa5b1022018d50cdd78bc1cf9e9b8f6119188972934ace1bbf67b581bf9b64663dbd8e04201"));
    p2wpkhtx.vin.back().scriptWitness.stack.emplace_back(unhex<bytevector>("025d4d8d0b078bb360a50682e39bd6cca383f13f620262c90e4b329b41e92a283d"));

    CAmount p2wpkh_fee = CalculateTxFee(1000, p2wpkhtx);

    std::clog << "P2WPKH vin vsize: " << (p2wpkh_fee - base_fee) << std::endl;

}


TEST_CASE("DeserializeContractAmount")
{
    const char* json = R"({"num_amount":1000, "str_amount":"0.00001", "wrong_amount":"wrong"})";
    UniValue val;
    val.read(json);

    std::optional<CAmount> from_num;
    std::optional<CAmount> from_str;
    std::optional<CAmount> another_amount = -1;

    CHECK_NOTHROW(IContractBuilder::DeserializeContractAmount(val["not_exist"], from_num, [](){ return "not_exist"; }));
    CHECK(!from_num.has_value());

    CHECK_NOTHROW(IContractBuilder::DeserializeContractAmount(val["num_amount"], from_num, [](){ return "num_amount"; }));
    CHECK(*from_num == 1000);
    CHECK_NOTHROW(IContractBuilder::DeserializeContractAmount(val["num_amount"], from_num, [](){ return "num_amount"; }));
    CHECK(*from_num == 1000);
    CHECK_THROWS_AS(IContractBuilder::DeserializeContractAmount(val["num_amount"], another_amount, [](){ return "num_amount"; }), ContractTermMismatch);
    CHECK(*another_amount == -1);

    CHECK_NOTHROW(IContractBuilder::DeserializeContractAmount(val["str_amount"], from_str, [](){ return "str_amount"; }));
    CHECK(*from_str == 1000);
    CHECK_NOTHROW(IContractBuilder::DeserializeContractAmount(val["str_amount"], from_str, [](){ return "str_amount"; }));
    CHECK(*from_str == 1000);
    CHECK_THROWS_AS(IContractBuilder::DeserializeContractAmount(val["str_amount"], another_amount, [](){ return "str_amount"; }), ContractTermMismatch);
    CHECK(*another_amount == -1);

    CHECK_THROWS_AS(IContractBuilder::DeserializeContractAmount(val["wrong_amount"], another_amount, [](){ return "wrong_amount"; }), ContractTermWrongValue);
    CHECK(*another_amount == -1);
}

TEST_CASE("DeserializeContractString")
{
    const char* json = R"({"str":"test", "num":0.00001, "json":"{\"key\":\"value\"}"})";
    UniValue val;
    val.read(json);

    std::optional<std::string> raw_str;
    std::optional<std::string> raw_json;
    std::optional<std::string> another_str = "bla";

    CHECK_NOTHROW(IContractBuilder::DeserializeContractString(val["not_exist"], raw_str, [](){ return "not_exist"; }));
    CHECK(!raw_str.has_value());

    CHECK_NOTHROW(IContractBuilder::DeserializeContractString(val["str"], raw_str, [](){ return "str"; }));
    CHECK(*raw_str == "test");
    CHECK_NOTHROW(IContractBuilder::DeserializeContractString(val["str"], raw_str, [](){ return "str"; }));
    CHECK(*raw_str == "test");
    CHECK_THROWS_AS(IContractBuilder::DeserializeContractString(val["json"], raw_str, [](){ return "json"; }), ContractTermMismatch);
    CHECK(*raw_str == "test");

    CHECK_NOTHROW(IContractBuilder::DeserializeContractString(val["json"], raw_json, [](){ return "json"; }));
    CHECK(*raw_json == "{\"key\":\"value\"}");
    CHECK_NOTHROW(IContractBuilder::DeserializeContractString(val["json"], raw_json, [](){ return "json"; }));
    CHECK(*raw_json == "{\"key\":\"value\"}");

    UniValue json_val;
    CHECK(json_val.read(*raw_json));
    CHECK(json_val["key"].get_str() == "value");
}

TEST_CASE("DeserializeContractHexData")
{
    const char* json = R"({"pubkey":"f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4","longpubkey":"f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa49988","sig":"f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4"})";

    UniValue val;
    CHECK(val.read(json));

    std::optional<xonly_pubkey> pk;
    CHECK_NOTHROW(IContractBuilder::DeserializeContractHexData(val["pubkey"], pk, [](){ return "pubkey"; }));
    CHECK(pk.has_value());
    CHECK(hex(*pk) == "f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4");
    CHECK_NOTHROW(IContractBuilder::DeserializeContractHexData(val["pubkey"], pk, [](){ return "pubkey"; }));
    CHECK(hex(*pk) == "f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4");

    std::optional<xonly_pubkey> long_pk;
    CHECK_THROWS_AS(IContractBuilder::DeserializeContractHexData(val["longpubkey"], long_pk, [](){ return "longpubkey"; }), ContractTermWrongValue);
    CHECK(!long_pk.has_value());

    std::optional<signature> sig;
    CHECK_NOTHROW(IContractBuilder::DeserializeContractHexData(val["sig"], sig, [](){ return "sig"; }));
    CHECK(sig.has_value());
    CHECK(hex(*sig) == "f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4f4bd18cdaa7c9212143b9ff0547e3b1f81379219dcbbe3cbb9743688e0a4daa4");
}


static const std::string p2tr_addr = "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak";
static const std::string p2wpkh_addr = "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97";
static const std::string p2pkh_addr = "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe";
static const std::string p2sh_addr = "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf";

TEST_CASE("p2address")
{
    auto [addr, type] = GENERATE_REF(std::make_tuple(p2tr_addr, P2TR::type),
                                     std::make_tuple(p2wpkh_addr, P2WPKH::type),
                                     std::make_tuple(p2pkh_addr, P2PKH::type),
                                     std::make_tuple(p2sh_addr, P2SH::type));

    auto p2tr = utxord::P2Address::Construct(MAINNET, 546, addr);

    CHECK(p2tr->Type() == type);
    CHECK(p2tr->Amount() == 546);
    CHECK(p2tr->Address() == addr);
}

TEST_CASE("dust_limit")
{
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 546, p2tr_addr));
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 545, p2tr_addr));
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 330, p2tr_addr));
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 329, p2tr_addr), ContractTermWrongValue);

    CHECK_NOTHROW(P2Address::Construct(MAINNET, 546, p2wpkh_addr));
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 545, p2wpkh_addr));
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 294, p2wpkh_addr));
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 293, p2wpkh_addr), ContractTermWrongValue);

    CHECK_NOTHROW(P2Address::Construct(MAINNET, 546, p2pkh_addr));
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 545, p2pkh_addr), ContractTermWrongValue);
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 330, p2pkh_addr), ContractTermWrongValue);
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 329, p2pkh_addr), ContractTermWrongValue);

    CHECK_NOTHROW(P2Address::Construct(MAINNET, 545, p2sh_addr));
    CHECK_NOTHROW(P2Address::Construct(MAINNET, 540, p2sh_addr));
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 539, p2sh_addr), ContractTermWrongValue);
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 330, p2sh_addr), ContractTermWrongValue);
    CHECK_THROWS_AS(P2Address::Construct(MAINNET, 329, p2sh_addr), ContractTermWrongValue);
}

TEST_CASE("p2address")
{
    std::string p2tr_addr = "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak";
    std::string p2wpkh_addr = "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97";
    std::string p2pkh_addr = "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe";
    std::string p2sh_addr = "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf";

    auto [addr, type] = GENERATE_REF(std::make_tuple(p2tr_addr, P2TR::type),
                                     std::make_tuple(p2wpkh_addr, P2WPKH::type),
                                     std::make_tuple(p2pkh_addr, P2PKH::type),
                                     std::make_tuple(p2sh_addr, P2SH::type));

    auto p2tr = utxord::P2Address::Construct(MAINNET, 546, addr);

    CHECK(p2tr->Type() == type);
    CHECK(p2tr->Amount() == 546);
    CHECK(p2tr->Address() == addr);
}

