#include "ecdsa.hpp"

#include "key.h"
#include "interpreter.h"

namespace utxord {

l15::bytevector EcdsaKeypair::SignTxHash(const uint256 &sighash, unsigned char sighashtype) const
{
    l15::signature sig_compact;

    unsigned char extra_entropy[32] = {0};
//    WriteLE32(extra_entropy, test_case);

    secp256k1_ecdsa_signature sig;
    uint32_t counter = 0;
    if(!secp256k1_ecdsa_sign(m_ctx, &sig, sighash.begin(), m_sk.data(), secp256k1_nonce_function_rfc6979, extra_entropy))
        throw l15::SignatureError("Signing error");

    secp256k1_ecdsa_signature_serialize_compact(m_ctx, sig_compact.data(), &sig);

    // Grind for low R
    while (sig_compact[0] & (uint8_t)0x80) {
        WriteLE32(extra_entropy, ++counter);
        if(!secp256k1_ecdsa_sign(m_ctx, &sig, sighash.begin(), m_sk.data(), secp256k1_nonce_function_rfc6979, extra_entropy))
            throw l15::SignatureError("Signing error");

        secp256k1_ecdsa_signature_serialize_compact(m_ctx, sig_compact.data(), &sig);
    }
//        ret = secp256k1_ecdsa_sign(m_ctx, &sig, sighash.begin(), m_sk.begin(), secp256k1_nonce_function_rfc6979, extra_entropy);
//    }

    // Additional verification step to prevent using a potentially corrupted signature
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(m_ctx, &pk, m_sk.data()))
        throw l15::KeyError();

    if (!secp256k1_ecdsa_verify(secp256k1_context_static, &sig, sighash.begin(), &pk))
        throw l15::SignatureError("Signature error");

    size_t sig_len = 72;
    l15::bytevector sig_der;
    sig_der.resize(sig_len);

    secp256k1_ecdsa_signature_serialize_der(m_ctx, sig_der.data(), &sig_len, &sig);
    sig_der.resize(sig_len);

    sig_der.push_back(sighashtype);

    return sig_der;
}

l15::bytevector EcdsaKeypair::SignSegwitV0Tx(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& pubkeyscript, const int hashtype) const
{
    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::move(spent_outputs), true);

    uint256 sighash = SignatureHash(pubkeyscript, tx, nin, hashtype, txdata.m_spent_outputs[nin].nValue, SigVersion::WITNESS_V0);

    std::clog << "ECDSA sighash: " << sighash.GetHex() << std::endl;

    return SignTxHash(sighash, hashtype);
}

l15::compressed_pubkey EcdsaKeypair::GetPubKey() const
{
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(m_ctx, &pk, m_sk.data()))
        throw l15::KeyError();

    l15::compressed_pubkey res;
    size_t len = 33;

    if (!secp256k1_ec_pubkey_serialize(m_ctx, res.data(), &len, &pk, SECP256K1_EC_COMPRESSED))
        throw l15::KeyError();

    return res;
}

}
