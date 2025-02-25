from binascii import hexlify
from libutxord_pybind import *

print("IsSamePubkeyAddress test")

try:
    key_registry = KeyRegistry(TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe")
    raw_keypair = key_registry.Derive("m/86'/1'/0'/0/300", True)
    keypair = key_registry.Derive("m/86'/1'/0'/0/300", False)

    addr = keypair.GetP2TRAddress(Bech32(BTC, TESTNET))
    pk = raw_keypair.GetSchnorrKeyPair().GetPubKey()

    print("addr: ", addr)
    print("pubkey: ", hexlify(pk).decode('ascii'))

    if not IsSamePubkeyAddress(TESTNET, pk, addr):
        print ("Taproot Fails")

    keypair1 = key_registry.Derive("m/84'/1'/0'/0/300", False)

    addr = keypair1.GetP2WPKHAddress(Bech32(BTC, TESTNET))
    pk = keypair1.GetEcdsaKeyPair().GetPubKey()

    print("addr: ", addr)
    print("pubkey: ", hexlify(pk).decode('ascii'))

    if not IsSamePubkeyAddress(TESTNET, pk, addr):
        print ("Segwit Fails")

    keypair2 = key_registry.Derive("m/49'/1'/0'/0/300", False)

    addr = keypair2.GetP2WPKH_P2SHAddress(TESTNET)
    pk = keypair2.GetEcdsaKeyPair().GetPubKey()

    print("addr: ", addr)
    print("pubkey: ", hexlify(pk).decode('ascii'))

    if not IsSamePubkeyAddress(TESTNET, pk, addr):
        print ("Nested Segwit Fails")

    keypair3 = key_registry.Derive("m/44'/1'/0'/0/300", False)

    addr = keypair3.GetP2PKHAddress(TESTNET)
    pk = keypair3.GetEcdsaKeyPair().GetPubKey()

    print("addr: ", addr)
    print("pubkey: ", hexlify(pk).decode('ascii'))

    if not IsSamePubkeyAddress(TESTNET, pk, addr):
        print ("Legacy PubKey Hash Fails")

except Exception as e:
    print("Failed: ", e)


