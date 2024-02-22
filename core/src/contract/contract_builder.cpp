#include "inscription_common.hpp"
#include "core_io.h"
#include "serialize.h"
#include "univalue.h"
#include "nlohmann/json.hpp"
#include <exception>
#include <ranges>
#include "interpreter.h"
#include "feerate.h"
#include "script_merkle_tree.hpp"
#include "channel_keys.hpp"
#include "contract_builder.hpp"
#include "utils.hpp"

#include <execution>
#include <atomic>

#include <limits>

namespace utxord {

using l15::FormatAmount;
using l15::ParseAmount;
using l15::SignatureError;
using l15::core::ChannelKeys;
using l15::core::MasterKey;

const std::string IJsonSerializable::name_type = "type";

UniValue WitnessStack::MakeJson() const
{
    UniValue stack(UniValue::VARR);
    for (const auto& elem: m_stack) {
        stack.push_back(hex(elem));
    }
    return stack;
}

void WitnessStack::ReadJson(const UniValue &stack)
{
    if (!stack.isArray()) {
        throw ContractTermWrongValue(std::string(TxInput::name_witness));
    }

    size_t i = 0;
    for (const auto& v: stack.getValues()) {
        if (!v.isStr()) {
            throw ContractTermWrongValue("witness[" + std::to_string(i) + ']');
        }

        if (i < m_stack.size()) {
            if (m_stack[i] != unhex<bytevector>(v.get_str())) throw ContractTermMismatch(move(((TxInput::name_witness + '[') += std::to_string(i)) += ']'));
        }
        else {
            m_stack.emplace_back(unhex<bytevector>(v.get_str()));
        }
        ++i;
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string TxInput::name_witness = "witness";

UniValue TxInput::MakeJson() const
{
    // Do not serialize underlying contract as transaction input: copy it as UTXO and write UTXO related values only
    // Lazy mode copy of an UTXO state is used to allow early-set of not completed contract as the input at any time
    UTXO utxo_out(bech32, *output);
    UniValue json = utxo_out.MakeJson();
    if (witness) {
        json.pushKV(name_witness, witness.MakeJson());
    }

    return json;
}

void TxInput::ReadJson(const UniValue &json)
{
    if (output) {
        auto new_output = std::make_shared<UTXO>(bech32, json);
        if (new_output->TxID() != output->TxID() || new_output->NOut() != output->NOut()) throw ContractTermMismatch("tx input"/*move((name_swap_inputs + '[') += std::to_string(i) += ']')*/);
    }
    else {
        output = std::make_shared<UTXO>(bech32, json);
    }

    {   const UniValue& val = json[name_witness];
        if (!val.isNull()) {
            witness.ReadJson(val);
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

std::vector<bytevector> P2WPKHSigner::Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const
{
    if (hashtype == SIGHASH_DEFAULT) hashtype = SIGHASH_ALL;

    int witver;
    std::vector<unsigned char> witprog;

    if (!spent_outputs[nin].scriptPubKey.IsWitnessProgram(witver, witprog) || witver != 0 || witprog.size() != 20)
        throw ContractError("not P2WPKH contract");

    CScript witnessscript;
    witnessscript << OP_DUP << OP_HASH160 << witprog << OP_EQUALVERIFY << OP_CHECKSIG;

    return {m_keypair.SignSegwitV0Tx(tx, nin, spent_outputs, witnessscript, hashtype), m_keypair.GetPubKey().as_vector()};
}

std::vector<bytevector> TaprootSigner::Sign(const CMutableTransaction &tx, uint32_t nin, std::vector<CTxOut> spent_outputs, int hashtype) const
{
    if (hashtype == SIGHASH_ALL) hashtype = SIGHASH_DEFAULT;
    return {m_keypair.SignTaprootTx(tx, nin, spent_outputs, {}, hashtype)};
}

/*--------------------------------------------------------------------------------------------------------------------*/

ZeroDestination::ZeroDestination(const UniValue &json)
{
    if (json[IContractBuilder::name_amount].getInt<CAmount>() != 0) throw ContractTermMismatch(std::string(IContractBuilder::name_amount));
}

UniValue ZeroDestination::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(IContractBuilder::name_amount, 0);
    return res;
}

void ZeroDestination::ReadJson(const UniValue &json)
{
    if (json[IContractBuilder::name_amount].getInt<CAmount>() != 0) throw ContractTermMismatch(std::string(IContractBuilder::name_amount));
}

/*--------------------------------------------------------------------------------------------------------------------*/

const char* P2Witness::type = "p2witness";

P2Witness::P2Witness(Bech32 bech, const UniValue &json): mBech(bech)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }
    m_amount = json[IContractBuilder::name_amount].getInt<CAmount>();
    m_addr = json[IContractBuilder::name_addr].get_str();
}

UniValue P2Witness::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, type);
    res.pushKV(IContractBuilder::name_amount, m_amount);
    res.pushKV(IContractBuilder::name_addr, m_addr);

    return res;
}

void P2Witness::ReadJson(const UniValue &json)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }
    if (m_amount != json[IContractBuilder::name_amount].getInt<CAmount>()) throw ContractTermMismatch(std::string(IContractBuilder::name_amount));
    if (m_addr != json[IContractBuilder::name_addr].get_str())  throw ContractTermMismatch(std::string(IContractBuilder::name_addr));
}

std::shared_ptr<IContractDestination> IContractDestination::ReadJson(Bech32 bech, const UniValue& json, bool allow_zero_destination)
{
    if (json[name_type].getValStr() == P2Witness::type) {
        P2Witness dest(bech, json);

        return P2Witness::Construct(bech, dest.m_amount, dest.m_addr);
    }
    else if (allow_zero_destination && !json.exists(name_type)){
        return std::make_shared<ZeroDestination>(json);
    }
    else throw std::domain_error("unknown destination type: " + json[name_type].getValStr());
}

std::shared_ptr<IContractDestination> P2Witness::Construct(Bech32 bech, CAmount amount, std::string addr)
{
    unsigned witver;
    bytevector data;
    std::tie(witver, data) = bech.Decode(addr);
    if (witver == 0) {
        return std::make_shared<P2WPKH>(bech.GetChainMode(), amount, move(addr));
    }
    else if (witver == 1){
        return std::make_shared<P2TR>(bech.GetChainMode(), amount, move(addr));
    }
    else {
        throw std::invalid_argument("address with wrong witness program version: " + std::to_string(witver));
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

std::shared_ptr<ISigner> P2WPKH::LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const
{
    unsigned witver;
    bytevector pkhash;
    std::tie(witver, pkhash) = mBech.Decode(m_addr);
    if (witver != 0) throw ContractTermWrongValue(std::string(IContractBuilder::name_addr));

    KeyPair keypair = masterKey.Lookup(m_addr, key_filter_tag);
    EcdsaKeypair ecdsa(masterKey.Secp256k1Context(), keypair.PrivKey());
    return std::make_shared<P2WPKHSigner>(move(ecdsa));
}

std::shared_ptr<ISigner> P2TR::LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const
{
    unsigned witver;
    bytevector pk;
    std::tie(witver, pk) = mBech.Decode(m_addr);
    if (witver != 1) throw ContractTermWrongValue(std::string(IContractBuilder::name_addr));

    KeyPair keypair = masterKey.Lookup(m_addr, key_filter_tag);
    ChannelKeys schnorr(masterKey.Secp256k1Context(), keypair.PrivKey());
    return std::make_shared<TaprootSigner>(move(schnorr));
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string UTXO::name_txid = "txid";
const std::string UTXO::name_nout = "nout";
const std::string UTXO::name_destination = "destination";

const char* UTXO::type = "utxo";

UniValue UTXO::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(IJsonSerializable::name_type, type);
    res.pushKV(name_txid, m_txid);
    res.pushKV(name_nout, m_nout);
    if (m_destination) res.pushKV(name_destination, m_destination->MakeJson());

    return res;
}

void UTXO::ReadJson(const UniValue &json)
{
    if (!json[IJsonSerializable::name_type].isStr() || json[IJsonSerializable::name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(IJsonSerializable::name_type));
    }
    m_txid = json[name_txid].get_str();
    m_nout = json[name_nout].getInt<uint32_t >();

    UniValue dest = json[name_destination];
    if (dest.isNull())
        throw ContractTermWrongValue(std::string(IJsonSerializable::name_type));

    m_destination = IContractDestination::ReadJson(mBech, dest);
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string IContractBuilder::name_contract_type = "contract_type";
const std::string IContractBuilder::name_params = "params";
const std::string IContractBuilder::name_version = "protocol_version";
const std::string IContractBuilder::name_mining_fee_rate = "mining_fee_rate";
const std::string IContractBuilder::name_market_fee = "market_fee";
const char* IContractBuilder::name_utxo = "utxo";
const std::string IContractBuilder::name_txid = "txid";
const std::string IContractBuilder::name_nout = "nout";
const std::string IContractBuilder::name_amount = "amount";
const std::string IContractBuilder::name_pk = "pubkey";
const std::string IContractBuilder::name_addr = "addr";
const std::string IContractBuilder::name_sig = "sig";
const std::string IContractBuilder::name_change_addr = "change_addr";

const std::string IContractBuilder::FEE_OPT_HAS_CHANGE = "change";
const std::string IContractBuilder::FEE_OPT_HAS_COLLECTION = "collection";
const std::string IContractBuilder::FEE_OPT_HAS_XTRA_UTXO = "extra_utxo";
const std::string IContractBuilder::FEE_OPT_HAS_P2WPKH_INPUT = "p2wpkh_utxo";

CScript IContractBuilder::MakeMultiSigScript(const xonly_pubkey& pk1, const xonly_pubkey& pk2)
{
    CScript script;
    script << pk1 << OP_CHECKSIG;
    script << pk2 << OP_CHECKSIGADD;
    script << 2 << OP_NUMEQUAL;
    return script;
}
CAmount IContractBuilder::CalculateWholeFee(const std::string& params) const
{
    auto txs = GetTransactions();
    return std::accumulate(txs.begin(), txs.end(), CAmount(0), [](CAmount sum, const auto &tx) {
        return sum + l15::CalculateTxFee(tx.first, tx.second);
    });
}

void IContractBuilder::VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs, const CScript& spend_script)
{
    if (sig.size() != 64 && sig.size() != 65) throw SignatureError("sig size");

    uint256 sighash;
    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::move(spent_outputs), true);

    ScriptExecutionData execdata;
    execdata.m_annex_init = true;
    execdata.m_annex_present = false; // Only support annex-less signing for now.

    if(!spend_script.empty()) {
        execdata.m_codeseparator_pos_init = true;
        execdata.m_codeseparator_pos = 0xFFFFFFFF; // Only support non-OP_CODESEPARATOR BIP342 signing for now.
        execdata.m_tapleaf_hash_init = true;
        execdata.m_tapleaf_hash = l15::TapLeafHash(spend_script);
    }

    uint8_t hashtype = SIGHASH_DEFAULT;
    if (sig.size() == 65) {
        hashtype = sig.back();
        if (hashtype == SIGHASH_DEFAULT) {
            throw SignatureError("sighash type");
        }
    }

    SigVersion sigversion = spend_script.empty() ? SigVersion::TAPROOT : SigVersion::TAPSCRIPT;

    if (!SignatureHashSchnorr(sighash, execdata, tx, nin, hashtype, sigversion, txdata, MissingDataBehavior::FAIL)) {
        throw SignatureError("sighash");
    }
    if (!pk.verify(ChannelKeys::GetStaticSecp256k1Context(), sig, sighash)) {
        throw SignatureError("sig");
    }
}

void IContractBuilder::VerifyTxSignature(const std::string& addr, const std::vector<bytevector>& witness, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut> spent_outputs) const
{
    uint32_t witver;
    bytevector keyid;

    std::tie(witver, keyid) = bech32().Decode(addr);
    if (witver == 1) {
        if (witness.size() != 1) throw SignatureError("witness stack size: " + std::to_string(witness.size()));
        if (witness[0].size() != 64 && witness[0].size() != 65) throw SignatureError("sig size");

        xonly_pubkey pk = move(keyid);
        signature sig = witness[0];

        VerifyTxSignature(pk, sig, tx, nin, move(spent_outputs), {});

    }
    else if (witver == 0) {
        if (witness.size() != 2) throw SignatureError("witness stack size: " + std::to_string(witness.size()));
        if (witness[1].size() != 33) throw SignatureError("pubkey size: " + std::to_string(witness[0].size()));

        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::move(spent_outputs), true);

        CScript witnessscript;
        witnessscript << OP_DUP << OP_HASH160 << keyid << OP_EQUALVERIFY << OP_CHECKSIG;

        uint256 sighash = SignatureHash(witnessscript, tx, nin, witness[0].back(), txdata.m_spent_outputs[nin].nValue, SigVersion::WITNESS_V0, &txdata);

        secp256k1_pubkey pubkey;
        secp256k1_ecdsa_signature signature;

        if (!secp256k1_ec_pubkey_parse(ChannelKeys::GetStaticSecp256k1Context(), &pubkey, witness[1].data(), 33)) throw SignatureError("pubkey");
        if (!secp256k1_ecdsa_signature_parse_der(ChannelKeys::GetStaticSecp256k1Context(), &signature, witness[0].data(), witness[0].size()-1)) throw SignatureError("signature format");
        if (!secp256k1_ecdsa_verify(ChannelKeys::GetStaticSecp256k1Context(), &signature, sighash.data(), &pubkey)) throw SignatureError("sig");
    }
    else {
        throw std::runtime_error("not implemented witver: " + std::to_string(witver));
    }
}

std::string IContractBuilder::GetNewInputMiningFee()
{
    return FormatAmount(CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_KEYSPEND_VIN_VSIZE));
}

std::string IContractBuilder::GetNewOutputMiningFee()
{
    return FormatAmount(CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_VOUT_VSIZE));
}

void IContractBuilder::DeserializeContractAmount(const UniValue &val, std::optional<CAmount> &target, std::function<std::string()> lazy_name)
{
    if (!val.isNull()) {
        CAmount amount;
        try {
            if (val.isNum())
                amount = val.getInt<CAmount>();
            else if (val.isStr())
                amount = ParseAmount(val.getValStr());
            else
                throw ContractTermWrongValue(move((lazy_name() += ": ") += val.getValStr()));
        }
        catch (const ContractError& ) {
            std::rethrow_exception(std::current_exception());
        }
        catch (...) {
            std::throw_with_nested(ContractTermWrongValue(move((lazy_name() += ": ") += val.getValStr())));
        }

        if (target) {
            if (*target != amount) throw ContractTermMismatch(move((lazy_name() += " is already set: ") += std::to_string(*target)));
        } else {
            target = amount;
        }
    }
}

void IContractBuilder::DeserializeContractString(const UniValue& val, std::optional<std::string> &target, std::function<std::string()> lazy_name)
{
    if (!val.isNull()) {
        std::string str;
        try {
            str = val.get_str();
        }
        catch (...) {
            std::throw_with_nested(ContractTermWrongValue(lazy_name()));
        }

        if (target) {
            if (*target != str) throw ContractTermMismatch(move((lazy_name() += " is already set: ") += *target));
        } else {
            target = move(str);
        }
    }
}


void IContractBuilder::DeserializeContractScriptPubkey(const UniValue &val, std::optional<xonly_pubkey> &pk, std::function<std::string()> lazy_name) {
    if (!val.isNull()) {
        xonly_pubkey new_pk;
        try {
            new_pk = unhex<xonly_pubkey>(val.get_str());
        }
        catch (...) {
            std::throw_with_nested(ContractTermWrongValue(lazy_name()));
        }

        if (pk) {
            if (*pk != new_pk) throw ContractTermMismatch(move((lazy_name() += " is already set: ") += hex(*pk)));
        } else {
            pk = move(new_pk);
        }
    }
}


} // utxord
