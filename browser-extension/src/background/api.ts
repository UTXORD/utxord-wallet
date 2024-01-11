import '~/libs/utxord.js';
import * as bip39 from '~/libs/bip39.browser.js';
import '~/libs/safe-buffer.js';
import '~/libs/crypto-js.js';
import winHelpers from '~/helpers/winHelpers';
import rest from '~/background/rest';
import { sendMessage } from 'webext-bridge';
import * as cbor from 'cbor-js';
import {
  EXCEPTION,
  WARNING,
  NOTIFICATION,
  SELL_INSCRIPTION,
  CREATE_INSCRIPTION,
  COMMIT_BUY_INSCRIPTION,
  BUY_INSCRIPTION,
  SELL_INSCRIBE_RESULT,
  COMMIT_BUY_INSCRIBE_RESULT,
  SIGN_BUY_INSCRIBE_RESULT,
  CONNECT_RESULT,
  ADDRESSES_TO_SAVE,
  CREATE_INSCRIBE_RESULT,
  DECRYPTED_WALLET
} from '~/config/events';
import { BASE_URL_PATTERN } from '~/config/index';
import Tab = chrome.tabs.Tab;
import {Exception} from "sass";

// import tabId = chrome.devtools.inspectedWindow.tabId;

const limitQuery = 1000;
let bgSiteQueryIndex = 0;
const closeWindowAfter = 6000;


const WALLET_TYPES = [
  'oth',
  'fund',
  'ord',
  'uns',
  'intsk',
  'scrsk',
  'auth'
];

const WALLET = {
  encrypted: false,
  secret: 'secret',
  tmp: null,
  root: {
    nick: null,
    seed: null,
    key: null,
  },
  oth: { // for other wallet systems
    index: 0,
    change: 0,
    account: 0,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["0'"],
      change: ["0"],
      index_range: "0-16384"
    }
  },
  fund: { // funding
    index: 0,
    change: 0,
    account: 1,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["1'"],
      change: ["0"],
      index_range: "0-16384"
    }
  },
  ord: { // ordinal
    index: 0,
    change: 0,
    account: 2,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["2'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  uns: { // unspendable
    index: 0,
    change: 0,
    account: 3,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["3'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  intsk:{ // internalSK
    index: 0,
    change: 0,
    account: 4,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["4'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  scrsk: { //scriptSK
    index: 0,
    change: 0,
    account: 5,
    coin_type: 1,
    key: null,
    p2tr: null,
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["5'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  xord:[],
  ext: {
    seeds: [{
      seed: null,
      sub: [{
        path: null,
        key: null,
        p2tr: null,
        pubKeyStr: null,
        privKeyStr: null,
      }],
      rootKey: null,
      rootP2tr: null,
      rootPubKeyStr: null,
      rootPrivKeyStr: null,
      type: null,
      }],
    paths: [{
      path: null,
      key: null,
      p2tr: null,
      pubKeyStr: null,
      privKeyStr: null,
      type: null,
      }],
    keys: []
  },
  auth: { //for auth keys
    index: 0,
    change: 214748364,
    account: 214748364,
    coin_type: 214748364,
    key: null,
    filter: {
      look_cache: true,
      key_type: "AUTH",
      accounts: ["214748364'"],
      change: ["214748364"],
      index_range: "0-1"
    }
  }
};

const STATUS_DEFAULT = {
    initAccountData: false,
};

class Api {
  KNOWN_CORE_ERRORS: string[] = [
    'ReferenceError',
    'TypeError',
    'ContractTermMissing',
    'ContractTermWrongValue',
    'ContractValueMismatch',
    'ContractTermWrongFormat',
    'ContractStateError',
    'ContractProtocolError',
    'SignatureError',
    'WrongKeyError',
    'KeyError',
    'TransactionError'
  ]

  constructor(network) {
    return (async () => {
      try {
        const myself = this;
        this.WinHelpers = new winHelpers();
        this.Rest = new rest();
        this.utxord = await utxord();
        this.bip39 = await bip39;
        this.network = this.setUpNetWork(network);
        this.bech = new this.utxord.Bech32(this.network);

        this.locked = false;
        this.status = STATUS_DEFAULT;
        this.wallet = WALLET;
        this.wallet_types = WALLET_TYPES;
        this.addresses = [];

        this.balances = [];
        this.fundings = [];
        this.inscriptions = [];
        this.all_addresses = [];
        this.connect = false;
        this.sync = false;

        await this.init(this);
        return this;
      } catch(e) {
        console.log('constructor->error:',e);
        chrome.runtime.reload();

      }
    })();
  }

  async init() {
    const myself = this
    try {
      const { seed } = await chrome.storage.sync.get(['seed']);
      if (seed) {
        myself.wallet.root.seed = seed;
      }
      const { ext_keys } = await chrome.storage.sync.get(['ext_keys']);
      if(ext_keys) {
        if(ext_keys.length > 0) {
           myself.resExtKeys(ext_keys);
        }
      }
      const { xord_keys } = await chrome.storage.sync.get(['xord_keys']);
      if(xord_keys) {
        if(xord_keys.length > 0) {
           myself.resXordKeys(xord_keys);
        }
      }
      await myself.rememberIndexes();
      console.log('init...');
      myself.genRootKey();
      if (myself.checkSeed() && myself.utxord && myself.bech) {
        myself.genKeys();
        myself.initPassword();
        const fund = myself.wallet.fund.key?.GetLocalPubKey()?.c_str();
        const auth = myself.wallet.auth.key?.GetLocalPubKey()?.c_str();
        if (fund && auth) {
          myself.status.initAccountData = true;
          return myself.status.initAccountData;
        }
      }
    } catch (e) {
      console.log('init()->error:', myself.getErrorMessage(e));
    }
  }

  async setIndexToStorage(type, value) {
    if(type==='xord' || type==='ext') return;
    const Obj = {};
    Obj[`${type}Index`] = Number(value);
    return await chrome.storage.sync.set(Obj);
  }

  async getIndexToStorage(type) {
    if(type==='xord' || type==='ext') return;
    return (await chrome.storage.sync.get([`${type}Index`]))[`${type}Index`] || 0
  }

  async rememberIndexes() {
    for (const wt of this.wallet_types) {
      this.wallet[wt].index = await this.getIndexToStorage(wt);
    }
    return true;
  }

  path(type) {
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='xord' || type==='ext') return false;
    //m / purpose' / coin_type' / account' / change / index
    let t = this.wallet[type].coin_type;
    if(this.network === this.utxord.MAINNET  && type!=='auth')  t = 0;
    const a = this.wallet[type].account;
    const c = this.wallet[type].change;
    const i = this.wallet[type].index;
     return `m/86'/${t}'/${a}'/${c}/${i}`;
  }

  async generateNewIndex(type) {
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='xord' || type==='ext') return false;
    this.wallet[type].index += 1;
    return await this.setIndexToStorage(type, this.wallet[type].index);
  }

  getIndex(type) {
    if(type==='xord' || type==='ext') return 0;
    if (this.wallet[type]) return this.wallet[type].index;
    return 0;
  }

  setIndex(type, index) {
    if(type==='xord' || type==='ext') return false;
    this.wallet[type].index = index;
    return true;
  }

  getNexIndex(type) {
    if(type==='xord' || type==='ext') return 0;
    return this.wallet[type].index + 1;
  }

  checkAddresess(ext_addresses = []) {
    if(ext_addresses.length === 0) return false;
    if(this.addresses.length === 0) return false;

    const list = [];
    for(const item of ext_addresses) {
      list.push(item.address);
    }
    for(const value of this.addresses) {
      if(list.indexOf(value.address) === -1){return false;}
    }
    return true;
  }

  checkAddress(address, addresses) {
    if(!addresses){addresses = this.addresses;}
    for(const item of addresses) {
      if(item.address === address){return true;}
    }
    return false;
  }

  checkAddressType(type, addresses) {
    if(!addresses){addresses = this.addresses;}
    for(const item of addresses) {
      if(item.type === type){return true;}
    }
    return false;
  }


  async restoreTypeIndexFromServer(type, addresses) {
    let sindex = 0;
    let store_index = 0;
    let wallet_index = 0;
    if(type!=='xord' && type!=='ext') {
      sindex = await this.getIndexToStorage(type);
      store_index = Number(sindex);
      wallet_index = Number(this.getIndex(type));
      if(store_index > wallet_index) {
        await this.setIndex(type, store_index)
      }
      if(store_index < wallet_index) {
        await this.setIndexToStorage(type, wallet_index);
      }
    }
    if(addresses) {
      for(const v of addresses) {
        if(v.type===type && type!=='xord' && type!=='ext') {
          const currind = Number(v.index.split('/').pop())
          if (currind > wallet_index) {
          //  console.log(`currind for type - ${type}:`,currind)
            await this.setIndex(type, currind)
            await this.setIndexToStorage(type, currind);
          }
        }else{
          if(type==='xord') {
            let p = v?.index?.split('/')[0];
            let xord = v?.index?.split('/')[1];
            this.addToXordPubKey(xord);
          }
          if(type==='ext') {
            let p = v?.index?.split('/')[0];
            let ext = v?.index?.split('/')[1];
            this.addToExternalKey(ext);
          }
        }

      }
    }
  }
  async restoreAllTypeIndexes(addresses) {
    for (const key of this.wallet_types) {
      //console.log(`this.restoreTypeIndexFromServer(${key}, ${addresses})`)
      await this.restoreTypeIndexFromServer(key, addresses)
    }
    return await this.genKeys();
  }

  setSeed(mnemonic, password) {
    const myself = this;
    // the password will not be used to generate seed phrases, only for encryption
    const seed = myself.bip39.mnemonicToSeedSync(mnemonic);
    chrome.storage.sync.set({ seed: seed.toString('hex') });
    myself.wallet.root.seed = seed.toString('hex');
    return seed;
  }

  setNick(nick) {
    const myself = this;
    myself.wallet.root.nick = nick;
    chrome.storage.sync.set({ nick: nick });
    return true;
  }

  getSeed() {
    return this.wallet.root.seed;
  }

  checkSeed() {
    if (this.getSeed()?.length > 0) return true;
    return false;
  }

  async unload() {

    this.locked = false;
    this.status = STATUS_DEFAULT;
    this.wallet = WALLET;
    this.wallet_types = WALLET_TYPES;
    this.addresses = [];

    this.balances = [];
    this.fundings = [];
    this.inscriptions = [];
    this.connect = false;
    this.sync = false;

    await chrome.storage.sync.clear();
    chrome.runtime.reload()
    const err = chrome.runtime.lastError;
    if (err) {
      console.error(err)
      return false
    } else {
      return true
    }
  }

  genRootKey() {
    if(!this.checkSeed()) return false;
    if (this.wallet.root.key) return this.wallet.root.key;
    console.log('seed:',this.getSeed());
    this.wallet.root.key = new this.utxord.KeyRegistry(this.network, this.getSeed());
    for(const type of this.wallet_types) {
      if(type !== 'auth') {
        console.log('type: ',type,'|json: ',JSON.stringify(this.wallet[type].filter))
        this.wallet.root.key.AddKeyType(type, JSON.stringify(this.wallet[type].filter));
      }
    }
    return this.wallet.root.key;
  }

  resXordKeys(xord_keys) {
    this.wallet.xord = [];
    for(const item of xord_keys) {
      this.addToXordPubKey(item);
    }
    return this.wallet.xord;
  }

  pubKeyStrToP2tr(publicKey) {
    return this.bech.Encode(publicKey).c_str();
  }

  addToXordPubKey(xordPubkey) {
    const myself = this;
    const tmpAddress = myself.pubKeyStrToP2tr(xordPubkey);
    if(!myself.checkAddress(tmpAddress, myself.wallet.xord)) {
      myself.wallet.xord.push({
        p2tr: tmpAddress,
        pubKeyStr: xordPubkey,
        index: `0/${xordPubkey}`
      });
      const xord_keys = []
      for(const item of this.wallet.xord) {
        xord_keys.push(item.pubKeyStr);
      }
      chrome.storage.sync.set({ xord_keys: xord_keys});
    }
    return true;
  }

  addToExternalKey(keyhex, pass) {
    const myself = this;

    const keypair = new myself.utxord.KeyPair(keyhex);
    const tmpAddress = this.bech.Encode(keypair.PubKey());
    if(!myself.checkAddress(tmpAddress, this.wallet.ext.keys)) {
      let enkeyhex = keyhex;
      let enFlag = false;
      if(pass) {
        enkeyhex = this.encrypt(keyhex, pass);
        enFlag = true;
      }
      console.log('ExternalKeyAddress:',tmpAddress);
      this.wallet.ext.keys.push({
        key: keypair,
        p2tr: tmpAddress,
        pubKeyStr: keypair.PubKey(),
        privKeyStr: enkeyhex,
        index:`${Number(enFlag)}/${enkeyhex}`,
        type: 'ext',
      });
      const ext_keys = []
      for(const item of this.wallet.ext.keys) {
        ext_keys.push(item.privKeyStr);
      }
      chrome.storage.sync.set({ ext_keys: ext_keys});
    }
    return true;
  }

  resExtKeys(ext_keys) {
    this.wallet.ext.keys = [];
    for(const item of ext_keys) {
      this.addToExternalKey(item);
    }
    return this.wallet.ext.keys;
  }

  genKey(type) {
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    this.genRootKey();
    const for_script = (type === 'uns' || type === 'intsk' || type === 'scrsk' || type === 'auth');
    this.wallet[type].key = this.wallet.root.key.Derive(this.path(type), for_script);
    this.wallet[type].address = this.wallet[type].key.GetP2TRAddress(this.network);
     return true;
  }

  genKeys() { //current keys
    const publicKeys = [];
    for(const key of this.wallet_types) {
      if(this.genKey(key)) {
        if(key!=='auth' && key!=='xord' && key!=='ext') {
          if(!this.checkAddress(this.wallet[key].address)) {
            if(!this.checkAddressType(key)) {
              this.addresses.push({
                address: this.wallet[key].address,
                type: key,
                index: this.path(key)
              });
            }else{
              for(const i in this.addresses) {
                if(this.addresses[i].type === key) {
                  this.addresses[i] = {
                    address: this.wallet[key].address,
                    type: key,
                    index: this.path(key)
                  };
                }
              }
            }
            publicKeys.push({pubKeyStr: this.wallet[key].key.PubKey(), type: key});
          }
        }
      }
    }
    //add ExternalKeyAddress
    //add XordPubKey
    for(const item of this.wallet.xord) {
      if(!this.checkAddress(item.address)) {
        this.addresses.push({
          address: item.address,
          type: 'xord',
          index: item.index
        });
        publicKeys.push({pubKeyStr: item.pubKeyStr, type: 'xord'});
      }
    }

    return { addresses: this.addresses, publicKeys };
  }

  async getBranchKey(path, item) {
    try{
      if(path[0]==='m') {
        const for_script = (item?.type === 'uns' || item?.type === 'intsk' || item?.type === 'scrsk' || item?.type === 'auth');
        const keypair = this.wallet.root.key.Derive(path, for_script);
        const address = keypair.GetP2TRAddress(this.network);
        return {address: address, key: keypair};
      }else{
        if(item?.type==='ext' || item?.type==='xord') {
          let pubkey='';
          if(item?.type === 'xord'){ pubkey = item.index.split('/')[1]; }
          return {address: item.address, key: { pubKeyStr: pubkey }};
        }
      }
    }catch(e) {
      console.log("getBranchKey->error:",e);
    }
  }

  async getAllAddresses(addresses) {
    const myself = this;
    const ret = [];
    if(addresses?.length) {
      for(const item of addresses) {
        let br = await myself.getBranchKey(item?.index, item);
        if(br?.address !== item?.address) {
          console.log("skip: getAllAddresses->1|address:",br?.address,"|item.address:",item?.address);
        }else{
          ret.push({
            ...item,
            path: item.index,
            key: br.key,
          });
        }
      }
    }
   return ret;
  }

 async freeBalance(balance) {
   for(const item of balance) {
     this.destroy(item.key);
   }
   return [];
 }

  async prepareBalances(balances) {
     const myself = this;
     let list = this.balances?.addresses;
     if(balances) {
       list = balances;
     }
     const funds = [];
     const inscriptions = [];
     if(list?.length) {
       for (const item of list) {
         for (const i of item?.utxo_set || []) {
           let br = await myself.getBranchKey(item.index, item);
             if(br?.address === item?.address) {
               if (!i?.is_inscription) {
                 funds.push({
                   ...i,
                   address: item.address,
                   path: item.index,
                   key: br.key,
                 });
                }else{
                inscriptions.push({
                     ...i,
                     address: item.address,
                     path: item.index,
                     key: br.key,
                   });
                }
             }else{
               console.log("skip: prepareBalances->1|address:", br?.address,"|item.address:", item?.address);
             }
          }
       }
    }
    return {funds, inscriptions};
  }

  async sumAllFunds(all_funds) {
    if(!all_funds) return 0;
    console.log('sumAllFunds:',all_funds)
    return all_funds?.reduce((a,b)=>a+b?.amount, 0);
  }

  async sumMyInscribtions(inscriptions) {
    if(!inscriptions) return 0;
    console.log('sumMyInscribtions:',inscriptions)
    return inscriptions?.reduce((a,b)=>a+b?.amount, 0);
  }


  async selectKeyByFundAddress(address, fundings) {
    const myself = this;
    let list = myself.fundings;
    if(list) {
      list = fundings
    }
    if(list) {
      for(const item of list) {
        if(address === item?.address) {
          return item.key;
        }
      }
    }
  }

  selectKeyByOrdAddress(address, inscriptions = []) {
    const myself = this;
    let list = myself.inscriptions;
    if(inscriptions.length > 0) {
      list = inscriptions;
    }
    if(list) {
      for(const item of list) {
        if(address === item?.address) {
          return item.key;
        }
      }
    }
  }

  selectByFundAddress(address, fundings) {
    const myself = this;
    let list = myself.fundings;
    if(list) {
      list = fundings
    }
    if(list) {
      for(const item of list) {
        if(address === item?.address) {
          return item;
        }
      }
    }
  }

  selectByOrdAddress(address, inscriptions = []) {
    const myself = this;
    let list = myself.inscriptions;
    if(list) {
      list = inscriptions
    }
    for(const item of list) {
      if(address === item?.address) {
        return item;
      }
    }
  }

  async selectKeyByFundOutput(txid, nout, fundings = []) {
    const myself = this;
    let list = myself.fundings;
    if(fundings.length > 0) {
      list = fundings
    }
    if(list) {
      for(const item of list) {
        if(txid === item.txid && nout === item.nout) {
          return item.key;
        }
      }
    }
  }

  async selectKeyByOrdOutput(txid, nout, inscriptions = []) {
    const myself = this;
    let list = myself.inscriptions;
    if(inscriptions.length > 0) {
      list = inscriptions
    }
    for(const item of list) {
      if(txid === item.txid && nout === item.nout) {
        return item.key;
      }
    }
    return;
  }

  async selectByFundOutput(txid, nout, fundings = []) {
    const myself = this;
    let list = myself.fundings;
    if(fundings.length > 0) {
      list = fundings;
    }
    if(list) {
      for(const item of list) {
        if(txid === item.txid && nout === item.nout) {
          return item;
        }
      }
    }
  }

  selectByOrdOutput(txid, nout, inscriptions = []) {
    const myself = this;
    let list = myself.inscriptions;
    console.log('list:',list)
    if(inscriptions.length > 0) {
      list = inscriptions;
    }
    console.log('list2:',list)
    if(list) {
      for(const item of list) {
        if(txid === item.txid && Number(nout) === Number(item.nout)) {
          console.log(txid,'txid === item.txid',item.txid)
          console.log(nout,'nout === item.nout',item.nout)
          return item;
        }
      }
    }
  }

  getCurrentNetWorkLabel() {
    if (!this.network) { return ' '; }
    switch (this.network) {
      case this.utxord.TESTNET:
        return 'TestNet';
      case this.utxord.REGTEST:
        return 'RegTest';
      case this.utxord.MAINNET:
        return ' ';
      default:
      return ' ';
    }
  }

  getNetWork(network) {
    switch (network) {
      case this.utxord.TESTNET:
        return 't'; //testnet
      case this.utxord.MAINNET:
        return 'm'; //mainnet
      case this.utxord.REGTEST:
        return 'r';  //regtest
      default:
        return 't'; //testnet
    }
  }

  setUpNetWork(type) {
    //        this.utxord.MAINNET;
    //        this.utxord.TESTNET;
    //        this.utxord.REGTEST;
    switch (type) {
      case 't':
        return this.utxord.TESTNET;
      case 'm':
        return this.utxord.MAINNET;
      case 'r':
        return this.utxord.REGTEST;
      default:
        return this.utxord.TESTNET;
    }
  }

  btcToSat(btc) {
    return Math.round(Number(btc) * 100000000);
  }

  satToBtc(sat) {
    return Number(sat) / 100000000;
  }

  hexToString(hex) {
    return buffer.Buffer.from(hex, 'hex');
  }

  getErrorMessage(exception: number | Exception) {
    if ('number' !== typeof(exception)) {
      return exception;
    }
    return this.utxord.Exception.prototype.getMessage(exception).c_str()
  }

  async sendNotificationMessage(type?: string, message: any) {
    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(NOTIFICATION , `${message}`, `popup@${currentWindow.id}`)
    console.log(NOTIFICATION, type, message);
    return true;
  }

  async sendWarningMessage(type?: string, warning: any) {
    let errorMessage;
    let errorStack;
    if(typeof warning === 'number') {
      errorMessage = this.getErrorMessage(Number.parseInt(warning, 10));

    }else{
      errorMessage = warning;
      if(warning?.message) {
        errorMessage = warning?.message;
        type = warning?.name;
        errorStack = warning?.stack;
      }
    }
    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(WARNING, errorMessage, `popup@${currentWindow.id}`)
    console.log(type, errorMessage, errorStack);
    return type+errorMessage+errorStack;
  }

  async sendExceptionMessage(type?: string, exception: any) {
    let errorMessage;
    let errorStack;
    if(typeof exception === 'number') {
      errorMessage = this.getErrorMessage(Number.parseInt(exception, 10));

    }else{
      errorMessage = exception;
      if(exception?.message) {
        errorMessage = exception?.message;
        type = exception?.name;
        errorStack = exception?.stack;
      }
    }
    if(errorMessage.indexOf('Aborted(OOM)')!==-1) {
      console.log('wasm->load::Error{',errorMessage,'}');
      return errorMessage;
    }
    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(EXCEPTION, errorMessage, `popup@${currentWindow.id}`)
    console.log(type, errorMessage, errorStack);
    return type+errorMessage+errorStack;
  }

  async fetchAddress(address: string) {
    const response = await this.Rest.get(`/api/address/${address}/balance/`);
    return response;
  }

  async fetchUSDRate() {
    const response = await this.Rest.get('/api/wallet/prices/');
    return response;
  }

  async fetchExternalAddresses() {
    if(this.wallet.ext.keys.length<1) return;

    console.log('this.wallet.ext.keys:',this.wallet.ext.keys);
    for(const item of this.wallet.ext.keys) {
      if(item.address) {
        let response = await this.fetchAddress(item.address);
        if(response?.data) {
          item.balance =  response.data?.confirmed || response.data?.unconfirmed;
          console.log('ExternalAddress:',item.address,'|',item.balance);
        }
      }

    }
  }

  async fetchBalance(address: string) {
    // this.fetchExternalAddresses();
    // TODO: add to wallet balance and save address to server and use for creating and paying
    // await this.fetchAddress(address);

    const response = {
      data: {
        confirmed: 0,
        unconfirmed: 0
      }
    };

    const my = this.inscriptions;
    const all_funds = this.fundings;

    const total = await this.sumAllFunds(all_funds);
    const sum_my_inscr = await this.sumMyInscribtions(my);
    console.log('this.all_addresses:',this.all_addresses);
    return {
      data: {
        sync: this.sync,
        connect: this.connect,
        confirmed: total || 0,
        to_address: response?.data?.confirmed || 0,
        unconfirmed: response?.data?.unconfirmed || 0,
        used_for_inscribtions: sum_my_inscr || 0,
        inscriptions: my || []
      }
    };
  }

  async sendMessageToWebPage(type, args, tabId: number | undefined = undefined): Promise<void> {
    const myself = this;

    let tabs: Tab[];
    if (tabId != null) {
      tabs = [await chrome.tabs.get(tabId)]
    } else {
      tabs = await chrome.tabs.query({
        windowType: 'normal',
        url: BASE_URL_PATTERN,
      });
    }
    console.log('args:', args,'type:', type);
    // console.log(`----- sendMessageToWebPage: there are ${tabs.length} tabs found`);
    for (let tab of tabs) {
      // if (tab?.url?.startsWith('chrome://') || tab?.url?.startsWith('chrome://new-tab-page/')) {
      //   setTimeout(async () => await myself.sendMessageToWebPage(type, args), 100);
      //   return;
      // }
      if(tab?.id) {
        // console.dir(tab);
        await chrome.scripting.executeScript({
          target: { tabId: tab?.id },
          func: function (t, a) {
            window.postMessage({ type: t, payload: a })
          },
          args: [type, args],
        });
      }
    }
    if (bgSiteQueryIndex >= limitQuery) {
      bgSiteQueryIndex = 0;
      console.log('Limit...');
      setTimeout(async () => await myself.sendMessageToWebPage(type, args, tabId), 1000);
    }
  }

  //------------------------------------------------------------------------------

  signToChallenge(challenge, tabId: number | undefined = undefined): boolean {
    const myself = this;
    if (myself.wallet.auth.key) {
      const signature = myself.wallet.auth.key.SignSchnorr(challenge).c_str();
      console.log("SignSchnorr::challengeResult:", signature);
      myself.sendMessageToWebPage(CONNECT_RESULT, {
        challenge: challenge,
        signature: signature,
        publickey: myself.wallet.auth.key.PubKey()
      }, tabId);
    myself.connect = true;
    myself.sync = false;
    } else {
      myself.sendMessageToWebPage(CONNECT_RESULT, null, tabId);
    }
    return myself.connect;
  }

  //------------------------------------------------------------------------------

  async getRawTransactions(builderObject, phase = undefined) {
    const raw_size = builderObject.TransactionCount(phase);
    const raw = [];
    for(let i = 0; i < raw_size; i += 1) {
      if(phase!==undefined) {
        raw.push(builderObject.RawTransaction(phase, i).c_str())
      }else{
        raw.push(builderObject.RawTransaction(i).c_str())
      }
    }
    return raw;
  }

  //------------------------------------------------------------------------------

  async createInscriptionContract(payload, theIndex = 0) {
    const myself = this;
    const outData = {
      xord: null,
      nxord: null,
      xord_address: null,
      data: null,
      // sk: null,  // TODO: use/create ticket for excluded sk (UT-???)
      amount: 0,
      output_mining_fee: 0,
      inputs_sum: 0,
      utxo_list: [],
      expect_amount: Number(payload.expect_amount),
      extra_amount: 0,
      fee_rate: payload.fee_rate,
      fee: payload.fee,
      size: (payload.content.length+payload.content_type.length),
      raw: [],
      errorMessage: null as string | null
    };
    try {
      console.log('createInscription payload: ', {...payload || {}});

      let collection;
      let flagsFundingOptions = "";
      let collection_utxo_key;

      if(payload?.collection?.genesis_txid) {
        // if collection is present.. than finding an output with collection
        collection = myself.selectByOrdOutput(
           payload.collection.owner_txid,
           payload.collection.owner_nout
       );
        console.log("payload.collection:",payload.collection)
        console.log('AddToCollectionSim:', collection);  // to use simulation contract to calculate fees
        flagsFundingOptions += "collection";
      }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.network,
        myself.utxord.INSCRIPTION
      );
      console.log('newOrd:',newOrd);
      // TODO: we need to receive it from backend via frontend
      const contract = payload?.contract || {
        "contract_type": "CreateInscription",
        "params": {"protocol_version": 8, "market_fee": {"amount": 0}}
      };
      newOrd.Deserialize(JSON.stringify(contract));

      newOrd.OrdAmount((myself.satToBtc(payload.expect_amount)).toFixed(8));

      // For now it's just a support for title and description
      if(payload.metadata) {
        console.log('payload.metadata:',payload.metadata);
        const encoded = cbor.encode(payload.metadata);
        await newOrd.SetMetaData(myself.arrayBufferToHex(encoded));
      }

      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee_rate)).toFixed(8));  // payload.fee_rate as Sat/kB

      if(payload?.collection?.genesis_txid) {
        // collection is empty no output has been found, see code above
        if(!collection) {
          myself.sendExceptionMessage(
            'CREATE_INSCRIPTION',
            `Collection(txid:${payload.collection.owner_txid}, nout:${payload.collection.owner_nout}) is not found in balances`
          );
          setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there are no fundings available.
          // FIXME: l2xl response: it shouldn't work until fundings added with newOrd.AddUTXO
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.raw = [];
          return outData;
       }

        newOrd.AddToCollection(
          `${payload.collection.genesis_txid}i0`,  // inscription ID = <genesis_txid>i<envelope(inscription)_number>
          payload.collection.owner_txid,  // current collection utxo
          payload.collection.owner_nout,  // current collection nout
          (myself.satToBtc(collection.amount)).toFixed(8),  // amount from collection utxo
          payload.collection.btc_owner_address
        )
      }

      await newOrd.Data(payload.content_type, payload.content);
      await newOrd.InscribeAddress(myself.wallet.ord.key.GetP2TRAddress(this.network));
      await newOrd.ChangeAddress(myself.wallet.fund.key.GetP2TRAddress(this.network));

      const min_fund_amount = myself.btcToSat(Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str()));
      outData.amount = min_fund_amount;
      if(!myself.fundings.length ) {
          // TODO: REWORK FUNDS EXCEPTION
          // myself.sendExceptionMessage(
          //   'CREATE_INSCRIPTION',
          //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
          // );
          // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
          outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
              "wait for several conformations or wait update on the server.";
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there are no fundings available.
          // FIXME: l2xl response: it shouldn't work until fundings added with newOrd.AddUTXO
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.raw = [];
          return outData;
      }
      const utxo_list = await myself.selectKeysByFunds(min_fund_amount);
      outData.utxo_list = utxo_list;
      const inputs_sum = await myself.sumAllFunds(utxo_list);
      outData.inputs_sum = inputs_sum;

      if(utxo_list?.length < 1) {
          // TODO: REWORK FUNDS EXCEPTION
          // this.sendExceptionMessage(
          //   'CREATE_INSCRIPTION',
          //   "There are no funds to create of the Inscription, please replenish the amount: "+
          //   `${min_fund_amount} sat`
          // );
          // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
          outData.errorMessage = "There are no funds to create of the Inscription, please replenish the amount: "+
            `${min_fund_amount} sat`
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there are no fundings available.
          // FIXME: l2xl response: it shouldn't work until fundings added with newOrd.AddUTXO
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.raw = [];
          return outData;
      }

      if(inputs_sum > Number(payload.expect_amount)) {
        if(payload?.collection?.genesis_txid){flagsFundingOptions += ","}
        flagsFundingOptions += "change";
      }

      const extra_amount = myself.btcToSat(Number(newOrd.GetGenesisTxMiningFee().c_str()));
      outData.extra_amount = extra_amount;

      console.log("extra_amount:",extra_amount);
      console.log("min_fund_amount:",min_fund_amount);
      console.log("utxo_list:",utxo_list);

      for(const fund of utxo_list) {
        await newOrd.AddUTXO(
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.address
        );
      }

      try {
        await newOrd.SignCommit(
          myself.wallet.root.key,
          'fund',
          myself.wallet.uns.key.PubKey()
        );
      } catch (e) {
        console.log('SignCommit:',myself.getErrorMessage(e));
      }

      try {
        // get front root ord and select to addres or pubkey
        // collection_utxo_key (root! image key) (current utxo key)
        if(payload?.collection?.genesis_txid) {
          await newOrd.SignCollection(
            myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
            'ord'
          );
        }
      } catch (e) {
        console.log('SignCollection:',myself.getErrorMessage(e));
      }

      try {
        await newOrd.SignInscription(
          myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
          'uns'
        );
      } catch (e) {
        console.log('SignInscription:',myself.getErrorMessage(e));
      }

      const min_fund_amount_final_btc = Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str());

      console.log('min_fund_amount_final_btc:',min_fund_amount_final_btc);
      const min_fund_amount_final = await myself.btcToSat(min_fund_amount_final_btc);

      console.log('min_fund_amount_final:',min_fund_amount_final);
      outData.amount = min_fund_amount_final;

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      outData.utxo_list = utxo_list_final;

      const output_mining_fee = myself.btcToSat(Number(newOrd.GetNewOutputMiningFee().c_str()));
      outData.output_mining_fee = output_mining_fee;
      console.log('output_mining_fee:', output_mining_fee);

      // SupportedVersions()
      outData.data = newOrd.Serialize(8, myself.utxord.INSCRIPTION_SIGNATURE).c_str();
      outData.raw = await myself.getRawTransactions(newOrd);
      const sk = newOrd.getIntermediateTaprootSK().c_str();
      myself.wallet.root.key.AddKeyToCache(sk);
      // outData.sk = newOrd.getIntermediateTaprootSK().c_str();  // TODO: use/create ticket for excluded sk (UT-???)
      // TODOO: remove sk before > 2 conformations
      // or wait and check utxo this translation on balances

      return outData;
    } catch (exception) {
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if (! myself.KNOWN_CORE_ERRORS.some(errId => eout.indexOf(errId) !== -1)) return;

      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000) {
        this.sendExceptionMessage(CREATE_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return outData;
      }
      return await myself.createInscriptionContract(payload, theIndex);
    }
  }

  //------------------------------------------------------------------------------
  async createInscription(payload_data) {
    const myself = this;
    if(!payload_data?.costs?.data) return;
    try{
      console.log("outData:", payload_data.costs.data);
      if(payload_data.costs) {
        if(payload_data.costs?.xord) {
          myself.addToXordPubKey(payload_data.costs?.xord);
        }
        if(payload_data.costs?.nxord) {
          myself.addToXordPubKey(payload_data.costs?.nxord);
        }
        // TODO: Check it against server/backend. Are we still need it?
        // if(payload_data.costs?.sk){
        //   myself.addToExternalKey(payload_data.costs?.sk);
        //   console.log(" sk:", payload_data.costs?.sk);
        // }

        await myself.genKeys();
        await myself.sendMessageToWebPage(ADDRESSES_TO_SAVE, myself.addresses, payload_data._tabId);
      }

      setTimeout(async () => {
        console.log(CREATE_INSCRIBE_RESULT,": ",{
          contract: JSON.parse(payload_data.costs.data),
          name: payload_data.name,
          description: payload_data?.description,
          type: payload_data?.type
        });

        // ======================================================================
        // TODO: to debug this part when backend will ready for addresses support
        // ----------------------------------------------------------------------
        myself.WinHelpers.closeCurrentWindow();
        await myself.sendMessageToWebPage(CREATE_INSCRIBE_RESULT, {
          contract: JSON.parse(payload_data.costs.data),
          name: payload_data.name,
          description: payload_data?.description,
          type: payload_data?.type
        }, payload_data._tabId);

        await myself.generateNewIndex('ord');
        await myself.generateNewIndex('uns');
        if(payload_data?.type!=='INSCRIPTION' && !payload_data?.collection?.genesis_txid && payload_data.costs?.xord) {
          await myself.generateNewIndex('intsk');
          await myself.generateNewIndex('scrsk');
        }
        myself.genKeys();
        // ======================================================================
      },1000);


    } catch (exception) {
      this.sendExceptionMessage(CREATE_INSCRIPTION, exception)
    }
  }

  //------------------------------------------------------------------------------

  async sellSignContract(utxoData, ord_price, market_fee, contract, txid, nout) {
    const myself = this;
    try {
      const sellOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      sellOrd.Deserialize(JSON.stringify(contract));
      sellOrd.CheckContractTerms(myself.utxord.ORD_TERMS);

      sellOrd.OrdUTXO(
        txid,
        nout,
        (myself.satToBtc(utxoData.amount)).toFixed(8),
        myself.wallet.ord.key.GetP2TRAddress(this.network)
      );

      sellOrd.FundsPayoffAddress(myself.wallet.fund.key.GetP2TRAddress(this.network));
      sellOrd.SignOrdSwap(
          myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
          'ord'
      );
      const raw = await myself.getRawTransactions(sellOrd, myself.utxord.ORD_SWAP_SIG);
      return {
        raw: raw,
        contract_data: sellOrd.Serialize(5, myself.utxord.ORD_SWAP_SIG).c_str()
      };
    } catch (exception) {
      this.sendExceptionMessage('SELL_SIGN_CONTRACT', exception)
    }
  }

  async sellInscriptionContract(payload) {
    const myself = this;
    try {
      const txid = payload.utxo_id.split(':')[0];
      const nout = Number(payload.utxo_id.split(':')[1]);
      console.log('sellInscriptionContract->payload:',payload);
      console.log('txid:',txid, 'nout:', nout)

      const utxoData = myself.selectByOrdOutput(txid, nout);
      console.log('utxoData:',utxoData);
      console.log("SwapInscriptionBuilder->args:", payload.ord_price,
      (myself.satToBtc(payload.swap_ord_terms.market_fee)).toFixed(8));

      const contract_list = [];
      const raws = [];
      for (const act of payload.swap_ord_terms.contracts) {
        let data = await myself.sellSignContract(
          utxoData,
          payload.ord_price,
          act.market_fee,
          act.contract,
          txid, nout
        );
        raws.push(data.raw);
        contract_list.push(JSON.parse(data.contract_data));
      }

      return {
        contract_uuid: payload.swap_ord_terms.contract_uuid,
        raw: raws,
        contracts: contract_list,
      };

    } catch (exception) {
      this.sendExceptionMessage('SELL_INSCRIPTION_CONTRACT', exception);
    }
  }

  async sellInscription(payload_data) {
    const myself = this;
    if(!payload_data?.costs) return;
    console.log("outData:", payload_data?.costs);
    const data = payload_data?.costs;
    try {
      await (async (data) => {
        console.log("SELL_DATA_RESULT:", data);
        myself.WinHelpers.closeCurrentWindow()
        myself.sendMessageToWebPage(
          SELL_INSCRIBE_RESULT,
          data,
          payload_data._tabId);
      })(data);

    } catch (exception) {
      this.sendExceptionMessage(SELL_INSCRIPTION, exception)
    }
  }

  //----------------------------------------------------------------------------

  exOutputs(items = []) {
    const out = []
    for(const item of items) {
      out.push(`${item.txid}:${item.nout}`);
    }
    return out;
  }

  //----------------------------------------------------------------------------

  async selectKeysByFunds(target: number, fundings = [], except_items = []) {
    const addr_list = [];
    let iter = 0;

    let all_funds_list = this.fundings;
    if(fundings.length > 0) {
      all_funds_list = fundings;
    }
    let all_funds = [];
    const excepts = this.exOutputs(except_items);
    console.log('excepts:',excepts);
    console.log('all_funds_list:',all_funds_list);
    for(const al of all_funds_list) {
      let aloutput=`${al.txid}:${al.nout}`;
      if(!excepts?.includes(aloutput)) {
        all_funds.push({...al, output: aloutput});
      }
    }
    console.log('all_funds:',all_funds);
    const sum_funds = await this.sumAllFunds(all_funds);

    if(sum_funds < target) {
      console.log(sum_funds,"sum<target",target);
      return [];
    }
    if(all_funds?.length === 0) {
      console.log(all_funds,"no funds");
      return [];

    }
    console.log("selectKeysByFunds->getAllFunds->all_funds:",all_funds);

    all_funds = all_funds.sort((a, b) => {
      if (a.amount > b.amount) { return 1; }
      if (a.amount < b.amount) { return -1; }
      return 0;
    });

    for(const al of all_funds) {
      if(al.amount >= target) {
        addr_list.push({...al});
        return addr_list;
      }
    }

    let sum_addr = 0;
    console.log('selectKeysByFunds2->all_funds:',all_funds);
    for(const utxo of all_funds) {
          sum_addr += utxo.amount;
          iter += 1;
          let br = await this.getBranchKey(utxo.path, utxo);
          if(br?.address !== utxo?.address) {
            console.log("selectKeysByFunds->2|address:",br?.address,"|utxo.address:",utxo?.address);
          }else{
            console.log('iter2->key:',br?.key,'|',utxo?.address);
            addr_list.push({
              amount: utxo?.amount,
              nout: utxo?.nout,
              txid: utxo?.txid,
              path: utxo?.path,
              address: utxo?.address,
              key: br?.key
            });
          }
          if(sum_addr>=target){ return addr_list;}
        }
}

  async  commitBuyInscriptionContract(payload, theIndex=0) {
    const myself = this;
    const outData = {
      data: null,
      min_fund_amount: 0,
      market_fee: payload.market_fee,
      ord_price: payload.ord_price,
      mining_fee: 0,
      utxo_list: [],
      raw: [],
      errorMessage: null as string | null
    };
    //const balances = structuredClone(myself.balances);
    //const fundings = structuredClone(myself.fundings);
    //const inscriptions = structuredClone(myself.inscriptions);
    //console.log("commitBuyInscription->payload",payload)
    try {
      const swapSim = new myself.utxord.SwapInscriptionBuilder(myself.network);
      swapSim.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
      swapSim.CheckContractTerms(myself.utxord.FUNDS_TERMS);

      const min_fund_amount = await myself.btcToSat(Number(swapSim.GetMinFundingAmount("").c_str()))


      if(!myself.fundings.length ) {
        // TODO: REWORK FUNDS EXCEPTION
        // myself.sendExceptionMessage(
        //   COMMIT_BUY_INSCRIPTION,
        //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
            "wait for several conformations or wait update on the server";
        outData.min_fund_amount = min_fund_amount;
        outData.mining_fee = Number(min_fund_amount) - Number(payload.market_fee) - Number(payload.ord_price);
        // outData.raw = await myself.getRawTransactions(swapSim, myself.utxord.FUNDS_TERMS);
        outData.raw = [];
        return outData;
      }

      const utxo_list = await myself.selectKeysByFunds(min_fund_amount + 682);
      console.log("min_fund_amount:",min_fund_amount);
      console.log("utxo_list:",utxo_list);

      const buyOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
      buyOrd.CheckContractTerms(myself.utxord.FUNDS_TERMS);
      // outData.raw = await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_TERMS);
      outData.raw = [];

      for(const fund of utxo_list) {
        buyOrd.AddFundsUTXO(
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.address
        );
      }

      buyOrd.ChangeAddress(myself.wallet.fund.key.GetP2TRAddress(myself.network));
      buyOrd.SwapScriptPubKeyB(myself.wallet.scrsk.key.PubKey()); //!!!

      buyOrd.SignFundsCommitment(
        myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
        'fund'
      );

      buyOrd.OrdPayoffAddress(myself.wallet.ord.key.GetP2TRAddress(myself.network));

      const min_fund_amount_final = await myself.btcToSat(Number(buyOrd.GetMinFundingAmount("").c_str()))
      outData.min_fund_amount = min_fund_amount_final;
      outData.mining_fee = Number(min_fund_amount_final) - Number(payload.market_fee) - Number(payload.ord_price);
      outData.utxo_list = utxo_list;
      outData.raw = await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_TERMS);
      if (utxo_list?.length < 1) {
        // setTimeout(() => {
        //   // TODO: REWORK FUNDS EXCEPTION
        //   myself.sendExceptionMessage(
        //     BUY_INSCRIPTION,
        //     "There are no funds to buying of the Inscription, please replenish the amount: " +
        //     `${min_fund_amount_final} sat`
        //   );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        // }, 500);
        outData.errorMessage = "There are no funds to buying of the Inscription, please replenish the amount: " +
            `${min_fund_amount_final} sat`;
        return outData;
      }
      outData.raw = await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_COMMIT_SIG);
      outData.data = buyOrd.Serialize(5, myself.utxord.FUNDS_COMMIT_SIG).c_str();
      return outData;
    } catch (exception) {
      const eout = await myself.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, exception)
      if (! myself.KNOWN_CORE_ERRORS.some(errId => eout.indexOf(errId) !== -1)) return;

      console.info(COMMIT_BUY_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000) {
        this.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return outData;
      }
      return await myself.commitBuyInscriptionContract(payload, theIndex);
    }
  }

  async commitBuyInscription(payload_data) {
      const myself = this;
      if(!payload_data.costs.data) return;
      if(!payload_data?.swap_ord_terms) return;
      try {
        console.log("data:", payload_data?.costs?.data," payload:", payload_data);
        myself.WinHelpers.closeCurrentWindow()
        myself.sendMessageToWebPage(
          COMMIT_BUY_INSCRIBE_RESULT, {
          contract_uuid: payload_data?.swap_ord_terms?.contract_uuid,
          contract: JSON.parse(payload_data.costs.data)
        }, payload_data?._tabId);
    } catch (exception) {
      this.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, exception)
    }
  }

  //------------------------------------------------------------------------------

  async signSwapInscription(payload, tabId: number | undefined = undefined) {
    const myself = this;
    try {
      console.log('//////////// signSwapInscription ARGS', payload);
      console.log("!!!!contract:",JSON.stringify(payload.swap_ord_terms.contract));

      const buyOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      // console.log("aaaa");
      buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
      buyOrd.CheckContractTerms(myself.utxord.MARKET_PAYOFF_SIG);
      buyOrd.SignFundsSwap(
        myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
        'scrsk'
      );
      const raw = [];  // await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_SWAP_SIG);
      const data = buyOrd.Serialize(5, myself.utxord.FUNDS_SWAP_SIG).c_str();

      (async (data, payload) => {
        console.log("SIGN_BUY_INSCRIBE_RESULT:", data);
        await myself.sendMessageToWebPage(
          SIGN_BUY_INSCRIBE_RESULT,
          { contract_uuid: payload.swap_ord_terms.contract_uuid, contract: JSON.parse(data) },
          tabId
        );
        await myself.generateNewIndex('scrsk');
        await myself.generateNewIndex('ord');
        await myself.genKeys();
      })(data, payload);

    } catch (exception) {
      this.sendExceptionMessage(BUY_INSCRIPTION, exception)
    }
  }

  encrypt (msg, password) {
    const keySize = 256;
    const ivSize = 128;
    const iterations = 100;

    const salt = self.CryptoJS.lib.WordArray.random(128/8);

    const key = self.CryptoJS.PBKDF2(password, salt, {
        keySize: keySize/32,
        iterations: iterations
      });

    const iv = self.CryptoJS.lib.WordArray.random(ivSize/8);

    const encrypted = self.CryptoJS.AES.encrypt(msg, key, {
      iv: iv,
      padding: self.CryptoJS.pad.Pkcs7,
      mode: self.CryptoJS.mode.CBC,
      hasher: self.CryptoJS.algo.SHA256
    });

    return salt.toString()+ iv.toString() + encrypted.toString();
  }

  decrypt (transitmessage, password) {
    const keySize = 256;
    const ivSize = 128;
    const iterations = 100;

    const salt = self.CryptoJS.enc.Hex.parse(transitmessage.substr(0, 32));
    const iv = self.CryptoJS.enc.Hex.parse(transitmessage.substr(32, 32))
    const encrypted = transitmessage.substring(64);

    const key = self.CryptoJS.PBKDF2(password, salt, {
        keySize: keySize/32,
        iterations: iterations
      });

    const decrypted = self.CryptoJS.AES.decrypt(encrypted, key, {
      iv: iv,
      padding: self.CryptoJS.pad.Pkcs7,
      mode: self.CryptoJS.mode.CBC,
      hasher: self.CryptoJS.algo.SHA256
    })
    return decrypted.toString(self.CryptoJS.enc.Utf8);
  }

  async encryptedWallet(password) {
    // loked wallet
    const isEnc = await this.isEncryptedWallet()
    if(isEnc) return true;
    this.wallet.secret = this.encrypt('secret', password);
/*
    const seed = this.encrypt(this.wallet.root.seed.toString('hex'));
    chrome.storage.sync.set({ seed: seed });
    this.wallet.root.seed = seed;
*/
    this.wallet.encrypted = true;
    chrome.storage.sync.set({ encryptedWallet: true });
    chrome.storage.sync.set({ secret: this.wallet.secret });
    return true;
  }

  async setUpPassword(password) {
    this.wallet.secret = await this.encrypt('secret', password);
    this.wallet.encrypted = true;
    await chrome.storage.sync.set({ encryptedWallet: true });
    await chrome.storage.sync.set({ secret: this.wallet.secret });
    return true;
  }

  async initPassword() {
    const {secret} = await chrome.storage.sync.get(['secret'])
    if(secret) {
      this.wallet.secret = secret;
      this.wallet.encrypted = true;
    }
  }

  async checkPassword(password) {
    try{
      const dsecret = this.decrypt(this.wallet.secret, password);
      console.log('dsecret',dsecret,'secret:',this.wallet.secret)
      if(dsecret !== 'secret') {
        return false;
      }
    }catch(e) {
      console.log('Api.checkPassword:',e);
    }
    return true;
  }

  async decryptedWallet(password) {
    // unlocked wallet
    // const isEnc = await this.isEncryptedWallet()
    // if(!isEnc) return true;
    const ps = await this.checkPassword(password);
    //console.log('ps',ps,'password:',password)
    if(!ps) {
      this.sendExceptionMessage(
        DECRYPTED_WALLET,
        "The wallet is encrypted please enter the correct password to unlock the keys"
      );
      return false;
    }
    /*
    const seed = this.decrypt(this.wallet.root.seed.toString('hex'));
    chrome.storage.sync.set({ seed: seed });
    this.wallet.root.seed = this.hexToString(seed);
    */
    this.wallet.encrypted = false;
    chrome.storage.sync.set({ encryptedWallet: false });
    return true
  }

  async isEncryptedWallet() {
    const {encryptedWallet} = await chrome.storage.sync.get(['encryptedWallet']);
    console.log("encryptedWallet: ",encryptedWallet);
    if(encryptedWallet===undefined) {
      chrome.storage.sync.set({ encryptedWallet: this.wallet.encrypted });
    }
    return this.wallet.encrypted;
  }

  destroy(variable) {
    if(variable) {
      this.utxord.destroy(variable);
      return true;
    }
    return false;
  }

  arrayBufferToHex(arrayBuffer) {
     const byteToHex = [];

     for (let n = 0; n <= 0xff; ++n)
     {
       const hexOctet = n.toString(16).padStart(2, "0");
       byteToHex.push(hexOctet);
     }
     const buff = new Uint8Array(arrayBuffer);
     const hexOctets = [];
     // new Array(buff.length) is even faster (preallocates necessary array size),
     // then use hexOctets[i] instead of .push()

   for (let i = 0; i < buff.length; ++i)
       hexOctets.push(byteToHex[buff[i]]);

   return hexOctets.join("");
  }

  typedArrayToBuffer(array: Uint8Array): ArrayBuffer {
    return array.buffer.slice(array.byteOffset, array.byteLength + array.byteOffset)
  }

  hexToArrayBuffer(hex) {
    const uit8arr = new Uint8Array(hex.match(/[\da-f]{2}/gi).map(function (h) {
      return parseInt(h, 16)
    }));
    return this.typedArrayToBuffer(uit8arr);
  }

  locked() {
    //TODO: locked all extension and wallet
  }

  unlocked() {
    //TODO: unlocked all extension and wallet
  }

  isLockedPlugin() {
    return this.locked;
  }

}
self.Api = Api;
self.utxord = utxord;

export default Api;
