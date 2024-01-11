// This file is used to work around SWIG misunderstanding of the template partial specs
// Do not include it directly, use "address.hpp" instead

namespace utxord {

template <ChainMode M> struct Hrp;
template <> struct Hrp<MAINNET> { const static char* const value; };
template <> struct Hrp<TESTNET> { const static char* const value; };
template <> struct Hrp<REGTEST> { const static char* const value; };

}