
(async ()=>{
    const api = await utxord();
    const bech = new api.Bech32(api.TESTNET);

    try {
        /*MasterKey*/

        let randomKey = new api.ChannelKeys();
        let randomPk = randomKey.GetLocalPubKey();

        console.log(randomPk.c_str());

        let masterKey = new api.MasterKey("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

        let key = masterKey.Derive("m/86'/2'/0'/0/0", false);

        let addr = bech.Encode(key.GetLocalPubKey().c_str(), api.BECH32M).c_str();

        console.log("adr:", addr);
        console.assert(addr === "tb1ptnn4tufj4yr8ql0e8w8tye7juxzsndnxgnlehfk2p0skftzks20sncm2dz");

        api.destroy(randomPk);
        api.destroy(randomKey);
        api.destroy(key);
        api.destroy(masterKey);

        console.log("MasterKey - OK!");
    } catch(e) {
        console.log("FAIL!");
        console.error(await api.Exception.prototype.getMessage(e).c_str());
    }


    try {
        let masterKey = new api.MasterKey("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

        let key = masterKey.Derive("m/86'/0'/1'/0/0", false);
        let pubkey = key.GetLocalPubKey().c_str();
        let addr = bech.Encode(pubkey, api.BECH32M).c_str();
        api.destroy(key);

        console.log("addr", addr);

        let outkey = masterKey.Derive("m/86'/0'/1'/0/1", false);
        let outpubkey = outkey.GetLocalPubKey().c_str();
        let outaddr = bech.Encode(outpubkey, api.BECH32M).c_str();
        api.destroy(outkey);

        let utxo = new api.UTXO(api.TESTNET, "8f3e642289eda5d79c3212b7c5cd990a81bbeed8e768a28400a79b090adb3166", 0, "0.0001", addr);
        let output = new api.P2TR(api.TESTNET, "0.00007", outaddr);

        let tx = new api.SimpleTransaction(api.TESTNET);

        tx.MiningFeeRate("0.00001");
        tx.AddInput(utxo);
        api.destroy(utxo);
        tx.AddOutput(output);
        api.destroy(output);

        tx.Sign(masterKey);
        api.destroy(masterKey);

        let contract = tx.Serialize();
        api.destroy(tx);

        console.log(contract);

        console.log("SimpleTx - OK!");

    } catch(e) {
        console.log("FAIL", e);
        console.error(api.Exception.prototype.getMessage(e).c_str());
    }

    try {
        let masterKey = new api.MasterKey("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

        let key = masterKey.Derive("m/86'/0'/1'/0/100", false);
        let pubkey = key.GetLocalPubKey().c_str();
        let addr = bech.Encode(pubkey, api.BECH32M).c_str();
        api.destroy(key);

        let intkey = masterKey.Derive("m/86'/0'/1'/0/101", false);
        let intpubkey = intkey.GetLocalPubKey().c_str();
        let intaddr = bech.Encode(intpubkey, api.BECH32M).c_str();
        api.destroy(intkey);

        let outkey = masterKey.Derive("m/86'/0'/1'/0/102", false);
        let outpubkey = outkey.GetLocalPubKey().c_str();
        let outaddr = bech.Encode(outpubkey, api.BECH32M).c_str();
        api.destroy(outkey);

        let changekey = masterKey.Derive("m/86'/0'/1'/0/99", false);
        let changeaddr = bech.Encode(changekey.GetLocalPubKey().c_str(), api.BECH32M).c_str();
        api.destroy(changekey);

        let utxo = new api.UTXO(api.TESTNET, "8f3e642289eda5d79c3212b7c5cd990a81bbeed8e768a28400a79b090adb3166", 0, "0.0001", addr);
        let output = new api.P2TR(api.TESTNET, "0.00000546", outaddr);

        let tx = new api.SimpleTransaction(api.TESTNET);
        let tx1 = new api.SimpleTransaction(api.TESTNET);

        tx.MiningFeeRate("0.00001");
        tx1.MiningFeeRate("0.00001");

        tx1.AddInput(tx);
        tx1.AddOutput(output);
        api.destroy(output);

        let intout = new api.P2TR(api.TESTNET, tx1.GetMinFundingAmount(), intaddr);

        tx.AddInput(utxo);
        api.destroy(utxo);
        tx.AddOutput(intout)
        api.destroy(intout);
        tx.AddChangeOutput(changeaddr);

        tx.Sign(masterKey);
        tx1.Sign(masterKey);
        api.destroy(masterKey);

        let contract = tx.Serialize();
        let contract1 = tx1.Serialize();

        api.destroy(tx);
        api.destroy(tx1);

        console.log(contract);
        console.log(contract1);

        console.log("SimpleTx chain - OK!");

    } catch(e) {
        console.log("FAIL", e);
        console.error(api.Exception.prototype.getMessage(e).c_str());
    }
})();
