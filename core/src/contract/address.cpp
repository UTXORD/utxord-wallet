#include "address.hpp"

namespace utxord {

const char* const Hrp<IBech32::MAINNET>::value = "bc";
const char* const Hrp<IBech32::TESTNET>::value = "tb";
const char* const Hrp<IBech32::REGTEST>::value = "bcrt";

}
