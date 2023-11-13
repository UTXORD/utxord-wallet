import '~/libs/utxord.js';
// Wasm Wrapper Api
class Wwa {

  constructor(network) {
      try {
        (async () => {
          this.network = network;
          this.utxord = await utxord();
          this.bech = new this.utxord.Bech32(this.network);
        })();
        return this;
      } catch(e) {
        console.log('Wwa::constructor->error:',this.getErrorMessage(e));
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
      console.log('Wwa::CreateInscriptionBuilder->error:',this.getErrorMessage(e));
    }
  }

  MarketFee(builder, amount, addr){
    try {
      return builder.MarketFee(amount, addr);
    } catch(e) {console.log('Wwa::MarketFee->error:',this.getErrorMessage(e));}
  }

  OrdAmount(builder, amount){
    try {
      return builder.OrdAmount(amount);
    } catch(e) {console.log('Wwa::OrdAmount->error:',this.getErrorMessage(e));}
  }

  MiningFeeRate(builder, rate){
    try {
      return builder.MiningFeeRate(rate);
  } catch(e) {console.log('Wwa::MiningFeeRate->error:',this.getErrorMessage(e));}
}

  AddUTXO(builder, txid,nout,amount,addr){
    try {
      return builder.AddUTXO(txid, nout, amount, addr);
  } catch(e) {console.log('Wwa::AddUTXO->error:',this.getErrorMessage(e));}
}

  Data(builder, contentType, hexData){
    try {
      return builder.Data(contentType, hexData);
  } catch(e) {console.log('Wwa::Data->error:',this.getErrorMessage(e));}
  }

  MetaData(builder, hexCborData){
    try {
      return builder.MetaData(hexCborData);
    } catch(e) {console.log('Wwa::MetaData->error:',this.getErrorMessage(e));}
  }

  InscribeAddress(builder, inscriptionAddr){
    try {
      return builder.InscribeAddress(inscriptionAddr);
    } catch(e) {console.log('Wwa::InscribeAddress->error:',this.getErrorMessage(e));}
  }

  ChangeAddress(builder, changeAddr){
    try {
      return builder.ChangeAddress(changeAddr);
  } catch(e) {console.log('Wwa::ChangeAddress->error:',this.getErrorMessage(e));}
 }

  AddToCollection(builder, collectionId, txid, nout, amount, collectionAddr){
    try {
    return builder.AddToCollection(collectionId, txid, nout, amount, collectionAddr);
  } catch(e) {console.log('Wwa::AddToCollection->error:',this.getErrorMessage(e));}}

  FundMiningFee(builder, txid, nout,  amount, fundMiningFeeAddr){
    try {
    return builder.FundMiningFee( txid, nout,  amount,  fundMiningFeeAddr);
  } catch(e) {console.log('Wwa::FundMiningFee->error:',this.getErrorMessage(e));}}

  SignCommit(builder, keyRegistry,  key_filter,  scriptPK){
    try {
    return builder.SignCommit(keyRegistry,  key_filter,  scriptPK);
  } catch(e) {console.log('Wwa::SignCommit->error:',this.getErrorMessage(e));}}

  SignInscription(builder, keyRegistry, script_key_filter){
    try {
    return builder.SignInscription(keyRegistry, script_key_filter);
  } catch(e) {console.log('Wwa::SignInscription->error:',this.getErrorMessage(e));}}

  SignCollection(builder, keyRegistry,  key_filter){
    try {
    return builder.SignCollection(keyRegistry,  key_filter);
  } catch(e) {console.log('Wwa::SignCollection->error:',this.getErrorMessage(e));}}

  Serialize(builder, ver, phase){
    try {
    return builder.Serialize(ver, phase);
  } catch(e) {console.log('Wwa::Serialize->error:',this.getErrorMessage(e));}}

  Deserialize(builder, data, phase){
    try {
    return builder.Deserialize(data, phase);
  } catch(e) {console.log('Wwa::Deserialize->error:',this.getErrorMessage(e));}}

  TransactionCount(builder,phase){
    try {
    return builder.TransactionCount(phase);
  } catch(e) {console.log('Wwa::TransactionCount->error:',this.getErrorMessage(e));}}

  RawTransaction(builder,phase, n){
    try {
    return builder.RawTransaction(phase, n);
  } catch(e) {console.log('Wwa::RawTransaction->error:',this.getErrorMessage(e));}}

  SupportedVersions(builder){
    try {
    return builder.SupportedVersions();
  } catch(e) {console.log('Wwa::SupportedVersions->error:',this.getErrorMessage(e));}}

  getIntermediateTaprootSK(builder){
    try {
    return builder.getIntermediateTaprootSK();
  } catch(e) {console.log('Wwa::getIntermediateTaprootSK->error:',this.getErrorMessage(e));}}

  MakeInscriptionId(builder){
    try {
    return  builder.MakeInscriptionId();
  } catch(e) {console.log('Wwa::MakeInscriptionId->error:',this.getErrorMessage(e));}}

  GetMinFundingAmount(builder, params){
    try {
    return builder.GetMinFundingAmount(params);
  } catch(e) {console.log('Wwa::GetMinFundingAmount->error:',this.getErrorMessage(e));}}

  GetGenesisTxMiningFee(builder){
    try {
    return builder.GetGenesisTxMiningFee();
  } catch(e) {console.log('Wwa::GetGenesisTxMiningFee->error:',this.getErrorMessage(e));}}

  GetNewInputMiningFee(builder){
    try {
    return builder.GetNewInputMiningFee();
  } catch(e) {console.log('Wwa::GetNewInputMiningFee->error:',this.getErrorMessage(e));}}

  GetNewOutputMiningFee(builder){
    try {
    return builder.GetNewOutputMiningFee();
  } catch(e) {console.log('Wwa::GetNewOutputMiningFee->error:',this.getErrorMessage(e));}}

  SwapInscriptionBuilder(network){
    try {
      return new this.utxord.SwapInscriptionBuilder(
        network
      );
    } catch(e) {
      console.log('Wwa::SwapInscriptionBuilder->error:',this.getErrorMessage(e));
    }
  }

  OrdPrice(builder,price){
    try {
      return builder.OrdPrice(price);
    } catch(e) {console.log('Wwa::OrdPrice->error:',this.getErrorMessage(e));}
  }
  OrdUTXO(builder,txid, nout, amount, addr){
    try {
      return builder.OrdUTXO(txid, nout, amount, addr);
    } catch(e) {
      console.log('Wwa::OrdUTXO->error:',this.getErrorMessage(e));
    }
  }
  AddFundsUTXO(builder,txid, nout, amount, addr){
    try {
      return builder.AddFundsUTXO(txid, nout, amount, addr);
    } catch(e) {
      console.log('Wwa::AddFundsUTXO->error:',this.getErrorMessage(e));
    }
  }
  OrdPayoffAddress(builder,addr){
    try {
      return builder.OrdPayoffAddress(addr);
    } catch(e) {
      console.log('Wwa::OrdPayoffAddress->error:',this.getErrorMessage(e));
    }
  }
  FundsPayoffAddress(builder,addr){
    try {
      return builder.FundsPayoffAddress(addr);
    } catch(e) {
      console.log('Wwa::FundsPayoffAddress->error:',this.getErrorMessage(e));
    }
  }
  SwapScriptPubKeyB(builder,v){
    try {
      return builder.SwapScriptPubKeyB(v);
    } catch(e) {
      console.log('Wwa::SwapScriptPubKeyB->error:',this.getErrorMessage(e));
    }
  }
  SignOrdSwap(builder,keyRegistry, key_filter){
    try {
      return builder.SignOrdSwap(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignOrdSwap->error:',this.getErrorMessage(e));
    }
  }
  SignFundsCommitment(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsCommitment(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsCommitment->error:',this.getErrorMessage(e));
    }
  }
  SignFundsSwap(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsSwap(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsSwap->error:',this.getErrorMessage(e));
    }
  }
  SignFundsPayBack(builder, keyRegistry, key_filter){
    try {
      return builder.SignFundsPayBack(keyRegistry, key_filter);
    } catch(e) {
      console.log('Wwa::SignFundsPayBack->error:',this.getErrorMessage(e));
    }
  }
  CheckContractTerms(builder, phase){
    try {
      return builder.CheckContractTerms(phase);
    } catch(e) {
      console.log('Wwa::CheckContractTerms->error:',this.getErrorMessage(e));
    }
  }

  Bech32(network){
    try {
      return new this.utxord.Bech32(
        network
      );
    } catch(e) {
      console.log('Wwa::Bech32->error:',this.getErrorMessage(e));
    }
  }

  KeyPair(sk) {
    try {
      return new this.utxord.KeyPair(sk);
    } catch(e) {
      console.log('Wwa::KeyPair->error:',this.getErrorMessage(e));
    }
  }

  PrivKey(keyPair){
    try {
      return keyPair.PrivKey();
    } catch(e) {
      console.log('Wwa::PrivKey->error:',this.getErrorMessage(e));
    }
  }
  PubKey(keyPair){
    try {
      return keyPair.PubKey();
    } catch(e) {
      console.log('Wwa::PubKey->error:',this.getErrorMessage(e));
    }
  }
  SignSchnorr(keyPair, m){
    try {
      return keyPair.SignSchnorr(m);
    } catch(e) {
      console.log('Wwa::SignSchnorr->error:',this.getErrorMessage(e));
    }
  }
  GetP2TRAddress(keyPair, network){
    try {
      return KeyPair.GetP2TRAddress(network);
    } catch(e) {
      console.log('Wwa::GetP2WPKHAddress->error:',this.getErrorMessage(e));
    }
  }
  GetP2WPKHAddress(keyPair, network){
    try {
      return KeyPair.GetP2WPKHAddress(network);
    } catch(e) {
      console.log('Wwa::GetP2WPKHAddress->error:',this.getErrorMessage(e));
    }
  }
  KeyRegistry(network, seed){
    try {
      return new this.utxord.KeyRegistry(network, seed);
    } catch(e) {
      console.log('Wwa::KeyRegistry->error:',this.getErrorMessage(e));
    }
  }
  AddKeyType(keyRegistry, name, filter_json){
    try {
      return keyRegistry.AddKeyType(name, filter_json);
    } catch(e) {
      console.log('Wwa::AddKeyType->error:',this.getErrorMessage(e));
    }
  }
  RemoveKeyType(keyRegistry, name){
    try {
      return keyRegistry.RemoveKeyType(name);
    } catch(e) {
      console.log('Wwa::RemoveKeyType->error:',this.getErrorMessage(e));
    }
  }
  AddKeyToCache(keyRegistry, sk){
    try {
      return keyRegistry.AddKeyToCache(sk);
    } catch(e) {
      console.log('Wwa::AddKeyToCache->error:',this.getErrorMessage(e));
    }
  }
  RemoveKeyFromCache(keyRegistry, sk){
    try {
      return keyRegistry.RemoveKeyFromCache(sk);
    } catch(e) {
      console.log('Wwa::RemoveKeyFromCache->error:',this.getErrorMessage(e));
    }
  }
  RemoveKeyFromCacheByAddress(keyRegistry, addr){
    try {
    return keyRegistry.RemoveKeyFromCacheByAddress(addr);
    } catch(e) {
      console.log('Wwa::RemoveKeyFromCacheByAddress->error:',this.getErrorMessage(e));
    }
  }
  Derive(keyRegistry, path, for_script){
    try {
      return keyRegistry.Derive(path, for_script);
    } catch(e) {
      console.log('Wwa::Derive->error:',this.getErrorMessage(e));
  }
}
  LookupPubKey(keyRegistry, pubkey, opt){
    try {
      return keyRegistry.LookupPubKey(pubkey, opt);
    } catch(e) {
      console.log('Wwa::LookupPubKey->error:',this.getErrorMessage(e));
    }
  }
  LookupAddress(keyRegistry, address, opt){
    try {
      return keyRegistry.LookupAddress(address, opt);
    } catch(e) {
      console.log('Wwa::LookupAddress->error:',this.getErrorMessage(e));
    }
  }

  getErrorMessage(exception: number) {
    return this.utxord.Exception.prototype.getMessage(exception)
  }

}
self.wwa = Wwa;

export default Wwa;
