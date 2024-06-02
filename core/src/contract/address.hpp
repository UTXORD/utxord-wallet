#pragma once

#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "smartinserter.hpp"

#include "bech32.h"
#include "util/strencodings.h"

#include "common.hpp"
#include "contract_error.hpp"

namespace utxord {

enum ChainMode {MAINNET, TESTNET, REGTEST};

}

#include "chainmode_spec.hpp"

namespace utxord {

class Bech32
{
    ChainMode chainmode;
    const char* hrptag;
public:

    template <ChainMode M>
    Bech32(Hrp<M>) : chainmode(M), hrptag(Hrp<M>::value) {}

    explicit Bech32(ChainMode m)
        : chainmode(m), hrptag(m == MAINNET ? Hrp<MAINNET>::value : (m == TESTNET ? Hrp<TESTNET>::value : Hrp<REGTEST>::value)) {}

    Bech32() : Bech32(MAINNET) {}
    Bech32(const Bech32& o) = default;

    Bech32& operator=(const Bech32& o) = default;

    ChainMode GetChainMode() const
    { return chainmode; }

    template <typename KeyType>
    std::string Encode(const KeyType& pk, bech32::Encoding encoding = bech32::Encoding::BECH32M) const {
        std::vector<unsigned char> bech32buf = {(encoding == bech32::Encoding::BECH32) ? (uint8_t)0 : (uint8_t)1};
        bech32buf.reserve(1 + ((pk.end() - pk.begin()) * 8 + 4) / 5);
        ConvertBits<8, 5, true>([&](unsigned char c) { bech32buf.push_back(c); }, pk.begin(), pk.end());
        return bech32::Encode(encoding, hrptag, bech32buf);
    }

    std::tuple<unsigned, l15::bytevector> Decode(const std::string& address) const
    {
        bech32::DecodeResult bech_result = bech32::Decode(address);
        if(bech_result.hrp != hrptag)
        {
            throw ContractTermWrongValue(std::string("Allowed prefix: ") + hrptag + ". Address: " + address);
        }
        if(bech_result.data.empty())
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

        l15::bytevector data;
        data.reserve(32);
        auto I = cex::smartinserter(data, data.end());
        if(!ConvertBits<5, 8, false>([&](unsigned char c) { *I++ = c; }, bech_result.data.begin() + 1, bech_result.data.end()))
        {
            throw ContractTermWrongValue(std::string("Wrong bech32 data: ") + address);
        }

        return std::tie(bech_result.data[0], data);
    }

    CScript PubKeyScript(const std::string& addr) const
    {
        auto res = Decode(addr);
        return CScript() << get<0>(res) << get<1>(res);
    }
};

}
