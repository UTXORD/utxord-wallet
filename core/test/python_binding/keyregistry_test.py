import os
from binascii import hexlify

print("KeyRegistry test")

from libutxord_pybind import *

key_registry = KeyRegistry(TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe")

key_registry.AddKeyType("market", '{"look_cache":true, "key_type":"TAPROOT", "accounts":["0\'"], "change":["0"], "index_range":"0-300"}')

keypair = key_registry.Derive("m/86'/1'/0'/0/300", False)

addr = keypair.GetP2TRAddress(Bech32(BTC, TESTNET))

print("address", addr)

key_registry.AddKeyToCache(keypair)

keypair1 = key_registry.Lookup(addr, "market")

addr1 = keypair1.GetP2TRAddress(Bech32(BTC, TESTNET))

print("address 1", addr1)
print("Done")
