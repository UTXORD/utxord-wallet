import {MAINNET, NETWORK, TESTNET, SIGNET, BASE_URL_PATTERN} from '~/config/index';
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
  SET_UP_PASSWORD,
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
  CHANGE_TYPE_FUND_ADDRESS,
  CHANGE_USE_DERIVATION,
  STATUS_DERIVATION,
  CHANGE_VIEW_MODE,
  STATUS_VIEW_MODE,
  STATUS_ERROR_REPORTING,
  CHANGE_ERROR_REPORTING,
  OPEN_EXPORT_KEY_PAIR_SCREEN,
  OPEN_START_PAGE,
  PLUGIN_ID,
  PLUGIN_PUBLIC_KEY,
  PLUGIN_SUPPORTED_VERSIONS,
  POPUP_HEARTBEAT,
  SAVE_DATA_FOR_EXPORT_KEY_PAIR,
  SAVE_DATA_FOR_SIGN,
  SAVE_GENERATED_SEED,
  SELL_INSCRIPTION,
  BUY_PRODUCT,
  SEND_BALANCES,
  SUBMIT_SIGN,
  UNLOAD,
  UNLOAD_SEED,
  UPDATE_PASSWORD,
  GET_CONNECT_STATUS,
  SEND_CONNECT_STATUS,
  UPDATE_PLUGIN_CONNECT,
  GET_INSCRIPTION_CONTRACT,
  GET_INSCRIPTION_CONTRACT_RESULT,
  CREATE_CHUNK_INSCRIPTION,
  GET_BULK_INSCRIPTION_ESTIMATION,
  GET_BULK_INSCRIPTION_ESTIMATION_RESULT,
  CREATE_CHUNK_INSCRIPTION_RESULT,
  TRANSFER_LAZY_COLLECTION,
  ESTIMATE_TRANSFER_LAZY_COLLECTION,
  ESTIMATE_TRANSFER_LAZY_COLLECTION_RESULT,
  TRANSFER_LAZY_COLLECTION_RESULT,
  ESTIMATE_PURCHASE_LAZY_INSCRIPTION,
  ESTIMATE_PURCHASE_LAZY_INSCRIPTION_RESULT,
  PURCHASE_LAZY_INSCRIPTION,
  PURCHASE_LAZY_INSCRIPTION_RESULT,
  CREATE_INSCRIBE_RESULT,
  BUY_PRODUCT_RESULT,
  SELL_INSCRIBE_RESULT,
  COMMIT_BUY_INSCRIBE_RESULT,
} from '~/config/events';
import {debugSchedule, defaultSchedule, Scheduler, ScheduleName, Watchdog} from "~/background/scheduler";
import Port = chrome.runtime.Port;
import {HashedStore} from "~/background/hashedStore";
import {bookmarks} from "webextension-polyfill";

if (NETWORK === MAINNET){
  if(self){
    self['console']['debug'] =
    self['console']['log'] =
    self['console']['error'] =
    self['console']['warn'] =
    self['console']['info']= () => {};
  }
}
// interface IInscription {
//   collection: {},
//   content: string,
//   content_type: string,
//   addresses: [],
//   fee: number, // fee rate
//   expect_amount: number,
//   type: string,
//   title: string,
//   description: string,
//   metadata: {
//     title?: string,
//     description?: string,
//   }
// }

interface ISingleInscriptionEstimation {
  content_length: number;
  content_type: string;
  expect_amount: number | undefined,
  fee_rate: number | undefined,
  fee: number | undefined,
}

interface IBulkInscriptionEstimation {
  expect_amount: number,
  fee_rate: number,
  fee: number,
  inscriptions_content: ISingleInscriptionEstimation[];
}

interface IBulkInscriptionEstimationResult {
  amount: number;
  expect_amount: number,
}

interface ISingleInscription {
  // To update after each inscription done, from out#1 of genesis tx
  draft_uuid: string,
  content: string,
  content_type: string,
  type: string,
  metadata: null | {},
}

interface IChunkInscription {
  job_uuid: string;  // plugin needs it to identify all chunks as a single bulk
  expect_amount: number,
  total_expect_amount: number,
  total_amount: number,
  fee_rate: number,
  fee: number,
  addresses: [],
  parent: null | {
    btc_owner_address: string,
    genesis_txid: string,
    owner_txid: string,
    owner_nout: number,
  },
  inscriptions: ISingleInscription[],
}

// interface IChunkInscriptionPopupData extends IChunkInscription {
//   content_store_key: string | null,
//   password: string
// }

interface ISingleInscriptionResult {
  draft_uuid: string,
  contract: {},  // from JSON from core
}

interface IChunkInscriptionResult {
  inscription_results: ISingleInscriptionResult[];
  last_inscription_out: null | {
    genesis_txid: string,
    owner_txid: string,
    owner_nout: number,
  },
  error: null | string
}

interface ICollectionTransfer {
  protocol_version: number,
  addresses: [],
  collection: {
    owner_txid: string,
    owner_nout: number,
    metadata: {  // unsure we need it
      title: string | null,
      description: string | null
    }
  },
  transfer_address: string,  // where to transfer collection
  mining_fee: number,
  market_fee: {
    address: string,  // where to send market fee for lazy inscribing
    amount: number
  }
}

interface ICollectionTransferResult {
  contract: object,
  errorMessage: string | null  // null or empty string means no error happened
}


(async () => {
  try {
    let store = HashedStore.getInstance();
    const Api = await new self.Api(NETWORK);
    Api.sentry();
    // We have to use chrome API instead of webext-bridge module due to following issue
    // https://github.com/zikaari/webext-bridge/issues/37
    let popupPort: Port | null = null;
    function postMessageToPopupIfOpen(msg: any) {
      if (popupPort != null) {
        popupPort.postMessage(msg);
      }
    }

    async function checkPossibleTabsAndTimeConflict() {
      const tabs = await chrome.tabs.query({
        windowType: 'normal',
        url: BASE_URL_PATTERN,
      });

      if (1 < tabs.length) {
        await Api.sendWarningMessage(
            'TABS',
            "Don't use UTXORD market in multiple tabs/windows simultaneously!",
            false
        );
        console.log(`----- sendMessageToWebPage: there are ${tabs.length} tabs found with BASE_URL_PATTERN: ${BASE_URL_PATTERN}`);
      }
      if(!await Api.fetchTimeSystem()){
        await Api.sendWarningMessage(
            'TIMEERROR',
            "Enable automatic date and time setting for your device",
            false
        );
      }
    }

    async function helloSite(tabId: number | undefined = undefined){
      console.log('helloSite->run')
      await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id, tabId);
      const user = await Api.wallet?.auth?.key?.PubKey();
      if(user){
        await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, user, tabId);
      }
      // const success = await Api.checkSeed();
      // if(success){
      //   await Api.sendMessageToWebPage(CONNECT_TO_SITE, true, tabId);
      // }
    }

    async function reConnectSession(unload = false){
      console.log('reConnectSession->run')
      await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id);
      const pubkey = await Api.getPublicKeyFromWebPage();
      const user = await Api.wallet?.auth?.key?.PubKey();
      if(user){
        await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, user);
      }
      if(pubkey !== user || !pubkey){
        if(unload){
          if(pubkey !== user && pubkey !== undefined){
              await Api.removePublicKeyToWebPage();
              await Api.sendMessageToWebPage(UNLOAD, chrome.runtime.id);
          }
          setTimeout(async () => {
              if(!pubkey){
                  await Api.setPublicKeyToWebPage();
              }
              helloSite();
              const addresses = await Api.getAddressForSave();
              await Api.sendMessageToWebPage(GET_BALANCES, addresses);
          }, 2000);
          return false;
        }
        return true;
      }
      return false;
    }

    helloSite();

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
          await checkPossibleTabsAndTimeConflict();
          const connect = await Api.connect;
          postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: connect });
          Scheduler.getInstance().action = async () => {
            postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: connect });
          }
      });
    });

    if(NETWORK === SIGNET){
      self.api = Api; // for debuging in devtols
    }
    const winManager = new WinManager();


    onMessage(GENERATE_MNEMONIC, async (payload) => {
      console.log('GENERATE_MNEMONIC->run')
      return await Api.generateMnemonic(payload.data?.length);
    });

    onMessage(CONNECT_TO_SITE, async (payload) => {
      console.log('CONNECT_TO_SITE->run')
      await reConnectSession(true);
      const success = await Api.checkSeed();
      console.log('checkSeed', success)
      if(success){
        await Api.sendMessageToWebPage(CONNECT_TO_SITE, success);
        setTimeout(async () => {
          const addresses = await Api.getAddressForSave();
          await Api.sendMessageToWebPage(GET_BALANCES, addresses);
        }, 1000);
      }
      Api.sentry();
      return true;
    });

    onMessage(SET_UP_PASSWORD, async (payload) => {
      console.log('SET_UP_PASSWORD->run')
      const sup = await Api.setUpPassword(payload.data.password);
      return sup;
    });

    onMessage(SAVE_GENERATED_SEED, async (payload) => {
      console.log('SAVE_GENERATED_SEED->run')
      await Api.setUpSeed(payload.data.seed, payload.data?.passphrase);
      Api.genKeys();
      if(Api.wallet.auth.key) {
        await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key?.PubKey());
      }
      return Api.checkSeed();
    });

    onMessage(UPDATE_PASSWORD, async (payload) => {
      console.log('UPDATE_PASSWORD->run')
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
      console.log('CHECK_PASSWORD->run')
      return await Api.checkPassword(payload.data.password);
    });

    onMessage(CHECK_AUTH, async() => {
      console.log('CHECK_AUTH->run')
      await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id);
      if(Api.wallet?.auth?.key?.ptr){
        await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet?.auth?.key?.PubKey());
      }
      const success = await Api.checkSeed();
      return success;
    });

    onMessage(GET_NETWORK, () => {
      console.log('GET_NETWORK->run')
      const network = Api.getCurrentNetWorkLabel();
      return network;
    });


    onMessage(UNLOAD_SEED, async () => {
      console.log('UNLOAD_SEED->run')
      await Api.removePublicKeyToWebPage();
      await Api.sendMessageToWebPage(UNLOAD, chrome.runtime.id);
      const success = await Api.unload();
      return success;
    });

    async function refreshBalanceAndAdressed(tabId: number | undefined = undefined) {
      console.log('refreshBalanceAndAdressed->run')
        await reConnectSession(true);
        const success = await Api.checkSeed();
        await Api.sendMessageToWebPage(AUTH_STATUS, success, tabId);
        await Api.sendMessageToWebPage(GET_CONNECT_STATUS, {}, tabId);
        const addresses = await Api.getAddressForSave();
        if(addresses.length > 0){
          Api.timeSync = false;
          await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses, tabId);
          await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses, tabId);
        }
        await Api.sendMessageToWebPage(GET_BALANCES, addresses, tabId);

    };


    onMessage(GET_BALANCE, async (payload: any) => {
      console.log('GET_BALANCE->run')
      const balance = await Api.fetchBalance(payload.data?.address);
      console.log('balance',balance)
      setTimeout(async () => {
        await refreshBalanceAndAdressed();
      }, 1000);
      return balance;
    });

    onMessage(GET_USD_RATE, async () => {
      console.log('GET_USD_RATE->run')
      const usdRate = await Api.fetchUSDRate();
      return usdRate;
    });

    onMessage(GET_ADDRESSES, async () => {
      console.log('GET_ADDRESSES->run')
      const addresses = await Api.genKeys();
      console.log('GET_ADDRESSES->addresses:',addresses)
      return addresses;
    });

    async function newAddress(){
      await Api.setDerivate(1);
      await Api.generateNewIndex('fund');
      const newKeys = await Api.genKeys();
      const addresses = await Api.getAddressForSave(newKeys.addresses);
      console.log('newKeys.addresses:',newKeys.addresses)
      console.log('ADDRESSES_TO_SAVE:', addresses);
      if(addresses.length > 0){
        await reConnectSession(true);
        await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
        return newKeys;
      }
      return newKeys;
    }

    onMessage(NEW_FUND_ADDRESS, async () => {
      console.log('NEW_FUND_ADDRESS->run')
      let addresses = newAddress()
      if(addresses.length > 0){
        Api.timeSync = false;
        await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
      }
      return addresses;
    });

    onMessage(CHANGE_TYPE_FUND_ADDRESS, async (payload: any) => {
      console.log('CHANGE_TYPE_FUND_ADDRESS:',payload.data?.type);
      await Api.setTypeAddress('fund', payload.data?.type);
      await Api.setTypeAddress('ord', payload.data?.type);
      const newKeys = await Api.genKeys();
      const addresses = await Api.getAddressForSave(newKeys.addresses);
      if(addresses.length > 0){
        await reConnectSession(true);
        await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
      }
      return newKeys;
    });

    onMessage(CHANGE_USE_DERIVATION, async (payload: any) => {
      console.log('CHANGE_USE_DERIVATION:',payload.data?.value);
      await Api.setDerivate(payload.data?.value);
      const newKeys = await Api.genKeys();
      const addresses = await Api.getAddressForSave(newKeys.addresses);
      if(addresses.length > 0){
        await reConnectSession(true);
        await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
      }
      return {derivate: Api.derivate, keys: newKeys};
    });

    onMessage(CHANGE_VIEW_MODE, async (payload: any) => {
      console.log('CHANGE_VIEW_MODE:',payload.data?.value);
      await Api.setViewMode(payload.data?.value);
      return {viewMode: Api.viewMode};
    });

    onMessage(STATUS_VIEW_MODE, async () => {
      console.log('STATUS_VIEW_MODE:', Api.viewMode);
      return {viewMode: Api.viewMode};
    });


    onMessage(STATUS_DERIVATION, async () => {
      console.log('STATUS_DERIVATION:', Api.derivate);
      const newKeys = await Api.genKeys();
      const addresses = await Api.getAddressForSave(newKeys.addresses);
      if(addresses.length > 0){
        await reConnectSession(true);
        await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
        await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
      }
      return {derivate: Api.derivate, keys: newKeys};
    });

    onMessage(CHANGE_ERROR_REPORTING, async (payload: any) => {
      console.log('CHANGE_ERROR_REPORTING:',payload.data?.value);
      await Api.setErrorReporting(payload.data?.value);
      return {error_reporting: Api.error_reporting};
    });

    onMessage(STATUS_ERROR_REPORTING, () => {
      console.log('STATUS_ERROR_REPORTING:', Api.error_reporting);
      return {error_reporting: Api.error_reporting};
    });


    onMessage(EXPORT_INSCRIPTION_KEY_PAIR, async (payload) => {
      console.log('EXPORT_INSCRIPTION_KEY_PAIR->run')
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


    function _addressesMap(addresses: object[]) {
      return Object.fromEntries(addresses.map(addr => [addr.address, addr]));
    }

    async function createChunkInscription(chunkData: IChunkInscription) {
      console.debug("createChunkInscription: chunkData:", chunkData);

      Scheduler.getInstance().deactivate();
      let parentInscription = null;
      let results = [];
      let error = null;
      let usedAddressesMap = {};

      let errorMessage = null;
      try {
        /*
        const debugContractResult = {}
        const debugOrd = new Api.utxord.CreateInscriptionBuilder(Api.network, Api.utxord.INSCRIPTION);
        debugOrd.Deserialize(JSON.stringify(debugContractResult));
        const debugRawTx = await Api.getRawTransactions(debugOrd);
        console.debug('createChunkInscription: debugContractResult:', debugContractResult);
        console.debug('createChunkInscription: debugRawTx:', debugRawTx);
        */

        // refresh balances first..
        const balances = await Api.prepareBalances(chunkData.addresses);
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

        // prepare first parent
        const inCollection = Boolean(chunkData.parent?.genesis_txid).valueOf();
        parentInscription = chunkData.parent?.genesis_txid ? {...chunkData.parent} : {
          btc_owner_address: "",
          genesis_txid: "",
          owner_txid: "",
          owner_nout: 0,
        };

        const inscriptions = Array.from(chunkData?.inscriptions || []);
        // console.debug("createChunkInscription: inscriptions:", inscriptions);

        for (let contractData of inscriptions) {
          // Collect used addressed
          usedAddressesMap = {...usedAddressesMap, ..._addressesMap(Api.addresses)};

          contractData.collection = parentInscription;
          console.debug('createChunkInscription: chunk contractData: ', contractData);
          const contract = await Api.createInscriptionContract(contractData, false, true);
          console.debug('createChunkInscription: contract: ', contract);

          if (contract.errorMessage) {
            errorMessage = contract.errorMessage;
            break;
          }

          // debug log output for rawTx
          const contractOrd = new Api.utxord.CreateInscriptionBuilder(Api.network, Api.utxord.INSCRIPTION);
          contractOrd.Deserialize(contract.data);
          const debugRawTx = await Api.getRawTransactions(contractOrd);
          console.debug('createChunkInscription: rawTx:', debugRawTx);

          // update parent for next inscription
          if (inCollection) {
            parentInscription = {
              btc_owner_address: chunkData.parent?.btc_owner_address,
              genesis_txid: chunkData.parent?.genesis_txid,
              owner_txid: contract.outputs?.collection?.TxID(),
              owner_nout: contract.outputs?.collection?.NOut(),
            };
          }

          console.debug('createChunkInscription: parentInscription:', parentInscription);

          // mark all used utxo as spent
          Api.updateFundsByOutputs(contract.utxo_list, {is_locked: true});

          // add change utxo (if any) to funding
          Api.pushChangeToFunds(contract.outputs?.change);
          // add ordinal utxo (if any) to inscriptions
          Api.pushOrdToInscriptions(contract.outputs?.inscription);
          // add collection new utxo (if any) to inscriptions
          Api.pushOrdToInscriptions(contract.outputs?.collection);
          // console.debug("createChunkInscription: Api.fundings:", [...Api.fundings]);

          const contractResult = await Api.createInscriptionForChunk({
            ...contractData,
            costs: contract
          });
          console.debug('createChunkInscription: contractResult: ', contractResult);

          results.push({
            draft_uuid: contractData.draft_uuid,
            contract: contractResult.contract
          });

          const contractParams = contractResult.contract.params;
          console.debug('createChunkInscription: result destination_addr:', contractParams.destination_addr);
          console.debug('createChunkInscription: result change_addr:', contractParams.change_addr);
        }
      } catch (e) {
        await Api.sendExceptionMessage("createChunkInscription", e);
      }

      const chunkResults = {
        inscription_results: results,
        last_inscription_out: parentInscription,
        error: errorMessage
      };

      // Add new current addresses
      usedAddressesMap = {...usedAddressesMap, ..._addressesMap(Api.addresses)};
      console.debug('createChunkInscription: usedAddressesMap:', usedAddressesMap);

      const usedAddresses = await Api.getAddressForSave(Object.values(usedAddressesMap));
      Api.timeSync = false;
      if(usedAddresses.length > 0) await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE,usedAddresses, chunkData?._tabId);
      await Api.sendMessageToWebPage(CREATE_CHUNK_INSCRIPTION_RESULT, chunkResults, chunkData?._tabId);
      const updatedAddresses = await Api.getAddressForSave();
      if(updatedAddresses.length > 0) await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, updatedAddresses, chunkData?._tabId);

      Scheduler.getInstance().activate();

      console.debug("createChunkInscription: chunkResults:", chunkResults);
      return chunkResults;
    }

    async function sendResult(
        action_message: string,
        result_message: string,
        result: object,
        used_wallets: Set<string> | string | undefined = undefined,
        tabId: number | undefined = undefined
    ): Promise<boolean> {
      try{
        console.log(`${action_message}.sendResult: `, {...result || {}});
        used_wallets = used_wallets || "";
        if (typeof(used_wallets) != 'string') {
          used_wallets = Array.from(used_wallets).join(' ');
        }
        // setTimeout(async () => {
          Api.WinHelpers.closeCurrentWindow();
          await Api.sendMessageToWebPage(result_message, result, tabId);
          if (await Api.generateNewIndexes(used_wallets)) {
            await Api.genKeys();
            const addresses = await Api.getAddressForSave();
            if(addresses.length > 0){
              Api.timeSync = false;
              await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses, tabId);
            }

          }
          const allAddresses = await Api.getAddressForSave();
          if(allAddresses.length > 0){await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, allAddresses, tabId);}

        // },1000);
        return true;
      } catch (exception) {
        await Api.sendExceptionMessage(action_message, exception);
        return false;
      }
    }

    onMessage(SUBMIT_SIGN, async (payload) => {
      console.debug("===== SUBMIT_SIGN: payload?.data", payload?.data)
      const signData = payload?.data;

      if (signData?.type === TRANSFER_LAZY_COLLECTION) {
        const transferData = payload?.data?.data;

        const res = await Api.decryptedWallet(payload.data.password);
        if(res){
          await Api.generateNewIndexes('ord, uns, intsk, scrsk');
          await Api.genKeys();

          await Api.sendMessageToWebPage(TRANSFER_LAZY_COLLECTION_RESULT, {
            contract: transferData?.costs?.data,
            errorMessage: transferData?.errorMessage
          }, payload?.data?.data?._tabId);
          setTimeout(async () => {
            Api.WinHelpers.closeCurrentWindow();
            const allAddresses = await Api.getAddressForSave();
            if(allAddresses.length > 0) await Api.sendMessageToWebPage(
              GET_ALL_ADDRESSES,
              allAddresses,
              payload?.data?.data?._tabId
            );
          }, 1000);
          await Api.encryptedWallet(payload.data.password);

          return true;
        }
        return false;
      }

      if (signData?.type === CREATE_CHUNK_INSCRIPTION) {
        const chunkData = payload?.data?.data;

        const res = await Api.decryptedWallet(signData.password);
        if (res) {
          const passwordId = `job:password:${chunkData.job_uuid}`;
          store.put(signData.password, passwordId);

          chunkData.inscriptions = store.pop(chunkData?.content_store_key) || [];
          delete chunkData['content_store_key'];

          const chunkResults = await createChunkInscription(chunkData);
          setTimeout(async () => {
            Api.WinHelpers.closeCurrentWindow();
          },1000);
          await Api.encryptedWallet(signData.password);

          const success = chunkResults?.inscription_results?.length == chunkData?.inscriptions?.length
              && !chunkResults.error;
          // console.debug(`SUBMIT_SIGN.CREATE_CHUNK_INSCRIPTION: success: ${success}`);
          return success;
        }
        return false;
      }

      if (signData?.type === CREATE_INSCRIPTION || signData?.type === PURCHASE_LAZY_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);  // just to check password value
        if(res){
          const is_lazy = signData?.type === PURCHASE_LAZY_INSCRIPTION;

          const payload_data = payload.data.data;
          payload_data.content = store.pop(payload_data?.content_store_key);

          // await Api.createInscription(payload_data);

          let success = true;
          if(payload_data?.costs?.data) {
            success = await sendResult(
                signData?.type,
                is_lazy ? PURCHASE_LAZY_INSCRIPTION_RESULT : CREATE_INSCRIBE_RESULT,
                {
                  contract: JSON.parse(payload_data?.costs?.data || "{}"),
                  title: payload_data?.title,
                  description: payload_data?.description,
                  type: payload_data?.type
                },
                payload_data?.costs?.used_wallets,
                payload_data?._tabId
            );
          }

          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }

      if (signData?.type === BUY_PRODUCT) {
        const res = await Api.decryptedWallet(payload.data.password);
        if(res){
          const payload_data = payload.data.data;

          let success = true;
          if(payload_data?.costs) {
            success = await sendResult(
                signData?.type,
                BUY_PRODUCT_RESULT,
                payload_data?.costs,
                "",
                payload_data?._tabId
            );
          }

          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }
      if (signData?.type === SELL_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);
        if(res){
          const payload_data = payload.data.data;

          // await Api.sellInscription(payload_data);

          let success = true;
          if(payload_data?.costs) {
            success = await sendResult(
                signData?.type,
                SELL_INSCRIBE_RESULT,
                payload_data?.costs,
                "",
                payload_data?._tabId
            );
          }

          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }

      if (signData?.type === COMMIT_BUY_INSCRIPTION) {
        const res = await Api.decryptedWallet(payload.data.password);
        Api.wallet.tmp = payload.data.password;
        if(res){
          const payload_data = payload.data.data;

          // await Api.commitBuyInscription(payload_data);

          let success = true;
          if(payload_data.costs.data && payload_data?.swap_ord_terms) {
            success = await sendResult(
                signData?.type,
                COMMIT_BUY_INSCRIBE_RESULT,
                {
                  contract_uuid: payload_data?.swap_ord_terms?.contract_uuid,
                  contract: JSON.parse(payload_data.costs.data)
                },
                "",
                payload_data?._tabId
            );
          }

          await Api.encryptedWallet(payload.data.password);
          return success;
        }
        return false;
      }
    });

    onMessage(POPUP_HEARTBEAT, async (payload) => {
      console.log('POPUP_HEARTBEAT->run')
          Watchdog.getNamedInstance(Watchdog.names.POPUP_WATCHDOG).reset();
          Scheduler.getInstance().activate();
      return true;
    });

    onMessage(BALANCE_CHANGE_PRESUMED, async (payload) => {
      console.log('BALANCE_CHANGE_PRESUMED->run')
          Scheduler.getInstance().changeScheduleTo(ScheduleName.BalanceChangePresumed);
      return true;
    });

    onMessage(ADDRESS_COPIED, async (payload) => {
      console.log('ADDRESS_COPIED->run')
          Scheduler.getInstance().changeScheduleTo(ScheduleName.AddressCopied);
      return true;
    });

    chrome.runtime.onConnect.addListener(port => {
      port.onDisconnect.addListener(() => {})
    })

    chrome.runtime.onMessageExternal.addListener(async (payload, sender) => {

          let tabId = sender?.tab?.id;
          if (typeof payload?.data === 'object' && payload?.data !== null) {
            payload.data._tabId = tabId;
          }

          console.debug(`----- message from frontend(tabId:${tabId}): ${payload?.type}, data: `, {...payload?.data || {}});

          if (payload.type === SEND_CONNECT_STATUS) {
            console.log('SEND_CONNECT_STATUS->run')
            // setTimeout(((payload)=>{
            //   return async()=>{
                await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id);
                Api.connect = payload?.data?.connected;
                postMessageToPopupIfOpen({id: UPDATE_PLUGIN_CONNECT, connect: Api.connect});
            //   };
            // })(payload), 0);
          }

          if (payload.type === CONNECT_TO_PLUGIN) {
            console.log('CONNECT_TO_PLUGIN->run')
            // setTimeout(((payload, tabId)=>{
            //   return async()=>{
                await Api.sendMessageToWebPage(PLUGIN_ID, chrome.runtime.id, tabId);
                let success = await Api.signToChallenge(payload.data, tabId);
                if (success) {
                  await postMessageToPopupIfOpen({id: UPDATE_PLUGIN_CONNECT, connect: true});
                  await refreshBalanceAndAdressed(tabId);
                }
          //   };
          // })(payload, tabId), 0);
          }


          if (payload.type === SEND_BALANCES) {
            console.log('SEND_BALANCES->run')
            // setTimeout(((payload)=>{
            //   return async()=>{
                await reConnectSession(true);
                console.log('SEND_BALANCES:',payload.data)
                if (payload.data?.addresses) {
                  Api.sync = true;    // FIXME: Seems useless because happening too much late.
                  Api.connect = true; // FIXME: However it's working for some reason in v1.1.5.
                                    // FIXME: Probably due to high balance refresh frequency.
                  Api.balances = await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);
                  console.log('Api.balances:',Api.balances);

                  const balance = await Api.fetchBalance("UNUSED_VALUE");  // FIXME: currently address is still unused
                  postMessageToPopupIfOpen({ id: BALANCE_REFRESH_DONE,  data: balance?.data});
                }
            //   };
            // })(payload), 0);
          }

          if (payload.type === GET_ALL_ADDRESSES) {
            console.log('GET_ALL_ADDRESSES->run')
            await reConnectSession(true);
             setTimeout(((payload,tabId)=>{
               return async()=>{
                console.log('GET_ALL_ADDRESSES: payload.data.addresses:', payload.data.addresses);
                await Api.restoreAllTypeIndexes(payload.data.addresses);
                const newKeys = await Api.genKeys();
                let allAddresses = await Api.getAddressForSave(Api.addresses);
                Api.all_addresses = await Api.prepareAddressToPlugin(payload.data.addresses); // used to determine addresses stored on the server
                const allAddressesSaved = await Api.hasAllLocalAddressesIn(payload.data.addresses);
                if(!allAddressesSaved){
                  allAddresses = await Api.getAddressForSave();
                  if(allAddresses.length>0){
                      Api.timeSync = false;
                      await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, allAddresses, tabId);
                  }
                }
                console.log('Api.restoreAllTypeIndexes:',payload.data.addresses);

            };
          })(payload, tabId), 0);
          }

          if (payload.type === GET_INSCRIPTION_CONTRACT || payload.type === ESTIMATE_PURCHASE_LAZY_INSCRIPTION) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);
            const is_lazy = payload.type === ESTIMATE_PURCHASE_LAZY_INSCRIPTION;
            const contract = await Api.createInscriptionContract({...payload.data, is_lazy}, true);
            await Api.sendMessageToWebPage(is_lazy ? ESTIMATE_PURCHASE_LAZY_INSCRIPTION_RESULT : GET_INSCRIPTION_CONTRACT_RESULT, contract, tabId);
          }

          if (payload.type === GET_BULK_INSCRIPTION_ESTIMATION) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            const data = payload.data as IBulkInscriptionEstimation;
            let bulkAmount = 0;
            let bulkExpectAmount = 0;
            for (const item of data.inscriptions_content) {
              item.expect_amount = data.expect_amount;
              item.fee_rate = data.fee_rate;
              item.fee = data.fee;
              const contract = await Api.estimateInscription(item);
              bulkAmount += contract.amount;
              bulkExpectAmount += contract.expect_amount;
            }
            await Api.sendMessageToWebPage(GET_BULK_INSCRIPTION_ESTIMATION_RESULT, {
              amount: bulkAmount,
              expect_amount: bulkExpectAmount
            }, tabId);
          }

          function _fixChunkInscriptionPayload(data: IChunkInscription) {
            for (let inscrData of Array.from(data?.inscriptions || [])) {
              inscrData.expect_amount = data.expect_amount;  // per item amount
              inscrData.fee_rate = data.fee_rate;  // per item mining fee rate
              inscrData.fee = data.fee;   // per item platform/market fee (if any)
            }
            return data;
          }

          function _prepareInscriptionForPopup(data: object) {
            // console.debug('_prepareInscriptionForPopup data:', {...data || {}});
            data.market_fee = data.platform_fee || 0;  // total platform/market fee
            data.costs = {
              expect_amount: data.total_expect_amount,  // total expect_amount
              amount: data.total_amount,  // total amount (including all total fees)
              fee_rate: data.fee_rate,  // per item mining fee rate
              mining_fee: data.total_mining_fee || 0,  // total mining fee
              fee: data.fee, // per item platform/market fee (if any)
              metadata: data.metadata,
              raw: []
            };
            // console.debug('_prepareInscriptionForPopup result:', {...data || {}});
            return data;
          }

          if (payload.type === CREATE_CHUNK_INSCRIPTION) {
            console.debug(`${CREATE_CHUNK_INSCRIPTION}: payload?.data:`, {...payload?.data || {}});
            // console.log('payload?.data?.type:', payload?.data?.type);
            // console.log('payload?.data?.collection?.genesis_txid:', payload?.data?.collection?.genesis_txid);

            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            let chunkData = _fixChunkInscriptionPayload(payload?.data) as IChunkInscription;

            const passwordId = `job:password:${chunkData.job_uuid}`;
            const password = store.get(passwordId);
            if (!password) {
              // Store content locally in background script. Don't pass it via message
              // NOTE: update for each chunk
              chunkData.content_store_key = store.put(chunkData.inscriptions);
              chunkData.inscriptions = [];

              // Simulate some CreateInscriptionContract fields setup to make popup use them
              _prepareInscriptionForPopup(chunkData) as IChunkInscription;

              winManager.openWindow('sign-create-inscription', async (id) => {
                setTimeout(async () => {
                  await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
                }, 1000);
              }, Api.viewMode);
              return true;
            } else {
              const res = await Api.decryptedWallet(password);
              if (res) {
                const chunkResults = await createChunkInscription(chunkData);
                await Api.encryptedWallet(password);
                const success = chunkResults?.inscription_results?.length == chunkData?.inscriptions?.length
                    && !chunkResults.error;
                // console.debug(`CREATE_CHUNK_INSCRIPTION: success: ${success}`);
                return success;
              }
              return false;
            }
          }

          if (payload.type === ESTIMATE_TRANSFER_LAZY_COLLECTION) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            const contract = await Api.transferForLazyInscriptionContract(payload.data, true);
            await Api.sendMessageToWebPage(ESTIMATE_TRANSFER_LAZY_COLLECTION_RESULT, {
              contract,
              errorMessage: contract.errorMessage
            }, tabId);
          }

          if (payload.type === TRANSFER_LAZY_COLLECTION) {
            let costs;
            console.log('payload:', {...payload})
            // console.log('payload?.data?.type:', payload?.data?.type)
            // console.log('payload?.data?.collection?.owner_txid:', payload?.data?.collection?.genesis_txid);

            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            costs = await Api.transferForLazyInscriptionContract(payload.data);
            console.log('costs:', costs)
            payload.data.costs = costs;

            payload.data.errorMessage = payload.data?.costs?.errorMessage;
            if(payload.data?.costs?.errorMessage) delete payload.data?.costs['errorMessage'];

            winManager.openWindow('sign-transfer-collection', async (id) => {
              setTimeout(async  () => {
                const changeAmount = payload.data.costs.change_amount
                if (changeAmount !== null  && changeAmount < 546) {
                  Api.sendNotificationMessage(
                    'TRANSFER_LAZY_COLLECTION',
                    'There are too few coins left after creation and they will become part of the inscription balance'
                  );
                }
                await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
          }

          if (payload.type === CREATE_INSCRIPTION || payload.type === PURCHASE_LAZY_INSCRIPTION) {
            let costs;
            console.log('payload:', {...payload})
            console.log('payload?.data?.type:', payload?.data?.type)
            console.log('payload?.data?.collection?.genesis_txid:', payload?.data?.collection?.genesis_txid);

            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            const is_lazy = payload.type === PURCHASE_LAZY_INSCRIPTION;
            costs = await Api.createInscriptionContract({...payload.data, is_lazy});
            console.log('costs:', costs)

            payload.data.costs = costs;
            console.log(`${payload.type}:`, {...payload.data});
            payload.data.content_store_key = store.put(payload.data.content);
            payload.data.content = null;
            payload.data.errorMessage = payload.data?.costs?.errorMessage;
            if(payload.data?.costs?.errorMessage) delete payload.data?.costs['errorMessage'];
            console.log(`${payload.type} (stored):`, {...payload.data});
            winManager.openWindow('sign-create-inscription', async (id) => {
              setTimeout(async  () => {
                // TODO: core API InscriptionBuilder needs to have change-related stuff implemented
                // const changeAmount = payload.data.costs.change_amount
                // if (changeAmount !== null && changeAmount < 546) {
                //   Api.sendNotificationMessage(
                //     payload.type,
                //     'There are too few coins left after creation and they will become part of the inscription balance'
                //   );
                // }
                await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
          }

          if (payload.type === BUY_PRODUCT) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            const costs = await Api.BuyProductContract(payload.data);

            payload.data.errorMessage = payload.data?.costs?.errorMessage;
            if(payload.data?.costs?.errorMessage) delete payload.data?.costs['errorMessage'];

            payload.data.costs = costs;
            console.log(BUY_PRODUCT+':',payload.data);
            winManager.openWindow('sign-buy-product', async (id) => {
              setTimeout(async  () => {
                const changeAmount = payload.data.costs.change_amount
                if (changeAmount !== null  && changeAmount < 546) {
                  Api.sendNotificationMessage(
                    'BUY_PRODUCT',
                    'There are too few coins left after creation and they will become part of the inscription balance'
                  );
                }
                await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
          }

          if (payload.type === SELL_INSCRIPTION) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            const costs = await Api.sellInscriptionContract(payload.data);
            payload.data.costs = costs;
            console.log(SELL_INSCRIPTION+':',payload.data);
            winManager.openWindow('sign-sell', async (id) => {
              setTimeout(async  () => {
                await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
          }

          if (payload.type === COMMIT_BUY_INSCRIPTION) {
            await Api.updateBalancesFrom(payload.type, payload?.data?.addresses);

            payload.data.costs = await Api.commitBuyInscriptionContract(payload.data);
            payload.data.errorMessage = payload.data?.costs?.errorMessage;
            if(payload.data?.costs?.errorMessage) delete payload.data?.costs['errorMessage'];
            console.log(COMMIT_BUY_INSCRIPTION+':',payload);
            //update balances before openWindow
            winManager.openWindow('sign-commit-buy', async (id) => {
              setTimeout(async () => {
                await sendMessage(SAVE_DATA_FOR_SIGN, payload, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
          }

          if (payload.type === OPEN_EXPORT_KEY_PAIR_SCREEN) {
            winManager.openWindow('export-keys', async (id) => {
              setTimeout(async () => {
                await sendMessage(SAVE_DATA_FOR_EXPORT_KEY_PAIR, payload.data, `popup@${id}`);
              }, 1000);
            }, Api.viewMode);
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
            console.log('OPEN_START_PAGE->run')
            winManager.openWindow('start',null, Api.viewMode);
          }

    });

    // SET PLUGIN ID TO WEB PAGE
    async function sendhello(tabId: number | undefined = undefined) {
      // console.log('sendhello->run')
      helloSite(tabId);
      const versions = await Api.getSupportedVersions();
      await Api.sendMessageToWebPage(PLUGIN_SUPPORTED_VERSIONS, versions, tabId);
      return true;
    }

    chrome.tabs.onActivated.addListener(async (activeInfo) => {
        //console.log('chrome.tabs.onActivated')
        await sendhello(activeInfo.tabId);
    });

    chrome.tabs.onCreated.addListener(async (tab) => {
      //console.log('chrome.tabs.onCreated')
        await sendhello(tab.id);
    });

    chrome.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
      //console.log('chrome.tabs.onUpdated')
        await sendhello(tabId);
    });

    chrome.tabs.onReplaced.addListener(async (addedTabId, removedTabId) => {
      //console.log('chrome.tabs.onReplaced')
        await sendhello(addedTabId);
    });


    let alarmName = "utxord_wallet.alarm.balance_refresh_main_schedule";
    await chrome.alarms.clear(alarmName);
    await chrome.alarms.create(alarmName, { periodInMinutes: 10 });
    chrome.alarms.onAlarm.addListener(async (alarm) => {
      if (alarm.name == alarmName) {
        const success = await Api.checkSeed();
        await Api.sendMessageToWebPage(AUTH_STATUS, success);
        if(success){
          const addresses = await Api.getAddressForSave();
          await Api.sendMessageToWebPage(GET_BALANCES, addresses);
          if(addresses.length > 0){
            Api.timeSync = false;
            await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, addresses);
            await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, addresses);
          }
        }
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
