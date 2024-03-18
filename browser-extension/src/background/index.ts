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
  CHANGE_TYPE_FUND_ADDRESS,
  CHANGE_USE_DERIVATION,
  STATUS_DERIVATION,
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
//   name: string,
//   description: string,
//   metadata: {
//     name?: string,
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

        postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: Api?.connect});
        Scheduler.getInstance().action = async () => {
          postMessageToPopupIfOpen({id: DO_REFRESH_BALANCE, connect: Api?.connect});
        }
      });
    });

    const Api = await new self.Api(NETWORK);
    if(NETWORK === TESTNET){
      self.api = Api; // for debuging in devtols
    }
    const winManager = new WinManager();

    onMessage(GENERATE_MNEMONIC, async (payload) => {
      return await Api.generateMnemonic(payload.data?.length);
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
      await Api.setUpSeed(payload.data.seed, payload.data?.passphrase);
      Api.genKeys();
      if(Api.wallet.auth.key) {
        await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key?.PubKey());
      }
      return Api.checkSeed();
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
        await Api.updateAddressesChallenges();
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
      const {addresses} = Api.genKeys();
      console.log('GET_ADDRESSES->addresses:',addresses)
      return addresses;
    });

    onMessage(NEW_FUND_ADDRESS, async () => {
      await Api.generateNewIndex('fund');
      const newKeys = Api.genKeys();
      await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses);
      return newKeys;
    });

    onMessage(CHANGE_TYPE_FUND_ADDRESS, async (payload: any) => {
      console.log('CHANGE_TYPE_FUND_ADDRESS:',payload.data?.type);
      await Api.setTypeAddress('fund', payload.data?.type);
      const newKeys = Api.genKeys();
      console.log('newKeys', newKeys);
      await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses);
      return newKeys;
    });

    onMessage(CHANGE_USE_DERIVATION, async (payload: any) => {
      console.log('CHANGE_USE_DERIVATION:',payload.data?.value);
      await Api.setDerivate(payload.data?.value);
      const newKeys = Api.genKeys();
      console.log('newKeys', newKeys);
      return {derivate: Api.derivate, keys: newKeys};
    });

    onMessage(STATUS_DERIVATION, () => {
      console.log('STATUS_DERIVATION:', Api.derivate);
      const newKeys = Api.genKeys();
      console.log('newKeys', newKeys);
      return {derivate: Api.derivate, keys: newKeys};
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
          const contract = await Api.createInscriptionContract(contractData, true);
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
              owner_txid: contract.outputs?.collection?.txid,
              owner_nout: contract.outputs?.collection?.nout,
            };
          }
          console.debug('createChunkInscription: parentInscription:', parentInscription);

          // mark all used utxo as spent
          Api.updateFundsByOutputs(contract.utxo_list, {is_locked: true});

          // add change utxo (if any) to funding
          const change = contract.outputs?.change;
          if (change?.txid) {
            Api.pushChangeToFunds(change);
          }
          // add ordinal utxo (if any) to inscriptions
          const ordinal = contract.outputs?.inscription;
          if (ordinal?.txid) {
            Api.pushOrdToInscriptions(ordinal);
          }
          // add collection new utxo (if any) to inscriptions
          const collection = contract.outputs?.collection;
          if (collection?.txid) {
            Api.pushOrdToInscriptions(collection);
          }
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
      await Api.updateAddressesChallenges();
      await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, Object.values(usedAddressesMap), chunkData?._tabId);
      await Api.sendMessageToWebPage(CREATE_CHUNK_INSCRIPTION_RESULT, chunkResults, chunkData?._tabId);
      await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, chunkData?._tabId);

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
            Api.genKeys();
            await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, Api.addresses, tabId);
          }
          await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, tabId);
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
          Api.genKeys();

          await Api.sendMessageToWebPage(TRANSFER_LAZY_COLLECTION_RESULT, {
            contract: transferData?.costs?.data,
            errorMessage: transferData?.errorMessage
          }, payload?.data?.data?._tabId);
          setTimeout(async () => {
            Api.WinHelpers.closeCurrentWindow();
            await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, Api.addresses, payload?.data?.data?._tabId);
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
                  name: payload_data?.name,
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
      console.debug(`----- message from frontend: ${payload?.type}, data: `, {...payload?.data || {}});

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
        if (payload.data?.addresses) {
          Api.balances = payload.data;
          // -------
          Api.sync = true;    // FIXME: Seems useless because happening too much late.
          Api.connect = true; // FIXME: However it's working for some reason in v1.1.5.
                              // FIXME: Probably due to high balance refresh frequency.
          // console.log('payload.data.addresses: ',payload.data.addresses);
          Api.fundings = await Api.freeBalance(Api.fundings);
          Api.inscriptions = await Api.freeBalance(Api.inscriptions);
          const balances = await Api.prepareBalances(payload.data.addresses);
          Api.fundings = balances.funds;
          Api.inscriptions = balances.inscriptions;
          // console.log('Api.fundings:', Api.fundings);
          // console.log('Api.inscriptions:', Api.inscriptions);

          const balance = await Api.fetchBalance("UNUSED_VALUE");  // FIXME: currently address is still unused
          console.debug('SEND_BALANCES: Api.fetchBalance:', balance);
          setTimeout(async () => {
            postMessageToPopupIfOpen({ id: BALANCE_REFRESH_DONE, data: { balance: balance?.data }});
          }, 1000);
          // -------
        }
      }

      if (payload.type === GET_ALL_ADDRESSES) {
        console.log('GET_ALL_ADDRESSES: payload.data.addresses:', payload.data.addresses);
        console.debug('GET_ALL_ADDRESSES: Api.addresses:', [...Api.addresses]);
        const allAddressesSaved = Api.hasAllLocalAddressesIn(payload.data.addresses);
        console.debug('Api.hasAllLocalAddressesIn payload.data.addresses: ', allAddressesSaved);
        if(!allAddressesSaved){
          setTimeout(async () => {
                  await Api.updateAddressesChallenges();
                  console.debug('ADDRESSES_TO_SAVE:', [...Api.addresses]);
                  await Api.sendMessageToWebPage(ADDRESSES_TO_SAVE, Api.addresses, tabId);
          }, 100);
        }
        // console.log('Api.restoreAllTypeIndexes:',payload.data.addresses);
        await Api.restoreAllTypeIndexes(payload.data.addresses);
        console.debug('GET_ALL_ADDRESSES: Api.addresses:', [...Api.addresses]);
      }

      if (payload.type === GET_INSCRIPTION_CONTRACT || payload.type === ESTIMATE_PURCHASE_LAZY_INSCRIPTION) {
        const is_lazy = payload.type === ESTIMATE_PURCHASE_LAZY_INSCRIPTION;
        const contract = await Api.createInscriptionContract({...payload.data, is_lazy});
        await Api.sendMessageToWebPage(is_lazy ? ESTIMATE_PURCHASE_LAZY_INSCRIPTION_RESULT : GET_INSCRIPTION_CONTRACT_RESULT, contract);
      }

      if (payload.type === GET_BULK_INSCRIPTION_ESTIMATION) {
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
        });
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
          });
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
        const balances = await Api.prepareBalances(payload?.data?.addresses);
        console.debug('ESTIMATE_TRANSFER_LAZY_COLLECTION balances:', {...balances || {}});
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

        const contract = await Api.transferForLazyInscriptionContract(payload.data);
        await Api.sendMessageToWebPage(ESTIMATE_TRANSFER_LAZY_COLLECTION_RESULT, {
          contract,
          errorMessage: contract.errorMessage
        });
      }

      if (payload.type === TRANSFER_LAZY_COLLECTION) {
        let costs;
        console.log('payload:', {...payload})
        // console.log('payload?.data?.type:', payload?.data?.type)
        // console.log('payload?.data?.collection?.owner_txid:', payload?.data?.collection?.genesis_txid);

        const balances = await Api.prepareBalances(payload?.data?.addresses);
        console.debug('TRANSFER_LAZY_COLLECTION balances:', {...balances || {}});
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

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
        });
      }

      if (payload.type === CREATE_INSCRIPTION || payload.type === PURCHASE_LAZY_INSCRIPTION) {
        let costs;
        console.log('payload:', {...payload})
        console.log('payload?.data?.type:', payload?.data?.type)
        console.log('payload?.data?.collection?.genesis_txid:', payload?.data?.collection?.genesis_txid);

        const balances = await Api.prepareBalances(payload?.data?.addresses);
        console.debug(`${payload.type} balances:`, {...balances || {}});
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

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
        });
      }

      if (payload.type === SELL_INSCRIPTION) {
        const balances = await Api.prepareBalances(payload?.data?.addresses);
        console.debug('SELL_INSCRIPTION balances:', {...balances || {}});
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

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
        const balances = await Api.prepareBalances(payload?.data?.addresses);
        console.debug('COMMIT_BUY_INSCRIPTION balances:', {...balances || {}});
        Api.fundings = balances.funds;
        Api.inscriptions = balances.inscriptions;

        payload.data.costs = await Api.commitBuyInscriptionContract(payload.data);
        payload.data.errorMessage = payload.data?.costs?.errorMessage;
        if(payload.data?.costs?.errorMessage) delete payload.data?.costs['errorMessage'];
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
      if(Api.wallet.auth.key) await Api.sendMessageToWebPage(PLUGIN_PUBLIC_KEY, Api.wallet.auth.key?.PubKey());
      const versions = await Api.getSupportedVersions();
      console.log(PLUGIN_SUPPORTED_VERSIONS, versions);
      await Api.sendMessageToWebPage(PLUGIN_SUPPORTED_VERSIONS, versions);
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
        // await Api.sendMessageToWebPage(GET_BALANCES, [...Api.addresses]);
        // await Api.sendMessageToWebPage(GET_ALL_ADDRESSES, [...Api.addresses]);
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
