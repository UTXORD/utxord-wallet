import {MAINNET, NETWORK, TESTNET} from '~/config/index';
import {onMessage, sendMessage} from 'webext-bridge'
import '~/background/api'
import WinManager from '~/background/winManager';
import {
  ADDRESS_COPIED,
  ADDRESSES_TO_SAVE,
  AUTH_STATUS,
  BALANCE_CHANGE_PRESUMED,
  BALANCE_REFRESH_DONE,
  CHECK_AUTH,
  CHECK_PASSWORD,
  COMMIT_BUY_INSCRIPTION,
  CONNECT_TO_PLUGIN,
  CONNECT_TO_SITE,
  CREATE_INSCRIPTION,
  DO_REFRESH_BALANCE,
  EXCEPTION,
  EXPORT_INSCRIPTION_KEY_PAIR,
  GENERATE_MNEMONIC,
  GET_ADDRESSES,
  GET_ALL_ADDRESSES,
  GET_BALANCE,
  GET_BALANCES,
  GET_USD_RATE,
  GET_NETWORK,
  NEW_FUND_ADDRESS,
  OPEN_EXPORT_KEY_PAIR_SCREEN,
  OPEN_START_PAGE,
  PLUGIN_ID,
  PLUGIN_PUBLIC_KEY,
  POPUP_HEARTBEAT,
  SAVE_DATA_FOR_EXPORT_KEY_PAIR,
  SAVE_DATA_FOR_SIGN,
  SAVE_GENERATED_SEED,
  SELL_INSCRIPTION,
  SEND_BALANCES,
  SUBMIT_SIGN,
  UNLOAD,
  UNLOAD_SEED,
  UPDATE_PASSWORD,
  GET_CONNECT_STATUS,
  SEND_CONNECT_STATUS,
  UPDATE_PLUGIN_CONNECT,
  GET_INSCRIPTION_CONTRACT,
  INSCRIPTION_CONTRACT_RESULT
} from '~/config/events';
import {debugSchedule, defaultSchedule, Scheduler, ScheduleName, Watchdog} from "~/background/scheduler";
import Port = chrome.runtime.Port;

if (NETWORK === MAINNET){
  if(self){
    self['console']['log'] =
    self['console']['error'] =
    self['console']['warn'] =
    self['console']['info']= () => {};
  }
}

(async () => {
  try {
    // We have to use chrome API instead of webext-bridge module due to following issue
    // https://github.com/zikaari/webext-bridge/issues/37
    let popupPort: Port | null = null;
    function postMessageToPopupIfOpen(msg: any) {
      if (popupPort != null) {
        popupPort.postMessage(msg);
      }
    }

    chrome.runtime.onConnect.addListener(async (port) => {
      if ('POPUP_MESSAGING_CHANNEL' != port?.name) return;

      popupPort = port;
      popupPort.onDisconnect.addListener(async (port) => {
        if ('POPUP_MESSAGING_CHANNEL' != port?.name) return;
        Scheduler.getInstance().action = null;
        popupPort = null;
      })

      port.onMessage.addListener(async (payload) => {
        if ('POPUP_MESSAGING_CHANNEL_OPEN' != payload?.id) return;

        postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: Api.connect});
        Scheduler.getInstance().action = async () => {
          postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: Api.connect});
        }
      });
    });

    const Api = await new self.Api(NETWORK);
    if(NETWORK === TESTNET){
      self.api = Api; // for debuging in devtols
    }
    const winManager = new WinManager();

    onMessage(GENERATE_MNEMONIC, async () => {
      return await Api.bip39.generateMnemonic();
    });

    onMessage(CONNECT_TO_SITE, async (payload) => {
      const success = await Api.checkSeed();
      console.log('checkSeed', success)
      if(success){
        await Api.sendMessageToWebPage(CONNECT_TO_SITE, success);
        setTimeout(async () => {
          await Api.sendMessageToWebPage(GET_BALANCES, Api.addresses);
        }, 1000);
      }
      return true;
    });

    onMessage(SAVE_GENERATED_SEED, async (payload) => {
      const sup = await Api.setUpPassword(payload.data.password);
      console.log('Api.setUpPassword:',sup);
      await Api.setSeed(payload.data.seed, payload.data?.passphrase);
      await Api.genKeys();
      await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key.PubKey());
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

    async function refreshBalanceAndAdressed(tabId: number | undefined = undefined) {
        const success = await Api.checkSeed();
        await Api.sendMessageToWebPage(AUTH_STATUS, success, tabId);
        await Api.sendMessageToWebPage(GET_CONNECT_STATUS, {}, tabId);
        await Api.sendMessageToWebPage(GET_BALANCES, Api.addresses, tabId);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, tabId);
    };

    onMessage(GET_BALANCE, async (payload: any) => {
      const balance = await Api.fetchBalance(payload.data?.address);
      setTimeout(async () => {
        await refreshBalanceAndAdressed();
      }, 1000);
      return balance;
    });

    onMessage(GET_USD_RATE, async () => {
      const usdRate = await Api.fetchUSDRate();
      return usdRate;
    });

    onMessage(GET_ADDRESSES, async () => {
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
          publicKey: item.key.PubKey(),
          privateKey: item.key.PrivKey()
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
          const success = await Api.sellInscription(payload.data.data);
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

    onMessage(POPUP_HEARTBEAT, async (payload) => {
      Watchdog.getNamedInstance(Watchdog.names.POPUP_WATCHDOG).reset();
      Scheduler.getInstance().activate();
      return true;
    });

    onMessage(BALANCE_CHANGE_PRESUMED, async (payload) => {
      Scheduler.getInstance().changeScheduleTo(ScheduleName.BalanceChangePresumed);
      return true;
    });

    onMessage(ADDRESS_COPIED, async (payload) => {
      Scheduler.getInstance().changeScheduleTo(ScheduleName.AddressCopied);
      return true;
    });

    chrome.runtime.onConnect.addListener(port => {
      port.onDisconnect.addListener(() => {})
    })

    chrome.runtime.onMessageExternal.addListener(async (payload, sender) => {
      // console.debug(`----- message from frontend: ${payload?.type}, data: `, {...payload?.data || {}});

      let tabId = sender?.tab?.id;
      if (typeof payload?.data === 'object' && payload?.data !== null) {
        payload.data._tabId = tabId;
      }

      if (payload.type === SEND_CONNECT_STATUS) {
        Api.connect = payload?.data?.connected;
        postMessageToPopupIfOpen({id: UPDATE_PLUGIN_CONNECT, connect: Api.connect});
      }

      if (payload.type === CONNECT_TO_PLUGIN) {
        let success = Api.signToChallenge(payload.data, tabId);
        if (success) {
          postMessageToPopupIfOpen({id: UPDATE_PLUGIN_CONNECT, connect: true});
          setTimeout(async () => {
            await refreshBalanceAndAdressed(tabId);
          }, 1000);
        }
      }

      if (payload.type === SEND_BALANCES) {
        console.log('SEND_BALANCES:',payload.data)
        if(payload.data?.addresses) {
          Api.balances = payload.data;
          // -------
          Api.sync = true;    // FIXME: Seems useless because happening too much late.
          Api.connect = true; // FIXME: However it's working for some reason in v1.1.5.
                              // FIXME: Probably due to high balance refresh frequency.
          console.log('payload.data.addresses: ',payload.data.addresses);
          Api.fundings = await Api.freeBalance(Api.fundings);
          // console.debug('... Api.fundings after Api.freeBalance:', Api.fundings);
          Api.inscriptions = await Api.freeBalance(Api.inscriptions);
          const balances = await Api.prepareBalances(payload.data.addresses);
          Api.fundings = balances.funds;
          // console.debug('... Api.fundings after Api.prepareBalances:', Api.fundings);
          Api.inscriptions = await balances.inscriptions;
          console.log('Api.fundings:', Api.fundings);
          console.log('Api.inscriptions:', Api.inscriptions);

          const balance = await Api.fetchBalance("UNUSED_VALUE");  // FIXME: currently address is still unused
          setTimeout(async () => {
            postMessageToPopupIfOpen({ id: BALANCE_REFRESH_DONE, data: { balance: balance?.data }});
          }, 1000);
          // -------
        }
      }

      if (payload.type === GET_ALL_ADDRESSES) {
        Api.all_addresses = await Api.freeBalance(Api.all_addresses);
        Api.all_addresses = await Api.getAllAddresses(payload.data.addresses);
        console.log('GET_ALL_ADDRESSES:', payload.data.addresses);
        // console.log('Api.addresses:', Api.addresses);
        const check = Api.checkAddresess(payload.data.addresses);
        console.log('Api.checkAddresess:', check);
        if(!check){
          setTimeout(async () => {
                  // console.log('ADDRESSES_TO_SAVE:', Api.addresses);
                  await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, Api.addresses, tabId);
          }, 100);
        }
        console.log('Api.restoreAllTypeIndexes:',payload.data.addresses);
        await Api.restoreAllTypeIndexes(payload.data.addresses);
      }

      if (payload.type === GET_INSCRIPTION_CONTRACT) {
        const contract = await Api.createInscriptionContract(payload.data);
        await Api.sendMessageToWebPage(INSCRIPTION_CONTRACT_RESULT, contract);
      }

      if (payload.type === CREATE_INSCRIPTION) {
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
        payload.data.errorMessage = payload.data?.costs?.errorMessage;
        delete payload.data.costs['errorMessage'];
        console.log(CREATE_INSCRIPTION+':',payload.data);
        winManager.openWindow('sign-create-inscription', async (id) => {
          setTimeout(async () => {
            if (payload.data.costs.output_mining_fee < 546) {
              Api.sendNotificationMessage(
                'CREATE_INSCRIPTION',
                'There are too few coins left after creation and they will become part of the inscription balance'
              );
            }
            await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === SELL_INSCRIPTION) {
        const costs = await Api.sellInscriptionContract(payload.data);
        payload.data.costs = costs;
        console.log(SELL_INSCRIPTION+':',payload.data);
        winManager.openWindow('sign-sell', async (id) => {
          setTimeout(async  () => {
            await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
          }, 1000);
        });
      }
      if (payload.type === COMMIT_BUY_INSCRIPTION) {
        payload.data.costs = await Api.commitBuyInscriptionContract(payload.data);
        payload.data.errorMessage = payload.data?.costs?.errorMessage;
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
      await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key.PubKey());
      return true;
    }

    // TODO: to implement usage of tabId ? (unsure)
    chrome.tabs.onActivated.addListener(sendhello);
    chrome.tabs.onCreated.addListener(sendhello);
    chrome.tabs.onUpdated.addListener(sendhello);
    chrome.tabs.onReplaced.addListener(sendhello);


    let alarmName = "utxord_wallet.alarm.balance_refresh_main_schedule";
    await chrome.alarms.clear(alarmName);
    await chrome.alarms.create(alarmName, { periodInMinutes: 10 });
    chrome.alarms.onAlarm.addListener(async (alarm) => {
      if (alarm.name == alarmName) {
        const success = await Api.checkSeed();
        await Api.sendMessageToWebPage(AUTH_STATUS, success);
        // await Api.sendMessageToWebPage(GET_BALANCES, Api.addresses);
        // await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses);
      }
    });

    let scheduler = Scheduler.getInstance();
    scheduler.schedule = defaultSchedule;
    // scheduler.schedule = debugSchedule;

    let watchdog = Watchdog.getNamedInstance(Watchdog.names.POPUP_WATCHDOG);
    watchdog.onTimeoutAction = () => {
      scheduler.deactivate();
    };
    await scheduler.run();
    await watchdog.run();

  } catch(e) {
    console.log('background:index.ts:',e);
  }
})();
