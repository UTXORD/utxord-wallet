#pragma once

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#include "random.h"

#include "common.hpp"
#include "channel_keys.hpp"

namespace utxord {

class EcdsaKeypair
{
    const secp256k1_context* m_ctx;
    l15::seckey m_sk;
public:
    EcdsaKeypair() : m_ctx(l15::core::ChannelKeys::GetStaticSecp256k1Context()), m_sk(l15::core::ChannelKeys::GetStrongRandomKey(m_ctx)) {}
    explicit EcdsaKeypair(l15::seckey sk): m_ctx(l15::core::ChannelKeys::GetStaticSecp256k1Context()), m_sk(std::move(sk)) {}
    explicit EcdsaKeypair(const secp256k1_context* secp256k1_ctx, l15::seckey sk): m_ctx(secp256k1_ctx), m_sk(std::move(sk)) {}

    EcdsaKeypair(const EcdsaKeypair&) = default;
    EcdsaKeypair(EcdsaKeypair &&old) noexcept: m_ctx(old.m_ctx), m_sk(std::move(old.m_sk)) {}

    EcdsaKeypair& operator= (const EcdsaKeypair&) = default;
    EcdsaKeypair& operator= (EcdsaKeypair &&old) noexcept
    { m_ctx = old.m_ctx; m_sk = std::move(old.m_sk); return *this; }

    l15::compressed_pubkey GetPubKey() const;

    l15::bytevector SignTxHash(const uint256 &sighash, unsigned char sighashtype) const;
    l15::bytevector SignSegwitV0Tx(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& pubkeyscript, const int hashtype) const;
};

}
