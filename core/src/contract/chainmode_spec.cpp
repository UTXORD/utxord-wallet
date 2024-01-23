#include "address.hpp"

namespace utxord {

const char* const Hrp<MAINNET>::value = "bc";
const char* const Hrp<TESTNET>::value = "tb";
const char* const Hrp<REGTEST>::value = "bcrt";

const uint8_t XPubPrefix<MAINNET>::value[4] {0x04, 0x88, 0xb2, 0x1e};
const uint8_t XPubPrefix<TESTNET>::value[4] {0x04, 0x35, 0x87, 0xcf};
const uint8_t XPubPrefix<REGTEST>::value[4] {0x04, 0x35, 0x87, 0xcf};

}
