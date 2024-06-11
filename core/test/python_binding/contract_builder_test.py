import os
from binascii import hexlify
from libutxord_pybind import *

key_registry = KeyRegistry(REGTEST, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe")
key_registry.AddKeyType("market", '{"look_cache":true, "key_type":"TAPROOT", "accounts":["0\'"], "change":["0"], "index_range":"0-300"}')

keypair = key_registry.Derive("m/86'/1'/0'/0/300", False)

addr = keypair.GetP2TRAddress(Bech32(REGTEST))

print("ContractBuilder as parent test")

create_inscription = CreateInscriptionBuilder(REGTEST, INSCRIPTION)

create_inscription.OrdDestination(546, addr)
create_inscription.MarketFee(1000, addr)
create_inscription.AuthorFee(0, addr)
create_inscription.MiningFeeRate(3000)
create_inscription.Data("text/html", bytes.fromhex("3c21444f43545950452068746d6c3e3c68746d6c3e3c686561643e3c7469746c653e546573743c2f7469746c653e3c2f686561643e3c626f64793e3c68313e41737365743c2f68313e3c2f626f64793e3c2f68746d6c3e"))

create_inscription.InscribeScriptPubKey(keypair.PubKey())
create_inscription.InscribeInternalPubKey(keypair.PubKey())
create_inscription.ChangeAddress(addr)

create_inscription.AddUTXO("345567678879784635241fbc876df9c345567678879784635241fbc876df9c", 0, 10000, addr)
create_inscription.SignCommit(key_registry, "market")
create_inscription.SignInscription(key_registry, "market")

print(create_inscription.Serialize(10, INSCRIPTION_SIGNATURE))

swap_inscription = SwapInscriptionBuilder(REGTEST)

swap_inscription.MarketFee(0, addr)
swap_inscription.SetOrdMiningFeeRate(3000)
swap_inscription.OrdPrice(10000)
swap_inscription.SetSwapScriptPubKeyM(keypair.PubKey())

print(swap_inscription.Serialize(5, ORD_TERMS))


trustless_swap = TrustlessSwapInscriptionBuilder(REGTEST)

trustless_swap.MarketFee(0, addr)
trustless_swap.MiningFeeRate(3000)
trustless_swap.OrdPrice(10000)
trustless_swap.MarketScriptPubKey(keypair.PubKey())
trustless_swap.CommitOrdinal("345567678879784635241fbc876df9c345567678879784635241fbc876df9c", 0, 546, addr)
trustless_swap.FundsPayoffAddress(addr)
trustless_swap.FundCommitOrdinal("345567678879784635241fbc876df9c345567678879784635241fbc876df9c", 0, 546, addr, addr)

print(trustless_swap.Serialize(6, TRUSTLESS_ORD_TERMS))


tx_contract = '{"contract_type":"transaction","params":{"protocol_version":2,"mining_fee_rate":1000,"utxo":[{"type":"utxo","txid":"dfe1b4626bde169f54a7f585880b992a3f25d5c3067339bf406c55815e591ecf","nout":0,"destination":{"type":"p2witness","amount":657,"addr":"bcrt1pm7l3k3ahhfqpzv9nlcc390esx8afn4kgccmcrzzvr8cqdq63kxxqwqqp2f"},"witness":["30720fa1ec981b35d7c1262a646d9a69c75e1a5a36edee6c204e8be757565af2ef7f57331e07f8ccc5da026366359fbe31c570aec39897b4ef62fa5f99335b37"]}],"outputs":[{"type":"p2witness","amount":546,"addr":"bcrt1ptge9ax57nlztgyx2xzjk78julf2wsjug0nw2nl3mrecheevtdxhqp7dzwk"}]}}'

simple_transaction = SimpleTransaction(REGTEST)
simple_transaction.Deserialize(tx_contract, TX_SIGNATURE)




print("Done")
