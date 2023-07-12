#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "util/translation.h"

#include "inscription_common.hpp"
#include "inscription.hpp"

#include "test_case_wrapper.hpp"

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

using namespace l15;
using namespace l15::utxord;

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

//static const uint256 txid_sample = uint256S("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");
//static const std::string txids[] = {
//        "fca48f7c79a947800f4aad96d7f8530901734953f3a73e6f28347ef9a2c341c5"
//};
//        "8f3e642289eda5d79c3212b7c5cd990a81bbeed8e768a28400a79b090adb3166",
//        "0be0174fe9603f6e6bae628d35efb6a0ce413538287ca469d198a32f0ae226b7",
//        "d4db73ed430c2e37ac1990cb37ee0be80a042f2152af605678b412eb0224bb6a",
//        "ed9700425df3fe0bfb3119090ce1b02930a3d237a2e1f190db2b06b288fc6569",
//        "98f842c6f2e16ceb7f1cff406bfabcfbd2a5b2529acf4f39d09aa16bae2891f7",
//        "a5f9edaff5472a73230b392f137d5b0d2deb05e8d6e867c1e7b68b6d10e7110a",
//        "4b84dc166254e72ca1ff77dc9e0602ec919137934e2c737c9763c23c5fa02d1d",
//        "0074143666a6afebe765853d9570029c05857ade3a7c14159448957156dc9c79",
//        "55e7d02d5a0e2bff72c100a6b068a27c821bf3f4a5fe961d0cfbf854ccc7f54b",
//        "e8b76647a5586e19f54f4dd575006aef5ffc8979ce094a107eae1a3d39118fe5",
//        "c68190a56e47359d1224013dd9474e9ed4f7bb9a6d9906ff80ba8409002287fa",
//        "b02e105dbf86280c60e64fca8d50bdeea14f2368ee9bd8647e41189bcf5e2394",
//        "9e0d0a3dcacbc570ac69aa6b52be1bdc9a3cdcbb2ac411593bf3c0cff7d10fef",
//        "bcdb80f550e76da18612fa15d87a55580e772e2d939e6d3f66da1f03c03446eb",
//        "c4566dc6148d9ff28dd8a617e5a3e07e3bf237f380b0210337277336b76cf5de",
//        "ab25e70ffb4f561d863133f46f878565d0c526983ca70139ebfb8420eb90676a",
//        "aea82208fe5365e49e65454e771f3065b604d1de5e9f53afae29ea2d36734db0",
//        "a554fc88282b907e28aab3fda93f017989fefee111d9f5e09ab1b507dd4c8a04",
//        "d435315308c20952802352a24ef2c1a7c54f2e1c0233acf5e29c2b47ecf23daa",
//        "8d3ac1d83b661c6b42b984cd7e93293cecaff4594d25d399f84522adf4ba4e13",
//        "821788963f83926b123f048fcaa4c2dae78a41cc4bbfb60373aff15bc71539f1",
//        "42dcbeb126bb8f4b1264775630940cd34d88d08c3c372a0685de230bd8265d60",
//        "66fab1ed875ac28e32decaa2d4abf0de50ea913b235a4cb4e1c60908f2af27d2",
//        "aa0265e61b0e609027bb2393d61e6f3a6030b32e82a8ca75b0c827dadfca5202",
//        "e998cd4ec8ce5bbede903a50722310547380af949b532e0f6f4432eece7325da",
//        "75df937d8a9fec3f865d973f868aab84c7d3de7b83431bbbdc9ecc3d0ebc2b2c",
//        "2fb2fd13f60041413f3ae63177c251c9176cd0d0635cf8bcc56550ecf66f8494",
//        "7331f9bbed073f987dbf1967f0ce58970c50a4acb7cc5ac033db84ee1f76e600",
//        "6bd1eb5cc60a807ac34087d40b9ec8c53bd2f3236c80ba33a3811d23ec0075bd",
//        "88170866d782d021b53671c5214288bb1aa627f9da78e2fb8e44f0babf9fb018",
//        "11ef23e2311fc204c483c21ca0f53aad46d94cc708be1ce21066f360ef6e7958",
//        "5982f4f3867affc8db2b20f3eb835269101e3d223f4162a46b7dc39897ff1be0",
//        "636dc4ab0a181fb1b40fe0ee6ffd7bdc7d713ae0054a229e8b1feff46fd55723",
//        "1333a2e5868386c8ad94561b8e10c6800bc7e51c9a07ffcb7902dba45831e29c",
//        "6bc218d915bbba60b762cc842d96244f4c845c5760ce63004a2f7032ff2bee99",
//        "3aff8b929b537c725cebef9d3cba63b8d8a8864b6cd175f00635564130847592",
//        "3aba6cc7b18bf71253020c3474e3b8591dd78ea2b0c14c69c7a79069aaa64d37",
//        "4d73e1dfa10a823fc9841fe770d372d26262b45c80530c6cfd30eeb28bb433c9",
//        "8df0426fad1eb4436a7da881e39606d79323f78ec5c2e9039bf04c2c925f8ece",
//        "e318cc4b253e2170a9bc07bd6476a76f994012ccabb91c838eba13adf5e3fbc1",
//        "a0fc670979c7a2564f0b22b9a255ef943df1af4e96293baac0088f5550ec0fd6",
//        "fa7a961c9766360cdfdbd6b89d99ff8351d56c56c478b433ac3032702762a233",
//        "5478b3b4577000871bb2863d2bb048015fba4ab75910c0d8c81f93d405e4c60e",
//        "7d8a728e7a42412f68ef5c40ade1a1976b11a825252c9f380f9dd77126d7f839",
//        "ea6936b5d61ec76c556e88ab325dd3e4e302662980e3a3465fc37c03c9dd4827",
//        "02d50a30e7e92dfb380187174fd17cc9079aecfc399728f4086b8ec8f0b7fe17",
//        "e77c2fbec4640430cab6e22756c92f84f5673bce17ab2b31b88912e00bcfc501",
//        "8dc7aa6c0f516bfa094d560daf108c88696e33568664548948d6789905455282",
//        "6c6e291709c25e752b0e59366dc9542e4e1d675793071f7fea86be0b46cb0775",
//        "a9befc1374f93779e566d7bc5fc268a7e09cd9430640f64cf742f13fb87ac481",
//        "65a1b2bb49e2e8c2619cbef0b241367de7060a2839fd23f20260bbb9c7649a88",
//        "6079f243b8b14e632b7729b78a3bdcf6643fe7a5c7bdde01687c8e2236028e2c",
//        "a256ea0f0686b4780b0d8a07a5ed617dce724b1d37b35830d7e881a886ec8b2b",
//        "c93bdf2a2ac25cc725db0b9673a629b4fcd4431bfe10ac87448e5b9ebfbafdff",
//        "019ae1e71d299769d5e6048f842274d500156da4e10520af14b9640b396311a7",
//        "77006382cd30d774a78b4660c6e37b5f7aa95cab6b05cf0f070004c034bc36fe",
//        "d9684a6ec65b45e91a1f5989958bd7148d04cd8dd013849c2e80ebbc66e9fcab",
//        "d4bcf73adaef590bba1be494b67eebef9b0bd0d668086500402205e0b238357c",
//        "1321d4cb7edc52682497141d988539b461dd5aecb52adf7eb2819b90bc1f8d93",
//        "c67af500dbe106335fcdf6caf7c72ac9d811d81dacd5b9cb5229b43db9dc4cb2",
//        "a258fa86e2277b18954164aa3085147ed779d7c4533376740f5b9cef11f9cfb7",
//        "43386c448d525457a05a33c770a294fdbc0296d378aa803b160b3315e9288529",
//        "0d053ba2543c2f0883ece4b17ca860f8595ea8ac35c132d2e42b8dc3d9a98381",
//        "c3b64c5a7217a546178899962f0064fb49331bb6243e376896d9a2033ed5b38a",
//        "d341f3799e6ca24c419f9a18377a1d1583e6b3de7d3266e477f15957305be401",
//        "c8a991de0341a5865f74fc389d74d9568a2a5ad2964019e4462b8ec437505515",
//        "79f14f5467c5ed92b28d44ed8c5469239cbe89ee88534b71e668f829303c5501",
//        "621a6d6d382387b6e7bd70b72b92d556ce7f270ae7d26b57a3403dcf3c24e2e1",
//        "c1b22c0edb4476c753c230fb7804b4568261f6328d4d1aa34313160a637e2b40",
//        "89a078660b53527e170b9adaa5520c98b7e90cf3e0bcf6ae6ee0324f7ce4e320",
//        "751a2baa92777a0458796a835dc856e77bb55a9ef978db83915d6c4aa59d9379",
//        "bb8ffded44aafcbb7a775033238c861d8d461c44c7d1c3b206c4f9016d6657a8",
//        "aefb3022e8d74000293f685e7f38039e68e3358a809146fab5f1797d4e5ee1f0",
//        "672db83dc896364d7180f8c71a1816b122d4016c0d50478ddbace363b09dabcf",
//        "c35f0635ef5455c34f1590e4963ff0c772458465d3cf990116d563cb4af0f4e8",
//        "1853cad718449fc5c2ffda856912bc7b0e71f8238fcff2b2a59a01a4c1c112e1",
//        "c808749bd73724f71ddf938571cb926b5cb43e4f217588e83fdea4d87cb063ee",
//        "55f7318046c23ab95fcab1a43b7bbe1bbe44b013d592ed180d3eb5ada0d734ff",
//        "8f4ce60dbc8f82a628268b74b6465b7947f10e8288a113753fca80cfc53cc5d3",
//        "ef4e142da4e29296f0ea3b9104f5e22a38b838419a641e52f103fb9ca8ca38a8",
//        "b70d967abb0fd98db906b74201ca08c341c4a02913c4047bcd0013e86a9de014",
//        "4c57442ab512ec4ace17f8ac7720cb72f56af0a544a14c2e6fb0ec851524141e",
//        "e303d5a619a1682282737082c25c6b0df8eede536f904ea48164cdb26053d912",
//        "9fc38ca2c4745cfd224a3ca1d89ae4d34cc2fa83e46016c33c6819748b53e1e2",
//        "a886b97d239ef83a4f890ae3b2ade15b47f1e4217fc3b1238e2eb4ab01a7037c",
//        "401194b561284bcdd29a34d7f70094473237141bd8beccd3b32e5ba91e17aaa8",
//        "09612feb9929c191d7e66cf30451c21a64707353997efe13bcd5840dea86cd5b",
//        "ca70ee980690792b2bb2a1b4dcb4910260d2947cb646c3282a61132fbf73a63c",
//        "a781a793ff1de32ec01569a8085b343c653ef2ce1029ee5ba347bac7c505ea25",
//        "1e130fa90c44961d3fec458f1a8ecba9f26fb5f4dd89a821c005c8fa609ae811",
//        "ebe2bd79048afb762a02f3f21e615e441d91a0bb0fcb08611bb2130f8612697e",
//        "a8b264f6b7536762ef70e71c3e850c2e952e30c17730e5234c112abf6da50fb6",
//        "ed3d0933477be8221b744684350f78e2e51a77ff4bc7197ae01c78e786a6718a",
//        "3be3e8cc21ec085e861266a65b4e6a2884bbc04aa78e6eb3328e792a2f52c62b",
//        "79f6703acc34b324fb625093567070d26d53c69682d9eeb1296a9892467c880d",
//        "b1b4edcecee5aaa8ae5631aaa06bf8594bf8008571c9021c7b8cfc4bdc585c3e",
//        "a99a3df1d1821a3caed80cf6f8bb1c6f8725680a70d6b85850e018b063fee8c2",
//        "b1ec3f8054d024ae1edb2de6936f49982ce3273056047c979bd18af3f9dc6cb5",
//        "4acd9b62d578cc656ad9c5e71f4f0b5e3c3b05149102798de98e6303324cfa60",
//        "4936ab7a37c70d1f1a0de6bf698bfe2552853b40ad5da645d8f957d1d846569f",
//        "269f8f376953a2644721d538569b7c6f35497c44ef310ff9faf6803f8e137376",
//        "fd16ef38a7eabee1b69c761b29b433b381becfc0469172953c50b1be72cb0e75",
//        "be7c0de84b6cbff5304ff589823e597c66898b14a7f2d3a5629f5405d0827bdf",
//        "011876d54f2b8ef79aee1566fde47e6fc8b5c7a18b4fe84dcd9ca6d9918bf3ab",
//        "82387692c103432d017de778281941f3846406bb4e6fa61894e0c45ba60dd2b5",
//        "2b608630b0d049250beaea5d743191d93228d6db84d3d189e4bf3b20b5d8b76c",
//        "a33293ad001eb26ccc05bace4874422c8796b3da0f2c5e5811d6c15e11dc01e6",
//        "6bfb4b3953f6921d76908231d1d193d8a915feb227350f494402194e035bcca7",
//        "eadfb4ecb5f0758b5e6fd3e53aac27b1e2b1ec28b9d5c710c8d8416db1ca394a",
//        "87c7086449f46a7fc38402277bf2fcf5a9d4a3c84547fcd80592518dd7b0cd0d",
//        "23e0fe730884b7f4ccc3780c4adf7701b5fcd899947a1a4f6bf051053aa10e49",
//        "adeb1761906e189d6ce64400adc26cdcc813dd8aaaa0b330f1a2be670105f667",
//        "b1b320449674ee59d059b2c1e062173ee0432fe438c907c54fd52818af9a90d2",
//        "4c6873c40c934743e835235c1c557ca28a9c4618b00151335ab2858c7f6f01ef",
//        "eaec9871210aaefbc9bc7db513804149f55bd5e7fec83612083b0028cf97c40a",
//        "079b95f0570067cfbdcd7c9d0cd05c8fc039c688cdef106830d5e688bb6b3171",
//        "a897452508fc5d9c0ef4290d0b864362e7ba02a73d7495781ef84f07ff28fe72",
//        "295b377e28b2bcf0b62c4e6bc647e5e05b93966c246985b940e31f359f1b9c67",
//        "13d95a8306c8f651aef4f60f9a6cacb2e2020698aad184c61328d2f0f6372bef",
//        "bc22ba231b110ea99945ccc5002ea14ec674b8ad11ba92a0b2df0028ca9acc21",
//        "1b9e9a3a919bcc32b3c2cb28746ad0a98bc691c03e22461c8b6ea5289da0d724",
//        "4e3c1937eb43d8b4a4ee341c2a4ddbaabcabc461d66475d285a367e6ad67f13f",
//        "f2a9465aa7900648dc83e358b941dcec25d943657b8e7a54cb2a0807c4bbd5ce",
//        "4d7c95e35396b06eaf461e76992866a7e48d94ed810758b67d5fd2076eed111a",
//        "8eed8dfa6efa3f86bacb3a48267953e784fb01ac6fab17b5b1b01a6bfda17ad5",
//        "5eab18fb6ae301f1aa9b672a668884043e1930a6f4eccfca581299971cadbfca",
//        "df16c4dca28152ffefc6ef07fa1a1fdea3e629f2dc1d767a67c7e0e7fb9d0384",
//        "0b08c3fe765a53a62f435b06796e9a5d90e2c6e3e02a9ae857edbdbf56f3736e",
//        "54bd72c6a5a4b27de30a41e43bb953a7b0171f2b0b43d788675046837eb5f0cc",
//        "aa16e7e1b2540c913c3346b01b3d7897e804e7ec0f17345a78c0c6be4f85f972",
//        "0630b591a5e557a1ed8304073dfcea6f4423ea33f579f738d6e8fe37d55d6635",
//        "7cc8c970e28bed8381455702b86e395d9a432a7b460e0650ffd2fff1a3790e21",
//        "0f3c5584030a06e0fdd16e67c4cc631445f76a003f76860b27f612894fc20c3e",
//        "c9798530ed2695cc4e6a89bc088973f716d552cbb6c0fe933f33d7c223507f73",
//        "9925a0fd4872b9cf23ddad57563f181fb909229e581518e90b5cd80a0697dc33",
//        "cb74ed9927577784cc3ca16f37fd0ec9c51ef5c2f7e8fb45c96d15aaf5f49db6",
//        "bd9cf70f06b54eab0719016b02ddfc543eeb3d22bdd139ce0f7261ce53415bd4",
//        "98cd26f6420fa22bf6c40d91b7bea3cb27118aa805647cb641607deeb2712884",
//        "49e7d8a0d4744c3683ea301ed89781f5b1fde4973e8040f0deda50eee51dd133",
//        "2abaf32189070c9c8d9af58023e88703c1e905dd10b138ad8c27696bc7ce5fa4",
//        "7bc29112f2ea4e03bc5b3ba90ba2ad5417ed714175688a292e80a39f41b058d7",
//        "02570bf2bdada212ae745a121545a97e5be7d6b5c1d63332973a2f41309bf144",
//        "f049d87acab62236538c80f7c2daa49664a71e0b75e805b15e9e6c2a8d963bc8",
//        "ad18b315ce1ff39f0f2a49ee61d8ed925619521227be5aea14ce4b039e4800f4",
//        "4d3d028855babe4e2ea4861121bec8eb2bb5a41f13b4b30e61dd8bcd9978133d",
//        "579d0c11fcd1c5b1ed81b3bc77f758da24eb08e78275b51e9e1ea1a9d98bc25f",
//        "58ee262f1a96ce49c8368f8dcb6f99fc4b2940a7c9c789c8230cc1f46a62d692",
//        "8eb59ee9803d1ab7c8465348de014c42813ab396e7a98f576075d10ad68ca135",
//        "b4f47009951706b79ca98f978b89f6428b674c49d487e8ac9ddb833fee0b3734",
//        "929ecc0c8e0961722b3fff4303f0e9fb0534e04f58a4579d4923095718fbee28",
//        "fae2847d2fd3b18ba5044f7911bc99c3932ef44f7a2ebbeaf2b630454685fa0f",
//        "85683f039d7d4351a82212b213f66568106b8d0b135499dbd19f5c6e27f11cb8",
//        "7878dbe04eeb4b0bf850d0e3abac70a1b79093bb471a04569d566056a6a1764c",
//        "b3d3f1acc772da2f7eab3d51d2705071cdbe05dbe524f4f336da936afb0f6c8a",
//        "5934aa852c1f8cf334dd14495008a30d5f1d88a89b6a1c46fcd29966d6da22a3",
//        "e17a4db28f45903ed41f0f9d485fdc87f83dae951decca2984119f6c304f1cfd",
//        "5092ddec28a2bc96238ae8eb3e99118e0aca4a121b6a8351c818dfa601ec9139",
//        "316b0e7b37cefe1c1afc972479fc29fc7f55e26659a858ec23dbe529158fc029",
//        "0f0226818f3d8b4a3d1c4cb1d5e9c8ddce83603a16b365a960b6ef9576ec1b93",
//};

//TEST_CASE("listtags")
//{
//    for (const auto& txid: txids) {
//        CTransaction tx = w->btc().GetTx(txid);
//
//        if (tx.vin[0].scriptWitness.stack.size() < 3) throw InscriptionFormatError("no witness script");
//
//        const auto& witness_stack = tx.vin[0].scriptWitness.stack;
//        CScript script(witness_stack[witness_stack.size() - 2].begin(), witness_stack[witness_stack.size() - 2].end());
//
//        auto tagged_data = ParseEnvelopeScript(script);
//
//        bool text = false;
//
//        std::clog << "Inscription: " << txid << " =====================================\n";
//        for(const auto& tag_value: tagged_data) {
//
//            if (tag_value.first == CONTENT_TYPE_TAG) {
//                std::string val = std::string (tag_value.second.begin(), tag_value.second.end());
//                text = val.find("text") != std::string::npos;
//            }
//
//            if (tag_value.first == CONTENT_TAG && !text) {
//                std::clog << hex(tag_value.first) << " -- " << hex(tag_value.second) << "\n";
//            } else {
//                std::clog << hex(tag_value.first) << " -- " << hex(tag_value.second) <<
//                          " [[" << std::string(tag_value.second.begin(), tag_value.second.end()) << "]]\n";
//            }
//        }
//
//        std::clog << std::endl;
//
//    }
//}

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
