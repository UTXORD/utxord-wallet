#pragma once

namespace utxord {

template <ChainMode M> struct Hrp;
template <> struct Hrp<MAINNET> { const static char* const value; };
template <> struct Hrp<TESTNET> { const static char* const value; };
template <> struct Hrp<REGTEST> { const static char* const value; };

template <ChainMode M> struct XPubPrefix;
template <> struct XPubPrefix<MAINNET> { const static uint8_t value[4]; };
template <> struct XPubPrefix<TESTNET> { const static uint8_t value[4]; };
template <> struct XPubPrefix<REGTEST> { const static uint8_t value[4]; };


}