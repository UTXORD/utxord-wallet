import os
import base64
from binascii import hexlify

from libutxord_pybind import *

print("Bip322 verify test")

try:
    sig:bytes = base64.b64decode("AkcwRAIgM2gBAQqvZX15ZiysmKmQpDrG83avLIT492QBzLnQIxYCIBaTpOaD20qRlEylyxFSeEA2ba9YOixpX8z46TSDtS40ASECx/EgAxlkQpQ9hYjgGu6EBCPMVPwVIVJqO4XCsMvViHI=")

    addr = "bc1q9vza2e8x573nczrlzms0wvx3gsqjx7vavgkx0l"
    checker = Bip322(MAINNET)
    if checker.Verify(sig, addr, bytes()):
        print("Done")
    else:
        print("Wrong signature")

except Exception as e:
    print("Failed: ", e)


