import '~/libs/utxord.js';
import winHelpers from '~/helpers/winHelpers';
import { sendMessage } from 'webext-bridge';
import {EXCEPTION} from '~/config/events';
// Wasm Wrapper Api
class Wwa {

  constructor(network, utxord) {
      try {
        (async () => {
          this.debug = 1;
          this.network = network;
          this.utxord = utxord;
          this.bech = new this.utxord.Bech32(this.network);
          this.WinHelpers = new winHelpers();
        })();
        return this;
      } catch(e) {
        console.log('Wwa::constructor->error:',this.setErrorMessage(e));
      }
    return this;
  }

  CreateInscriptionBuilder(network, type){
    try {
      return new this.utxord.CreateInscriptionBuilder(
        network,
        type
      );
    } catch(e) {
      console.log('Wwa::CreateInscriptionBuilder->error:',this.setErrorMessage(e));
    }
  }

  MarketFee(builder, amount, addr){
    try {
      return builder.MarketFee(amount, addr);
    } catch(e) {
      console.log('Wwa::MarketFee->error:',this.setErrorMessage(e));
    }
  }

  OrdAmount(builder, amount){
    try {
      return builder.OrdAmount(amount);
    } catch(e) {
      console.log('Wwa::OrdAmount->error:',this.setErrorMessage(e));
    }
  }

  MiningFeeRate(builder, rate){
    try {
      return builder.MiningFeeRate(rate);
    } catch(e) {
      console.log('Wwa::MiningFeeRate->error:',this.setErrorMessage(e));
    }
  }

  AddUTXO(builder, txid,nout,amount,addr){
    try {
      return builder.AddUTXO(txid, nout, amount, addr);
    } catch(e) {
      console.log('Wwa::AddUTXO->error:',this.setErrorMessage(e));
    }
  }

  Data(builder, contentType, hexData){
    try {
      return builder.Data(contentType, hexData);
    } catch(e) {
      console.log('Wwa::Data->error:',this.setErrorMessage(e));
    }
  }

  MetaData(builder, hexCborData){
    try {
      return builder.MetaData(hexCborData);
    } catch(e) {
      console.log('Wwa::MetaData->error:',this.setErrorMessage(e));
    }
  }

  InscribeAddress(builder, inscriptionAddr){
    try {
      return builder.InscribeAddress(inscriptionAddr);
    } catch(e) {
      console.log('Wwa::InscribeAddress->error:',this.setErrorMessage(e));
    }
  }

  ChangeAddress(builder, changeAddr){
    try {
      return builder.ChangeAddress(changeAddr);
    } catch(e) {
      console.log('Wwa::ChangeAddress->error:',this.setErrorMessage(e));
    }
  }

  AddToCollection(builder, collectionId, txid, nout, amount, collectionAddr){
    try {
      return builder.AddToCollection(collectionId, txid, nout, amount, collectionAddr);
    } catch(e) {
      console.log('Wwa::AddToCollection->error:',this.setErrorMessage(e));
    }
  }

  FundMiningFee(builder, txid, nout,  amount, fundMiningFeeAddr){
    try {
      return builder.FundMiningFee( txid, nout,  amount,  fundMiningFeeAddr);
    } catch(e) {
      console.log('Wwa::FundMiningFee->error:',this.setErrorMessage(e));
    }
  }

  SignCommit(builder, keyRegistry,  key_filter,  scriptPK){
    try {
      return builder.SignCommit(keyRegistry,  key_filter,  scriptPK);
    } catch(e) {
      console.log('Wwa::SignCommit->error:',this.setErrorMessage(e));
    }
  }

  SignInscription(builder, keyRegistry, script_key_filter){
    try {
      return builder.SignInscription(keyRegistry, script_key_filter);
    } catch(e) {
      console.log('Wwa::SignInscription->error:',this.setErrorMessage(e));
    }
  }

  SignCollection(builder, keyRegistry,  key_filter){
    try {
      return builder.SignCollection(keyRegistry,  key_filter);
    } catch(e) {
      console.log('Wwa::SignCollection->error:',this.setErrorMessage(e));
    }
  }

  Serialize(builder, ver, phase){
    try {
      return builder.Serialize(ver, phase);
    } catch(e) {
      console.log('Wwa::Serialize->error:',this.setErrorMessage(e));
    }
  }

  Deserialize(builder, data, phase){
    try {
      return builder.Deserialize(data, phase);
    } catch(e) {
      console.log('Wwa::Deserialize->error:',this.setErrorMessage(e));
  }
}

  TransactionCount(builder,phase){
    try {
      return builder.TransactionCount(phase);
    } catch(e) {
      console.log('Wwa::TransactionCount->error:',this.setErrorMessage(e));
    }
  }

  RawTransaction(builder,phase, n){
    try {
      return builder.RawTransaction(phase, n);
    } catch(e) {
      console.log('Wwa::RawTransaction->error:',this.setErrorMessage(e));
    }
  }

  SupportedVersions(builder){
    try {
      return builder.SupportedVersions();
    } catch(e) {
      console.log('Wwa::SupportedVersions->error:',this.setErrorMessage(e));
    }
  }

  getIntermediateTaprootSK(builder){
    try {
      return builder.getIntermediateTaprootSK();
    } catch(e) {
      console.log('Wwa::getIntermediateTaprootSK->error:',this.setErrorMessage(e));
    }
  }

  MakeInscriptionId(builder){
    try {
      return  builder.MakeInscriptionId();
    } catch(e) {
      console.log('Wwa::MakeInscriptionId->error:',this.setErrorMessage(e));
    }
  }

  GetMinFundingAmount(builder, params){
    try {
      return builder.GetMinFundingAmount(params);
    } catch(e) {
      console.log('Wwa::GetMinFundingAmount->error:',this.setErrorMessage(e));
    }
  }

  GetGenesisTxMiningFee(builder){
    try {
      return builder.GetGenesisTxMiningFee();
    } catch(e) {
      console.log('Wwa::GetGenesisTxMiningFee->error:',this.setErrorMessage(e));
    }
  }

  GetNewInputMiningFee(builder){
    try {
      return builder.GetNewInputMiningFee();
    } catch(e) {
      console.log('Wwa::GetNewInputMiningFee->error:',this.setErrorMessage(e));
    }
  }

  GetNewOutputMiningFee(builder){
    try {
      return builder.GetNewOutputMiningFee();
    } catch(e) {
      console.log('Wwa::GetNewOutputMiningFee->error:',this.setErrorMessage(e));
    }
  }

  SwapInscriptionBuilder(network){
    try {
      return new this.utxord.SwapInscriptionBuilder(
        network
      );
    } catch(e) {
      console.log('Wwa::SwapInscriptionBuilder->error:',this.setErrorMessage(e));
    }
  }

  OrdPrice(builder,price){
    try {
      return builder.OrdPrice(price);
    } catch(e) {console.log('Wwa::OrdPrice->error:',this.setErrorMessage(e));}
  }
  OrdUTXO(builder,txid, nout, amount, addr){
    try {
      return builder.OrdUTXO(txid, nout, amount, addr);
    } catch(e) {
      console.log('Wwa::OrdUTXO->error:',this.setErrorMessage(e));
    }
  }
  AddFundsUTXO(builder,txid, nout, amount, addr){
    try {
      return builder.AddFundsUTXO(txid, nout, amount, addr);
    } catch(e) {
      console.log('Wwa::AddFundsUTXO->error:',this.setErrorMessage(e));
    }
  }
  OrdPayoffAddress(builder,addr){
    try {
      return builder.OrdPayoffAddress(addr);
    } catch(e) {
      console.log('Wwa::OrdPayoffAddress->error:',this.setErrorMessage(e));
    }
  }
  FundsPayoffAddress(builder,addr){
    try {
      return builder.FundsPayoffAddress(addr);
    } catch(e) {
      console.log('Wwa::FundsPayoffAddress->error:',this.setErrorMessage(e));
    }
  }
  SwapScriptPubKeyB(builder,v){
    try {
      return builder.SwapScriptPubKeyB(v);
    } catch(e) {
      console.log('Wwa::SwapScriptPubKeyB->error:',this.setErrorMessage(e));
    }
  }
  SignOrdSwap(builder,keyRegistry, key_filter){
    try {
      return builder.SignOrdSwap(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignOrdSwap->error:',this.setErrorMessage(e));
    }
  }
  SignFundsCommitment(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsCommitment(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsCommitment->error:',this.setErrorMessage(e));
    }
  }
  SignFundsSwap(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsSwap(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsSwap->error:',this.setErrorMessage(e));
    }
  }
  SignFundsPayBack(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsPayBack(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsPayBack->error:',this.setErrorMessage(e));
    }
  }
  CheckContractTerms(builder, phase){
    try {
      return builder.CheckContractTerms(phase);
    } catch(e) {
      console.log('Wwa::CheckContractTerms->error:',this.setErrorMessage(e));
    }
  }

  Bech32(network){
    try {
      return new this.utxord.Bech32(
        network
      );
    } catch(e) {
      console.log('Wwa::Bech32->error:',this.setErrorMessage(e));
    }
  }

  KeyPair(sk) {
    try {
      return new this.utxord.KeyPair(sk);
    } catch(e) {
      console.log('Wwa::KeyPair->error:',this.setErrorMessage(e));
    }
  }
  PrivKey(keyPair){
    try {
      return keyPair.PrivKey();
    } catch(e) {
      console.log('Wwa::PrivKey->error:',this.setErrorMessage(e));
    }
  }
  PubKey(keyPair){
    try {
      return keyPair.PubKey();
    } catch(e) {
      console.log('Wwa::PubKey->error:',this.setErrorMessage(e));
    }
  }
  SignSchnorr(keyPair, m){
    try {
      return keyPair.SignSchnorr(m);
    } catch(e) {
      console.log('Wwa::SignSchnorr->error:',this.setErrorMessage(e));
    }
  }
  GetP2TRAddress(keyPair, network){
    try {
      return keyPair.GetP2TRAddress(network);
    } catch(e) {
      console.log('Wwa::GetP2WPKHAddress->error:',this.setErrorMessage(e));
    }
  }
  GetP2WPKHAddress(keyPair, network){
    try {
      return keyPair.GetP2WPKHAddress(network);
    } catch(e) {
      console.log('Wwa::GetP2WPKHAddress->error:',this.setErrorMessage(e));
    }
  }
  KeyRegistry(network, seed){
    try {
      return new this.utxord.KeyRegistry(network, seed);
    } catch(e) {
      console.log('Wwa::KeyRegistry->error:',this.setErrorMessage(e));
    }
  }
  AddKeyType(keyRegistry, name, filter_json){
    try {
      return keyRegistry.AddKeyType(name, filter_json);
    } catch(e) {
      console.log('Wwa::AddKeyType->error:',this.setErrorMessage(e));
    }
  }
  RemoveKeyType(keyRegistry, name){
    try {
      return keyRegistry.RemoveKeyType(name);
    } catch(e) {
      console.log('Wwa::RemoveKeyType->error:',this.setErrorMessage(e));
    }
  }
  AddKeyToCache(keyRegistry, sk){
    try {
      return keyRegistry.AddKeyToCache(sk);
    } catch(e) {
      console.log('Wwa::AddKeyToCache->error:',this.setErrorMessage(e));
    }
  }
  RemoveKeyFromCache(keyRegistry, sk){
    try {
      return keyRegistry.RemoveKeyFromCache(sk);
    } catch(e) {
      console.log('Wwa::RemoveKeyFromCache->error:',this.setErrorMessage(e));
    }
  }
  RemoveKeyFromCacheByAddress(keyRegistry, addr){
    try {
    return keyRegistry.RemoveKeyFromCacheByAddress(addr);
    } catch(e) {
      console.log('Wwa::RemoveKeyFromCacheByAddress->error:',this.setErrorMessage(e));
    }
  }
  Derive(keyRegistry, path, for_script){
    try {
      return keyRegistry.Derive(path, for_script);
    } catch(e) {
      console.log('Wwa::Derive->error:',this.setErrorMessage(e));
    }
  }
  LookupPubKey(keyRegistry, pubkey, opt){
    try {
      return keyRegistry.LookupPubKey(pubkey, opt);
    } catch(e) {
      console.log('Wwa::LookupPubKey->error:',this.setErrorMessage(e));
    }
  }
  LookupAddress(keyRegistry, address, opt){
    try {
      return keyRegistry.LookupAddress(address, opt);
    } catch(e) {
      console.log('Wwa::LookupAddress->error:',this.setErrorMessage(e));
    }
  }

  setErrorMessage(exception: any){
    switch (this.debug) {
      case 0:
         return;
        break;
        case 2:
         return this.sendExceptionMessage('wasm=>', exception);
        break;
        case 1:
        default:
         return this.getErrorMessage(exception);
        break;
    }
  }

  getErrorMessage(exception: any) {
    if(typeof exception === 'number'){
      return this.utxord.Exception.prototype.getMessage(Number.parseInt(exception, 10))
    }
    return exception;
  }

  async sendExceptionMessage(type: string, exception: any) {
    let errorStack = '';
    exception = this.getErrorMessage(exception);
    const errorMessage = exception?.message || exception;
    if(exception?.name) type = exception?.name;
    if(exception?.stack) errorStack = exception?.stack;

    if(errorMessage.indexOf('Aborted(OOM)')!==-1){
      console.log('wasm->load::Error{',errorMessage,'}');
      return errorMessage;
    }

    const currentWindow = await this.WinHelpers.getCurrentWindow()
    sendMessage(EXCEPTION, errorMessage, `popup@${currentWindow.id}`)
    console.log(type, errorMessage, errorStack);
    return `${type} ${errorMessage} ${errorStack}`;
  }

}
self.wwa = Wwa;

export default Wwa;
