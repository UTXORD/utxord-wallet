
(async ()=>{
    const api = await utxord();
    const bech = new api.Bech32(api.TESTNET);
    const funds_filter = "{\"look_cache\":true, \"key_type\":\"DEFAULT\", \"accounts\":[\"0'\",\"1'\"], \"change\":[\"0\",\"1\"], \"index_range\":\"0-256\"}";

    try {
        /*KeyRegistry*/

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

        console.log("MasterKey - OK!");
    } catch(e) {
        console.log("FAIL!");
        console.error(await api.Exception.prototype.getMessage(e));
    }


    try {
        let masterKey = new api.KeyRegistry(api.TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");
        masterKey.AddKeyType("funds", funds_filter);

        let key = masterKey.Derive("m/86'/1'/1'/0/0", false);
        let pubkey = key.PubKey();
        let addr = bech.Encode(pubkey, api.BECH32M);
        let addr2 = key.GetP2TRAddress(api.TESTNET);
        api.destroy(key);

        console.log("addr: ", addr);
        console.log("addr2: ", addr2);
        console.assert(addr == addr2);


        let outkey = masterKey.Derive("m/86'/1'/1'/0/1", false);
        let outpubkey = outkey.PubKey();
        let outaddr = bech.Encode(outpubkey, api.BECH32M);
        api.destroy(outkey);

        let utxo = new api.UTXO(api.TESTNET, "8f3e642289eda5d79c3212b7c5cd990a81bbeed8e768a28400a79b090adb3166", 0, "0.0001", addr);
        let output = new api.P2TR(api.TESTNET, "0.00007", outaddr);

        let tx = new api.SimpleTransaction(api.TESTNET);

        tx.MiningFeeRate("0.00001");
        tx.AddInput(utxo);
        api.destroy(utxo);
        tx.AddOutput(output);
        api.destroy(output);

        tx.Sign(masterKey, "funds");
        api.destroy(masterKey);

        let contract = tx.Serialize(2, api.TX_SIGNATURE);
        api.destroy(tx);

        console.log(contract);

        console.log("SimpleTx - OK!");

    } catch(e) {
        console.log("FAIL", e);
        console.error(api.Exception.prototype.getMessage(e));
    }

    try {
        let masterKey = new api.KeyRegistry(api.TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");
        masterKey.AddKeyType("funds", funds_filter);

        let key = masterKey.Derive("m/86'/1'/1'/0/100", false);
        let pubkey = key.PubKey();
        let addr = bech.Encode(pubkey, api.BECH32M);
        api.destroy(key);

        let intkey = masterKey.Derive("m/86'/1'/1'/0/101", false);
        let intpubkey = intkey.PubKey();
        let intaddr = bech.Encode(intpubkey, api.BECH32M);
        api.destroy(intkey);

        let outkey = masterKey.Derive("m/86'/1'/1'/0/102", false);
        let outpubkey = outkey.PubKey();
        let outaddr = bech.Encode(outpubkey, api.BECH32M);
        api.destroy(outkey);

        let changekey = masterKey.Derive("m/86'/1'/1'/0/99", false);
        let changeaddr = bech.Encode(changekey.PubKey(), api.BECH32M);
        api.destroy(changekey);

        let utxo = new api.UTXO(api.TESTNET, "8f3e642289eda5d79c3212b7c5cd990a81bbeed8e768a28400a79b090adb3166", 0, "0.0001", addr);
        let output = new api.P2TR(api.TESTNET, "0.00000546", outaddr);

        let tx = new api.SimpleTransaction(api.TESTNET);
        let tx1 = new api.SimpleTransaction(api.TESTNET);

        tx.MiningFeeRate("0.00001");
        tx1.MiningFeeRate("0.00001");

        tx1.AddInput(tx.Output(0));
        tx1.AddOutput(output);
        api.destroy(output);

        const intout = new api.P2TR(api.TESTNET, "0.00001", intaddr);

        tx.AddInput(utxo);
        tx.AddOutput(intout);

        intout.SetAmount(tx1.GetMinFundingAmount());

        tx.AddChangeOutput(changeaddr);

        api.destroy(utxo);
        api.destroy(intout);

        tx.Sign(masterKey, funds_filter);
        tx1.Sign(masterKey, funds_filter);
        api.destroy(masterKey);

        let contract = tx.Serialize(2, api.TX_SIGNATURE);
        let contract1 = tx1.Serialize(2, api.TX_SIGNATURE);

        api.destroy(tx);
        api.destroy(tx1);

        console.log(contract);
        console.log(contract1);

        console.log("SimpleTx chain - OK!");

    } catch(e) {
        console.log("FAIL", e);
        console.error(api.Exception.prototype.getMessage(e));
    }
})();
