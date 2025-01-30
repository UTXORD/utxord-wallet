
#include "bip322.hpp"

#include <deque>

#include "streams.h"

#include "contract_builder.hpp"
#include "hash_helper.hpp"

namespace utxord {

using namespace l15;

namespace {

const CSHA256 BIP322_HASH = PrecalculatedTaggedHash("BIP0322-signed-message");

}

CMutableTransaction Bip322::ToSignTx(uint256 to_spend_txid) const
{
    CMutableTransaction tx;
    tx.nVersion = 0;
    tx.vin.emplace_back(Txid::FromUint256(to_spend_txid), 0, CScript(), 0);
    tx.vout.emplace_back(0, CScript() << OP_RETURN);

    return tx;
}

bytevector Bip322::Hash(const bytevector& m)
{
    l15::HashWriter hash(BIP322_HASH);
    hash.write(m);
    return hash;
}

uint256 Bip322::ToSpendTxID(const l15::bytevector &m, std::string addr)
{
    CMutableTransaction tx;
    tx.nVersion = 0;
    tx.vin.emplace_back(Txid::FromUint256(uint256()), 0xFFFFFFFF, CScript() << OP_0 << Hash(m), 0);
    tx.vout.emplace_back(P2Address::Construct(m_chain, {}, addr)->TxOutput());
    tx.vout.back().nValue = 0;
    return tx.GetHash();
}

l15::bytevector Bip322::Sign(l15::core::KeyRegistry &keyreg, std::string keyhint, const std::string &addr, const l15::bytevector &message)
{
    auto tx = ToSignTx(ToSpendTxID(message, addr));

    auto prevout = P2Address::Construct(m_chain, {}, addr);
    std::vector<CTxOut> prevouts = { prevout->TxOutput()};
    prevouts.front().nValue = 0;

    auto signer = prevout->LookupKey(keyreg, keyhint);

    auto witness = signer->Sign(tx, 0, prevouts, SIGHASH_DEFAULT);

    DataStream serdata;
    serdata << witness;

    bytevector res; res.reserve(serdata.size());
    std::ranges::transform(serdata, cex::smartinserter(res, res.end()), [](std::byte b){return static_cast<uint8_t>(b);});

    return res;
}

bool Bip322::Verify(const l15::bytevector &sig, const std::string &addr, const l15::bytevector &message)
{
    auto tx = ToSignTx(ToSpendTxID(message, addr));

    auto prevout = P2Address::Construct(m_chain, {}, addr);
    std::vector<CTxOut> prevouts = { prevout->TxOutput()};
    prevouts.front().nValue = 0;

    DataStream serdata(sig);
    serdata >> tx.vin.front().scriptWitness.stack;

    try {
        IContractBuilder::VerifyTxSignature(m_chain, addr, tx.vin.front().scriptWitness.stack, tx, 0, move(prevouts));
        return true;
    } catch (const SignatureError& e) {
        return false;
    }
}

} // utxord
