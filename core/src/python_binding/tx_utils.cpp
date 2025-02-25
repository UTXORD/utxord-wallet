#include "tx_utils.hpp"

#include <iostream>
#include <ostream>

#include "bech32.hpp"
#include "base58.hpp"
#include "contract_builder.hpp"

namespace utxord {

using l15::bytevector;
using l15::xonly_pubkey;
using l15::hex;
using l15::unhex;
using l15::ChainMode;

bool IsTaproot(const CTxOut &out)
{
    int witversion;
    std::vector<unsigned char> witnessprogram;
    bool segwit =  out.scriptPubKey.IsWitnessProgram(witversion, witnessprogram);
    return segwit && witversion == 1;
}

std::string GetTaprootPubKey(const CTxOut &out)
{
    int witversion;
    bytevector witnessprogram;
    if (!out.scriptPubKey.IsWitnessProgram(witversion, witnessprogram)) {
        throw l15::TransactionError("Not SegWit output");
    }
    if (witversion != 1) {
        throw l15::TransactionError("Wrong SegWit version: " + std::to_string(witversion));
    }
    return hex(witnessprogram);
}

std::string GetTaprootAddress(const std::string& chain_mode, const std::string& pubkey)
{
    if (chain_mode == "testnet") {
        return l15::Bech32(l15::BTC, l15::TESTNET).Encode(unhex<xonly_pubkey>(pubkey));
    }
    else if (chain_mode == "mainnet") {
        return l15::Bech32(l15::BTC, l15::MAINNET).Encode(unhex<xonly_pubkey>(pubkey));
    }
    else if (chain_mode == "regtest") {
        return l15::Bech32(l15::BTC, l15::REGTEST).Encode(unhex<xonly_pubkey>(pubkey));
    }

    throw l15::IllegalArgument(std::string("chain_mode: ") + chain_mode);
}

std::string GetAddress(const std::string& chain_mode, const bytevector& pubkeyscript)
{
    ChainMode chain = l15::MAINNET;
    if (chain_mode == "testnet" || chain_mode == "signet") chain = l15::TESTNET;
    else if (chain_mode == "regtest") chain = l15::REGTEST;
    else if (chain_mode != "mainnet") throw l15::IllegalArgument(std::string("chain_mode: " + chain_mode));

    int witver;
    bytevector witnessprogram;
    CScript script(pubkeyscript.begin(), pubkeyscript.end());
    bool segwit =  script.IsWitnessProgram(witver, witnessprogram);
    if (segwit) {
        return Bech32(l15::BTC, chain).Encode(witnessprogram, witver == 0 ? bech32::Encoding::BECH32 : bech32::Encoding::BECH32M);
    }

    if (script.IsPayToScriptHash()) {
          /*script[0] == OP_HASH160 &&
            script[1] == 0x14 &&
            script[22] == OP_EQUAL); */

        return l15::Base58(chain).Encode(std::span(script.begin()+2, script.begin() + 22), l15::SCRIPT_HASH);
    }

    // IsPayToPublicKeyHash
    if (script.size() == 25 &&
        script[0] == OP_DUP &&
        script[1] == OP_HASH160 &&
        script[2] == 0x14 &&
        script[23] == OP_EQUALVERIFY &&
        script[24] == OP_CHECKSIG) {

        return l15::Base58(chain).Encode(std::span(script.begin()+3, script.begin() + 23), l15::PUB_KEY_HASH);
    }

    return "";
}

bool IsSamePubkeyAddress(ChainMode chain, const bytevector& pubkey, const std::string& addr)
{
    bool res = false;
    Bech32 bech(BTC, chain);
    if (addr.starts_with(bech.GetHrp())) {
        auto [witver, data] = bech.Decode(addr);

        if (witver == 0)
            res = data == l15::cryptohash<bytevector>(pubkey, CHash160());
        else if (witver == 1) {
            auto [tweaked_pubkey, _] = SchnorrKeyPair::AddTapTweak(KeyPair::GetStaticSecp256k1Context(), pubkey);
            res = data == tweaked_pubkey;
        }
        else
            throw l15::IllegalArgument((std::ostringstream() << addr << " unknown witness ver: " << witver).str());
    }
    else {
        auto [addrtype, keyid] = l15::Base58(chain).Decode(addr);
        if (addrtype == l15::PUB_KEY_HASH)
            res = keyid == l15::cryptohash<bytevector>(pubkey, CHash160());
        else if (addrtype == l15::SCRIPT_HASH) {
            CScript redeemScript;
            redeemScript << 0 << l15::cryptohash<bytevector>(pubkey, CHash160());
            bytevector scripthash = l15::cryptohash<bytevector>(redeemScript, CHash160());
            res = scripthash == keyid;
        }
    }
    return res;
}

}
