import '~/libs/utxord.js';
import '~/libs/safe-buffer.js';
import * as CryptoJS from 'crypto-js';
import winHelpers from '~/helpers/winHelpers';
import rest from '~/background/rest';
import { sendMessage } from 'webext-bridge';
import * as cbor from 'cbor-js';
import * as Sentry from "@sentry/browser"
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
  TRANSFER_LAZY_COLLECTION
} from '~/config/events';
import { BASE_URL_PATTERN, NETWORK } from '~/config/index';
import Tab = chrome.tabs.Tab;
import {Exception} from "sass";

import * as WebBip39 from 'web-bip39';
import wordlist from 'web-bip39/wordlists/english';


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
        this.network = this.setUpNetWork(network);
        this.bech = new this.utxord.Bech32(this.network);

        this.locked = false;
        this.status = STATUS_DEFAULT;
        this.wallet = WALLET;
        this.wallet_types = WALLET_TYPES;
        this.addresses = [];
        this.all_addresses = [];

        this.balances = [];
        this.fundings = [];
        this.inscriptions = [];
        this.connect = false;
        this.sync = false;
        this.derivate = false;
        this.error_reporting = true;

        await this.init(this);
        return this;
      } catch(e) {
        Sentry.captureException(e);
        console.log('constructor->error:',e);
        chrome.runtime.reload();
      }
    })();
  }

  async init() {
    const myself = this

    try {
      const { seed } = await chrome.storage.local.get(['seed']);
      if (seed) {
        myself.wallet.root.seed = seed;
      }
      const { ext_keys } = await chrome.storage.local.get(['ext_keys']);
      if(ext_keys) {
        if(ext_keys.length > 0) {
           myself.resExtKeys(ext_keys);
        }
      }
      await myself.rememberIndexes();
      console.log('init...');
      myself.genRootKey();

      if (myself.checkSeed() && myself.utxord && myself.bech && this.wallet.root.key) {
        myself.genKeys();
        myself.initPassword();
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

  sentry(){
    const myself = this;
    if(this.error_reporting){
      Sentry.configureScope( async (scope) => {
        scope.setTag('system_state', myself.error_reporting)
        scope.setTag('network', NETWORK)
        if(myself.wallet.auth?.key) {
          scope.setUser({
            othKey: myself.wallet.oth?.key?.PubKey(),
            fundKey: myself.wallet.fund?.key?.PubKey(),
            authKey: myself.wallet.auth?.key?.PubKey()
          })
         }
        const check = await myself.checkSeed();
        const system_state = {
          check_auth: check,
          addresses: JSON.stringify(myself.addresses),
          filtred_addresses: JSON.stringify(myself.getAddressForSave()),
          balances: JSON.stringify(myself.balances),
          fundings: JSON.stringify(myself.fundings),
          inscriptions: JSON.stringify(myself.inscriptions),
          connect: myself.connect,
          sync: myself.sync,
          derivate: myself.derivate,
        }
        scope.setExtra('system',  system_state);

      });
    }
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

getKey(type: string, typeAddress: number | undefined = undefined){
    if (!this.wallet_types.includes(type)) return false;
    if (!this.checkSeed()) return false;
    this.genRootKey();
    const for_script = (type === 'uns' || type === 'intsk' || type === 'intsk2' || type === 'scrsk' || type === 'auth');
    if(!typeAddress) typeAddress = this.wallet[type].typeAddress
    return this.wallet.root.key.Derive(this.path(type, typeAddress), for_script);
  }

  getAddress(type: string, key: object | undefined = undefined, typeAddress: number | undefined = undefined){
    if(!key) key = this.wallet[type].key;
    if(!typeAddress) typeAddress = this.wallet[type].typeAddress;
    return typeAddress === 1
        ? key.GetP2WPKHAddress(this.network)
        : key.GetP2TRAddress(this.network);
  }

getChallenge(type: string, typeAddress: number | undefined = undefined ){
  const key = this.getKey(type, typeAddress);
  const challenge = this.challenge();
  const dhash = this.sha256x2(challenge);
  return {
    challenge: challenge,
    public_key: key.PubKey(),
    signature: key.SignSchnorr(dhash)
  };
}

updateAddressesChallenges(addresses: object[] | undefined = undefined){
 if(!addresses) addresses = this.addresses;
 for (let addr of addresses) {
   if(addr?.public_key){
     let ch = this.getChallenge(addr.type, addr.typeAddress);
     addr = Object.assign(addr, ch);
   }
 }
}

 getAddressForSave(addresses: object[] | undefined = undefined){
    const list = [];
    const pubKeylist = [];
    if(!addresses) addresses = this.addresses;
    this.updateAddressesChallenges(addresses);
    for(const item of addresses){
      //console.log('item?.address:',item?.address, this.hasAddress(item?.address, this.all_addresses), this.all_addresses)
      if(!this.hasAddress(item?.address, this.all_addresses)){
        if(item?.public_key && !pubKeylist.includes(item?.public_key)){
          pubKeylist.push(item?.public_key);
        }else{
           delete item.public_key;
           delete item.challenge;
           delete item.signature;
        }
        list.push(item);
      }
    }
    return list;
  }

  async setIndexToStorage(type, value) {
    if(type==='ext') return;
    const Obj = {};
    Obj[`${type}Index`] = Number(value);
    const check_index = await this.getIndexFromStorage(type);
    if(check_index) return;
    return setTimeout(() => {
      chrome.storage.local.set(Obj);
    }, 3000);

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

  path(type: string, typeAddress: number | undefined = undefined) {
    if (!this.wallet_types.includes(type)) return false;
    if (!this.checkSeed()) return false;
    if (type === 'ext') return false;
    //m / purpose' / coin_type' / account' / change / index
    if(!typeAddress) typeAddress = this.wallet[type].typeAddress
    const ignore = ['uns', 'intsk', 'intsk2', 'scrsk', 'auth'];
    const t = (this.network === this.utxord.MAINNET && type !== 'auth') ? 0 : this.wallet[type].coin_type;
    const a = (this.derivate || ignore.includes(type)) ? this.wallet[type].account : 0 ;
    const c = this.wallet[type].change;
    const i = (this.derivate || ignore.includes(type)) ? this.wallet[type].index : 0;
    const purpose = (typeAddress === 1) ? 84 : 86;
    return `m/${purpose}'/${t}'/${a}'/${c}/${i}`;
  }

  async setDerivate(value) {
    if (!this.checkSeed()) return false;
    this.derivate = Boolean(value);
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
    if(!this.derivate) return false;
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='ext') return false;
    this.wallet[type].index += 1;
    return await this.setIndexToStorage(type, this.wallet[type].index);
  }

  getIndex(type) {
    if(type==='ext') return 0;
    if (this.wallet[type]) return this.wallet[type].index;
    return 0;
  }

  setIndex(type, index) {
    if(type==='ext') return false;
    this.wallet[type].index = index;
    return true;
  }

  getNexIndex(type) {
    if(!this.derivate) return false;
    if(type==='ext') return 0;
    return this.wallet[type].index + 1;
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
    if (!addresses) {
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
    if (!addresses) {
      addresses = this.addresses;
    }
    for (const item of addresses) {
      if (item.address === address) {
        return true;
      }
    }
    return false;
  }

  hasAddressType(type: string, addresses: object[] | undefined = undefined) {
    if (!addresses) {
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
    if (!addresses) {
      addresses = this.addresses;
    }
    for (const item of addresses) {
      if (item.address === address && item.type === type && item.typeAddress === typeAddress) {
        return true;
      }
    }
    return false;
  }

  async restoreTypeIndexFromServer(type, addresses) {
    let store_index = 0;
    let wallet_index = 0;
    if (type !== 'ext') {
      store_index = Number(await this.getIndexFromStorage(type));
      wallet_index = Number(this.getIndex(type));
      if (store_index > wallet_index) {
        await this.setIndex(type, store_index)
      }
      if (store_index < wallet_index) {
        await this.setIndexToStorage(type, wallet_index);
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
      //console.log(`this.restoreTypeIndexFromServer(${type}, ${addresses})`)
      await this.restoreTypeIndexFromServer(type, addresses)
    }
    return await this.genKeys();
  }

  bytesToHexString(byteArray: Uint8Array) {
    return Array.from(byteArray, byte => {
      return ('0' + (byte & 0xFF).toString(16)).slice(-2);
    }).join('');
  }

  async generateMnemonic(length: number = 12){
    const strength = length / 3 * 32;
    const mnemonic = await WebBip39.generateMnemonic(wordlist, strength);
    return mnemonic;
  }

  async setUpSeed(mnemonic, passphrase = ''){
    const buffer_seed = await WebBip39.mnemonicToSeed(mnemonic, passphrase);
    const seed = this.bytesToHexString(buffer_seed);
    chrome.storage.local.set({ seed: seed });
    this.wallet.root.seed = seed;
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

    await chrome.storage.local.clear();
    chrome.runtime.reload()
    const lastErr = chrome.runtime.lastError;
    if (lastErr) {
      console.error(lastErr)
      return false
    } else {
      return true
    }
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
    return this.bech.Encode(publicKey)?.c_str();
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

  genKey(type: string, typeAddress: number | undefined = undefined) {
   if (!this.wallet_types.includes(type)) return false;
   if (!this.checkSeed()) return false;
   if(!typeAddress) typeAddress = this.wallet[type].typeAddress
   this.wallet[type].key = this.getKey(type, typeAddress);
   this.wallet[type].address = this.getAddress(type, this.wallet[type].key, typeAddress);
   return true;
 }

  addPopAddresses(){
    if (this.genKey('oth', 0)) {
      if (!this.hasAddressFields(this.wallet['oth'].address, 'oth', 0)) {
        let ch = this.getChallenge('oth', 0);
        this.addresses.push({ // add m86 fund address
          address: this.wallet['oth'].address,
          type: 'oth',
          typeAddress: 0,
          index: this.path('oth', 0),
          ...(!this.hasPublicKey(ch.public_key) ? ch : {})
        });
      }
    }

    if (this.genKey('oth', 1)) { // add m84 fund address
      if (!this.hasAddressFields(this.wallet['oth'].address, 'oth', 1)) {
        let ch = this.getChallenge('oth', 1);
        this.addresses.push({
          address: this.wallet['oth'].address,
          type: 'oth',
          typeAddress: 1,
          index: this.path('oth', 1),
          ...(!this.hasPublicKey(ch.public_key) ? ch : {})
        });
      }
    }

    if (this.genKey('fund', 1)) {
      if (!this.hasAddressFields(this.wallet['fund'].address,'fund', 1)) {
        let ch = this.getChallenge('fund', 1);
        this.addresses.push({ // add m84 fund address
          address: this.wallet['fund'].address,
          type: 'fund',
          typeAddress: 1,
          index: this.path('fund', 1),
        });
      }
    }

    if (this.genKey('fund', 0)) { // add m86 fund address
      if (!this.hasAddressFields(this.wallet['fund'].address,'fund', 0)) {
        let ch = this.getChallenge('fund', 0);
        this.addresses.push({
          address: this.wallet['fund'].address,
          type: 'fund',
          typeAddress: 0,
          index: this.path('fund', 0),
        });
      }
    }

    if (this.genKey('ord', 1)) {
      if (!this.hasAddressFields(this.wallet['ord'].address,'ord', 1)) {
        let ch = this.getChallenge('ord', 1);
        this.addresses.push({ // add m84 fund address
          address: this.wallet['ord'].address,
          type: 'ord',
          typeAddress: 1,
          index: this.path('ord', 1),
        });
      }
    }

    if (this.genKey('ord', 0)) { // add m86 fund address
      if (!this.hasAddressFields(this.wallet['ord'].address,'ord', 0)) {
        let ch = this.getChallenge('ord', 0);
        this.addresses.push({
          address: this.wallet['ord'].address,
          type: 'ord',
          typeAddress: 0,
          index: this.path('ord', 0),
        });
      }
    }

  }

  genKeys() { //current keys
    this.addPopAddresses(); // add popular addresses
    const publicKeys = [];
    for (const type of this.wallet_types) {
      if (this.genKey(type, this.wallet[type].typeAddress)) {
        if (type !== 'auth' && type !== 'ext') {
          if (!this.hasAddress(this.wallet[type].address)) {
              if (!this.hasAddressType(type)) {
                let ch = this.getChallenge(type);
                const newAddress = {
                  address: this.wallet[type].address,
                  type: type,
                  typeAddress: this.wallet[type].typeAddress,
                  index: this.path(type),
                  ...ch
                };
                console.debug(`genKeys(): push new "${type}" addresses:`, newAddress);
                this.addresses.push(newAddress);
                publicKeys.push({
                  pubKeyStr: this.wallet[type].key.PubKey(),
                  type: type,
                });
            }
          }
        }
      }
    }
    return {addresses: this.addresses, publicKeys};
  }

  async freeBalance(balance) {
    for (const item of balance) {
      this.destroy(item.key);
    }
    return [];
  }

  async updateBalancesFrom(msgType: string, addresses: []) {
    const balances = await this.prepareBalances(addresses);
    console.debug(`${msgType} balances:`, {...balances || {}});
    this.fundings = balances.funds;
    this.inscriptions = balances.inscriptions;
    return balances;
  }

  async prepareBalances(balances) {
    const myself = this;
    let list = this.balances?.addresses;
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

  getErrorMessage(exception: any) {
   if(typeof exception === 'number'){
     return this.utxord.Exception.prototype.getMessage(Number.parseInt(exception, 10))
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

  async sendWarningMessage(type?: string, warning: any) {
    let errorStack = '';
    warning = this.getErrorMessage(warning);
    const errorMessage = warning?.message || warning;
    if(warning?.name) type = warning?.name;
    if(warning?.stack) errorStack = warning?.stack;

    const currentWindow = await this.WinHelpers.getCurrentWindow()
    this.warning_count += 1;
    if(this.warning_count === 1){
      sendMessage(WARNING, errorMessage, `popup@${currentWindow.id}`);
      setTimeout(() =>this.warning_count = 0, 3000);
    }
    this.sentry();
    Sentry.captureException(warning);
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
    this.sentry();
    Sentry.captureException(exception);
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
    console.log('all_funds:',all_funds);

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

  pushChangeToFunds(change) {
    this.fundings.push({
      address: change?.address,
      txid: change?.txid,
      nout: change?.nout,
      amount: this.btcToSat(change?.amount || "0.0"),
      is_inscription: false,
      is_rune: false,
      is_locked: false,
      in_queue: true,
      path: "",
    });
  }

  pushOrdToInscriptions(ord) {
    this.inscriptions.push({
      address: ord?.address,
      txid: ord?.txid,
      nout: ord?.nout,
      amount: this.btcToSat(ord?.amount || "0.0"),
      is_inscription: true,
      is_rune: false,
      is_locked: false,
      in_queue: true,
      path: "",
    });
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
    if(!args){
      console.warn('sendMessageToWebPage-> error no args:', args,'type:', type);
      return null;
    }
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
      const signature = myself.wallet.auth.key.SignSchnorr(challenge);
      console.log("SignSchnorr::challengeResult:", signature);
      myself.sendMessageToWebPage(CONNECT_RESULT, {
        publickey: myself.wallet.auth.key.PubKey(),
        challenge: challenge,
        signature: signature
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
    let raw_size;
    if(!phase){
      raw_size = builderObject.TransactionCount();
    }else{
      raw_size = builderObject.TransactionCount(phase);
    }
    const raw = [];
    for(let i = 0; i < raw_size; i += 1) {
      if(phase!==undefined) {
        raw.push(builderObject.RawTransaction(phase, i)?.c_str())
      }else{
        raw.push(builderObject.RawTransaction(i)?.c_str())
      }
    }
    return raw;
  }

  //------------------------------------------------------------------------------

  async estimateInscription(payload) {
    return await this.createInscriptionContract({
      ...payload,
      content: "00".repeat(payload.content_length),
      content_length: undefined
    });
  }

  //------------------------------------------------------------------------------

  async transferForLazyInscriptionContract(payload) {
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

      const transferOutput = new myself.utxord.P2TR(
          this.network,
          myself.satToBtc(collection.amount).toFixed(8),
          payload.transfer_address
      );
      tx.AddOutput(transferOutput);
      myself.utxord.destroy(transferOutput);

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

      if (!myself.fundings.length) {
        outData.errorMessage = "Insufficient funds. Please add.";
        // outData.raw = [];
        return outData;
      }

      const input_mining_fee = myself.btcToSat(tx.GetNewInputMiningFee());
      const utxo_list = await myself.smartSelectFundsByAmount(min_fund_amount, input_mining_fee);
      outData.utxo_list = utxo_list;

      console.log("min_fund_amount:", min_fund_amount);
      console.log("utxo_list:", utxo_list);

      if (utxo_list?.length < 1) {
        outData.errorMessage = "Insufficient funds. Please add.";
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
      tx.Sign(myself.wallet.root.key, "fund");
      outData.raw = await myself.getRawTransactions(tx);

      // TODO: core API needs to be refactored to make change-related stuff more usable
      const changeOutput = tx.ChangeOutput();
      if (changeOutput.ptr != 0) {
        const changeDestination = changeOutput.Destination();
        if (changeDestination.ptr != 0) {
          outData.change_amount = myself.btcToSat(changeDestination.Amount().c_str()) || null;
          myself.destroy(changeDestination);
        }
        myself.destroy(changeOutput);
      }
      console.debug('change_amount: ', outData.change_amount);

      outData.data = tx.Serialize(SIMPLE_TX_PROTOCOL_VERSION, myself.utxord.TX_SIGNATURE);
      outData.total_mining_fee = myself.btcToSat(tx.GetTotalMiningFee(""));
      const contractData = JSON.parse(outData.data);
      console.debug('transferForLazyInscriptionContract: contractData:', contractData);
      myself.utxord.destroy(tx);

      return outData;
    } catch (e) {
      outData.errorMessage = await myself.sendExceptionMessage(TRANSFER_LAZY_COLLECTION, e);
      setTimeout(() => myself.WinHelpers.closeCurrentWindow(), closeWindowAfter);
      return outData;
    }
  }

  //------------------------------------------------------------------------------

  async createInscriptionContract(payload, use_funds_in_queue = false) {
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
        console.log("payload.collection:",payload.collection)
        console.debug('selectByOrdOutput collection:', collection);
      }
      if (payload?.collection?.genesis_txid || is_lazy) {
        flagsFundingOptions += flagsFundingOptions ? "," : "";
        flagsFundingOptions += "collection";
      }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.network,
        is_lazy ? myself.utxord.LASY_INSCRIPTION : myself.utxord.INSCRIPTION
      );
      console.log('newOrd:',newOrd);
      // TODO: we need to receive it from backend via frontend
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
        if(versions.indexOf(protocol_version) === -1) {
          outData.errorMessage = 'Please update the plugin to latest version.';
          outData.raw = [];
          return outData;
        }
      }
      newOrd.Deserialize(
          JSON.stringify(contract),
          is_lazy ? myself.utxord.LASY_INSCRIPTION_MARKET_TERMS : myself.utxord.MARKET_TERMS
      );
      newOrd.OrdAmount((myself.satToBtc(payload.expect_amount)).toFixed(8));

      // For now it's just a support for title and description
      if(payload.metadata) {
        console.log('payload.metadata:',payload.metadata);
        await newOrd.MetaData(myself.arrayBufferToHex(cbor.encode(payload.metadata)));
      }

      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee_rate)).toFixed(8));  // payload.fee_rate as Sat/kB

      let collection_addr = null;
      if (payload?.collection?.genesis_txid) {
        // collection is empty no output has been found, see code above
        if(!collection) {
          myself.sendWarningMessage(
            'CREATE_INSCRIPTION',
            `Collection(txid:${payload.collection.owner_txid}, nout:${payload.collection.owner_nout}) is not found in balances`
          );
          setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
          // FIXME: l2xl response: it shouldn't work until SignCommit get executed
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.errorMessage = "Collection is not found in balances.";
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
        collection_addr = payload.collection.btc_owner_address;
      }

      await newOrd.Data(payload.content_type, payload.content);

      await newOrd.InscribeScriptPubKey(myself.wallet.scrsk.key.PubKey());
      await newOrd.InscribeInternalPubKey(myself.wallet.intsk.key.PubKey());

      if (is_lazy) {
        await newOrd.FundMiningFeeInternalPubKey(myself.wallet.intsk2.key.PubKey());
        outData.used_wallets.add('intsk2');
      }

      const dst_address = payload.inscription_destination_address || myself.wallet.ord.address;
      await newOrd.InscribeAddress(dst_address);
      await newOrd.ChangeAddress(myself.wallet.fund.address);
      outData.used_wallets.add('uns');

      const min_fund_amount = myself.btcToSat(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      )?.c_str());
      outData.amount = min_fund_amount;
      if(!myself.fundings.length ) {
          // TODO: REWORK FUNDS EXCEPTION
          // myself.sendExceptionMessage(
          //   'CREATE_INSCRIPTION',
          //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
          // );
          // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
          // outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
          //     "wait for several conformations or wait update on the server.";
          outData.errorMessage = "Insufficient funds. Please add.";
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
          // FIXME: l2xl response: it shouldn't work until SignCommit get executed
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.raw = [];
          return outData;
      }
      const utxo_list = await myself.selectKeysByFunds(min_fund_amount, [], [], use_funds_in_queue);
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
          outData.errorMessage = "Insufficient funds. Please add.";
          // FIXME: it produces "ContractTermMissing: inscribe_script_pk" error in case there is no PK provided.
          // FIXME: l2xl response: it shouldn't work until SignCommit get executed
          // outData.raw = await myself.getRawTransactions(newOrd);
          outData.raw = [];
          return outData;
      }

      if(inputs_sum > Number(payload.expect_amount)) {
        flagsFundingOptions += flagsFundingOptions ? "," : "";
        flagsFundingOptions += "change";
      }

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

      await newOrd.SignCommit(
        myself.wallet.root.key,
        'fund'
      );

      // get front root ord and select to addres or pubkey
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

      const min_fund_amount_final_btc = Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      )?.c_str());

      console.log('min_fund_amount_final_btc:',min_fund_amount_final_btc);
      const min_fund_amount_final = await myself.btcToSat(min_fund_amount_final_btc);

      console.log('min_fund_amount_final:',min_fund_amount_final);
      outData.amount = min_fund_amount_final;

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final, [], [], use_funds_in_queue);
      outData.utxo_list = utxo_list_final;

      // TODO: core API InscriptionBuilder needs to have change-related stuff implemented
      // TODO: core API needs to be refactored to make change-related stuff more usable
      // const changeOutput = newOrd.ChangeOutput();
      // if (changeOutput.ptr != 0) {
      //   const changeDestination = changeOutput.Destination();
      //   if (changeDestination.ptr != 0) {
      //     outData.change_amount = myself.btcToSat(changeDestination.Amount().c_str()) || null;
      //     myself.destroy(changeDestination);
      //   }
      //   myself.destroy(changeOutput);
      // }
      // console.debug('change_amount: ', outData.change_amount);

      outData.data = await newOrd.Serialize(
          protocol_version,
          is_lazy ? myself.utxord.LASY_INSCRIPTION_SIGNATURE : myself.utxord.INSCRIPTION_SIGNATURE
      )?.c_str();
      outData.raw = await myself.getRawTransactions(newOrd);

      outData.total_mining_fee = myself.btcToSat(newOrd.GetTotalMiningFee("")?.c_str() || 0);

      const sk = newOrd.GetIntermediateSecKey()?.c_str();
      myself.wallet.root.key.AddKeyToCache(sk);
      // outData.sk = newOrd.GetIntermediateSecKey()?.c_str();  // TODO: use/create ticket for excluded sk (UT-???)
      // TODOO: remove sk before > 2 conformations
      // or wait and check utxo this translation on balances

      outData.outputs = {
        collection: is_lazy ? {} : {
          ...JSON.parse(newOrd.GetCollectionLocation()?.c_str() || "{}"),
          address: collection_addr
        },
        inscription: is_lazy ? {} : {
          ...JSON.parse(newOrd.GetInscriptionLocation()?.c_str() || "{}"),
          address: dst_address
        },
        change: {
          ...JSON.parse(newOrd.GetChangeLocation()?.c_str() || "{}"),
          address: myself.wallet.fund.address
        },
      };

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
    myself.genKeys();

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
        contract_data: sellOrd.Serialize(protocol_version, myself.utxord.ORD_SWAP_SIG)?.c_str()
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

      const min_fund_amount = await myself.btcToSat(swapSim.GetMinFundingAmount("")?.c_str());

      if(!myself.fundings.length ) {
        // TODO: REWORK FUNDS EXCEPTION
        // myself.sendExceptionMessage(
        //   COMMIT_BUY_INSCRIPTION,
        //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        // outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
        //     "wait for several conformations or wait update on the server";
        outData.errorMessage = "Insufficient funds. Please add.";
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

      const min_fund_amount_final = myself.btcToSat(buyOrd.GetMinFundingAmount("")?.c_str());
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
      outData.data = buyOrd.Serialize(protocol_version, myself.utxord.FUNDS_COMMIT_SIG)?.c_str();
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
      const data = buyOrd.Serialize(protocol_version, myself.utxord.FUNDS_SWAP_SIG)?.c_str();

      await (async (data, payload) => {
        console.log("SIGN_BUY_INSCRIBE_RESULT:", data);
        await myself.sendMessageToWebPage(
          SIGN_BUY_INSCRIBE_RESULT,
          { contract_uuid: payload?.swap_ord_terms?.contract_uuid, contract: JSON.parse(data) },
          tabId
        );
        await myself.generateNewIndexes('scrsk, ord');
        myself.genKeys();
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
      Sentry.captureException(e);
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
