import { describe, expect, test } from 'vitest'
import wordlist from 'web-bip39/wordlists/english';


import utxord from '~/libs/utxord.js'

describe('CoreAPI', async () => {
  const api = await utxord();
  const bech = new api.Bech32(api.TESTNET);
  const funds_filter = "{\"look_cache\":true, \"key_type\":\"DEFAULT\", \"accounts\":[\"0'\",\"1'\"], \"change\":[\"0\",\"1\"], \"index_range\":\"0-256\"}";
  const script_filter = "{\"look_cache\":false, \"key_type\":\"TAPSCRIPT\", \"accounts\":[\"0'\",\"1'\"], \"change\":[\"0\",\"1\"], \"index_range\":\"0-256\"}";

  test('KeyRegistry', () => {
    let randomKey = new api.KeyPair("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");
    let randomPk = randomKey.PubKey();

    console.log(randomPk);

    let masterKey = new api.KeyRegistry(api.TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

    let key = masterKey.Derive("m/86'/1'/0'/0/0", false);

    let addr = key.GetP2TRAddress(api.TESTNET);

    console.log("adr:", addr);
    console.assert(addr === "tb1pe8ml9zuyx6zrngmk7fudevrz7ka7d5mlcfgtrcl2epuf30k4me9s900plz");

    api.destroy(randomKey);
    api.destroy(key);
    api.destroy(masterKey);
  })

  test('TxStructure', () => {
    const rawtx = '010000000001012987a1116f859a5b658da3ad2f84747f47152c63fe4e222da870983a103621a800000000232200200497bdbf4b42546101fc366bf5abcd9f019dfc6fef2c3767000630bdd687955efdffffff09f25f020000000000160014e60220d2e1c30f9fc37813400076c7bdd1617cce2a62705000000000220020ebf2147fa6fa7f3d98b769b8d02adae68d1c4f2efc6361fa53bb9bd0e1d853f43366556700000000220020ae10929cf6ada58facf00f6c4c523b56233e306daeab171b781b8669cd492a169a2da9b10000000022002084034ec4b281b2fd5151d56d4793fc2e16db1f6ae643ce55127e8081232546a6bead51dc00000000220020c2842f0bfa7113d44eaaa3843f7a44dcbab8a10b14efe13e4c5ed2e93a05bda3d769591e010000002200209541e139d329cf4a7568ad1d0e52a3c78f340d1ff33c8d1b722bc179de9c53a7101cfcc501000000220020e3bc700d65304701bef3b50564e158eb62f7c5f8328e46232976727712677602fddb52e6010000002200209e88b80e8fdcd0baa02e8e5f64719341652e791e1d10c226cb014ba348fa7ac24e9e9b5702000000220020ac0d609676c99f3ff0adb236192c55fe6e3ac6cab4b9f54cb73a4c9d1f544cf20400483045022100a1c24f476e7660b51b112c9988f2ba16801022e8012a98d09745c4fd6a2c2dfa022034aa1e366e57ff821e0c9b5df6bf053632eda5e7e30d590766d118c2111cbe3f01483045022100f2d58ac3c6e953eb1ef943aee8b76cec3d3490be4c16dfe0294d7992ac18aaf802204cf036f34c52f97d85951be12986e4e10bc2799ea4115ea29be47df12dbc8b540169522102d4eda526938a10b5a8e4edf84c9aa7d411df86ac3ab3bd7b24053f9ecd8cea5b210282d2224d56a8b187a9cd213d346d6bc2041029b65201b1dc1935e619b802e267210241f7e8827549778a896b336b7af86fd10acfee2958a2551941b69082fe521d0153ae00000000'

    const txJson = api.Util.prototype.LogTx(api.TESTNET, rawtx)

    console.log("Tx structure: ", txJson)

  })

  describe('Mnemonics', async () => {

    const mnemonicParser = new api.MnemonicParser(`["${wordlist.join('","')}"]`);
    const mnemonicPhrase = 'vendor patrol opinion thumb clock teach chalk stomach bike body rent infant';
    const mnemonicEntropyHex = 'f234266df0b2b7bd097eb116231ed939';

    test('GenerateMnemonic', () => {
      let phrase = null;
      expect(() => {phrase = mnemonicParser.EncodeEntropy(mnemonicEntropyHex)}).not.toThrowError();
      expect(phrase).toEqual(mnemonicPhrase);
    })

    test('ValidateMnemonic', () => {
      let entropy = null;
      expect(() => {entropy = mnemonicParser.DecodeEntropy(mnemonicPhrase)}).not.toThrowError();
      expect(entropy).toEqual(mnemonicEntropyHex);
    })

  })

})
