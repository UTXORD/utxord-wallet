import { NETWORK, MAINNET, TESTNET, REGTEST } from '~/config/index';
if (NETWORK === MAINNET){
  if(self){
    self['console']['log'] =
    self['console']['error'] =
    self['console']['warn'] =
    self['console']['info']= () => {};
  }
}

import { onMessage, sendMessage } from 'webext-bridge'
import '~/background/api'
import WinManager from '~/background/winManager';
import {
    EXCEPTION,
    SAVE_GENERATED_SEED,
    CHECK_AUTH,
    UNLOAD_SEED,
    CONNECT_TO_PLUGIN,
    CREATE_INSCRIPTION,
    SELL_INSCRIPTION,
    COMMIT_BUY_INSCRIPTION,
    NEW_FUND_ADDRESS,
    GET_BALANCE,
    GET_BALANCES,
    GET_ADDRESSES,
    GET_ALL_ADDRESSES,
    SAVE_DATA_FOR_SIGN,
    SAVE_DATA_FOR_EXPORT_KEY_PAIR,
    OPEN_EXPORT_KEY_PAIR_SCREEN,
    SUBMIT_SIGN,
    PLUGIN_ID,
    PLUGIN_PUBLIC_KEY,
    AUTH_STATUS,
    ADDRESSES_TO_SAVE,
    UNLOAD,
    GENERATE_MNEMONIC,
    UPDATE_PASSWORD,
    CHECK_PASSWORD,
    SEND_BALANCES,
    GET_NETWORK,
    OPEN_START_PAGE,
    EXPORT_INSCRIPTION_KEY_PAIR
} from '~/config/events';

(async () => {
 try{
    const Api = await new self.Api(NETWORK);
    if(NETWORK === TESTNET){
      self.api = Api; // for debuging in devtols
    }
    const winManager = new WinManager();

    onMessage(GENERATE_MNEMONIC, async () => {
      return await Api.bip39.generateMnemonic();
    });

    onMessage(SAVE_GENERATED_SEED, async (payload) => {
      const sup = await Api.setUpPassword(payload.data.password);
      console.log('Api.setUpPassword:',sup);
      await Api.setSeed(payload.data.seed, payload.data?.passphrase);
      await Api.genKeys();
      await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key.GetLocalPubKey().c_str());
      return await Api.checkSeed();
    });
    onMessage(UPDATE_PASSWORD, async (payload) => {
      const checkOld = await Api.checkPassword(payload.data.old);
      if(!checkOld){
        Api.sendExceptionMessage(
          EXCEPTION,
          "Old password is incorrect"
        );
        return;
      }
      return await Api.setUpPassword(payload.data.password);
    });
    onMessage(CHECK_PASSWORD, async (payload) => {
      return await Api.checkPassword(payload.data.password);
    });

    onMessage(CHECK_AUTH, () => {
      const success = Api.checkSeed();
      return success;
    });

    onMessage(GET_NETWORK, () => {
      const network = Api.getCurrentNetWorkLabel();
      return network;
    });


    onMessage(UNLOAD_SEED, async () => {
      Api.sendMessageToWebPage(UNLOAD, chrome.runtime.id);
      const success = await Api.unload();
      return success;
    });

    onMessage(GET_BALANCE, (payload: any) => {
      const balance = Api.fetchBalance(payload.data?.address);
      setTimeout(async () => {
              await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses);
      }, 1000);
      return balance;
    });

    onMessage(GET_ADDRESSES, async() => {
      const {addresses} = await Api.genKeys();
      console.log('addresses:',addresses)
      return addresses;
    });

    onMessage(NEW_FUND_ADDRESS, async () => {
      await Api.generateNewIndex('fund');
      const newKeys = Api.genKeys();
      await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses)
      return newKeys;
    });

    onMessage(EXPORT_INSCRIPTION_KEY_PAIR, async (payload) => {
      const res = await Api.decryptedWallet(payload.data.password);
      if (res) {
        const item = Api.selectByOrdOutput(payload.data.txid, payload.data.nout);
        const keypair = {
          publicKey: item.key.GetLocalPubKey().c_str(),
          privateKey: item.key.GetLocalPrivKey().c_str()
        };
        await Api.encryptedWallet(payload.data.password);
        return keypair;
      }
      return false;
    });

    onMessage(SUBMIT_SIGN, async (payload) => {

      // console.debug("===== SUBMIT_SIGN: payload?.data", payload?.data)
      // console.debug("===== SUBMIT_SIGN: payload?.data?._tabId", payload?.data?._tabId)
      // console.debug("===== SUBMIT_SIGN: payload?.data?.data?._tabId", payload?.data?.data?._tabId)

      if (payload.data.type === CREATE_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);
        if(res){
          const success = await Api.createInscription(payload.data.data);
          await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, payload?.data?.data?._tabId);
          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }

      if (payload.data.type === SELL_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);
        if(res){
          const success = Api.sellInscription(payload.data.data);
          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }

      if (payload.data.type === COMMIT_BUY_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);
        Api.wallet.tmp = payload.data.password;
        if(res){
          const success = await Api.commitBuyInscription(payload.data.data);
          await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, payload?.data?.data?._tabId);
          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }
    });

    chrome.runtime.onConnect.addListener(port => {
      port.onDisconnect.addListener(() => {})
    })

    chrome.runtime.onMessageExternal.addListener(async (payload, sender) => {
      let tabId = sender?.tab?.id;
      if (typeof payload?.data === 'object' && payload?.data !== null) {
        payload.data._tabId = tabId;
      }

      if (payload.type === CONNECT_TO_PLUGIN) {
        await Api.signToChallenge(payload.data, tabId)
      }

      if (payload.type === SEND_BALANCES) {
        console.log('SEND_BALANCES:',payload.data)
        if(payload.data?.addresses){
          Api.balances = payload.data;
          Api.sync = true;
          Api.connect = true;
          console.log('payload.data.addresses: ',payload.data.addresses);
          const balances = await Api.prepareBalances(payload.data.addresses);
          Api.fundings = balances.funds;
          Api.inscriptions = balances.inscriptions;
          console.log('Api.fundings:', Api.fundings);
          console.log('Api.inscriptions:', Api.inscriptions);
        }
      }

      if (payload.type === GET_ALL_ADDRESSES) {
        Api.all_addresses = await Api.getAllAddresses(payload.data.addresses);
        console.log('GET_ALL_ADDRESSES:',payload.data.addresses);
        const check = Api.checkAddresess(payload.data.addresses);
        console.log('Api.checkAddresess:',check)
        if(!check){
          setTimeout(async () => {
                  console.log('ADDRESSES_TO_SAVE:',Api.addresses);
                  await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, Api.addresses, tabId);
          }, 100);
        }
        console.log('Api.restoreAllTypeIndexes:',payload.data.addresses);
        await Api.restoreAllTypeIndexes(payload.data.addresses);
      }

      if (payload.type === CREATE_INSCRIPTION) {
        payload.data.fee_rate = payload.data.fee;

        let costs;
        console.log('payload?.data?.type:',payload?.data?.type)
        console.log('payload?.data?.collection?.genesis_txid:',payload?.data?.collection?.genesis_txid);
        /*
        if(payload?.data?.type==='AVATAR'){
          console.log('run: Independent Collection')
          costs =  await Api.createIndependentCollectionContract(payload.data);
        }
        if(payload?.data?.type==='INSCRIPTION'){
          if(payload?.data?.collection?.genesis_txid){
            console.log('run: Inscription In Collection');
            costs =  await Api.createInscriptionInCollectionContract(payload.data);
          }else{
            console.log('run: Independent Inscription');
            costs =  await Api.createIndependentInscriptionContract(payload.data);
          }
        }
        if(payload?.data?.type==='COLLECTION'){
          if(payload?.data?.collection?.genesis_txid){
            console.log('run: Collection With Parent');
            costs =  await Api.createCollectionWithParentContract(payload.data);
          }else{
            console.log('run: Independent Collection');
            costs =  await Api.createIndependentCollectionContract(payload.data);
          }
        }
*/
        costs = await Api.createInscriptionContract(payload.data);
        payload.data.costs = costs;
        payload.data.errorMessage = payload.data.costs.errorMessage;
        delete payload.data.costs['errorMessage'];
        console.log(CREATE_INSCRIPTION+':',payload.data);
        winManager.openWindow('sign-create-inscription', async (id) => {
          setTimeout(async  () => {
            await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === SELL_INSCRIPTION) {
        winManager.openWindow('sign-sell', async (id) => {
          setTimeout(async  () => {
            await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === COMMIT_BUY_INSCRIPTION) {
        payload.data.costs = await Api.commitBuyInscriptionContract(payload.data);
        payload.data.errorMessage = payload.data.costs.errorMessage;
        delete payload.data.costs['errorMessage'];
        console.log(COMMIT_BUY_INSCRIPTION+':',payload);
        //update balances before openWindow
        winManager.openWindow('sign-commit-buy', async (id) => {
          setTimeout(async () => {
            await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === OPEN_EXPORT_KEY_PAIR_SCREEN) {
        winManager.openWindow('export-keys', async (id) => {
          setTimeout(async () => {
            await sendMessage(SAVE_DATA_FOR_EXPORT_KEY_PAIR, payload.data, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === 'OPEN_SIGN_BUY_INSCRIBE_PAGE') { // hidden mode
        console.log("OPEN_SIGN_BUY_INSCRIBE_PAGE:", payload.data);
        const res = await Api.decryptedWallet(Api.wallet.tmp);
        if(res){
          await Api.signSwapInscription(payload.data, tabId);
          await Api.encryptedWallet(Api.wallet.tmp);
          Api.wallet.tmp = ''
        }
      }
      if (payload.type === OPEN_START_PAGE) {
        winManager.openWindow('start');
      }
    })

    // SET PLUGIN ID TO WEB PAGE
    async function sendhello() {
      // const [tab] = await chrome.tabs.query({ active: true });
      // if (tab?.url?.startsWith('chrome://') || tab?.url?.startsWith('chrome://new-tab-page/')) return;

      console.log(PLUGIN_ID, chrome.runtime.id);
      console.log(PLUGIN_PUBLIC_KEY, Api.wallet.auth);
      await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id);
      await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key.GetLocalPubKey().c_str());
      return true;
    }

    // TODO: to implement usage of tabId ? (unsure)
    chrome.tabs.onActivated.addListener(sendhello);
    chrome.tabs.onCreated.addListener(sendhello);
    chrome.tabs.onUpdated.addListener(sendhello);
    chrome.tabs.onReplaced.addListener(sendhello);


    async function listener(index: number) {
      const success = await Api.checkSeed();
      await Api.sendMessageToWebPage(AUTH_STATUS, success);
      await Api.sendMessageToWebPage(GET_BALANCES, Api.addresses);
      await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses);

    }
    let index = 0
    chrome.alarms.create("listener", { periodInMinutes: 0.2 });
    chrome.alarms.onAlarm.addListener(function() {
      listener(index);
      index++;
    });
  }catch(e){
    console.log('background:index.ts:',e);
  }
})();
