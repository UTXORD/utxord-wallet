#include "address.hpp"

namespace utxord {

const char* const Hrp<MAINNET>::value = "bc";
const char* const Hrp<TESTNET>::value = "tb";
const char* const Hrp<REGTEST>::value = "bcrt";

}
