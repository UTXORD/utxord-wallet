#pragma once

#include "keyregistry.hpp"

namespace utxord {

using l15::bytevector;

class Bip322 {
    const l15::ChainMode m_chain;

    CMutableTransaction ToSignTx(uint256 to_spend_txid) const ;

public:
    static bytevector Hash(const l15::bytevector& m);

    constexpr explicit Bip322(l15::ChainMode c) : m_chain(c){}

    uint256 ToSpendTxID(const l15::bytevector& m, std::string addr);
    bytevector Sign(l15::core::KeyRegistry& keyreg, std::string keyhint, const std::string& addr, const l15::bytevector& message);
    bool Verify(const bytevector &sig, const std::string& addr, const bytevector& message);
};

} // utxord

