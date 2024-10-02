import os
from binascii import hexlify

from libutxord_pybind import *

print("KeyRegistry test")

try:
    key_registry = KeyRegistry(TESTNET, "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe")

    key_registry.AddKeyType("market", '{"look_cache":true, "key_type":"TAPROOT", "accounts":["0\'"], "change":["0"], "index_range":"0-300"}')

    keypair = key_registry.Derive("m/86'/1'/0'/0/300", False)

    addr = keypair.GetP2TRAddress(Bech32(BTC, TESTNET))

    print("address", addr)

    pk = keypair.PubKey()

    print(pk)

    key_registry.AddKeyToCache(keypair)

    keypair1 = key_registry.Lookup(addr, "market")

    addr1 = keypair1.GetP2TRAddress(Bech32(BTC, TESTNET))

    print("address 1", addr1)
    print("Done")
except Exception as e:
    print("Failed: ", e)

print("KeyRegistry exception test 1")

try:
    key_registry =\
        KeyRegistry(TESTNET,
                    "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe")

    key_registry.AddKeyType("market",
                            '{"look_cache":true, "key_type", "accounts":["0\'"], "change":["0"], "index_range":"0-300"}'
                            )

    print("Failed: no exception")

except WrongKeyLookupFilter as e:
    print("Done")

except Exception as e:
    print("Failed: unknown Exception")


print("KeyRegistry exception test 2")

try:
    key_registry = (
        KeyRegistry(TESTNET,
                    "b37f263befa23efb352f0ba45a5e452363963fabc64c946a75df155244630ebaa1ac8056b873e79232486d5dd36809f8925c9c5ac8322f5380940badc64cc6fe"))

    key_registry.AddKeyType("market",
                            '{"look_cache":true, "key_type":"TAPROOT", "accounts":["0\'"], "change":["0"], "index_range":"0-300"}')

    keypair = key_registry.Derive("m/a'/b'/z'/0/300", False)

except WrongDerivationPath as e:
    print("Done")

except Exception as e:
    print("Failed: unknown Exception")

