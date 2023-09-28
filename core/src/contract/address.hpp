#pragma once

#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "smartinserter.hpp"

#include "bech32.h"
#include "util/strencodings.h"
#include "crypto/sha256.h"
#include "script/script.h"
#include "uint256.h"
#include "amount.h"

#include "common.hpp"

#include "contract_error.hpp"

namespace utxord {

using l15::bytevector;
using l15::xonly_pubkey;

class IBech32 {

public:
    enum ChainMode {MAINNET, TESTNET, REGTEST};

    virtual ~IBech32() = default;
    virtual std::string Encode(const xonly_pubkey& pk) const = 0;
    virtual std::tuple<unsigned, bytevector> Decode(const std::string& address) const = 0;
    virtual CScript PubKeyScript(const std::string& addr) const = 0;
};

template <IBech32::ChainMode M> struct Hrp;
template <> struct Hrp<IBech32::MAINNET> { const static char* const value; };
template <> struct Hrp<IBech32::TESTNET> { const static char* const value; };
template <> struct Hrp<IBech32::REGTEST> { const static char* const value; };


template <IBech32::ChainMode M> class Bech32: public IBech32
{
public:
    typedef Hrp<M> hrp;

    ~Bech32() override = default;
    std::string Encode(const xonly_pubkey& pk) const override {
        std::vector<unsigned char> bech32buf = {1};
        bech32buf.reserve(1 + ((pk.end() - pk.begin()) * 8 + 4) / 5);
        ConvertBits<8, 5, true>([&](unsigned char c) { bech32buf.push_back(c); }, pk.begin(), pk.end());
        return bech32::Encode(bech32::Encoding::BECH32M, hrp::value, bech32buf);
    }

    std::tuple<unsigned, bytevector> Decode(const std::string& address) const override {
        bech32::DecodeResult bech_result = bech32::Decode(address);
        if(bech_result.hrp != hrp::value)
        {
            throw ContractTermWrongValue(std::string("Address prefix should be ") + hrp::value + ". Address: " + address);
        }
        if(bech_result.data.size() < 1)
        {
            throw ContractTermWrongValue(std::string("Wrong bech32 data (no data decoded): ") + address);
        }
        if(bech_result.data[0] == 0 && bech_result.encoding != bech32::Encoding::BECH32)
        {
            throw ContractTermWrongValue("Version 0 witness address must use Bech32 checksum");
        }
        if(bech_result.data[0] != 0 && bech_result.encoding != bech32::Encoding::BECH32M)
        {
            throw ContractTermWrongValue("Version 1+ witness address must use Bech32m checksum");
        }

        bytevector data;
        data.reserve(32);
        auto I = cex::smartinserter(data, data.end());
        if(!ConvertBits<5, 8, false>([&](unsigned char c) { *I++ = c; }, bech_result.data.begin() + 1, bech_result.data.end()))
        {
            throw ContractTermWrongValue(std::string("Wrong bech32 data: ") + address);
        }

        return std::tie(bech_result.data[0], data);
    }

    CScript PubKeyScript(const std::string& addr) const override
    {
        auto res = Decode(addr);
        return CScript() << get<0>(res) << get<1>(res);
    }
};

}
