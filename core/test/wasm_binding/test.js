
(async ()=>{
    const api = await utxord();

    try {
        let data = {
            testField: 'awesome'
        };

        let wow = new api.TestClass(api.Emval.toHandle(data));

        console.log(data);
    }
    catch(e) {
        console.log(e);
    }


    try {
        let bech = new api.Bech32(api.TESTNET);
        let pk = bech.Decode("tb1ptnn4tufj4yr8ql0e8w8tye7juxzsndnxgnlehfk2p0skftzks20sncm2dz");

        console.log("pk: ", pk.c_str());

        api.destroy(pk);

        let randomKey = new api.ChannelKeys();
        let randomPk = randomKey.GetLocalPubKey();

        console.log(randomPk.c_str());

        let masterKey = new api.MasterKey("b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe");

        let key = masterKey.Derive("m/86'/2'/0'/0/0", false);

        let addr = bech.Encode(key.GetLocalPubKey().c_str()).c_str();

        console.log("adr:", addr);
        console.assert(addr === "tb1ptnn4tufj4yr8ql0e8w8tye7juxzsndnxgnlehfk2p0skftzks20sncm2dz");

        api.destroy(randomPk);
        api.destroy(randomKey);
        api.destroy(bech);
        api.destroy(key);
        api.destroy(masterKey);

        console.log("OK!");
    } catch(e) {
        console.log("FAIL!");
        console.error(api.Exception.prototype.getMessage(e).c_str());
    }

})();
