import { describe, expect, test } from 'vitest'
import wordlist from 'web-bip39/wordlists/english';


describe('Examples', () => {
  test('2 + 2 = 4', () => {
    expect(2 + 2).toBe(4)
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
