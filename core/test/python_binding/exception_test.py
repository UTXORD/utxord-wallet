import unittest
import sys
from binascii import hexlify

sys.path.append("../build/src/python_binding")

import libutxord_pybind as l15

class ExceptionsTestCase(unittest.TestCase):
    def test_call_plugin_function(self):
        try:
            str = l15.Version()
            self.assertFalse(str == "")
        except:
            self.fail("Library cannot be loaded")

    def test_std_exception(self):
        networkMode = "wrongNetwork"
        with self.assertRaisesRegex(Exception, "wrong chain mode: " + networkMode):
            l15.CreateInscriptionBuilder(networkMode)

    def test_l15_exception(self):
        with self.assertRaisesRegex(Exception, "No destination public key is provided"):
            builder = l15.CreateInscriptionBuilder("regtest")

            builder.UTXO("abcdefgh", 1, "1").\
                Data("text", hexlify("content".encode()).decode()).\
                FeeRate("0.00005").\
                Sign("34234234")

if __name__ == '__main__':
    unittest.main()
