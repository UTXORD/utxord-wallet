import unittest
import sys
from binascii import hexlify

sys.path.append("../build/src/python_binding")

import libutxord_pybind as utxord

class ExceptionsTestCase(unittest.TestCase):

    def test_std_exception(self):
        networkMode = "wrongNetwork"
        with self.assertRaisesRegex(Exception, "wrong chain mode: " + networkMode):
            utxord.CreateInscriptionBuilder(networkMode, utxord.INSCRIPTION)

    def test_l15_exception(self):
        with self.assertRaisesRegex(Exception, "No destination public key is provided"):
            builder = utxord.CreateInscriptionBuilder(utxord.REGTEST, utxord.INSCRIPTION)

            builder.UTXO("abcdefgh", 1, "1")
            builder.Data("text", hexlify("content".encode()).decode())
            builder.FeeRate("0.00005")
            builder.MarketFee("0.00005", "abc");

if __name__ == '__main__':
    unittest.main()
