#pragma once

#include <string>

#include "common.hpp"
#include "utils.hpp"

namespace utxord {

using l15::ChainMode;

bool IsTaproot(const CTxOut& out);
std::string GetTaprootPubKey(const CTxOut& out);
std::string GetTaprootAddress(const std::string& chain_mode, const std::string& pubkey);

std::string GetAddress(const std::string& chain_mode, const l15::bytevector& pubkeyscript);
bool IsSamePubkeyAddress(ChainMode chain, const l15::bytevector& pubkey, const std::string& address);

} // l15

