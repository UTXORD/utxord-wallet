import '~/libs/utxord.js';
import * as bip39 from '~/libs/bip39.browser.js';
import '~/libs/safe-buffer.js';
import '~/libs/crypto-js.js';
import winHelpers from '~/helpers/winHelpers';
import rest from '~/background/rest';
import { sendMessage } from 'webext-bridge';
import {
  EXCEPTION,
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
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
  },
  fund: { // funding
    index: 0,
    change: 0,
    account: 1,
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
  },
  ord: { // ordinal
    index: 0,
    change: 0,
    account: 2,
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
  },
  uns: { // unspendable
    index: 0,
    change: 0,
    account: 3,
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
  },
  intsk:{ // internalSK
    index: 0,
    change: 0,
    account: 4,
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
  },
  scrsk: { //scriptSK
    index: 0,
    change: 0,
    account: 5,
    coin_type: 0,
    key: null,
    p2tr: null,
    pubKeyStr: null,
    privKeyStr: null,
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
    pubKeyStr: null,
    privKeyStr: null,
  }
};

const STATUS_DEFAULT = {
  initAccountData: false,
};

class Api {
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
      if(ext_keys){
        if(ext_keys.length > 0){
           myself.resExtKeys(ext_keys);
        }
      }
      const { xord_keys } = await chrome.storage.sync.get(['xord_keys']);
      if(xord_keys){
        if(xord_keys.length > 0){
           myself.resXordKeys(xord_keys);
        }
      }
      await myself.rememberIndexes();
      console.log('init...');
      if (myself.checkSeed() && myself.utxord && myself.bech) {
        myself.genRootKey();
        myself.genKeys();
        myself.initPassword();
        if (myself.wallet.fund.pubKeyStr && myself.wallet.auth.privKeyStr && myself.wallet.auth.pubKeyStr) {
          myself.status.initAccountData = true;
          return myself.status.initAccountData;
        }
      }
    } catch (e) {
      console.log('init()->error:', e);
    }
  }

  async setIndexToStorage(type, value){
    if(type==='xord' || type==='ext') return;
    const Obj = {};
    Obj[`${type}Index`] = Number(value);
    return await chrome.storage.sync.set(Obj);
  }

  async getIndexToStorage(type){
    if(type==='xord' || type==='ext') return;
    return (await chrome.storage.sync.get([`${type}Index`]))[`${type}Index`] || 0
  }

  async rememberIndexes(){
    for (const wt of this.wallet_types){
      this.wallet[wt].index = await this.getIndexToStorage(wt);
    }
    return true;
  }
  path(type){
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='xord' || type==='ext') return false;
    //m / purpose' / coin_type' / account' / change / index
    const t = this.wallet[type].coin_type;
    const a = this.wallet[type].account;
    const c = this.wallet[type].change;
    const i = this.wallet[type].index;
     return `m/86'/${t}'/${a}'/${c}/${i}`;
  }
  async generateNewIndex(type){
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    if(type==='xord' || type==='ext') return false;
    this.wallet[type].index += 1;
    return await this.setIndexToStorage(type, this.wallet[type].index);
  }
  getIndex(type){
    if(type==='xord' || type==='ext') return 0;
    if (this.wallet[type]) return this.wallet[type].index;
    return 0;
  }
  setIndex(type, index){
    if(type==='xord' || type==='ext') return false;
    this.wallet[type].index = index;
    return true;
  }
  getNexIndex(type){
    if(type==='xord' || type==='ext') return 0;
    return this.wallet[type].index + 1;
  }

  checkAddresess(ext_addresses = []){
    if(ext_addresses.length === 0) return false;
    if(this.addresses.length === 0) return false;

    const list = [];
    for(const item of ext_addresses){
      list.push(item.address);
    }
    for(const value of this.addresses){
      if(list.indexOf(value.address) === -1){return false;}
    }
    return true;
  }

  checkAddress(address, addresses){
    if(!addresses){addresses = this.addresses;}
    for(const item of addresses){
      if(item.address === address){return true;}
    }
    return false;
  }

  checkAddressType(type, addresses){
    if(!addresses){addresses = this.addresses;}
    for(const item of addresses){
      if(item.type === type){return true;}
    }
    return false;
  }


  async restoreTypeIndexFromServer(type, addresses){
    let sindex = 0;
    let store_index = 0;
    let wallet_index = 0;
    if(type!=='xord' && type!=='ext'){
      sindex = await this.getIndexToStorage(type);
      store_index = Number(sindex);
      wallet_index = Number(this.getIndex(type));
      if(store_index > wallet_index) {
        await this.setIndex(type, store_index)
      }
      if(store_index < wallet_index){
        await this.setIndexToStorage(type, wallet_index);
      }
    }
    if(addresses){
      for(const v of addresses){
        if(v.type===type && type!=='xord' && type!=='ext'){
          const currind = Number(v.index.split('/').pop())
          if (currind > wallet_index){
          //  console.log(`currind for type - ${type}:`,currind)
            await this.setIndex(type, currind)
            await this.setIndexToStorage(type, currind);
          }
        }else{
          if(type==='xord'){
            let p = v?.index?.split('/')[0];
            let xord = v?.index?.split('/')[1];
            this.addToXordPubKey(xord);
          }
          if(type==='ext'){
            let p = v?.index?.split('/')[0];
            let ext = v?.index?.split('/')[1];
            this.addToExternalKey(ext);
          }
        }

      }
    }
  }
  async restoreAllTypeIndexes(addresses){
    for (const key of this.wallet_types){
      //console.log(`this.restoreTypeIndexFromServer(${key}, ${addresses})`)
      await this.restoreTypeIndexFromServer(key, addresses)
    }
    return await this.genKeys();
  }
  /*
  getKey(type, index){

  }
  getNewKey(type){

  }
*/

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
  genRootKey(){
    if(!this.checkSeed()) return false;
    if (this.wallet.root.key) return this.wallet.root.key;
    this.wallet.root.key = new this.utxord.MasterKey(this.getSeed());
    return this.wallet.root.key;
  }
  resXordKeys(xord_keys){
    this.wallet.xord = [];
    for(const item of xord_keys){
      this.addToXordPubKey(item);
    }
    return this.wallet.xord;
  }

  pubKeyStrToP2tr(publicKey){
    return this.bech.Encode(publicKey).c_str();
  }

  addToXordPubKey(xordPubkey){
    const myself = this;
    const tmpAddress = myself.pubKeyStrToP2tr(xordPubkey);
    if(!myself.checkAddress(tmpAddress, myself.wallet.xord)){
      myself.wallet.xord.push({
        p2tr: tmpAddress,
        pubKeyStr: xordPubkey,
        index: `0/${xordPubkey}`
      });
      const xord_keys = []
      for(const item of this.wallet.xord){
        xord_keys.push(item.pubKeyStr);
      }
      chrome.storage.sync.set({ xord_keys: xord_keys});
    }
    return true;
  }

  addToExternalKey(keyhex, pass){
    const myself = this;

    const keypair = new myself.utxord.ChannelKeys(keyhex);
    const tmpAddress = this.bech.Encode(keypair.GetLocalPubKey().c_str()).c_str();
    if(!myself.checkAddress(tmpAddress, this.wallet.ext.keys)){
      let enkeyhex = keyhex;
      let enFlag = false;
      if(pass){
        enkeyhex = this.encrypt(keyhex, pass);
        enFlag = true;
      }
      console.log('ExternalKeyAddress:',tmpAddress);
      this.wallet.ext.keys.push({
          key: keypair,
          p2tr: tmpAddress,
          pubKeyStr: keypair.GetLocalPubKey().c_str(),
          privKeyStr: enkeyhex,
          index:`${Number(enFlag)}/${enkeyhex}`,
          type: 'ext',
        });
        const ext_keys = []
        for(const item of this.wallet.ext.keys){
          ext_keys.push(item.privKeyStr);
        }
      chrome.storage.sync.set({ ext_keys: ext_keys});
  }
    return true;
  }

  resExtKeys(ext_keys){
    this.wallet.ext.keys = [];
    for(const item of ext_keys){
      this.addToExternalKey(item);
    }
    return this.wallet.ext.keys;
  }

  genKey(type){
    if(!this.wallet_types.includes(type)) return false;
    if(!this.checkSeed()) return false;
    this.genRootKey();
    const for_script = (type === 'uns' || type === 'intsk' || type === 'scrsk' || type === 'auth');
    this.wallet[type].key = this.wallet.root.key.Derive(this.path(type), for_script);
    this.wallet[type].pubKeyStr = this.wallet[type].key.GetLocalPubKey().c_str();
    this.wallet[type].privKeyStr = this.wallet[type].key.GetLocalPrivKey().c_str();
    const address = this.bech.Encode(this.wallet[type].pubKeyStr).c_str();
    this.wallet[type].p2tr = address;
     return true;
  }

  genKeys() { //current keys
    const publicKeys = [];
    for(const key of this.wallet_types) {
      if(this.genKey(key)){
        if(key!=='auth' && key!=='xord' && key!=='ext'){
          if(!this.checkAddress(this.wallet[key].p2tr)){
            if(!this.checkAddressType(key)){
              this.addresses.push({
                address: this.wallet[key].p2tr,
                type: key,
                index: this.path(key)
              });
            }else{
              for(const i in this.addresses){
                if(this.addresses[i].type === key){
                  this.addresses[i] = {
                    address: this.wallet[key].p2tr,
                    type: key,
                    index: this.path(key)
                  };
                }
              }
            }
            publicKeys.push({pubKeyStr: this.wallet[key].pubKeyStr, type: key});
          }
        }
      }
    }
    //add ExternalKeyAddress
    //add XordPubKey
    for(const item of this.wallet.xord){
      if(!this.checkAddress(item.p2tr)){
        this.addresses.push({
          address: item.p2tr,
          type: 'xord',
          index: item.index
        });
        publicKeys.push({pubKeyStr: item.pubKeyStr, type: 'xord'});
      }
    }

    return { addresses: this.addresses, publicKeys };
  }

async getBranchKey(path, item){
  try{
    if(path[0]==='m'){
      const for_script = (item?.type === 'uns' || item?.type === 'intsk' || item?.type === 'scrsk' || item?.type === 'auth');
      const keypair = this.wallet.root.key.Derive(path, for_script);
      const address = this.bech.Encode(keypair.GetLocalPubKey().c_str()).c_str();
      return {address: address, key: keypair};
    }else{
      if(item?.type==='ext' || item?.type==='xord'){
        let pubkey='';
        if(item?.type === 'xord'){ pubkey = item.index.split('/')[1]; }
        return {address: item.address, key:{
          pubKeyStr: pubkey
        }};
      }
    }
  }catch(e){
    console.log("getBranchKey->error:",e);
  }
}

async genAllBranchKeys(type, deep = 0){
    await this.genRootKey();
    const addresses = [];
    const keys = [];
    if(deep < this.wallet[type].index){
      deep = this.wallet[type].index;
    }
    for(let index = deep; index >= 0; index--){
    let path = `m/86'/${this.wallet[type].coin_type}'/${this.wallet[type].account}'/${this.wallet[type].change}/${index}`;
    let for_script = (type === 'uns' || type === 'intsk' || type === 'scrsk' || type === 'auth');
    let keypair = this.wallet.root.key.Derive(path, for_script);
    let address = this.bech.Encode(keypair.GetLocalPubKey().c_str()).c_str();
    addresses.push(address);
    path = `m/86'/${this.wallet[type].account}'/${this.wallet[type].coin_type}'/${this.wallet[type].change}/${index}`;
    for_script = (type === 'uns' || type === 'intsk' || type === 'scrsk' || type === 'auth');
    keypair = this.wallet.root.key.Derive(path, for_script);
    address = this.bech.Encode(keypair.GetLocalPubKey().c_str()).c_str();
    addresses.push(address);
    keys.push(keypair);
      }
      return {addresses: addresses, keys: keys};
  }


  async genAllKeys(){
    const list = []
    for(const key of this.wallet_types){
      if(key !== 'auth'){
        let br = await this.genAllBranchKeys(key);
        list.push(br);
      }
    }
    return list;
  }

  async getAllAddresses(addresses){
    const myself = this;
    const ret = [];
    if(addresses?.length){
      for(const item of addresses){
        let br = await myself.getBranchKey(item?.index, item);
        if(br?.address !== item?.address){
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

  async getInscriptions(inscriptions, balances){
    const myself = this;
    let list = this.balances?.addresses;
    let my = this.inscriptions;
    if(inscriptions){
      my = inscriptions;
    }
    if(balances){
      list = balances;
    }
    const ret = [];
    for(const item of my){
      for (const bal of list) {
        for (const i of bal?.utxo_set || []) {
          if (i?.is_inscription && Number(i?.nout) === Number(item?.owner_nout) && i?.txid == item?.owner_txid) {
            let br = await myself.getBranchKey(bal.index, bal);
            if(br?.address !== bal?.address){
              console.log("getInscriptions->1|address:",br?.address,"|bal.address:",bal?.address);
            }else{
              ret.push({
                ...i,
                address: bal?.address,
                path: bal?.index,
                owner_nout: item?.owner_nout,
                owner_txid: item?.owner_txid,
                genesis_nout: item?.genesis_nout,
                genesis_txid: item?.genesis_txid,
                btc_owner_address: item?.btc_owner_address,
                btc_creator_address: item?.btc_creator_address,
                key: br.key,
              });
            }
          }
        }
      }
    }
    return ret;
  }

 async getAllFunds(balances){
   const myself = this;
   let list = this.balances?.addresses;
   if(balances){
     list = balances;
   }
   const ret = [];
   if(list?.length){
     for (const item of list) {
       for (const i of item?.utxo_set || []) {
         if (!i?.is_inscription) {
           let br = await myself.getBranchKey(item.index, item);
           if(br?.address !== item?.address){
             console.log("skip: getAllFunds->1|address:", br?.address,"|item.address:", item?.address);
           }else{
             ret.push({
               ...i,
               address: item.address,
               path: item.index,
               key: br.key,
             });
           }
         }
       }
     }
   }
  return ret;
  }

  async sumAllFunds(all_funds){
    if(!all_funds) return 0;
    console.log('sumAllFunds:',all_funds)
    return all_funds?.reduce((a,b)=>a+b?.amount, 0);
  }

  async sumMyInscribtions(inscriptions){
    if(!inscriptions) return 0;
    console.log('sumMyInscribtions:',inscriptions)
    return inscriptions?.reduce((a,b)=>a+b?.amount, 0);
  }


async selectKeyByFundAddress(address, fundings){
    const myself = this;
    let list = myself.fundings;
    if(list){
      list = fundings
    }
    if(list){
      for(const item of list){
        if(address === item.address){
          return item.key;
        }
      }
    }
  }

  selectKeyByOrdAddress(address, inscriptions = []){
    const myself = this;
    let list = myself.inscriptions;
    if(inscriptions.length > 0){
      list = inscriptions;
    }
    if(list){
      for(const item of list){
        if(address === item.address){
          return item.key;
        }
      }
    }
  }

  selectByFundAddress(address, fundings){
    const myself = this;
    let list = myself.fundings;
    if(list){
      list = fundings
    }
    if(list){
      for(const item of list){
        if(address === item.address){
          return item;
        }
      }
    }
  }

  selectByOrdAddress(address, inscriptions = []){
    const myself = this;
    let list = myself.inscriptions;
    if(list){
      list = inscriptions
    }
    for(const item of list){
      if(address === item.address){
        return item;
      }
    }
  }

  async selectKeyByFundOutput(txid, nout, fundings = []){
    const myself = this;
    let list = myself.fundings;
    if(fundings.length > 0){
      list = fundings
    }
    if(list){
    for(const item of list){
      if(txid === item.txid && nout === item.nout){
        return item.key;
      }
    }
  }
  }

  async selectKeyByOrdOutput(txid, nout, inscriptions = []){
    const myself = this;
    let list = myself.inscriptions;
    if(inscriptions.length > 0){
      list = inscriptions
    }
    for(const item of list){
      if(txid === item.txid && nout === item.nout){
        return item.key;
      }
    }
      return;
  }

  async selectByFundOutput(txid, nout, fundings = []){
    const myself = this;
    let list = myself.fundings;
    if(fundings.length > 0){
      list = fundings;
    }
    if(list){
      for(const item of list){
        if(txid === item.txid && nout === item.nout){
          return item;
        }
      }
    }
  }

selectByOrdOutput(txid, nout, inscriptions = []){
    const myself = this;
    let list = myself.inscriptions;
    console.log('list:',list)
    if(inscriptions.length > 0){
      list = inscriptions;
    }
    console.log('list2:',list)
    if(list){
      for(const item of list){
        if(txid === item.txid && Number(nout) === Number(item.nout)){
          console.log(txid,'txid === item.txid',item.txid)
          console.log(nout,'nout === item.nout',item.nout)
          return item;
        }
      }
    }
  }

async matchTapRootKey(payload, target, deep = 0){
  if(!payload?.collection?.genesis_txid) return;
   await this.genRootKey();
   if(deep < this.wallet['intsk'].index){
     deep = this.wallet['intsk'].index;
   }
   for(let index = deep; index >= 0; index--){
   let path_intsk = `m/86'/${this.wallet['intsk'].coin_type}'/${this.wallet['intsk'].account}'/${this.wallet['intsk'].change}/${index}`;
   let path_scrsk = `m/86'/${this.wallet['scrsk'].coin_type}'/${this.wallet['scrsk'].account}'/${this.wallet['scrsk'].change}/${index}`;

   let keypair_intsk = this.wallet.root.key.Derive(path_intsk, true);
   let keypair_scrsk = this.wallet.root.key.Derive(path_scrsk, true);

   let taprootkey = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
      `${payload.collection.genesis_txid}i0`,
      keypair_scrsk.GetLocalPubKey().c_str(),
      keypair_intsk.GetLocalPubKey().c_str()
    );
    console.log('taprootkey:',taprootkey.c_str(),'|target',target);
    if(taprootkey.c_str() === target){
      return {scrsk: keypair_scrsk, intsk: keypair_intsk};
    }

}

 }

  getCurrentNetWorkLabel(){
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

  getErrorMessage(exception: number) {
    return this.utxord.Exception.prototype.getMessage(exception).c_str()
  }

  async sendExceptionMessage(type?: string, exception: any) {
    let errorMessage;
    let errorStack;
    if(typeof exception === 'number'){
      errorMessage = this.getErrorMessage(Number.parseInt(exception, 10));

    }else{
      errorMessage = exception;
      if(exception?.message){
        errorMessage = exception?.message;
        type = exception?.name;
        errorStack = exception?.stack;
      }
    }
    if(errorMessage.indexOf('Aborted(OOM)')!==-1){
      console.log('wasm->load::Error{',errorMessage,'}');
      return errorMessage;
    }
    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(EXCEPTION, errorMessage, `popup@${currentWindow.id}`)
    console.log(type, errorMessage, errorStack);
    return type+errorMessage+errorStack;
  }
  async fetchAddress(address: string){
    const response = await this.Rest.get(`/api/address/${address}/balance/`);
    return response;
  }
  async fetchExternalAddresses(){
    if(this.wallet.ext.keys.length<1) return;

    console.log('this.wallet.ext.keys:',this.wallet.ext.keys);
    for(const item of this.wallet.ext.keys){
      if(item.p2tr){
        let response = await this.fetchAddress(item.p2tr);
        if(response?.data){
          item.balance =  response.data?.confirmed || response.data?.unconfirmed;
          console.log('ExternalAddress:',item.p2tr,'|',item.balance);
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

    // console.log(`----- sendMessageToWebPage: there are ${tabs.length} tabs found`);
    for (let tab of tabs) {
      // if (tab?.url?.startsWith('chrome://') || tab?.url?.startsWith('chrome://new-tab-page/')) {
      //   setTimeout(async () => await myself.sendMessageToWebPage(type, args), 100);
      //   return;
      // }
      if(tab?.id){
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

  signToChallenge(challenge, tabId: number | undefined = undefined) {
    const myself = this;
    if (myself.wallet.auth.privKeyStr) {
      const keypair = new myself.utxord.ChannelKeys(myself.wallet.auth.privKeyStr);
      const signature = keypair.SignSchnorr(challenge).c_str();
      console.log("SignSchnorr::challengeResult:", signature);
      myself.sendMessageToWebPage(CONNECT_RESULT, {
        challenge: challenge,
        signature: signature,
        publickey: myself.wallet.auth.pubKeyStr
      }, tabId);
    myself.destroy(keypair);
    myself.connect = true;
    myself.sync = false;
    } else {
      myself.sendMessageToWebPage(CONNECT_RESULT, null, tabId);
    }
  }
//----------------------------------------------------------------------------
/*
async createIndependentInscriptionContract(payload, theIndex = 0) {
  const myself = this;
  try {
    console.log('createInscription payload: ', payload);
      if(!myself.fundings.length ){
        // TODO: REWORK FUNDS EXCEPTION
        myself.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
     }



    const ordSim = new myself.utxord.CreateInscriptionBuilder(
      myself.utxord.INSCRIPTION,
      (myself.satToBtc(payload.expect_amount)).toFixed(8)
    );
    await ordSim.MiningFeeRate(myself.satToBtc(payload.fee).toFixed(8));
    let flagsFundingOptions = "";

    const destination_keypair = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);
    const change_keypair = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);
    // const scriptSK = new myself.utxord.ChannelKeys(myself.wallet.scrsk.privKeyStr);
    // const internalSK = new myself.utxord.ChannelKeys(myself.wallet.intsk.privKeyStr);

    await ordSim.Data(payload.content_type, payload.content);


    const min_fund_amount = await myself.btcToSat(Number(ordSim.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str()))


    const utxo_list = await myself.selectKeysByFunds(min_fund_amount);

    const inputs_sum = await myself.sumAllFunds(utxo_list);

    if(inputs_sum > Number(payload.expect_amount)){
      flagsFundingOptions += "change";
    }
    const extra_amount = myself.btcToSat(Number(ordSim.GetGenesisTxMiningFee().c_str()));
    console.log("extra_amount:",extra_amount);
    console.log("min_fund_amount:",min_fund_amount);
    console.log("utxo_list:",utxo_list);

    if(utxo_list?.length < 1){
        // TODO: REWORK FUNDS EXCEPTION
        this.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "There are no funds to create of the Inscription, please replenish the amount: "+
          `${min_fund_amount} sat`
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
    }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.utxord.INSCRIPTION,
        (myself.satToBtc(payload.expect_amount)).toFixed(8)
      );
      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee)).toFixed(8));
      await newOrd.Data(payload.content_type, payload.content);
      await newOrd.InscribePubKey(destination_keypair.GetLocalPubKey().c_str());
      await newOrd.ChangePubKey(change_keypair.GetLocalPubKey().c_str());

      for(const fund of utxo_list){
        await newOrd.AddUTXO (
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.key.GetLocalPubKey().c_str()
        );
      }

      //generate new keypair script_key
      const script_key = new myself.utxord.ChannelKeys(myself.wallet.uns.privKeyStr);

      for(const id in utxo_list){
        await newOrd.SignCommit(
          id,
          utxo_list[id].key.GetLocalPrivKey().c_str(),
          script_key.GetLocalPubKey().c_str()
        );
      }

      await newOrd.SignInscription(
        script_key.GetLocalPrivKey().c_str()
      )
      const min_fund_amount_final = await myself.btcToSat(Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str()))

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      const data = newOrd.Serialize().c_str();
      const sk = newOrd.getIntermediateTaprootSK().c_str();
      const inscrId = newOrd.MakeInscriptionId().c_str();

      // console.log('inscrId:',inscrId);
      // const xord = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
      //   inscrId,
      //   collectionScriptSK.GetLocalPubKey().c_str(),
      //   collectionIntSK.GetLocalPubKey().c_str()
      //   ).c_str();
      // console.log('xord:',xord);

      myself.destroy(destination_keypair);
      myself.destroy(change_keypair);
      myself.destroy(script_key);
      return {
        xord: null,
        nxord: null,
        xord_address: null,
        data,
        sk,
        amount: min_fund_amount_final,
        inputs_sum: inputs_sum,
        utxo_list: utxo_list_final,
        expect_amount: Number(payload.expect_amount),
        extra_amount: extra_amount,
        fee_rate: payload.fee,
        size: (payload.content.length+payload.content_type.length)

      };
    } catch (exception){
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000){
        this.sendExceptionMessage(CREATE_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
      }
      return await myself.createIndependentInscriptionContract(payload, theIndex);

    }
} //
*/
//------------------------------------------------------------------------------
/*
async createIndependentCollectionContract(payload, theIndex = 0) {
  const myself = this;
  try {
    console.log('createInscription payload: ', payload);
      if(!myself.fundings.length ){
        // TODO: REWORK FUNDS EXCEPTION
        myself.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
     }

    const ordSim = new myself.utxord.CreateInscriptionBuilder(
      myself.utxord.COLLECTION,
      (myself.satToBtc(payload.expect_amount)).toFixed(8)
    );
    await ordSim.MiningFeeRate(myself.satToBtc(payload.fee).toFixed(8));
    let flagsFundingOptions = "";

    const destination_keypair = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);
    const change_keypair = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);
    const collectionScriptSK = new myself.utxord.ChannelKeys(myself.wallet.scrsk.privKeyStr);
    const collectionIntSK = new myself.utxord.ChannelKeys(myself.wallet.intsk.privKeyStr);

    await ordSim.Data(payload.content_type, payload.content);


    const min_fund_amount = await myself.btcToSat(Number(ordSim.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str()));
    console.log("min_fund_amount_btc:",ordSim.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str());

    const utxo_list = await myself.selectKeysByFunds(min_fund_amount);

    const inputs_sum = await myself.sumAllFunds(utxo_list);

    if(inputs_sum > Number(payload.expect_amount)){
      flagsFundingOptions += "change";
    }
    const extra_amount = myself.btcToSat(Number(ordSim.GetGenesisTxMiningFee().c_str()));
    console.log("extra_amount:",extra_amount);
    console.log("min_fund_amount:",min_fund_amount);
    console.log("utxo_list:",utxo_list);

    if(utxo_list?.length < 1){
        // TODO: REWORK FUNDS EXCEPTION
        this.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "There are no funds to create of the Inscription, please replenish the amount: "+
          `${min_fund_amount} sat`
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
    }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.utxord.COLLECTION,
        (myself.satToBtc(payload.expect_amount)).toFixed(8)
      );
      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee)).toFixed(8));
      await newOrd.Data(payload.content_type, payload.content);
      await newOrd.InscribePubKey(destination_keypair.GetLocalPubKey().c_str());
      await newOrd.CollectionCommitPubKeys(
        collectionScriptSK.GetLocalPubKey().c_str(),
        collectionIntSK.GetLocalPubKey().c_str()
      )

      await newOrd.ChangePubKey(change_keypair.GetLocalPubKey().c_str());

      for(const fund of utxo_list){
        await newOrd.AddUTXO (
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.key.GetLocalPubKey().c_str()
        );
      }

      //generate new keypair script_key
      const script_key = new myself.utxord.ChannelKeys(myself.wallet.uns.privKeyStr);

      for(const id in utxo_list){
        await newOrd.SignCommit(
          id,
          utxo_list[id].key.GetLocalPrivKey().c_str(),
          script_key.GetLocalPubKey().c_str()
        );
      }

      await newOrd.SignInscription(
        script_key.GetLocalPrivKey().c_str()
      );
      await newOrd.SignCollectionRootCommit(
        destination_keypair.GetLocalPrivKey().c_str()
      );
      const min_fund_amount_final = await myself.btcToSat(Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str()))
      console.log("min_fund_amount_final_btc:",newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str());

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      const data = newOrd.Serialize().c_str();
      const sk = newOrd.getIntermediateTaprootSK().c_str();

      const inscrId = newOrd.MakeInscriptionId().c_str();
      console.log('inscrId:',inscrId);
      console.log('collectionScriptSK.GetLocalPubKey().c_str():',collectionScriptSK.GetLocalPubKey().c_str());
      console.log('collectionIntSK.GetLocalPubKey().c_str():',collectionIntSK.GetLocalPubKey().c_str());
      const xord = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
        inscrId,
        collectionScriptSK.GetLocalPubKey().c_str(),
        collectionIntSK.GetLocalPubKey().c_str()
        ).c_str();
        console.log('xord:',xord);
        console.log("min_fund_amount_final_btc_2:",newOrd.GetMinFundingAmount(
            `${flagsFundingOptions}`
        ).c_str());
        myself.destroy(destination_keypair);
        myself.destroy(change_keypair);
        myself.destroy(collectionScriptSK);
        myself.destroy(collectionIntSK);
        myself.destroy(script_key);
      return {
        xord,
        nxord: null,
        xord_address: myself.pubKeyStrToP2tr(xord),
        data,
        sk,
        amount: min_fund_amount_final,
        inputs_sum: inputs_sum,
        utxo_list: utxo_list_final,
        expect_amount: Number(payload.expect_amount),
        extra_amount: extra_amount,
        fee_rate: payload.fee,
        size: (payload.content.length+payload.content_type.length)

      };
    } catch (exception){
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000){
        this.sendExceptionMessage(CREATE_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
      }
      return await myself.createIndependentCollectionContract(payload, theIndex);

    }
} // √
*/
//------------------------------------------------------------------------------
/*
async createInscriptionInCollectionContract(payload, theIndex = 0) {
  const myself = this;
  try {
    console.log('createInscription payload: ', payload);
      if(!myself.fundings.length ){
        // TODO: REWORK FUNDS EXCEPTION
        myself.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
     }

    const ordSim = new myself.utxord.CreateInscriptionBuilder(
      myself.utxord.INSCRIPTION,
      (myself.satToBtc(payload.expect_amount)).toFixed(8)
    );
    await ordSim.MiningFeeRate(myself.satToBtc(payload.fee).toFixed(8));
    let flagsFundingOptions = "collection";

    const destination_keypair = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);
    const change_keypair = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);


    let collection = myself.selectByOrdOutput(
         payload.collection.owner_txid,
         payload.collection.owner_nout
     );
     const tapkeys = await myself.matchTapRootKey(payload, collection.key.pubKeyStr);
      console.log("payload.collection:",payload.collection);
      console.log('AddToCollectionSim:', collection);

    const collectionScriptSK = new myself.utxord.ChannelKeys(tapkeys.scrsk.privateKey.toString('hex'));
    const collectionIntSK = new myself.utxord.ChannelKeys(tapkeys.intsk.privateKey.toString('hex'));

    const newCollectionScriptSK = new myself.utxord.ChannelKeys(myself.wallet.scrsk.privKeyStr);
    const newCollectionIntSK = new myself.utxord.ChannelKeys(myself.wallet.intsk.privKeyStr);

    let collectionOutPubkey = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
       `${payload.collection.genesis_txid}i0`,
       newCollectionScriptSK.GetLocalPubKey().c_str(),
       newCollectionIntSK.GetLocalPubKey().c_str()
     );
     console.log('collectionPutPubkey:',collectionOutPubkey.c_str());


    await ordSim.Data(payload.content_type, payload.content);


    const min_fund_amount = await myself.btcToSat(Number(ordSim.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str()))
console.log("min_fund_amount_btc:",ordSim.GetMinFundingAmount(
    `${flagsFundingOptions}`
).c_str());

    const utxo_list = await myself.selectKeysByFunds(min_fund_amount);

    const inputs_sum = await myself.sumAllFunds(utxo_list);

    if(inputs_sum > Number(payload.expect_amount)){
      flagsFundingOptions += ",change";
    }
    const extra_amount = myself.btcToSat(Number(ordSim.GetGenesisTxMiningFee().c_str()));
    console.log("extra_amount:",extra_amount);
    console.log("min_fund_amount:",min_fund_amount);
    console.log("utxo_list:",utxo_list);

    if(utxo_list?.length < 1){
        // TODO: REWORK FUNDS EXCEPTION
        this.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "There are no funds to create of the Inscription, please replenish the amount: "+
          `${min_fund_amount} sat`
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
    }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.utxord.INSCRIPTION,
        (myself.satToBtc(payload.expect_amount)).toFixed(8)
      );
      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee)).toFixed(8));
      await newOrd.Data(payload.content_type, payload.content);
      await newOrd.InscribePubKey(destination_keypair.GetLocalPubKey().c_str());
      newOrd.AddToCollection(
        `${payload.collection.genesis_txid}i0`,
        payload.collection.owner_txid,
        payload.collection.owner_nout,
        (myself.satToBtc(collection.amount)).toFixed(8),
        collectionScriptSK.GetLocalPubKey().c_str(),
        collectionIntSK.GetLocalPubKey().c_str(),
        collectionOutPubkey.c_str()
      );
      await newOrd.ChangePubKey(change_keypair.GetLocalPubKey().c_str());

      for(const fund of utxo_list){
        await newOrd.AddUTXO (
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.key.GetLocalPubKey().c_str()
        );
      }

      //generate new keypair script_key
      const script_key = new myself.utxord.ChannelKeys(myself.wallet.uns.privKeyStr);

      for(const id in utxo_list){
        await newOrd.SignCommit(
          id,
          utxo_list[id].key.GetLocalPrivKey().c_str(),
          script_key.GetLocalPubKey().c_str()
        );
      }

      await newOrd.SignInscription(
        script_key.GetLocalPrivKey().c_str()
      );

      await newOrd.SignCollection(
        collectionScriptSK.GetLocalPrivKey().c_str()
      );
      const min_fund_amount_final = await myself.btcToSat(Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str()))
console.log('min_fund_amount_final:',newOrd.GetMinFundingAmount(
    `${flagsFundingOptions}`
).c_str());
      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      const data = newOrd.Serialize().c_str();
      const sk = newOrd.getIntermediateTaprootSK().c_str();
      const xord = collectionOutPubkey.c_str();
      console.log('xord:',xord);
      myself.destroy(destination_keypair);
      myself.destroy(change_keypair);
      myself.destroy(collectionScriptSK);
      myself.destroy(collectionIntSK);
      myself.destroy(newCollectionScriptSK);
      myself.destroy(newCollectionIntSK);
      myself.destroy(script_key);

      return {
        xord,
        nxord: null,
        xord_address: myself.pubKeyStrToP2tr(xord),
        data,
        sk,
        amount: min_fund_amount_final,
        inputs_sum: inputs_sum,
        utxo_list: utxo_list_final,
        expect_amount: Number(payload.expect_amount),
        extra_amount: extra_amount,
        fee_rate: payload.fee,
        size: (payload.content.length+payload.content_type.length)

      };
    } catch (exception){
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1){
        this.sendExceptionMessage(
            CREATE_INSCRIPTION,
            'error loading wasm libraries, try reloading the extension or this page'
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
      }
      return await myself.createInscriptionInCollectionContract(payload, theIndex);

    }
} //√
*/
//------------------------------------------------------------------------------
/*
async createCollectionWithParentContract(payload, theIndex = 0) {
  const myself = this;
  try {
    console.log('createInscription payload: ', payload);
      if(!myself.fundings.length ){
        // TODO: REWORK FUNDS EXCEPTION
        myself.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
     }

    const ordSim = new myself.utxord.CreateInscriptionBuilder(
      myself.utxord.COLLECTION,
      (myself.satToBtc(payload.expect_amount)).toFixed(8)
    );
    await ordSim.MiningFeeRate(myself.satToBtc(payload.fee).toFixed(8));
    let flagsFundingOptions = "collection";

    const destination_keypair = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);
    const change_keypair = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);


    let collection = myself.selectByOrdOutput(
         payload.collection.owner_txid,
         payload.collection.owner_nout
     );
     const tapkeys = await myself.matchTapRootKey(payload, collection.key.pubKeyStr);
      console.log("payload.collection:",payload.collection);
      console.log('AddToCollectionSim:', collection);

    const collectionScriptSK = new myself.utxord.ChannelKeys(tapkeys.scrsk.privateKey.toString('hex'));
    const collectionIntSK = new myself.utxord.ChannelKeys(tapkeys.intsk.privateKey.toString('hex'));

    const newCollectionScriptSK = new myself.utxord.ChannelKeys(myself.wallet.scrsk.privKeyStr);
    const newCollectionIntSK = new myself.utxord.ChannelKeys(myself.wallet.intsk.privKeyStr);

    await myself.generateNewIndex('intsk');
    await myself.generateNewIndex('scrsk');
    await myself.genKeys();

    const newCollectionScriptSK_B = new myself.utxord.ChannelKeys(myself.wallet.scrsk.privKeyStr);
    const newCollectionIntSK_B = new myself.utxord.ChannelKeys(myself.wallet.intsk.privKeyStr);

    let collectionOutPubkey = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
       `${payload.collection.genesis_txid}i0`,
       newCollectionScriptSK.GetLocalPubKey().c_str(),
       newCollectionIntSK.GetLocalPubKey().c_str()
     );
     console.log('collectionPutPubkey:',collectionOutPubkey.c_str());


    await ordSim.Data(payload.content_type, payload.content);


    const min_fund_amount = await myself.btcToSat(Number(ordSim.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str()))


    const utxo_list = await myself.selectKeysByFunds(min_fund_amount);

    const inputs_sum = await myself.sumAllFunds(utxo_list);

    if(inputs_sum > Number(payload.expect_amount)){
      flagsFundingOptions += ",change";
    }
    const extra_amount = myself.btcToSat(Number(ordSim.GetGenesisTxMiningFee().c_str()));
    console.log("extra_amount:",extra_amount);
    console.log("min_fund_amount:",min_fund_amount);
    console.log("utxo_list:",utxo_list);

    if(utxo_list?.length < 1){
        // TODO: REWORK FUNDS EXCEPTION
        this.sendExceptionMessage(
          CREATE_INSCRIPTION,
          "There are no funds to create of the Inscription, please replenish the amount: "+
          `${min_fund_amount} sat`
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
    }

      const newOrd = new myself.utxord.CreateInscriptionBuilder(
        myself.utxord.COLLECTION,
        (myself.satToBtc(payload.expect_amount)).toFixed(8)
      );
      await newOrd.MiningFeeRate((myself.satToBtc(payload.fee)).toFixed(8));
      await newOrd.Data(payload.content_type, payload.content);
      await newOrd.InscribePubKey(destination_keypair.GetLocalPubKey().c_str());
      await newOrd.CollectionCommitPubKeys(
        newCollectionScriptSK_B.GetLocalPubKey().c_str(),
        newCollectionIntSK_B.GetLocalPubKey().c_str()
      )
      newOrd.AddToCollection(
        `${payload.collection.genesis_txid}i0`,
        payload.collection.owner_txid,
        payload.collection.owner_nout,
        (myself.satToBtc(collection.amount)).toFixed(8),
        collectionScriptSK.GetLocalPubKey().c_str(),
        collectionIntSK.GetLocalPubKey().c_str(),
        collectionOutPubkey.c_str()
      );
      await newOrd.ChangePubKey(change_keypair.GetLocalPubKey().c_str());

      for(const fund of utxo_list){
        await newOrd.AddUTXO (
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.key.GetLocalPubKey().c_str()
        );
      }

      //generate new keypair script_key
      const script_key = new myself.utxord.ChannelKeys(myself.wallet.uns.privKeyStr);

      for(const id in utxo_list){
        await newOrd.SignCommit(
          id,
          utxo_list[id].key.GetLocalPrivKey().c_str(),
          script_key.GetLocalPubKey().c_str()
        );
      }

      await newOrd.SignInscription(
        script_key.GetLocalPrivKey().c_str()
      );

      await newOrd.SignCollection(
        collectionScriptSK.GetLocalPrivKey().c_str()
      );

      await newOrd.SignCollectionRootCommit(
        destination_keypair.GetLocalPrivKey().c_str()
      );

      const min_fund_amount_final = await myself.btcToSat(Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str()))

      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      const data = newOrd.Serialize().c_str();
      const sk = newOrd.getIntermediateTaprootSK().c_str();

      const xord = collectionOutPubkey.c_str();
      console.log('xord:',xord);

      const inscrId = newOrd.MakeInscriptionId().c_str();
      console.log('inscrId:',inscrId);

      const nxord = this.utxord.Collection.prototype.GetCollectionTapRootPubKey(
        inscrId,
        newCollectionScriptSK_B.GetLocalPubKey().c_str(),
        newCollectionIntSK_B.GetLocalPubKey().c_str()
        ).c_str();
      console.log('nxord:',nxord);
      myself.destroy(destination_keypair);
      myself.destroy(change_keypair);
      myself.destroy(collectionScriptSK);
      myself.destroy(collectionIntSK);
      myself.destroy(newCollectionScriptSK);
      myself.destroy(newCollectionIntSK);
      myself.destroy(newCollectionScriptSK_B);
      myself.destroy(newCollectionIntSK_B);
      myself.destroy(script_key);
      return {
        xord,
        nxord,
        xord_address: myself.pubKeyStrToP2tr(xord),
        nxord_address: myself.pubKeyStrToP2tr(nxord),
        data,
        sk,
        amount: min_fund_amount_final,
        inputs_sum: inputs_sum,
        utxo_list: utxo_list_final,
        expect_amount: Number(payload.expect_amount),
        extra_amount: extra_amount,
        fee_rate: payload.fee,
        size: (payload.content.length+payload.content_type.length)

      };
    } catch (exception){
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000){
        this.sendExceptionMessage(CREATE_INSCRIPTION, 'error loading wasm libraries, try reloading the extension or this page');
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return false;
      }
      return await myself.createInscriptionInCollectionContract(payload, theIndex);

    }
}
*/
//------------------------------------------------------------------------------
async createInscriptionContract(payload, theIndex = 0) {
  const myself = this;
  const outData = {
    xord: null,
    nxord: null,
    xord_address: null,
    data: null,
    sk: null,
    amount: 0,
    inputs_sum: 0,
    utxo_list: [],
    expect_amount: Number(payload.expect_amount),
    extra_amount: 0,
    fee_rate: payload.fee,
    size: (payload.content.length+payload.content_type.length),
    errorMessage: null as string | null
  };
  try {
    console.log('createInscription payload: ', payload);

    let collection;
    let flagsFundingOptions = "";
    let collection_utxo_key;


    if(payload?.collection?.genesis_txid){
      collection = myself.selectByOrdOutput(
         payload.collection.owner_txid,
         payload.collection.owner_nout
     );
      console.log("payload.collection:",payload.collection)
      console.log('AddToCollectionSim:', collection);
      flagsFundingOptions += "collection";
    }

    const destination_keypair = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);
    const change_keypair = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);
    const newOrd = new myself.utxord.CreateInscriptionBuilder(
      myself.utxord.INSCRIPTION,
      (myself.satToBtc(payload.expect_amount)).toFixed(8)
    );
    if(payload.metadata) {
      await newOrd.SetMetaData(JSON.stringify(payload.metadata));
    }
    await newOrd.MiningFeeRate((myself.satToBtc(payload.fee)).toFixed(8));

    if(payload?.collection?.genesis_txid){
      if(!collection){
        myself.sendExceptionMessage(
          'CREATE_INSCRIPTION',
          `Collection(txid:${payload.collection.owner_txid}, nout:${payload.collection.owner_nout}) is not found in balances`
        );
        setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        return outData;
     }

      collection_utxo_key = myself.selectKeyByOrdAddress(payload.collection.btc_owner_address);
      console.log('SignCollection:', collection_utxo_key.GetLocalPubKey().c_str());

      newOrd.AddToCollection(
        `${payload.collection.genesis_txid}i0`,
        payload.collection.owner_txid,
        payload.collection.owner_nout,
        (myself.satToBtc(collection.amount)).toFixed(8),
        collection_utxo_key.GetLocalPubKey().c_str()
      )
    }

    await newOrd.Data(payload.content_type, payload.content);
    await newOrd.InscribePubKey(destination_keypair.GetLocalPubKey().c_str());
    await newOrd.ChangePubKey(change_keypair.GetLocalPubKey().c_str());

    const min_fund_amount = await myself.btcToSat(Number(newOrd.GetMinFundingAmount(
        `${flagsFundingOptions}`
    ).c_str()))
    outData.amount = min_fund_amount;
    if(!myself.fundings.length ){
        // TODO: REWORK FUNDS EXCEPTION
        // myself.sendExceptionMessage(
        //   'CREATE_INSCRIPTION',
        //   "Insufficient funds, if you have replenish the balance, wait for several conformations or wait update on the server"
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        outData.errorMessage = "Insufficient funds, if you have replenish the balance, " +
            "wait for several conformations or wait update on the server.";
        return outData;
     }
    const utxo_list = await myself.selectKeysByFunds(min_fund_amount);
    outData.utxo_list = utxo_list;
    const inputs_sum = await myself.sumAllFunds(utxo_list);
    outData.inputs_sum = inputs_sum;

    if(utxo_list?.length < 1){
        // TODO: REWORK FUNDS EXCEPTION
        // this.sendExceptionMessage(
        //   'CREATE_INSCRIPTION',
        //   "There are no funds to create of the Inscription, please replenish the amount: "+
        //   `${min_fund_amount} sat`
        // );
        // setTimeout(()=>myself.WinHelpers.closeCurrentWindow(),closeWindowAfter);
        outData.errorMessage = "There are no funds to create of the Inscription, please replenish the amount: "+
          `${min_fund_amount} sat`
        return outData;
    }

    if(inputs_sum > Number(payload.expect_amount)){
      if(payload?.collection?.genesis_txid){flagsFundingOptions += ","}
      flagsFundingOptions += "change";
    }

    const extra_amount = myself.btcToSat(Number(newOrd.GetGenesisTxMiningFee().c_str()));
    outData.extra_amount = extra_amount;

    console.log("extra_amount:",extra_amount);
    console.log("min_fund_amount:",min_fund_amount);
    console.log("utxo_list:",utxo_list);

      for(const fund of utxo_list){
        await newOrd.AddUTXO (
          fund.txid,
          fund.nout,
          (myself.satToBtc(fund.amount)).toFixed(8),
          fund.key.GetLocalPubKey().c_str()
        );
      }

      //generate new keypair script_key
      const script_key = new myself.utxord.ChannelKeys(myself.wallet.uns.privKeyStr);

      for(const id in utxo_list){
        await newOrd.SignCommit(
          id,
          utxo_list[id].key.GetLocalPrivKey().c_str(),
          script_key.GetLocalPubKey().c_str()
        );
      }

      // get front root ord and select to addres or pubkey
      // collection_utxo_key (root! image key) (current utxo key)
      if(payload?.collection?.genesis_txid) {
        await newOrd.SignCollection(
          collection_utxo_key.GetLocalPrivKey().c_str()
        )
      }

      await newOrd.SignInscription(
        script_key.GetLocalPrivKey().c_str()
      )
      const min_fund_amount_final_btc = Number(newOrd.GetMinFundingAmount(
          `${flagsFundingOptions}`
      ).c_str());
      console.log('min_fund_amount_final_btc:',min_fund_amount_final_btc);
      const min_fund_amount_final = await myself.btcToSat(min_fund_amount_final_btc);
      console.log('min_fund_amount_final:',min_fund_amount_final);
      outData.amount = min_fund_amount_final;
      const utxo_list_final = await myself.selectKeysByFunds(min_fund_amount_final);
      outData.utxo_list = utxo_list_final;


      outData.data = newOrd.Serialize().c_str();
      outData.sk = newOrd.getIntermediateTaprootSK().c_str();
      myself.destroy(destination_keypair);
      myself.destroy(change_keypair);
      myself.destroy(script_key);
      return outData;
    } catch (exception){
      const eout = await myself.sendExceptionMessage(CREATE_INSCRIPTION, exception);
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(CREATE_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000){
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
/*  let contract;
  if(payload?.type==='AVATAR'){
    contract =  await myself.createIndependentCollectionContract(payload_data);
  }
  if(payload?.type==='INSCRIPTION'){
    if(payload?.collection?.genesis_txid){
      contract =  await myself.createInscriptionInCollectionContract(payload_data);
    }else{
      contract =  await myself.createIndependentInscriptionContract(payload_data); //int--
    }
  }
  if(data?.type==='COLLECTION'){
    if(data?.collection?.genesis_txid){
      contract =  await myself.createCollectionWithParentContract(payload_data); //inckeys++
    }else{
      contract =  await myself.createIndependentCollectionContract(payload_data);
    }
  }
*/
  try{
      console.log("outData:", payload_data.costs.data);
      if(payload_data.costs){
        if(payload_data.costs?.xord){
          myself.addToXordPubKey(payload_data.costs?.xord);
        }
        if(payload_data.costs?.nxord){
          myself.addToXordPubKey(payload_data.costs?.nxord);
        }
        if(payload_data.costs?.sk){
          myself.addToExternalKey(payload_data.costs?.sk);
          console.log(" sk:", payload_data.costs?.sk);
        }

        await myself.genKeys();
        await myself.sendMessageToWebPage(ADDRESSES_TO_SAVE, myself.addresses, payload_data._tabId);
      }

      setTimeout(async () => {
        myself.WinHelpers.closeCurrentWindow();
        await myself.sendMessageToWebPage(CREATE_INSCRIBE_RESULT, {
          contract: JSON.parse(payload_data.costs.data),
          name: payload_data.name,
          description: payload_data?.description,
          type: payload_data?.type
        }, payload_data._tabId);
        await myself.generateNewIndex('ord');
        await myself.generateNewIndex('uns');
        if(payload_data?.type!=='INSCRIPTION' && !payload_data?.collection?.genesis_txid && payload_data.costs?.xord){
          await myself.generateNewIndex('intsk');
          await myself.generateNewIndex('scrsk');
        }
        await myself.genKeys();
      },1000);


    } catch (exception) {
      this.sendExceptionMessage(CREATE_INSCRIPTION, exception)
    }
  }
  //------------------------------------------------------------------------------
async sellSignContract(utxoData, ord_price, market_fee, contract, txid, nout) {
  const myself = this;
  try {
  const sellOrd = new myself.utxord.SwapInscriptionBuilder(
    ord_price,
    (myself.satToBtc(market_fee)).toFixed(8)
  );
  sellOrd.Deserialize(JSON.stringify(contract));
  sellOrd.CheckContractTerms(myself.utxord.ORD_TERMS);
  // console.log("myself.selectKeysByOrdAddress(utxoData.address)",
  // utxoData?.address,
  // myself.selectKeyByOrdAddress(utxoData.address),
  // payload.addresses.length);

  // const utxo_keypair = myself.selectKeyByOrdAddress(utxoData.address);
  // const swap_keypair_A = new myself.utxord.ChannelKeys(myself.wallet.fund.privKeyStr);

  // console.log("| myself.wallet.fund.privKeyStr:",myself.wallet.fund.privKeyStr);
  // console.log("utxo_pubkey:",utxo_keypair.GetLocalPubKey().c_str(),
  // "swap_pubkey_A:",swap_keypair_A.GetLocalPubKey().c_str())

  sellOrd.OrdUTXO(
    txid,
    nout,
    (myself.satToBtc(utxoData.amount)).toFixed(8)
  );
  sellOrd.SwapScriptPubKeyA(myself.wallet.fund.key.GetLocalPubKey().c_str());
  sellOrd.SignOrdSwap(utxoData.key.GetLocalPrivKey().c_str());
  return sellOrd.Serialize(myself.utxord.ORD_SWAP_SIG).c_str();
} catch (exception) {
  this.sendExceptionMessage(SELL_INSCRIPTION, exception)
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

      const data = await myself.sellSignContract(
        utxoData,
        payload.ord_price,
        payload.swap_ord_terms.market_fee,
        payload.swap_ord_terms.contract,
        txid, nout
      );
      const contract_list = [];
      for (const act of payload.swap_ord_terms.contracts) {
        let contractData = await myself.sellSignContract(
          utxoData,
          payload.ord_price,
          act.market_fee,
          act.contract,
          txid, nout
        );
        contract_list.push(JSON.parse(contractData));
      }

      return {
        contract_uuid: payload.swap_ord_terms.contract_uuid,
//        contract: data,
        contracts: contract_list
      };

    } catch (exception) {
      this.sendExceptionMessage(SELL_INSCRIPTION, exception)
    }

}

async sellInscription(payload_data) {
    const myself = this;
    try {
      const data = await myself.sellInscriptionContract(payload_data);
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
  exOutputs(items = []){
    const out = []
    for(const item of items) {
      out.push(`${item.txid}:${item.nout}`);
    }
    return out;
  }
  //----------------------------------------------------------------------------
  async selectKeysByFunds(target: number, fundings = [], except_items = []){
    const addr_list = [];
    let iter = 0;

    let all_funds_list = this.fundings;
    if(fundings.length > 0){
      all_funds_list = fundings;
    }
    let all_funds = [];
    const excepts = this.exOutputs(except_items);
    console.log('excepts:',excepts);
    console.log('all_funds_list:',all_funds_list);
    for(const al of all_funds_list){
      let aloutput=`${al.txid}:${al.nout}`;
      if(!excepts?.includes(aloutput)){
        all_funds.push({...al, output: aloutput});
      }
    }
    console.log('all_funds:',all_funds);
    const sum_funds = await this.sumAllFunds(all_funds);

    if(sum_funds < target){
      console.log(sum_funds,"sum<target",target);
      return [];
    }
    if(all_funds?.length === 0){
      console.log(all_funds,"no funds");
      return [];

    }
    console.log("selectKeysByFunds->getAllFunds->all_funds:",all_funds);
    console.log("selectKeysByFunds->fundings->all_funds:",all_funds);
    console.log("selectKeysByFunds->all_funds:",all_funds);

    all_funds = all_funds.sort((a, b) => {
      if (a.amount > b.amount) { return 1; }
      if (a.amount < b.amount) { return -1; }
      return 0;
    });

    for(const al of all_funds){
      if(al.amount >= target){
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
          if(br?.address !== utxo?.address){
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
      errorMessage: null as string | null
    };
    //const balances = structuredClone(myself.balances);
    //const fundings = structuredClone(myself.fundings);
    //const inscriptions = structuredClone(myself.inscriptions);
    //console.log("commitBuyInscription->payload",payload)
    try {
      const swapSim = new myself.utxord.SwapInscriptionBuilder(
        (myself.satToBtc(payload.ord_price)).toFixed(8),
        (myself.satToBtc(payload.market_fee)).toFixed(8)
      );
      swapSim.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
      swapSim.CheckContractTerms(myself.utxord.FUNDS_TERMS);

      const min_fund_amount = await myself.btcToSat(Number(swapSim.GetMinFundingAmount("").c_str()))


      if(!myself.fundings.length ){
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
        return outData;
      }

      const utxo_list = await myself.selectKeysByFunds(min_fund_amount + 682);
      console.log("min_fund_amount:",min_fund_amount);
      console.log("utxo_list:",utxo_list);

        const buyOrd = new myself.utxord.SwapInscriptionBuilder(
          (myself.satToBtc(payload.ord_price)).toFixed(8),
          (myself.satToBtc(payload.market_fee)).toFixed(8)
        );
        buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
        buyOrd.CheckContractTerms(myself.utxord.FUNDS_TERMS);

        const swap_keypair_B = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);

        for(const fund of utxo_list){
          buyOrd.AddFundsUTXO(
            fund.txid,
            fund.nout,
            (myself.satToBtc(fund.amount)).toFixed(8),
            fund.key.GetLocalPubKey().c_str());
        }

        buyOrd.SwapScriptPubKeyB(swap_keypair_B.GetLocalPubKey().c_str());

        for(const id in utxo_list){
          let utxo_keypair = utxo_list[id].key;
          buyOrd.SignFundsCommitment(
            id,
            utxo_keypair.GetLocalPrivKey().c_str()
          );
        }
        const min_fund_amount_final = await myself.btcToSat(Number(buyOrd.GetMinFundingAmount("").c_str()))
        outData.min_fund_amount = min_fund_amount_final;
        outData.mining_fee = Number(min_fund_amount_final) - Number(payload.market_fee) - Number(payload.ord_price);
        outData.utxo_list = utxo_list;
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
        outData.data = buyOrd.Serialize(myself.utxord.FUNDS_COMMIT_SIG).c_str();
        myself.destroy(swap_keypair_B);
        return outData;
    } catch (exception) {
      const eout = await myself.sendExceptionMessage(COMMIT_BUY_INSCRIPTION, exception)
      if(eout.indexOf('ReferenceError')!==-1) return;
      if(eout.indexOf('TypeError')!==-1) return;
      if(eout.indexOf('ContractTermMissing')!==-1) return;
      if(eout.indexOf('ContractTermWrongValue')!==-1) return;
      if(eout.indexOf('ContractValueMismatch')!==-1) return;
      if(eout.indexOf('ContractTermWrongFormat')!==-1) return;
      if(eout.indexOf('ContractStateError')!==-1) return;
      if(eout.indexOf('ContractProtocolError')!==-1) return;
      if(eout.indexOf('SignatureError')!==-1) return;
      if(eout.indexOf('WrongKeyError')!==-1) return;
      if(eout.indexOf('KeyError')!==-1) return;
      if(eout.indexOf('TransactionError')!==-1) return;
      console.info(COMMIT_BUY_INSCRIPTION,'call:theIndex:',theIndex);
      theIndex++;
      if(theIndex>1000){
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


  signSwapInscription(payload, tabId: number | undefined = undefined) {
    const myself = this;
    if(!payload.ord_price){
      console.error("signSwapInscription->error:ord_price required field")
      return
    }
     //payload.ord_price = (myself.satToBtc(4000)).toFixed(8);

    try {
      // chrome.storage.sync.remove('signSwapInscriptionResult')
      console.log('//////////// signSwapInscription ARGS', payload);
      console.log("!!!!contract:",JSON.stringify(payload.swap_ord_terms.contract));
      const buyOrd = new myself.utxord.SwapInscriptionBuilder(
        payload.ord_price.toString(),
        (myself.satToBtc(payload.market_fee)).toFixed(8)
      );
      // console.log("aaaa");
      buyOrd.Deserialize(JSON.stringify(payload.swap_ord_terms.contract));
      buyOrd.CheckContractTerms(myself.utxord.MARKET_PAYOFF_SIG);
      const swap_keypair_B = new myself.utxord.ChannelKeys(myself.wallet.ord.privKeyStr);

      buyOrd.SignFundsSwap(swap_keypair_B.GetLocalPrivKey().c_str());
      const data = buyOrd.Serialize(myself.utxord.FUNDS_SWAP_SIG).c_str();
      myself.destroy(swap_keypair_B);
      (async (data, payload) => {
        console.log("SIGN_BUY_INSCRIBE_RESULT:", data);
        await myself.sendMessageToWebPage(
          SIGN_BUY_INSCRIBE_RESULT,
          { contract_uuid: payload.swap_ord_terms.contract_uuid, contract: JSON.parse(data) },
          tabId
        );
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

  async encryptedWallet(password){
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
  async setUpPassword(password){
    this.wallet.secret = await this.encrypt('secret', password);
    this.wallet.encrypted = true;
    await chrome.storage.sync.set({ encryptedWallet: true });
    await chrome.storage.sync.set({ secret: this.wallet.secret });
    return true;
  }
  async initPassword(){
    const {secret} = await chrome.storage.sync.get(['secret'])
    if(secret){
      this.wallet.secret = secret;
      this.wallet.encrypted = true;
    }
  }
  async checkPassword(password){
    try{
      const dsecret = this.decrypt(this.wallet.secret, password);
      console.log('dsecret',dsecret,'secret:',this.wallet.secret)
      if(dsecret !== 'secret'){
        return false;
      }
    }catch(e){
      console.log('Api.checkPassword:',e);
    }
    return true;
  }

  async decryptedWallet(password){
    // unlocked wallet
    // const isEnc = await this.isEncryptedWallet()
    // if(!isEnc) return true;
    const ps = await this.checkPassword(password);
    //console.log('ps',ps,'password:',password)
    if(!ps){
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

  async isEncryptedWallet(){
    const {encryptedWallet} = await chrome.storage.sync.get(['encryptedWallet']);
    console.log("encryptedWallet: ",encryptedWallet);
    if(encryptedWallet===undefined){
      chrome.storage.sync.set({ encryptedWallet: this.wallet.encrypted });
    }
    return this.wallet.encrypted;
  }
  destroy(variable){
    if(variable){
      this.utxord.destroy(variable);
      return true;
    }
    return false;
  }
  locked(){
    //TODO: locked all extension and wallet
  }

  unlocked(){
    //TODO: unlocked all extension and wallet
  }
  isLockedPlugin(){
    return this.locked;
  }

}
self.Api = Api;
self.utxord = utxord;

export default Api;
