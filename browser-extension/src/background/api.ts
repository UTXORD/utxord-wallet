import '~/libs/utxord.js';
import '~/libs/safe-buffer.js';
import * as CryptoJS from 'crypto-js';
import winHelpers from '~/helpers/winHelpers';
import rest from '~/background/rest';
import { sendMessage } from 'webext-bridge';
import * as cbor from 'cbor-js';
import * as messages from '~/config/messages'
// import * as Sentry from "@sentry/browser";
// import { wasmIntegration } from "@sentry/wasm";
import {version} from '~/../package.json';
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
  DECRYPTED_WALLET,
  TRANSFER_LAZY_COLLECTION,
  BUY_PRODUCT,
  SEND_TO,
  SEND_TO_ADDRESS,
  SEND_TO_ADDRESS_RESULT,
} from '~/config/events';

import {
  BASE_URL_PATTERN,
  PROD_URL_PATTERN,
  STAGE_URL_PATTERN,
  ETWOE_URL_PATTERN,
  LOCAL_URL_PATTERN,
  NETWORK
 } from '~/config/index';

import Tab = chrome.tabs.Tab;
import {Exception} from "sass";

import * as bip39 from "~/config/bip39";
import {logger} from "../../scripts/utils";

function GetEnvironment(){
  switch (BASE_URL_PATTERN) {
    case PROD_URL_PATTERN: return 'production';
    case STAGE_URL_PATTERN: return 'staging';
    case ETWOE_URL_PATTERN: return 'staging';
    case LOCAL_URL_PATTERN: return 'debuging';
    default: return 'debuging';
  }
}

// Sentry.init({
//   environment: GetEnvironment(),
//   dsn: "https://da707e11afdae2f98c9bcd1b39ab9c16@sntry.utxord.com/6",
//   // Performance Monitoring
//   tracesSampleRate: 1.0, //  Capture 100% of the transactions
//   integrations: [wasmIntegration()],
//   ignoreErrors: [
//     'Frame with ID 0',
//     'Cannot access contents',
//     'No tab with id:',
//     'Cannot access a chrome://',
//     'Failed to fetch'
//   ]
// });

// import tabId = chrome.devtools.inspectedWindow.tabId;

const limitQuery = 1000;
let bgSiteQueryIndex = 0;
const closeWindowAfter = 6000;

const SIMPLE_TX_PROTOCOL_VERSION = 2;

class UtxordExtensionApiError extends Error {
  constructor(readonly message?: string) {
    super(message);
  }
}


const WALLET_TYPES = [
  'oth',
  'fund',
  'ord',
  'uns',
  'intsk',
  'intsk2',
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
    address: null,
    typeAddress: 0,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["0'", "1'", "2'"],
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
    address: null,
    typeAddress: 0,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["0'", "1'", "2'"],
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
    address: null,
    typeAddress: 0,
    filter: {
      look_cache: true,
      key_type: "DEFAULT",
      accounts: ["0'","2'"],
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
    address: null,
    typeAddress: 0,
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["3'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  intsk: { // internalSK
    index: 0,
    change: 0,
    account: 4,
    coin_type: 1,
    key: null,
    p2tr: null,
    address: null,
    typeAddress: 0,
    // seems we don't need intsk filter
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["6'", "5'", "4'"],
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
    address: null,
    typeAddress: 0,
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["6'", "5'", "4'"],
      change: ["0"],
      index_range: "0-16384"
    }

  },
  intsk2: { // internalSK #2
    index: 0,
    change: 0,
    account: 6,
    coin_type: 1,
    key: null,
    p2tr: null,
    address: null,
    typeAddress: 0,
    // seems we don't need intsk filter
    filter: {
      look_cache: true,
      key_type: "TAPSCRIPT",
      accounts: ["6'", "5'", "4'"],
      change: ["0"],
      index_range: "0-16384"
    }
  },
  ext: {
    seeds: [{
      seed: null,
      sub: [{
        path: null,
        key: null,
        p2tr: null,
        address: null,
        typeAddress: 0,
        pubKeyStr: null,
        privKeyStr: null,
      }],
      rootKey: null,
      rootP2tr: null,
      address: null,
      typeAddress: 0,
      rootPubKeyStr: null,
      rootPrivKeyStr: null,
      type: null,
    }],
    paths: [{
      path: null,
      key: null,
      p2tr: null,
      address: null,
      typeAddress: 0,
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
    address: null,
    typeAddress: 0,
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
  exception_count: number = 0
  warning_count: number = 0

  constructor(network) {
    return (async () => {
      try {
        const myself = this;
        this.WinHelpers = new winHelpers();
        this.Rest = new rest();
        this.utxord = await utxord();
        if(!this.utxord.SIGNET){
           this.utxord.SIGNET = 1;
        }
        this.network = this.setUpNetWork(network);
        this.bech = new this.utxord.Bech32(this.network);

        this.locked = false;
        this.status = STATUS_DEFAULT;
        this.wallet = WALLET;
        this.wallet_types = WALLET_TYPES;
        this.addresses = [];
        this.all_addresses = [];
        this.keyCache = [];
        this.skipAddresses = [];
        this.max_index_range = 1;

        this.balances = [];
        this.fundings = [];
        this.inscriptions = [];
        this.expects = {};
        this.connect = false;
        this.sync = false;
        this.derivate = false;
        this.error_reporting = true;
        this.timeSync = false;
        this.viewMode = false;
        // view mode true = sidePanel enabled
        // view mode false = popup enabled
        this.mnemonicParserInstances = {};

        await this.init(this);
        // await this.sentry();
        return this;
      } catch(e) {
        // this.sentry(e);
        console.log('constructor->error:',e);
        this.removePublicKeyToWebPage();
        chrome.runtime.reload();
      }
    })();
  }

  async init() {
    const myself = this
    try {
      const { seed } = await chrome.storage.local.get(['seed']);
      const { derivate } = await chrome.storage.local.get(['derivate']);
      const { viewMode } = await chrome.storage.local.get(['viewMode']);
      if(viewMode){
        this.viewMode = viewMode;
      }
      if (seed) {
        myself.wallet.root.seed = seed;
      }
      if(derivate){
        myself.derivate = derivate;
      }
      const { ext_keys } = await chrome.storage.local.get(['ext_keys']);
      if(ext_keys) {
        if(ext_keys.length > 0) {
           myself.resExtKeys(ext_keys);
        }
      }

      await myself.rememberIndexes();


      console.log('NETWORK:',myself.utxord.SIGNET)
      await myself.upgradeProps(myself.utxord, 'utxord'); // add wrapper
      console.log('init...');
      myself.genRootKey();

      if (myself.checkSeed() && myself.utxord && myself.bech && this.wallet.root.key) {
        await myself.addPopAddresses();
        await myself.genKeys();
        await myself.initPassword();
        const fund = myself.wallet.fund.key?.PubKey();
        const auth = myself.wallet.auth.key?.PubKey();
        if (fund && auth) {
          myself.status.initAccountData = true;
          return myself.status.initAccountData;
        }
      }
    } catch (e) {
       myself.sendExceptionMessage('PLUGIN_INIT', e);
    }
  }

  // sentry(e = undefined) {
  //   const myself = this;
  //   if(this.error_reporting){
  //     Sentry.configureScope( async (scope) => {
  //       scope.setTag('environment', GetEnvironment())
  //       scope.setTag('system_state', myself.error_reporting)
  //       scope.setTag('network', myself.getCurrentNetWorkText())
  //       scope.setTag('base_url', BASE_URL_PATTERN)
  //       scope.setTag('plugin', version)
  //
  //       if(myself.wallet.auth?.key) {
  //         scope.setUser({
  //           othKey: myself.wallet.oth?.key?.PubKey(),
  //           fundKey: myself.wallet.fund?.key?.PubKey(),
  //           authKey: myself.wallet.auth?.key?.PubKey()
  //         })
  //        }
  //       const check = await myself.checkSeed();
  //       const system_state = {
  //         check_auth: check,
  //         addresses: JSON.stringify(myself.addresses),
  //         filtred_addresses: JSON.stringify(myself.getAddressForSave()),
  //         balances: JSON.stringify(myself.balances),
  //         fundings: JSON.stringify(myself.fundings),
  //         inscriptions: JSON.stringify(myself.inscriptions),
  //         connect: myself.connect,
  //         sync: myself.sync,
  //         derivate: myself.derivate,
  //       }
  //       scope.setExtra('system',  system_state);
  //     });
  //     if(e) Sentry.captureException(e);
  //   }
  // }

  async upgradeProps(obj, name = '', props = {}, list = [], args = 0, proto = false, lvl = 0){
    const out = {name, props, list, args, proto, lvl};
    const methodsToDontWarn = ['LookupAddress'];
    const propsToDontWrap = ['DecodeEntropy']
    const myself = this;
    if(!obj) return out;
    let methods = Object.getOwnPropertyNames(obj).filter((n) => n[0]!== '_');
    if(methods.length === 3 && methods.indexOf('length') !== -1 && methods.indexOf('prototype') !== -1){
      out.args = obj?.length || 0;
      out.lvl += 1;
      out.proto = true;
      return await this.upgradeProps(
        obj.prototype,
        out.name,
        out.props,
        out.list,
        out.args,
        out.proto,
        out.lvl
      );
    }
    for (let m of methods) {
      if (propsToDontWrap.includes(m)) continue;
      if((typeof obj[m]) === 'function' &&
        m.indexOf('dynCall') === -1 &&
        m.indexOf('constructor') === -1 &&
        out.lvl < 8){
        out.list.push(m);
        let prps = await this.upgradeProps(obj[m],m,{},[],0,false, out.lvl+1);
        out.props[m] = prps;
        obj[`$_${m}`] = obj[m];
        obj[`$_${m}`].prototype = obj[m].prototype;
        obj[`_${m}`] = function(){
            try{
              let o;
              if(!out.proto && out.lvl === 0){
                o = new (obj[`$_${m}`])(...arguments);
              }else{
                o = this[`$_${m}`](...arguments);
              }
              return o;
            }catch(e){
              if (!methodsToDontWarn.includes(m)) myself.sendWarningMessage(m, e);
              return null;
            }
        };
        delete(obj[m]);
        obj[m] = obj[`_${m}`];
        obj[m].prototype = obj[`$_${m}`].prototype;
        delete(obj[`_${m}`]);

      }
    }
    return out;
  }

  zeroPad(n,length){
     return n.toString().padStart(length, '0');
  }

  challenge(){
    const d = new Date();
    const bytes = CryptoJS.lib.WordArray.random(20)
    let salt = 0
    for (let i = 0; i < bytes.words.length; i++) {
      salt *= 256;
      if (bytes.words[i] < 0) {
          salt += 256 + Math.abs(bytes.words[i]);
      } else {
          salt += bytes.words[i];
        }
    }
    salt = salt.toString().substr(0, 16)
    const year = d.getUTCFullYear()
    const month = this.zeroPad((d.getUTCMonth()+1), 2)
    const day = this.zeroPad(d.getUTCDate(), 2)
    const hour = this.zeroPad(d.getUTCHours(), 2)
    const minute = this.zeroPad(d.getUTCMinutes(), 2)
    const second = this.zeroPad(d.getUTCSeconds(), 2)
    const timeformat = `${year}-${month}-${day}-${hour}:${minute}:${second}`
    return `Verify address salt: ${salt} Requested at: ${timeformat}`
}

sha256x2(word) {
  const hash = CryptoJS.SHA256(CryptoJS.enc.Utf8.parse(word)).toString()
  const bytes = CryptoJS.enc.Hex.parse(hash)
  const dhash = CryptoJS.SHA256(bytes).toString(CryptoJS.enc.Hex)
  return dhash
}

getKey(type: string, typeAddress: number | undefined = undefined, index: number | undefined = undefined){
    if (!this.wallet_types.includes(type)) return false;
    if (!this.checkSeed()) return false;
    this.genRootKey();
    const for_script = (type === 'uns' || type === 'intsk' || type === 'intsk2' || type === 'scrsk' || type === 'auth');
    if(typeAddress===undefined){ typeAddress = this.wallet[type].typeAddress;}
    return this.wallet.root.key.Derive(this.path(type, typeAddress, index), for_script);
  }

  getAddress(type: string, key: object | undefined = undefined, typeAddress: number | undefined = undefined){
    try{
      if(key===undefined){ key = this.wallet[type].key;}
      if(typeAddress===undefined){ typeAddress = this.wallet[type].typeAddress;}
      return (typeAddress === 1)? key?.GetP2WPKHAddress(this.network) : key?.GetP2TRAddress(this.network);
    }catch(e){
      console.log(e.type, e.message);
      return null;
    }

  }

getChallengeFromType(type: string, typeAddress: number | undefined = undefined ){
  const key = this.getKey(type, typeAddress);
  const challenge = this.challenge();
  const dhash = this.sha256x2(challenge);
  return {
    challenge: challenge,
    public_key: key.PubKey(),
    signature: key.SignSchnorr(dhash)
  };
}

getChallengeFromAddress(address: striong, type = undefined, path = undefined){
  const key = this.getKeyFromKeyRegistry(address, type, path);
  if(!key?.ptr){
     //console.log('getChallengeFromAddress->no ptr', key,'|address:',address,'|max_index_range:',this.max_index_range);
     return null;
  }
  const challenge = this.challenge();
  const dhash = this.sha256x2(challenge);
  return {
    challenge: challenge,
    public_key: key.PubKey(),
    signature: key.SignSchnorr(dhash)
  };
}

 async prepareAddressToPlugin(addresses){
   const list = []
   if(addresses.length > 0){
     for(const item of addresses){
       if(this.hasAddressKeyRegistry(item?.address, item?.type, item?.index)){
          list.push(item);
        }
     }
   }
  return list;
 }

 getAddressForSave(addresses: object[] | undefined = undefined, all_addresses: object[] | undefined = undefined){
    const list = [];
    if(addresses === undefined) {addresses = this.addresses; }
    if(all_addresses === undefined) {all_addresses = this.all_addresses; }
    for(let item of addresses){
      //console.log('item?.address:',item?.address, this.hasAddress(item?.address, this.all_addresses), this.all_addresses)
      if(!this.hasAddress(item?.address, all_addresses)){
        if((!this.hasAddress(item?.address, list) && this.derivate) || !this.derivate){
          let ch = this.getChallengeFromAddress(item?.address, item?.type, item?.index);
          if(ch){
            // console.log('item:',item,'|ch:',ch); //!!!
            item = {...item,...ch};
            list.push(item);
          }
        }
      }
    }
    return list;
  }

  async setIndexToStorage(type, value) {
    if(type==='ext') return;
    const Obj = {};
    Obj[`${type}Index`] = Number(value);
    return setTimeout(() => {
      chrome.storage.local.set(Obj);
    }, 2000);

  }

  async getIndexFromStorage(type) {
    if(type==='ext') return;
    return (await chrome.storage.local.get([`${type}Index`]))[`${type}Index`] || 0
  }

  async rememberIndexes() {
    for (const wt of this.wallet_types) {
      this.wallet[wt].index = await this.getIndexFromStorage(wt);
    }
    return true;
  }

  path(type: string, typeAddress: number | undefined = undefined, index: number | undefined = undefined) {
    if (!this.wallet_types.includes(type)) return false;
    if (!this.checkSeed()) return false;
    if (type === 'ext') return false;
    //m / purpose' / coin_type' / account' / change / index
    if(typeAddress===undefined){ typeAddress = this.wallet[type].typeAddress;}
    const ignore = ['uns', 'intsk', 'intsk2', 'scrsk', 'auth'];
    const t = (this.network === this.utxord.MAINNET && type !== 'auth') ? 0 : this.wallet[type].coin_type;
    const a = (this.derivate || ignore.includes(type)) ? this.wallet[type].account : 0 ;
    const c = this.wallet[type].change;
    let i = 0;
    if(index === undefined){
      i = (this.derivate || ignore.includes(type)) ? this.wallet[type].index : 0;
      this.setIndexRange(i);
    }else{
      i = index;
    }
    const purpose = (typeAddress === 1) ? 84 : 86;
    return `m/${purpose}'/${t}'/${a}'/${c}/${i}`;
  }

  async setDerivate(value) {
    if (!this.checkSeed()) return false;
    this.derivate = Boolean(value);
    setTimeout(() => {
      chrome.storage.local.set({derivate: this.derivate});
    }, 2000);
    return true;
  }

  async setViewMode(value){
    this.viewMode = Boolean(value);
    setTimeout(() => {
      chrome.storage.local.set({viewMode: this.viewMode});
    }, 2000);
    return true;
  }

  async setErrorReporting(value) {
    if (!this.checkSeed()) return false;
    this.error_reporting = Boolean(value);
    return true;
  }

  async setTypeAddress(type, value) {
    if (!this.wallet_types.includes(type)) return false;
    if (!this.checkSeed()) return false;
    if (type === 'ext') return false;
    this.wallet[type].typeAddress = Number(value);
    return true;
  }

  async generateNewIndexes(types: string | string[]) {
    if (types && typeof(types) === 'string') {
      types = types.split(/\s+|\s*,\s*/);
    }
    for (const type of types) {
      await this.generateNewIndex(type);
    }
    return 0 < types.length;
  }

  async generateNewIndex(type) {
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='ext') return false;
    this.wallet[type].index += 1;
    await this.setIndexToStorage(type, this.wallet[type].index);
    await this.setIndexRange(this.wallet[type].index);
    return true;
  }

  getIndex(type) {
    if(type==='ext') return 0;
    if (this.wallet[type]) return this.wallet[type].index;
    return 0;
  }

  setIndex(type, index) {
    if(type==='ext') return false;
    this.wallet[type].index = index;
    this.setIndexRange(index);
    return true;
  }

  getNexIndex(type) {
    if(!this.derivate) return false;
    if(type==='ext') return 0;
    const index = this.wallet[type].index + 1;
    this.setIndexRange(index);
    return index;
  }

  hasAllLocalAddressesIn(ext_addresses = []) {
    if (ext_addresses.length === 0) return false;
    if (this.addresses.length === 0) return false;

    const list = [];
    for (const item of ext_addresses) {
      list.push(item.address);
    }
    for (const value of this.addresses) {
      if (list.indexOf(value.address) === -1) {
        return false;
      }
    }
    return true;
  }

  hasPublicKey(public_key: string, addresses: object[] | undefined = undefined) {
    if (addresses === undefined) {
      addresses = this.addresses;
    }
    for (const item of addresses) {
      if (item?.public_key === public_key) {
        return true;
      }
    }
    return false;
  }

  hasAddress(address: string, addresses: object[] | undefined = undefined) {
    if (addresses === undefined) {
      addresses = this.addresses;
    }
    if(!addresses.length){ return false;}
    for (const item of addresses) {
      if (item.address === address) {
        return true;
      }
    }
    return false;
  }

  hasAddressType(type: string, addresses: object[] | undefined = undefined) {
    if (addresses === undefined) {
      addresses = this.addresses;
    }
    for (const item of addresses) {
      if (item.type === type) {
        return true;
      }
    }
    return false;
  }

  hasAddressFields(address: string, type: string, typeAddress: number, addresses: object[] | undefined = undefined) {
    if (addresses === undefined) {
      addresses = this.addresses;
    }
    for (const item of addresses) {
      if (item.address === address && item.type === type && item.typeAddress === typeAddress) {
        return true;
      }
    }
    return false;
  }

  async setIndexRange(index){
    if(index > this.max_index_range) this.max_index_range = index;
    return true;
  }

  async restoreAddressesFromIndex(type, index){
    const myself = this;
    for(let i=0; i < index; i += 1 ){
      for(let x = 0; x < 2; x += 1){
        if (type !== 'auth' && type !== 'ext') {
          let address = await myself.getAddress(type, myself.getKey(type, x, index), x);
          if (!myself.hasAddress(address)) {
            if (!myself.hasAddressType(type)) {
              const path = myself.path(type, x, index);
              const newAddress = {
                address: address,
                type: type,
                typeAddress: x,
                index: path,
                ...myself.getChallengeFromAddress(address, type, path)
              };
              myself.addresses.push(newAddress);
            }
          }
        }
      }
    }
  }

  async restoreTypeIndexFromServer(type, addresses) {
    let store_index = 0;
    let wallet_index = 0;
    if (type !== 'ext') {
      store_index = Number(await this.getIndexFromStorage(type));
      wallet_index = Number(this.getIndex(type));
      if (store_index > wallet_index) {
        await this.setIndex(type, store_index);
        await this.setIndexRange(store_index);
        await this.restoreAddressesFromIndex(type, store_index);
      }
      if (store_index < wallet_index) {
        await this.setIndexToStorage(type, wallet_index);
        await this.setIndexRange(wallet_index);
        await this.restoreAddressesFromIndex(type, wallet_index);
      }
    }
    if (addresses) {
      for (const addr of addresses) {
        if (addr.type === type && type !== 'ext') {
          const currind = Number(addr.index.split('/').pop())
          if (currind > wallet_index) {
            //  console.log(`currind for type - ${type}:`,currind)
            await this.setIndex(type, currind)
            await this.setIndexToStorage(type, currind);
            await this.setIndexRange(currind);
            await this.restoreAddressesFromIndex(type, currind);
          }
        } else {
          if (type === 'ext') {
            let p = addr?.index?.split('/')[0];
            let ext = addr?.index?.split('/')[1];
            this.addToExternalKey(ext);
          }
        }
      }
    }
  }

  async restoreAllTypeIndexes(addresses) {
    for (const type of this.wallet_types) {
      console.log(`restoreTypeIndexFromServer(${type})`)
      await this.restoreTypeIndexFromServer(type, addresses)
    }
  }

  bytesToHexString(byteArray: Uint8Array) {
    return Array.from(byteArray, byte => {
      return ('0' + (byte & 0xFF).toString(16)).slice(-2);
    }).join('');
  }

  getMnemonicParserFor(language: string = bip39.MNEMONIC_DEFAULT_LANGUAGE) {
    if (!this.mnemonicParserInstances[language]) {
      this.mnemonicParserInstances[language] = new this.utxord.MnemonicParser(`["${bip39.getWordlistFor(language).join('","')}"]`);
    }
    return this.mnemonicParserInstances[language];
  }

  randomBytes(bytesLength = 32): Uint8Array {
    if (crypto && typeof crypto.getRandomValues === 'function') {
      return crypto.getRandomValues(new Uint8Array(bytesLength));
    }
    throw new Error('crypto.getRandomValues must be defined');
  }

  async generateMnemonic(length: number = bip39.MNEMONIC_DEFAULT_LENGTH, language: string = bip39.MNEMONIC_DEFAULT_LANGUAGE) {
    // console.info('===== generateMnemonic:', arguments);
    let strength = length / 3 * 32;
    if (strength === void 0) {
      strength = 128;
    }
    if (!(Number.isSafeInteger(strength) &&
        strength > 0 &&
        strength <= 256 &&
        strength % 32 === 0)) {
      throw new TypeError('Invalid strength');
    }
    const random_hex = this.bytesToHexString(this.randomBytes(strength / 8));
    const parser = this.getMnemonicParserFor(language);
    return await parser.EncodeEntropy(random_hex);
  }

  async validateMnemonic(mnemonic: string, language: string = bip39.MNEMONIC_DEFAULT_LANGUAGE) {
    console.info('===== validateMnemonic:', arguments);
    try {
    const parser = this.getMnemonicParserFor(language);
      return await parser.DecodeEntropy(mnemonic);
    } catch (ex) {
      return false;
    }
  }

  // [Const] DOMString MakeSeed([Const] DOMString phrase, [Const] DOMString passphrase);
  async setUpSeed(mnemonic, passphrase = '', language: string = bip39.MNEMONIC_DEFAULT_LANGUAGE) {
    const valid = this.validateMnemonic(mnemonic, language);
    if(!valid) return 'Invalid checksum';
    const parser = this.getMnemonicParserFor(language);
    const seed = await parser.MakeSeed(mnemonic, passphrase);
    chrome.storage.local.set({ seed: seed });
    this.wallet.root.seed = seed;
    return 'success';
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
    this.expects = {};
    this.connect = false;
    this.sync = false;
    await this.removePublicKeyToWebPage();
    await chrome.storage.local.clear();
    chrome.runtime.reload()
    const lastErr = chrome.runtime.lastError;
    if (lastErr) {
      console.error(lastErr)
      return false
    }
   return true
  }

  genRootKey() {
    if(!this.checkSeed()) return false;
    if (this.wallet.root.key) return this.wallet.root.key;
    console.log('seed:',this.getSeed());
    this.wallet.root.key = new this.utxord?.KeyRegistry(this.network, this.getSeed());
    console.log('root',this.wallet.root);
    for(const type of this.wallet_types) {
      if(type !== 'auth') {
        console.log('type: ',type,'|json: ',JSON.stringify(this.wallet[type].filter))
        this.wallet.root.key.AddKeyType(type, JSON.stringify(this.wallet[type].filter));
      }
    }
    return this.wallet.root.key;
  }

  pubKeyStrToP2tr(publicKey) {
    return this.bech.Encode(publicKey);
  }

  addToExternalKey(keyhex, pass) {
    const myself = this;

    const keypair = new myself.utxord.KeyPair(keyhex);
    const tmpAddress = this.bech.Encode(keypair.PubKey());
    if (!myself.hasAddress(tmpAddress, this.wallet.ext.keys)) {
      let enkeyhex = keyhex;
      let enFlag = false;
      if (pass) {
        enkeyhex = this.encrypt(keyhex, pass);
        enFlag = true;
      }
      console.log('ExternalKeyAddress:', tmpAddress);
      this.wallet.ext.keys.push({
        key: keypair,
        p2tr: tmpAddress,
        pubKeyStr: keypair.PubKey(),
        privKeyStr: enkeyhex,
        index: `${Number(enFlag)}/${enkeyhex}`,
        type: 'ext',
      });
      const ext_keys = []
      for (const item of this.wallet.ext.keys) {
        ext_keys.push(item.privKeyStr);
      }
      setTimeout(() => {
        chrome.storage.local.set({ext_keys: ext_keys});
      }, 3000);

    }
    return true;
  }

  resExtKeys(ext_keys) {
    this.wallet.ext.keys = [];
    for (const item of ext_keys) {
      this.addToExternalKey(item);
    }
    return this.wallet.ext.keys;
  }

  genKey(type: string, typeAddress: number | undefined = undefined, index: number | undefined = undefined) {
   if (!this.wallet_types.includes(type)) return false;
   if (!this.checkSeed()) return false;
   if(typeAddress === undefined){
     typeAddress = this.wallet[type].typeAddress;
   }
   this.wallet[type].key = this.getKey(type, typeAddress, index);
   this.wallet[type].address = this.getAddress(type, this.wallet[type].key, typeAddress);

   return true;
 }

  addPopAddresses(){
    if (this.genKey('oth', 0)) {
      if (!this.hasAddressFields(this.wallet['oth'].address, 'oth', 0)) {
        let ch = this.getChallengeFromType('oth', 0);
        this.addresses.push({ // add m86 fund address
          address: this.wallet['oth'].address,
          type: 'oth',
          typeAddress: 0,
          index: this.path('oth', 0),
          ...ch
        });
      }
    }

    if (this.genKey('oth', 1)) { // add m84 fund address
      if (!this.hasAddressFields(this.wallet['oth'].address, 'oth', 1)) {
        let ch = this.getChallengeFromType('oth', 1);
        this.addresses.push({
          address: this.wallet['oth'].address,
          type: 'oth',
          typeAddress: 1,
          index: this.path('oth', 1),
          ...ch
        });
      }
    }

    if (this.genKey('fund', 1)) {
      if (!this.hasAddressFields(this.wallet['fund'].address,'fund', 1)) {
        let ch = this.getChallengeFromType('fund', 1);
        this.addresses.push({ // add m84 fund address
          address: this.wallet['fund'].address,
          type: 'fund',
          typeAddress: 1,
          index: this.path('fund', 1),
          ...ch
        });
      }
    }

    if (this.genKey('fund', 0)) { // add m86 fund address
      if (!this.hasAddressFields(this.wallet['fund'].address,'fund', 0)) {
        let ch = this.getChallengeFromType('fund', 0);
        this.addresses.push({
          address: this.wallet['fund'].address,
          type: 'fund',
          typeAddress: 0,
          index: this.path('fund', 0),
          ...ch
        });
      }
    }

    if (this.genKey('ord', 1)) {
      if (!this.hasAddressFields(this.wallet['ord'].address,'ord', 1)) {
        let ch = this.getChallengeFromType('ord', 1);
        this.addresses.push({ // add m84 fund address
          address: this.wallet['ord'].address,
          type: 'ord',
          typeAddress: 1,
          index: this.path('ord', 1),
          ...ch
        });
      }
    }

    if (this.genKey('ord', 0)) { // add m86 fund address
      if (!this.hasAddressFields(this.wallet['ord'].address,'ord', 0)) {
        let ch = this.getChallengeFromType('ord', 0);
        this.addresses.push({
          address: this.wallet['ord'].address,
          type: 'ord',
          typeAddress: 0,
          index: this.path('ord', 0),
          ...ch
        });
      }
    }

  }

getKeyFromKeyCache(address: string){
  if(!address) return;
  if(!this.keyCache.length) return;
  const out = this.keyCache?.find((item) =>item.address === address);
  return out?.key;
}

hasAddressFromKeyCache(address: string){
  if(!address) return false;
  if(!this.keyCache.length) return false;
  const key = this.getKeyFromKeyCache(address);
  if(key?.ptr > 0){
    return true;
  }
  return false;
}

checkAddressFromKeyRegistry(address, type = undefined, path = undefined){
  const myself = this;
  if(myself.hasAddressFromKeyCache(address) || myself.hasAddress(address, myself.skipAddresses)){
    // console.log('checkAddressFromKeyRegistry->address:', address,' =>detect');
    return;
  }

  // console.log('checkAddressFromKeyRegistry->address:', address, ' type:',type, ' path:',path)
   // return setTimeout(((address, type, path, myself) =>{
   //   return ()=>{
       try {
  let min_index = 0;
  let max_index_range = myself.max_index_range + 1;
  let accounts = ["0'", "1'", "2'"];
  let change = "0";
  let key_type = "DEFAULT";
  let out = null;

  if(type){
    const for_script = (type === 'uns' || type === 'intsk' || type === 'intsk2' || type === 'scrsk' || type === 'auth');
    key_type = for_script? "TAPSCRIPT" : "DEFAULT";
    accounts = for_script ? ["3'", "4'", "5'", "6'"] : ["0'", "1'", "2'"];
  }

  if(path){
  const pres = path.replace('m/86\'/','').replace('m/84\'/','').split('/');
    accounts = [pres[1]];
    change = pres[2];
    min_index = pres[3];
    max_index_range = pres[3] + 1;
    switch (pres[1]) {
    case "0'": case "1'": case "2'": default: key_type = "DEFAULT"; break;
    case "3'": case "4'": case "5'": case "6'": key_type = "TAPSCRIPT"; break;
    }
  }
  out = myself.wallet.root.key.LookupAddress(address, JSON.stringify({
        look_cache: true,
        key_type: key_type,
        accounts: accounts,
        change: [change],
        index_range: `${min_index}-${max_index_range}`
      }));
    if(out?.ptr > 0) {
      if(!myself.hasAddress(address, myself.keyCache)){
        myself.keyCache.push({address: address, key: out});
      }
      return out;
    }

    if(!type && !path){
      key_type = "TAPSCRIPT";
      accounts = ["3'", "4'", "5'", "6'"];
      out = myself.wallet.root.key.LookupAddress(address, JSON.stringify({
                look_cache: true,
                key_type: key_type,
                accounts: accounts,
                change: [change],
                index_range: `${min_index}-${max_index_range}`
              }));
              if(out?.ptr > 0) {
                if(!myself.hasAddress(address, myself.keyCache)){
                  myself.keyCache.push({address: address, key: out});
                }
                return out;
              }
    }
    if(!myself.hasAddress(address, myself.skipAddresses)){
      myself.skipAddresses.push({address: address});
    }
    return null;
  } catch (e) {
    console.log('checkAddressFromKeyRegistry:',myself.getErrorMessage(e));
    if(!myself.hasAddress(address, myself.skipAddresses)){
      myself.skipAddresses.push({address: address});
    }
    return null;
  }
   //   };
   // })(address,type, path, myself), 0);
}

getKeyFromKeyRegistry(address: string, type = undefined, path = undefined){
  this.checkAddressFromKeyRegistry(address, type, path);
  if(this.hasAddress(address, this.skipAddresses)){
    return null;
  }
  const key = this.getKeyFromKeyCache(address);
  if(key?.ptr){
    return key;
  }
}

hasAddressKeyRegistry(address: string, type = undefined, path = undefined){
    const out = this.getKeyFromKeyRegistry(address, type, path);
    if(out?.ptr > 0){
      return true;
    }
    return false;
  }


  async genKeys() { //current keys
    const myself = this;
    myself.addPopAddresses();
    for (const type of myself.wallet_types) {
      //console.log('genKeys->',type);
      // setTimeout(((type, myself, publicKeys) =>{
      //   return ()=>{
      const key_0 = myself.getKey(type, 0);
      const address_0 = myself.getAddress(type, key_0, 0);
      const key_1 = myself.getKey(type, 1);
      const address_1 = myself.getAddress(type, key_1, 1);

      switch (myself.wallet[type].typeAddress) {
        case 0:
          myself.wallet[type].key = key_0;
          myself.wallet[type].address = address_0;
        break;
        case 1:
          myself.wallet[type].key = key_1;
          myself.wallet[type].address = address_1;
        break;
      }
      if(type !== 'auth' && type !== 'ext'){
        if (!myself.hasAddress(address_0) || !myself.hasAddress(address_1)) {
          const path_0 = myself.path(type, 0)
          myself.addresses.push({
              address: address_0,
              type: type,
              typeAddress: 0,
              index: path_0,
              ...myself.getChallengeFromAddress(address_0, type, path_0)
          });
          const path_1 = myself.path(type, 1)
          myself.addresses.push({
              address: address_1,
              type: type,
              typeAddress: 1,
              index: path_1,
              ...myself.getChallengeFromAddress(address_1, type, path_1)
          });
          console.debug(`genKeys(): push new "${type}" and typeAddress: Taproot addresses:`, address_0);
          console.debug(`genKeys(): push new "${type}" and typeAddress: SegWit  addresses:`, address_1);
        }
      }
  //   };
  // })(type, myself, publicKeys), 0);
    }
    return {addresses: this.addresses};
  }

  async freeBalance(balance) {
    for (const item of balance) {
      this.destroy(item.key);
    }
    return [];
  }

  async updateBalancesFrom(msgType: string, addresses: []) {
    const list = await this.prepareAddressToPlugin(addresses);
    const balances = await this.prepareBalances(list);
    console.debug(`${msgType} balances:`, {...balances || {}});
    this.fundings = balances.funds;
    this.inscriptions = balances.inscriptions;
    return list;
  }

  parseExpectsData(msgType: string, data: object) {
    return {
      defaultMiningFeeRate: data?.mining_fee_rate || null,
      miningFeeRates: {
        priority: data?.mining_fee_rates?.priority || null,
        normal: data?.mining_fee_rates?.normal || null,
        min: data?.mining_fee_rates?.min || null,
        max: data?.mining_fee_rates?.max || null,
      },
      ordExpectedAmount: data?.ord_expected_amount || null,
    };
  }

  async prepareBalances(balances) {
    const myself = this;
    let list = this.balances;
    if (balances) {
      list = balances;
    }
    const funds = [];
    const inscriptions = [];
    if (list?.length) {
      for (const item of list) {
        for (const i of item?.utxo_set || []) {
          if (!i?.is_inscription && !i?.is_rune) {
            if (!i?.is_locked) {
              funds.push({
                ...i,
                address: item.address,
                path: item.index,
              });
            }
          } else {
            inscriptions.push({
              ...i,
              address: item.address,
              path: item.index,
            });
          }
        }
      }
    }
    return {funds, inscriptions};
  }

  async sumAllFunds(all_funds) {
    if(!all_funds) return 0;
    // console.log('sumAllFunds:',all_funds)
    return all_funds?.reduce((a,b)=>a+b?.amount, 0);
  }

  async sumMyInscribtions(inscriptions) {
    if(!inscriptions) return 0;
    // console.log('sumMyInscribtions:',inscriptions)
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

  selectByFundOutput(txid, nout, fundings = []) {
    const myself = this;
    let list = myself.fundings;
    if (fundings.length > 0) {
      list = fundings;
    }
    if (list) {
      for (const item of list) {
        if (txid === item.txid && nout === item.nout) {
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

  async getSupportedVersions(builderObject: object | undefined = undefined) {
    builderObject = builderObject || new this.utxord.CreateInscriptionBuilder(
      this.network,
      this.utxord.INSCRIPTION
    );
    const supported_versions = builderObject.SupportedVersions();
    // console.debug('getSupportedVersions: builderObject.SupportedVersions():', supported_versions);
    const versions = JSON.parse(supported_versions || '[8]');
    return versions;
  }
  getCurrentNetWorkText() {
    if (!this.network) { return 'mainnet'; }
    switch (this.network) {
      case this.utxord.SIGNET:
        return 'signet';
      case this.utxord.TESTNET:
        return 'testnet';
      case this.utxord.REGTEST:
        return 'regtest';
      case this.utxord.MAINNET:
        return 'mainnet';
      default:
      return 'mainnet';
    }
  }
  getCurrentNetWorkLabel() {
    if (!this.network) { return ' '; }
    switch (this.network) {
      case this.utxord.SIGNET:
          return 'SigNet';
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
      case this.utxord.SIGNET:
        return 's'; //signet
      case this.utxord.TESTNET:
        return 't'; //testnet
      case this.utxord.MAINNET:
        return 'm'; //mainnet
      case this.utxord.REGTEST:
        return 'r';  //regtest
      default:
        return 's'; //testnet
    }
  }

  setUpNetWork(type) {
    //        this.utxord.MAINNET;
    //        this.utxord.TESTNET;
    //        this.utxord.REGTEST;
    //        this.utxord.SIGNET;
    switch (type) {
      case 't':
        return this.utxord.TESTNET;
      case 'm':
        return this.utxord.MAINNET;
      case 'r':
        return this.utxord.REGTEST;
      case 's':
        return this.utxord.SIGNET;
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

  getErrorMessage(exception: any) {
   if(typeof exception === 'number'){
     let errorMessage = this.utxord.Exception.prototype.getMessage(Number.parseInt(exception, 10));
     if(errorMessage.indexOf('funds amount too small') > -1){
       errorMessage = 'TransactionError: not enough funds';
     }
     return errorMessage;
   }
   return exception;
 }

  async sendNotificationMessage(type?: string, message: any) {
    console.info(NOTIFICATION, type, message);
    if(this.exception_count > 0 || this.warning_count > 0) return ;
    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(NOTIFICATION , `${message}`, `popup@${currentWindow.id}`)
    return `${message}`;
  }

  async sendWarningMessage(type?: string, warning: any, reportToSentry: boolean = true) {
    let errorStack = '';
    warning = this.getErrorMessage(warning);
    const errorMessage = warning?.message || warning;
    if(warning?.name) type = warning?.name;
    if(warning?.stack) errorStack = warning?.stack;

    const currentWindow = await this.WinHelpers.getCurrentWindow()
    // console.debug("sendWarningMessage: currentWindow:", currentWindow);
    this.warning_count += 1;
    if(this.warning_count === 1){
      sendMessage(WARNING, errorMessage, `popup@${currentWindow.id}`);
      setTimeout(() =>this.warning_count = 0, 3000);
    }
    // if (reportToSentry) {
    //   this.sentry(warning);
    // }
    console.warn(type, errorMessage, errorStack);


    return `${type} ${errorMessage} ${errorStack}`;
  }

  async sendExceptionMessage(type?: string, exception: any) {
    let errorStack = '';
    exception = this.getErrorMessage(exception);
    const errorMessage = exception?.message || exception;
    if(exception?.name) type = exception?.name;
    if(exception?.stack) errorStack = exception?.stack;

    const currentWindow = await this.WinHelpers.getCurrentWindow()
    this.exception_count += 1;
    if(this.exception_count === 1){
      sendMessage(EXCEPTION, errorMessage, `popup@${currentWindow.id}`)
      setTimeout(() =>this.exception_count = 0, 3000);
    }
    // this.sentry(exception);
    console.error(type, errorMessage, errorStack);

    return `${type} ${errorMessage} ${errorStack}`;
  }

  async fetchAddress(address: string) {
    const response = await this.Rest.get(`/api/address/${address}/balance/`);
    return response;
  }

  async fetchUSDRate() {
    const response = await this.Rest.get('/api/wallet/prices/');
    return response;
  }

  async fetchTimeSystem(){
    if(this.timeSync){
      return true;
    }
    const datetime_server_resp = await this.Rest.get('/api/datetime');
    const datetime_from_server = datetime_server_resp?.data?.datetime;
    if(!datetime_from_server){
      return true;
    }
    const d = new Date(datetime_from_server);

    console.debug('datetime from server:', d);
    const serverYear = d.getUTCFullYear()
    const serverMonth = this.zeroPad((d.getUTCMonth()+1), 2)
    const serverDay = this.zeroPad(d.getUTCDate(), 2)
    const serverHour = this.zeroPad(d.getUTCHours(), 2)
    const serverMinute = this.zeroPad(d.getUTCMinutes(), 2)
    const serverSecond = this.zeroPad(d.getUTCSeconds(), 2)

    const sd = new Date();
    // console.log('sd:',sd);
    const sysYear = sd.getUTCFullYear()
    const sysMonth = this.zeroPad((sd.getUTCMonth()+1), 2)
    const sysDay = this.zeroPad(sd.getUTCDate(), 2)
    const sysHour = this.zeroPad(sd.getUTCHours(), 2)
    const sysMinute = this.zeroPad(sd.getUTCMinutes(), 2)
    const sysSecond = this.zeroPad(sd.getUTCSeconds(), 2)
    if(serverYear !== sysYear ||
      serverMonth !== sysMonth ||
      serverDay !== sysDay ||
      serverHour !== sysHour ||
      serverMinute !== sysMinute ||
      Math.abs(serverSecond-sysSecond) > 5
    ){
      console.log('serverSecond',serverSecond,'| sysSecond:',sysSecond)
      console.log('Enable automatic date and time setting for your device'); return false;
    }
    this.timeSync = true;
    return true;
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
    console.log('all_funds:', [...all_funds || []]);

    const funds_in_queue = await this.selectFundsByFlags(all_funds, false, true);
    const available_funds = await this.selectFundsByFlags(all_funds, false, false);

    const total_sum = await this.sumAllFunds(all_funds);
    const available_sum = await this.sumAllFunds(available_funds);
    const in_queue_sum = await this.sumAllFunds(funds_in_queue);

    const sum_my_inscr = await this.sumMyInscribtions(my);
    return {
      data: {
        sync: this.sync,
        connect: this.connect,
        confirmed: available_sum || 0,
        unconfirmed: in_queue_sum || 0,
        to_address: response?.data?.confirmed || 0,
        used_for_inscribtions: sum_my_inscr || 0,
        inscriptions: my || []
      }
    };
  }

  updateFundsByOutputs(fund_list, update = {}) {
    for (const fund of fund_list) {
      let utxo = this.selectByFundOutput(fund.txid, fund.nout);
      Object.assign(utxo, update);
    }
  }

  pushChangeToFunds(arg) {
    if (arg?.ptr) {
      this.fundings.push({
        address: arg?.Address(),
        txid: arg?.TxID(),
        nout: arg?.NOut(),
        amount: this.btcToSat(arg?.Amount() || "0.0"),
        is_inscription: false,
        is_rune: false,
        is_locked: false,
        in_queue: true,
        path: "",
      });
      this.destroy(arg);
    }
  }

  pushOrdToInscriptions(arg) {
    if (arg?.ptr) {
      this.inscriptions.push({
        address: arg?.Address(),
        txid: arg?.TxID(),
        nout: arg?.NOut(),
        amount: this.btcToSat(arg?.Amount() || "0.0"),
        is_inscription: true,
        is_rune: false,
        is_locked: false,
        in_queue: true,
        path: "",
      });
      this.destroy(arg);
    }
  }

  async removePublicKeyToWebPage(tabId: number | undefined = undefined): Promise<void> {
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
    if (1 < tabs.length) {
      console.warn(`----- removePublicKeyToWebPage: there are ${tabs.length} tabs found with tdbId: ${tabId}`);
    }
    for (let tab of tabs) {
      if(tab?.id) {
        const [{result}] = await chrome.scripting.executeScript({
          target: { tabId: tab?.id },
          func: () =>window.localStorage.removeItem('publickey'),
          args: [],
        });
        if(result) return result;
      }
    }
  }

  async getPublicKeyFromWebPage(tabId: number | undefined = undefined): Promise<void> {
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
    if (1 < tabs.length) {
      console.warn(`----- getPublicKeyFromWebPage: there are ${tabs.length} tabs found with tdbId: ${tabId}`);
    }
    for (let tab of tabs) {
      if(tab?.id) {
        const [{result}] = await chrome.scripting.executeScript({
          target: { tabId: tab?.id },
          func: () =>window.localStorage.getItem('publickey'),
          args: [],
        });
        if(result) return result;
      }
    }
  }

  async setPublicKeyToWebPage(tabId: number | undefined = undefined): Promise<void> {
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
    if (1 < tabs.length) {
      console.warn(`----- setPublicKeyToWebPage: there are ${tabs.length} tabs found with tdbId: ${tabId}`);
    }
    for (let tab of tabs) {
      if(tab?.id) {
        const [{result}] = await chrome.scripting.executeScript({
          target: { tabId: tab?.id },
          func: (a) =>window.localStorage.setItem('publickey', a),
          args: [myself.wallet.auth.key?.PubKey()],
        });
        if(result) return result;
      }
    }
  }

  async sendMessageToWebPage(type, args, tabId: number | undefined = undefined): Promise<void> {
    const myself = this;
    const base_url = BASE_URL_PATTERN.replace('*', '');

    let tabs: Tab[];
    if (tabId != null) {
      tabs = [await chrome.tabs.get(tabId)]
    } else {
      tabs = await chrome.tabs.query({
        windowType: 'normal',
        url: BASE_URL_PATTERN,
      });
    }
    if(!args){
      console.warn('sendMessageToWebPage-> error no args:', args,'type:', type);
      return null;
    }
    if (1 < tabs.length) {
      console.warn(`----- sendMessageToWebPage: there are ${tabs.length} tabs found with tdbId: ${tabId}`);
    }
    if(!await this.fetchTimeSystem()){
      return null;
    }
    for (let tab of tabs) {
      const url = tab.url || tab.pendingUrl;
      if(tab?.id &&
         url?.startsWith(base_url) &&
         !url?.startsWith('chrome-extension://') &&
         !url?.startsWith('chrome://')) {
        await chrome.scripting.executeScript({
          target: { tabId: tab?.id },
          func: function (t, a) {
            window.postMessage({ type: t, payload: a })
          },
          args: [type, args],
        });
      }
    }
  }

  //------------------------------------------------------------------------------

  signToChallenge(challenge, tabId: number | undefined = undefined): boolean {
    const myself = this;
    myself.removePublicKeyToWebPage(tabId);
    if (myself.wallet.auth.key) {
      const signature = myself.wallet.auth.key.SignSchnorr(challenge);
      console.log("SignSchnorr::challengeResult:", signature);
      myself.sendMessageToWebPage(CONNECT_RESULT, {
        publickey: myself.wallet.auth.key.PubKey(),
        challenge: challenge,
        signature: signature
      }, tabId);
    myself.setPublicKeyToWebPage(tabId);
    myself.connect = true;
    myself.sync = false;
    } else {
      myself.sendMessageToWebPage(CONNECT_RESULT, null, tabId);
    }
    return myself.connect;
  }

  //------------------------------------------------------------------------------

  async getRawTransactions(builderObject, phase = undefined) {
    let raw_size;
    if(!phase){
      raw_size = builderObject.TransactionCount();
    }else{
      raw_size = builderObject.TransactionCount(phase);
    }
    const raw = [];
    for(let i = 0; i < raw_size; i += 1) {
      if(phase!==undefined) {
        raw.push(builderObject.RawTransaction(phase, i))
      }else{
        raw.push(builderObject.RawTransaction(i))
      }
    }
    return raw;
  }

  //------------------------------------------------------------------------------

  async estimateInscription(payload) {
    return await this.createInscriptionContract(
        {
          ...payload,
          content: "00".repeat(payload.content_length),
          content_length: undefined
        },
        true
    );
  }

  //------------------------------------------------------------------------------
  async buyProductContract(payload, estimate: boolean = false) {
    const myself = this;

    const outData = {
      data: null,
      amount: 0,
      change_amount: null,
      inputs_sum: 0,
      utxo_list: [],
      expect_amount: Number(payload.expect_amount),
      fee_rate: payload.fee_rate,
      fee: payload.fee,
      fund: myself.wallet.fund,
      raw: [],
      total_mining_fee: 0,
      errorMessage: null as string | null
    };
    try {
      console.log('buyProductContract payload: ', {...payload || {}});

      let tx = new myself.utxord.SimpleTransaction(myself.network);

      const protocol_version = Number(payload?.protocol_version);
      if (protocol_version) {
        const versions = await myself.getSupportedVersions(tx);
        if(versions.indexOf(protocol_version) === -1) {
          outData.errorMessage = 'Please update the plugin to latest version.';
          outData.raw = [];
          return outData;
        }
      }

      tx.MiningFeeRate(myself.satToBtc(payload.fee_rate).toFixed(8));

      if (payload.owner_fee?.amount) {
        tx.AddOutput(
            myself.satToBtc(payload.market_fee?.amount).toFixed(8),
            payload.owner_fee?.address
        );
      }


      if (payload.market_fee?.amount) {
        tx.AddOutput(
            myself.satToBtc(payload.market_fee?.amount).toFixed(8),
            payload.market_fee?.address
        );
      }

      let min_fund_amount = myself.btcToSat(tx.GetMinFundingAmount("")); // empty string for now
      min_fund_amount += myself.btcToSat(tx.GetNewOutputMiningFee());  // to take in account a change output
      outData.amount = min_fund_amount;

      if (!myself.fundings.length && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      const input_mining_fee = myself.btcToSat(tx.GetNewInputMiningFee());
      const utxo_list = await myself.smartSelectFundsByAmount(min_fund_amount, input_mining_fee);
      outData.utxo_list = utxo_list;

      console.log("min_fund_amount:", min_fund_amount);
      console.log("utxo_list:", utxo_list);

      if (utxo_list?.length < 1 && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      outData.inputs_sum = await myself.sumAllFunds(utxo_list);

      for(const fund of utxo_list) {
        const utxo = new myself.utxord.UTXO(myself.network,
            fund.txid,
            fund.nout,
            myself.satToBtc(fund.amount).toFixed(8),
            fund.address
        );
        tx.AddInput(utxo);
        myself.utxord.destroy(utxo);
      }

      tx.AddChangeOutput(myself.wallet.fund.address);  // should be last in/out definition

      if (!estimate) {
        tx.Sign(myself.wallet.root.key, "fund");
        outData.raw = await myself.getRawTransactions(tx);

        // TODO: core API needs to be refactored to make change-related stuff more usable
        const changeOutput = tx.ChangeOutput();
        if (changeOutput.ptr != 0) {
          const changeDestination = changeOutput.Destination();
          if (changeDestination.ptr != 0) {
            outData.change_amount = myself.btcToSat(changeDestination.Amount()) || null;
            myself.destroy(changeDestination);
          }
          myself.destroy(changeOutput);
        }
        console.debug('change_amount: ', outData.change_amount);

        outData.data = tx.Serialize(SIMPLE_TX_PROTOCOL_VERSION, myself.utxord.TX_SIGNATURE);

        const contractData = JSON.parse(outData.data);
        console.debug('buyProductContract: contractData:', contractData);
      }

      outData.total_mining_fee = myself.btcToSat(tx.GetTotalMiningFee(""));
      myself.utxord.destroy(tx);

      return outData;
    } catch (e) {
      outData.errorMessage = await myself.sendExceptionMessage(BUY_PRODUCT, e);
      setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
      return outData;
    }
  }

  //------------------------------------------------------------------------------
  async sendToAddressContract(payload, estimate: boolean = false) {
    const myself = this;

    const outData = {
      data: null,
      change_amount: null,
      inputs_sum: 0,
      utxo_list: [],
      amount: Number(payload.amount),
      expect_amount: Number(payload.amount),
      fee_rate: payload.fee_rate,
      address: payload.address,
      fund: myself.wallet.fund,
      raw: [],
      total_mining_fee: 0,
      errorMessage: null as string | null
    };
    try {
      console.log('sendToAddressContract payload: ', {...payload || {}});

      let tx = new myself.utxord.SimpleTransaction(myself.network);

      const protocol_version = Number(payload?.protocol_version);
      if (protocol_version) {
        const versions = await myself.getSupportedVersions(tx);
        if(versions.indexOf(protocol_version) === -1) {
          outData.errorMessage = 'Please update the plugin to latest version.';
          outData.raw = [];
          return outData;
        }
      }

      tx.MiningFeeRate(myself.satToBtc(payload.fee_rate).toFixed(8));


        const sendOutput = new myself.utxord.P2TR(
            myself.network,
            myself.satToBtc(outData.amount).toFixed(8),
            payload.address
        );
        tx.AddOutput(sendOutput);
        myself.utxord.destroy(sendOutput);



      if (payload.market_fee?.amount) {
        const marketOutput = new myself.utxord.P2TR(
            myself.network,
            myself.satToBtc(payload.market_fee?.amount).toFixed(8),
            payload.market_fee?.address
        );
        tx.AddOutput(marketOutput);
        myself.utxord.destroy(marketOutput);
      }

      let min_fund_amount = myself.btcToSat(tx.GetMinFundingAmount("")); // empty string for now
      min_fund_amount += myself.btcToSat(tx.GetNewOutputMiningFee());  // to take in account a change output
      outData.amount = min_fund_amount;

      if (!myself.fundings.length && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      const input_mining_fee = myself.btcToSat(tx.GetNewInputMiningFee());
      const utxo_list = await myself.smartSelectFundsByAmount(min_fund_amount, input_mining_fee);
      outData.utxo_list = utxo_list;

      console.log("min_fund_amount:", min_fund_amount);
      console.log("utxo_list:", utxo_list);

      if (utxo_list?.length < 1 && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      outData.inputs_sum = await myself.sumAllFunds(utxo_list);

      for(const fund of utxo_list) {
        const utxo = new myself.utxord.UTXO(myself.network,
            fund.txid,
            fund.nout,
            myself.satToBtc(fund.amount).toFixed(8),
            fund.address
        );
        tx.AddInput(utxo);
        myself.utxord.destroy(utxo);
      }

      tx.AddChangeOutput(myself.wallet.fund.address);  // should be last in/out definition

      if (!estimate) {
        tx.Sign(myself.wallet.root.key, "fund");
        outData.raw = await myself.getRawTransactions(tx);

        // TODO: core API needs to be refactored to make change-related stuff more usable
        const changeOutput = tx.ChangeOutput();
        if (changeOutput.ptr != 0) {
          const changeDestination = changeOutput.Destination();
          if (changeDestination.ptr != 0) {
            outData.change_amount = myself.btcToSat(changeDestination.Amount()) || null;
            myself.destroy(changeDestination);
          }
          myself.destroy(changeOutput);
        }
        console.debug('change_amount: ', outData.change_amount);

        outData.data = tx.Serialize(SIMPLE_TX_PROTOCOL_VERSION, myself.utxord.TX_SIGNATURE);

        const contractData = JSON.parse(outData.data);
        console.debug('sendToAddressContract: contractData:', contractData);
      }

      outData.total_mining_fee = myself.btcToSat(tx.GetTotalMiningFee(""));
      myself.utxord.destroy(tx);

      return outData;
    } catch (e) {
      outData.errorMessage = await myself.sendExceptionMessage(SEND_TO_ADDRESS, e);
      //setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
      return outData;
    }
  }

  //------------------------------------------------------------------------------

  async transferForLazyInscriptionContract(payload, estimate: boolean = false) {
    const myself = this;

    // // temporary safeguard
    // if (! Number(payload.fee_rate)) {
    //   payload.fee_rate = 1000;
    // }

    const outData = {
      data: null,
      // sk: null,  // TODO: use/create ticket for excluded sk (UT-???)
      amount: 0,
      change_amount: null,
      inputs_sum: 0,
      utxo_list: [],
      expect_amount: Number(payload.expect_amount),
      fee_rate: payload.fee_rate,
      fee: payload.fee,
      fund: myself.wallet.fund,
      // size: (payload.content.length + payload.content_type.length),
      raw: [],
      total_mining_fee: 0,
      errorMessage: null as string | null
    };
    try {
      console.log('transferForLazyInscriptionContract payload: ', {...payload || {}});

      let tx = new myself.utxord.SimpleTransaction(myself.network);

      const protocol_version = Number(payload?.protocol_version);
      if (protocol_version) {
        const versions = await myself.getSupportedVersions(tx);
        if(versions.indexOf(protocol_version) === -1) {
          outData.errorMessage = 'Please update the plugin to latest version.';
          outData.raw = [];
          return outData;
        }
      }

      tx.MiningFeeRate(myself.satToBtc(payload.fee_rate).toFixed(8));

      let collection = null;
      if (payload?.collection?.owner_txid) {
        // if collection is present.. than finding an output with collection
        collection = myself.selectByOrdOutput(
            payload.collection.owner_txid,
            payload.collection.owner_nout
        );
        console.log("payload.collection:", payload.collection)
        console.debug('selectByOrdOutput collection:', collection);
      }

      if (!collection) {
        myself.sendWarningMessage(
            'TRANSFER_LAZY_COLLECTION',
            `Collection(txid:${payload.collection.owner_txid}, nout:${payload.collection.owner_nout}) is not found in balances`
        );
        setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
        outData.errorMessage = "Collection is not found in balances.";
        // outData.raw = [];
        return outData;
      }

      const collectionUtxo = new myself.utxord.UTXO(
          myself.network,
          payload.collection.owner_txid,
          payload.collection.owner_nout,
          myself.satToBtc(collection.amount).toFixed(8),
          collection.address
      );
      tx.AddInput(collectionUtxo);
      myself.utxord.destroy(collectionUtxo);

      tx.AddOutput(
          myself.satToBtc(collection.amount).toFixed(8),
          payload.transfer_address
      );

      if (payload.market_fee?.amount) {
        tx.AddOutput(
            myself.satToBtc(payload.market_fee?.amount).toFixed(8),
            payload.market_fee?.address
        );
      }

      let min_fund_amount = myself.btcToSat(tx.GetMinFundingAmount("")); // empty string for now
      min_fund_amount += myself.btcToSat(tx.GetNewOutputMiningFee());  // to take in account a change output
      outData.amount = min_fund_amount;

      if (!myself.fundings.length && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      const input_mining_fee = myself.btcToSat(tx.GetNewInputMiningFee());
      const utxo_list = await myself.smartSelectFundsByAmount(min_fund_amount, input_mining_fee);
      outData.utxo_list = utxo_list;

      console.log("min_fund_amount:", min_fund_amount);
      console.log("utxo_list:", utxo_list);

      if (utxo_list?.length < 1 && !estimate) {
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        // outData.raw = [];
        return outData;
      }

      outData.inputs_sum = await myself.sumAllFunds(utxo_list);

      for(const fund of utxo_list) {
        const utxo = new myself.utxord.UTXO(myself.network,
            fund.txid,
            fund.nout,
            myself.satToBtc(fund.amount).toFixed(8),
            fund.address
        );
        tx.AddInput(utxo);
        myself.utxord.destroy(utxo);
      }

      tx.AddChangeOutput(myself.wallet.fund.address);  // should be last in/out definition

      if (!estimate) {
        tx.Sign(myself.wallet.root.key, "fund");
        outData.raw = await myself.getRawTransactions(tx);

        // TODO: core API needs to be refactored to make change-related stuff more usable
        const changeOutput = tx.ChangeOutput();
        if (changeOutput.ptr != 0) {
          const changeDestination = changeOutput.Destination();
          if (changeDestination.ptr != 0) {
            outData.change_amount = myself.btcToSat(changeDestination.Amount()) || null;
            myself.destroy(changeDestination);
          }
          myself.destroy(changeOutput);
        }
        console.debug('change_amount: ', outData.change_amount);

        outData.data = tx.Serialize(SIMPLE_TX_PROTOCOL_VERSION, myself.utxord.TX_SIGNATURE);

        const contractData = JSON.parse(outData.data);
        console.debug('transferForLazyInscriptionContract: contractData:', contractData);
      }

      outData.total_mining_fee = myself.btcToSat(tx.GetTotalMiningFee(""));
      myself.utxord.destroy(tx);

      return outData;
    } catch (e) {
      outData.errorMessage = await myself.sendExceptionMessage(TRANSFER_LAZY_COLLECTION, e);
      setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
      return outData;
    }
  }

  //------------------------------------------------------------------------------

  async createInscriptionContract(payload, estimate: boolean = false, use_funds_in_queue = false) {
    const myself = this;
    const outData = {
      data: null,
      // sk: null,  // TODO: use/create ticket for excluded sk (UT-???)
      amount: 0,
      change_amount: null,
      inputs_sum: 0,
      utxo_list: [],
      expect_amount: Number(payload.expect_amount),
      fee_rate: payload.fee_rate,
      fee: payload.fee,
      fund: myself.wallet.fund,
      size: (payload.content.length + payload.content_type.length),
      total_mining_fee: 0,
      market_fee: payload.platform_fee || 0,
      purchase_price: payload?.purchase_price || -1,
      raw: [],
      outputs: {
        collection: {} as object | null,
        inscription: {} as object | null,
        change: {} as object | null,
      },
      used_wallets: new Set<string>(),
      errorMessage: null as string | null
    };
    try {
      console.log('createInscriptionContract payload: ', {...payload || {}});
      // const contract_provided = payload?.contract ? true : false;
      const is_lazy = payload?.is_lazy ? true : false;

      let collection = null;
      let flagsFundingOptions = "";
      // let collection_utxo_key;

      if (is_lazy) {
        payload.collection = null;
      }

      // check if fund address is SegWit one
      if (myself.wallet['fund'].typeAddress === 1) {
        flagsFundingOptions += flagsFundingOptions ? "," : "";
        flagsFundingOptions += "p2wpkh_utxo";
      }

      if (payload?.collection?.genesis_txid) {
        // if collection is present.. than finding an output with collection
        collection = myself.selectByOrdOutput(
            payload.collection.owner_txid,
            payload.collection.owner_nout
        );
        console.log("payload.collection:", payload.collection)
        console.debug('selectByOrdOutput collection:', collection);
      }
      if (payload?.collection?.genesis_txid || is_lazy) {
        flagsFundingOptions += flagsFundingOptions ? "," : "";
        flagsFundingOptions += "collection";
      }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
          myself.network,
          is_lazy ? myself.utxord.LAZY_INSCRIPTION : myself.utxord.INSCRIPTION
      );
      console.log('newOrd:', newOrd);

      if (is_lazy && !payload?.contract) {
        outData.errorMessage = await myself.sendExceptionMessage(CREATE_INSCRIPTION, "(Internal error) No contract provided");
        setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
        outData.raw = [];
        return outData;
      }
      const contract = payload?.contract || {
        "contract_type": "CreateInscription",
        "params": {
          "protocol_version": 8,
          "market_fee": {"amount": 0},
          "author_fee": {"amount": 0}
        }
      };
      const protocol_version = Number(contract?.params?.protocol_version);
      if (protocol_version) {
        const versions = await myself.getSupportedVersions(newOrd);
        if (versions.indexOf(protocol_version) === -1) {
          outData.errorMessage = messages.PLEASE_UPDATE_PLUGIN;
          outData.raw = [];
          return outData;
        }
      }
      newOrd.Deserialize(
          JSON.stringify(contract),
          is_lazy ? myself.utxord.LAZY_INSCRIPTION_MARKET_TERMS : myself.utxord.MARKET_TERMS
      );

      const dst_address = payload.inscription_destination_address || myself.wallet.ord.address;
      await newOrd.OrdOutput((myself.satToBtc(payload.expect_amount)).toFixed(8), dst_address);

      // For now it's just a support for title and description
      if (payload.metadata) {
        console.log('payload.metadata:', payload.metadata);
        await newOrd.MetaData(myself.arrayBufferToHex(cbor.encode(payload.metadata)));
      }

      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee_rate)).toFixed(8));  // payload.fee_rate as Sat/kB

      let collection_addr = null;
      if (payload?.collection?.genesis_txid) {
        // collection is empty no output has been found, see code above
        if (!collection) {
          myself.sendWarningMessage(
              'CREATE_INSCRIPTION',
              `Collection(txid:${payload.collection.owner_txid}, nout:${payload.collection.owner_nout}) is not found in balances`
          );
          setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
          // FIXME: l2xl response: it shouldn't work until SignCommit get executed
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.errorMessage = messages.COLLECTION_NOT_FOUND;
          outData.raw = [];
          return outData;
        }
        newOrd.AddCollectionUTXO(
            `${payload.collection.genesis_txid}i0`,  // inscription ID = <genesis_txid>i<envelope(inscription)_number>
            payload.collection.owner_txid,  // current collection utxo
            payload.collection.owner_nout,  // current collection nout
            (myself.satToBtc(collection.amount)).toFixed(8),  // amount from collection utxo
            payload.collection.btc_owner_address
        )
        collection_addr = payload.collection.btc_owner_address;
      }

      await newOrd.Data(payload.content_type, payload.content);

      await newOrd.InscribeScriptPubKey(myself.wallet.scrsk.key.PubKey());
      await newOrd.InscribeInternalPubKey(myself.wallet.intsk.key.PubKey());

      if (is_lazy) {
        await newOrd.FundMiningFeeInternalPubKey(myself.wallet.intsk2.key.PubKey());
        outData.used_wallets.add('intsk2');
      }


      await newOrd.ChangeAddress(myself.wallet.fund.address);
      outData.used_wallets.add('uns');

      const min_fund_amount = myself.btcToSat(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ));
      outData.amount = min_fund_amount;
      if (!myself.fundings.length && !estimate) {
        // TODO: REWORK FUNDS EXCEPTION
        // myself.sendExceptionMessage(
        //   'CREATE_INSCRIPTION',
        //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        // outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
        //     "wait for several conformations or wait update on the server.";
        // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
        // FIXME: l2xl response: it shouldn't work until SignCommit get executed
        // outData.raw = await myself.getRawTransactions(newOrd);
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        outData.raw = [];
        return outData;
      }
      const utxo_list = await myself.selectKeysByFunds(min_fund_amount, [], [], use_funds_in_queue);
      outData.utxo_list = utxo_list;
      const inputs_sum = await myself.sumAllFunds(utxo_list);
      outData.inputs_sum = inputs_sum;

      if (utxo_list?.length < 1 && !estimate) {
        // TODO: REWORK FUNDS EXCEPTION
        // this.sendExceptionMessage(
        //   'CREATE_INSCRIPTION',
        //   "There are no funds to create of the Inscription, please replenish the amount: "+
        //   `${min_fund_amount} sat`
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
        // FIXME: l2xl response: it shouldn't work until SignCommit get executed
        // outData.raw = await myself.getRawTransactions(newOrd);
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        outData.raw = [];
        return outData;
      }

      if (inputs_sum > Number(payload.expect_amount)) {
        flagsFundingOptions += flagsFundingOptions ? "," : "";
        flagsFundingOptions += "change";
      }

      console.log("min_fund_amount:", min_fund_amount);
      console.log("utxo_list:", [...utxo_list || []]);

      for (const fund of utxo_list) {
        await newOrd.AddUTXO(
            fund.txid,
            fund.nout,
            (myself.satToBtc(fund.amount)).toFixed(8),
            fund.address
        );
      }

      if (!estimate) {
        await newOrd.SignCommit(
            myself.wallet.root.key,
            'fund'
        );

        // get front root ord and select to address or pubkey
        // collection_utxo_key (root! image key) (current utxo key)
        if (payload?.collection?.genesis_txid) {
          await newOrd.SignCollection(
              myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
              'ord'
          );
        }
        outData.used_wallets.add('ord');

        await newOrd.SignInscription(
            myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
            'scrsk'
        );
        // TODO: unsure we need it
        // outData.used_wallets.add('scrsk');
      }
      const min_fund_amount_final_btc = Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ));

      console.log('min_fund_amount_final_btc:', min_fund_amount_final_btc);
      const min_fund_amount_final = await myself.btcToSat(min_fund_amount_final_btc);

      console.log('min_fund_amount_final:', min_fund_amount_final);
      outData.amount = min_fund_amount_final;

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final, [], [], use_funds_in_queue);
      outData.utxo_list = utxo_list_final;

      // TODO: core API InscriptionBuilder needs to have change-related stuff implemented
      // TODO: core API needs to be refactored to make change-related stuff more usable
      // const changeOutput = newOrd.ChangeOutput();
      // if (changeOutput.ptr != 0) {
      //   const changeDestination = changeOutput.Destination();
      //   if (changeDestination.ptr != 0) {
      //     outData.change_amount = myself.btcToSat(changeDestination.Amount()) || null;
      //     myself.destroy(changeDestination);
      //   }
      //   myself.destroy(changeOutput);
      // }
      // console.debug('change_amount: ', outData.change_amount);

      if (!estimate) {
        outData.data = await newOrd.Serialize(
            protocol_version,
            is_lazy ? myself.utxord.LAZY_INSCRIPTION_SIGNATURE : myself.utxord.INSCRIPTION_SIGNATURE
        );
        outData.raw = await myself.getRawTransactions(newOrd);
      }

      outData.total_mining_fee = myself.btcToSat(newOrd.GetTotalMiningFee("") || 0);

      if (!estimate) {
        //const sk = newOrd.GetIntermediateSecKey();
        //myself.wallet.root.key.AddKeyToCache(sk);
        // outData.sk = newOrd.GetIntermediateSecKey();  // TODO: use/create ticket for excluded sk (UT-???)
        // TODOO: remove sk before > 2 conformations
        // or wait and check utxo this translation on balances

        outData.outputs = {
          change: newOrd.ChangeOutput(),
        };
        if(!is_lazy){
          outData.outputs.collection= newOrd.CollectionOutput();
          outData.outputs.inscription= newOrd.InscriptionOutput();
        }
      }

      return outData;
    } catch (e) {
      outData.errorMessage = await myself.sendExceptionMessage(CREATE_INSCRIPTION, e);
      setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
      return outData;

      // if (! myself.KNOWN_CORE_ERRORS.some(errId => outData.errorMessage.indexOf(errId) !== -1)) return outData;

      // console.info('createInscriptionContract: call:theIndex:',theIndex);
      // theIndex++;
      // // if (theIndex > 1000) {
      // if (theIndex > 0) {
      //   const error = 'error loading wasm libraries, try reloading the extension or this page';
      //   console.error(error);
      //   await this.sendExceptionMessage(CREATE_INSCRIPTION, error);
      //   setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
      //   return outData;
      // }
      // return await myself.createInscriptionContract(payload, theIndex);
    }
  }

  //------------------------------------------------------------------------------

  async createInscriptionForChunk(payload_data) {
    const myself = this;
    console.log('createInscriptionForChunk payload: ', {...payload_data || {}});

    if(!payload_data?.costs?.data) return null;

    const result = {
      contract: JSON.parse(payload_data.costs.data),
      title: payload_data.title,
      description: payload_data?.description,
      type: payload_data?.type
    };

    // TODO: to debug this part when backend will ready for addresses support
    await myself.generateNewIndexes('ord, uns');
    await myself.genKeys();

    return result;
  }

  //------------------------------------------------------------------------------

  async sellSignContract(utxoData, ord_price, market_fee, contract, txid, nout) {
    const myself = this;
    try {
      const sellOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      const protocol_version = Number(contract?.params?.protocol_version);
      sellOrd.Deserialize(JSON.stringify(contract), myself.utxord.ORD_TERMS);

      sellOrd.OrdUTXO(
        txid,
        nout,
        (myself.satToBtc(utxoData.amount)).toFixed(8),
        utxoData.address
      );

      sellOrd.FundsPayoffAddress(myself.wallet.fund.address);
      sellOrd.SignOrdSwap(
          myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
          'ord'
      );
      const raw = await myself.getRawTransactions(sellOrd, myself.utxord.ORD_SWAP_SIG);
      return {
        raw: raw,
        contract_data: sellOrd.Serialize(protocol_version, myself.utxord.ORD_SWAP_SIG)
      };
    } catch (exception) {
      await this.sendExceptionMessage('SELL_SIGN_CONTRACT', exception)
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
        // raw: raws,
        contracts: contract_list,
      };

    } catch (exception) {
      await this.sendExceptionMessage('SELL_INSCRIPTION_CONTRACT', exception);
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

  async selectFundsByFlags(fundings = [], select_is_locked = false, select_in_queue = false) {
    let list = this.fundings;
    if (fundings.length > 0) {
      list = fundings;
    }
    return list.filter((item) => item.is_locked === select_is_locked && item.in_queue === select_in_queue);
  }
  //----------------------------------------------------------------------------

  async smartSelectFundsByAmount(
      target: number,
      new_input_fee: number,
      fundings = [],
      except_items = [],
      iterations_limit = 10
  ) {
    let utxo_list = await this.selectKeysByFunds(target, fundings);
    if (0 < utxo_list.length) {
      let utxo_summ = utxo_list.reduce((summ, utxo) => summ + utxo.amount, 0);
      let new_target = target + new_input_fee * utxo_list.length;

      while (utxo_summ < new_target && 0 < iterations_limit--) {
        const more_utxo_list = await this.selectKeysByFunds(target, fundings, utxo_list);
        if (0 == more_utxo_list.length) break;

        utxo_list = [...utxo_list, ...more_utxo_list];
        utxo_summ += more_utxo_list.reduce((summ, utxo) => summ + utxo.amount, 0);
        new_target += new_input_fee * more_utxo_list.length
      }
    }
    return utxo_list;
  }

  //----------------------------------------------------------------------------

  async selectKeysByFunds(target: number, fundings = [], except_items = [], use_funds_in_queue = false) {
    const addr_list = [];

    // use custom funds if provided, otherwise use default ones
    let all_funds_list = this.fundings;
    if (fundings.length > 0) {
      all_funds_list = fundings;
    }
    let selected_funds = [];

    // select non-excluded funds
    const excepts = this.exOutputs(except_items);
    console.log('excepts:', excepts);
    console.log('all_funds_list:', all_funds_list);
    for (const al of all_funds_list) {
      let aloutput = `${al.txid}:${al.nout}`;
      if (!excepts?.includes(aloutput) && !al.is_locked && (!al.in_queue || use_funds_in_queue)) {
        selected_funds.push({...al, output: aloutput});
      }
    }
    console.log('selected_funds:', selected_funds);

    // check total available amount
    const sum_funds = await this.sumAllFunds(selected_funds);
    if (sum_funds < target) {
      console.log(sum_funds, "sum<target", target);
      return [];
    }
    if (selected_funds?.length === 0) {
      console.log(selected_funds, "no funds");
      return [];
    }
    console.log("selectKeysByFunds->getAllFunds->selected_funds:", selected_funds);

    selected_funds = selected_funds.sort((a, b) => {
      if (a.amount > b.amount) return 1;
      if (a.amount < b.amount) return -1;
      return 0;
    });

    // Use single fund that is enough if possible
    for (const selected of selected_funds) {
      if (selected.amount >= target) {
        addr_list.push({...selected});
        return addr_list;
      }
    }

    // Collect funds for target amount
    let sum_addr = 0;
    console.log('selectKeysByFunds2->selected_funds:', selected_funds, 'target: ', target);
    for (const utxo of selected_funds) {
      console.log('utxo?.amount:', utxo?.amount, '|sum_addr:', sum_addr);
      sum_addr += utxo?.amount;
      addr_list.push({
        amount: utxo?.amount,
        nout: utxo?.nout,
        txid: utxo?.txid,
        path: utxo?.path,
        address: utxo?.address,
      });
      if (sum_addr >= target) {
        return addr_list;
      }
    }
    // It shouldn't happen due to sum_funds check above. However, let it be for a sake of style.
    return [];
  }

  async  commitBuyInscriptionContract(payload) {
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
      swapSim.Deserialize(JSON.stringify(payload.swap_ord_terms.contract), myself.utxord.FUNDS_TERMS);

      const min_fund_amount = await myself.btcToSat(swapSim.GetMinFundingAmount(""));
      const utxo_list = await myself.selectKeysByFunds(min_fund_amount + 682);
      const fund_sum = await myself.sumAllFunds(utxo_list);
      // console.log('fund_sum:',fund_sum,'|min_fund_amount:',min_fund_amount);
      // console.log('utxo_list:',utxo_list);

      if(utxo_list?.length < 1 || fund_sum < min_fund_amount) {
        // TODO: REWORK FUNDS EXCEPTION
        // myself.sendExceptionMessage(
        //   COMMIT_BUY_INSCRIPTION,
        //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        // outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
        //     "wait for several conformations or wait update on the server";
        outData.errorMessage = messages.INSUFFICIENT_FUNDS;
        outData.min_fund_amount = min_fund_amount;
        outData.mining_fee = Number(min_fund_amount) - Number(payload.market_fee) - Number(payload.ord_price);
        // outData.raw = await myself.getRawTransactions(swapSim, myself.utxord.FUNDS_TERMS);
        outData.raw = [];
        return outData;
      }

      const buyOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      const protocol_version = Number(payload.swap_ord_terms.contract?.params?.protocol_version);
      buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract), myself.utxord.FUNDS_TERMS);
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

      buyOrd.ChangeAddress(myself.wallet.fund.address);
      buyOrd.SwapScriptPubKeyB(myself.wallet.scrsk.key.PubKey()); //!!!
      buyOrd.SignFundsCommitment(
        myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
        'fund'
      );

      const dst_address = payload.inscription_destination_address || myself.wallet.ord.address;
      buyOrd.OrdPayoffAddress(dst_address);

      const min_fund_amount_final = myself.btcToSat(buyOrd.GetMinFundingAmount(""));

      outData.min_fund_amount = min_fund_amount_final || min_fund_amount;
      outData.mining_fee = Number(min_fund_amount_final) - Number(payload.market_fee) - Number(payload.ord_price);
      outData.utxo_list = utxo_list;
      outData.raw = await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_TERMS);
      const inputs_sum = await myself.sumAllFunds(utxo_list);
      // console.log('inputs_sum:',inputs_sum,'| outData.min_fund_amount:',outData.min_fund_amount)
      if (utxo_list?.length < 1 || inputs_sum < outData.min_fund_amount) {
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
      outData.data = buyOrd.Serialize(protocol_version, myself.utxord.FUNDS_COMMIT_SIG);
      return outData;
    } catch (exception) {
      const eout = await myself.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, exception)
      if (! myself.KNOWN_CORE_ERRORS.some(errId => eout.indexOf(errId) !== -1)) return;
      //
      // console.info(COMMIT_BUY_INSCRIPTION,'call:theIndex:',theIndex);
      // theIndex++;
      // if(theIndex>1000) {
      //   await this.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
      //   setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
      //   return outData;
      // }
      // return await myself.commitBuyInscriptionContract(payload, theIndex);
    }
  }

  //------------------------------------------------------------------------------

  async signSwapInscription(payload, tabId: number | undefined = undefined) {
    const myself = this;
    try {
      console.log('//////////// signSwapInscription ARGS', payload);
      console.log("!!!!contract:",JSON.stringify(payload?.swap_ord_terms?.contract));

      if (!payload?.swap_ord_terms){
        await this.sendExceptionMessage(BUY_INSCRIPTION, 'Undefined payload.swap_ord_terms');
        return;
      }
      const buyOrd = new myself.utxord.SwapInscriptionBuilder(myself.network);
      // console.log("aaaa");
      const protocol_version = Number(payload?.swap_ord_terms?.contract?.params?.protocol_version);
      buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract), myself.utxord.MARKET_PAYOFF_SIG);
      buyOrd.SignFundsSwap(
        myself.wallet.root.key,  // TODO: rename/move wallet.root.key to wallet.keyRegistry?
        'scrsk'
      );
      const raw = [];  // await myself.getRawTransactions(buyOrd, myself.utxord.FUNDS_SWAP_SIG);
      const data = buyOrd.Serialize(protocol_version, myself.utxord.FUNDS_SWAP_SIG);

      await (async (data, payload) => {
        console.log("SIGN_BUY_INSCRIBE_RESULT:", data);
        await myself.sendMessageToWebPage(
          SIGN_BUY_INSCRIBE_RESULT,
          { contract_uuid: payload?.swap_ord_terms?.contract_uuid, contract: JSON.parse(data) },
          tabId
        );
        await myself.generateNewIndexes('scrsk, ord');
        await myself.genKeys();
      })(data, payload);

    } catch (exception) {
      await this.sendExceptionMessage(BUY_INSCRIPTION, exception)
    }
  }

  encrypt (msg, password) {
    const keySize = 256;
    const ivSize = 128;
    const iterations = 100;

    const salt = CryptoJS.lib.WordArray.random(128/8);

    const key = CryptoJS.PBKDF2(password, salt, {
        keySize: keySize/32,
        iterations: iterations
      });

    const iv = CryptoJS.lib.WordArray.random(ivSize/8);

    const encrypted = CryptoJS.AES.encrypt(msg, key, {
      iv: iv,
      padding: CryptoJS.pad.Pkcs7,
      mode: CryptoJS.mode.CBC,
      hasher: CryptoJS.algo.SHA256
    });

    return salt.toString()+ iv.toString() + encrypted.toString();
  }

  decrypt (transitmessage, password) {
    const keySize = 256;
    const ivSize = 128;
    const iterations = 100;

    const salt = CryptoJS.enc.Hex.parse(transitmessage.substr(0, 32));
    const iv = CryptoJS.enc.Hex.parse(transitmessage.substr(32, 32))
    const encrypted = transitmessage.substring(64);

    const key = CryptoJS.PBKDF2(password, salt, {
        keySize: keySize/32,
        iterations: iterations
      });

    const decrypted = CryptoJS.AES.decrypt(encrypted, key, {
      iv: iv,
      padding: CryptoJS.pad.Pkcs7,
      mode: CryptoJS.mode.CBC,
      hasher: CryptoJS.algo.SHA256
    })
    return decrypted.toString(CryptoJS.enc.Utf8);
  }

  async encryptedWallet(password) {
    // loked wallet
    const isEnc = await this.isEncryptedWallet()
    if(isEnc) return true;
    this.wallet.secret = this.encrypt('secret', password);
/*
    const seed = this.encrypt(this.wallet.root.seed.toString('hex'));
    chrome.storage.local.set({ seed: seed });
    this.wallet.root.seed = seed;
*/
    this.wallet.encrypted = true;
    chrome.storage.local.set({encryptedWallet: true, secret: this.wallet.secret });
    return true;
  }

  async setUpPassword(password) {
    this.wallet.secret = await this.encrypt('secret', password);
    this.wallet.encrypted = true;
    await chrome.storage.local.set({encryptedWallet: true, secret: this.wallet.secret });
    return true;
  }

  async initPassword() {
    const {secret} = await chrome.storage.local.get(['secret'])
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
      await this.sendWarningMessage(
        DECRYPTED_WALLET,
        "The wallet is encrypted please enter the correct password to unlock the keys"
      );
      return false;
    }
    /*
    const seed = this.decrypt(this.wallet.root.seed.toString('hex'));
    chrome.storage.local.set({ seed: seed });
    this.wallet.root.seed = this.hexToString(seed);
    */
    this.wallet.encrypted = false;
    chrome.storage.local.set({ encryptedWallet: false });
    return true
  }

  async isEncryptedWallet() {
    const {encryptedWallet} = await chrome.storage.local.get(['encryptedWallet']);
    console.log("encryptedWallet: ",encryptedWallet);
    if(encryptedWallet===undefined) {
      chrome.storage.local.set({ encryptedWallet: this.wallet.encrypted });
    }
    return this.wallet.encrypted;
  }

  destroy(variable) {
    if(variable && variable?.ptr > 0) {
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
