import { describe, expect, test } from 'vitest'

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
})
