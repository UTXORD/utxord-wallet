#include "inscription_common.hpp"
#include "core_io.h"
#include "serialize.h"
#include "univalue.h"
#include "nlohmann/json.hpp"
#include <exception>
#include "interpreter.h"
#include "feerate.h"
#include "script_merkle_tree.hpp"
#include "schnorr.hpp"
#include "contract_builder.hpp"
#include "contract_builder_factory.hpp"
#include "utils.hpp"

#include <atomic>

namespace utxord {

using l15::FormatAmount;
using l15::ParseAmount;
using l15::SignatureError;
using l15::core::SchnorrKeyPair;
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

void WitnessStack::ReadJson(const UniValue &stack, const std::function<std::string()> &lazy_name)
{
    if (!stack.isArray()) {
        throw ContractTermWrongFormat(std::string(TxInput::name_witness));
    }

    size_t i = 0;
    try {
        for (const auto& v: stack.getValues()) {
            if (!v.isStr()) {
                throw ContractTermWrongFormat("witness[" + std::to_string(i) + ']');
            }

            if (i < m_stack.size()) {
                if (m_stack[i] != unhex<bytevector>(v.get_str())) throw ContractTermMismatch((std::ostringstream() << lazy_name() << '[' << i << ']').str());
            }
            else {
                m_stack.emplace_back(unhex<bytevector>(v.get_str()));
            }
            ++i;
        }
    }
    catch(std::logic_error& e) {
        std::throw_with_nested(ContractTermWrongFormat("witness[" + std::to_string(i) + ']'));
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string TxInput::name_witness = "witness";

UniValue TxInput::MakeJson() const
{
    // Do not serialize underlying contract as transaction input: copy it as UTXO and write UTXO related values only
    // Lazy mode copy of an UTXO state is used to allow early-set of not completed contract as the input at any time
    UTXO utxo_out(chain, *output);
    UniValue json = utxo_out.MakeJson();
    if (witness) {
        json.pushKV(name_witness, witness.MakeJson());
    }

    return json;
}

void TxInput::ReadJson(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (output) {
        auto new_output = std::make_shared<UTXO>(chain, json, lazy_name);
        if (new_output->TxID() != output->TxID() || new_output->NOut() != output->NOut()) throw ContractTermMismatch(lazy_name());
    }
    else {
        output = std::make_shared<UTXO>(chain, json, lazy_name);
    }

    {   const UniValue& val = json[name_witness];
        if (!val.isNull()) {
            witness.ReadJson(val, [&](){ return (lazy_name() += ".") += name_witness; });
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

ZeroDestination::ZeroDestination(const UniValue &json, const std::function<std::string()>& lazy_name)
{
    if (json[name_amount].getInt<CAmount>() != 0) throw ContractTermMismatch(move((lazy_name() += '.') += name_amount));
}

UniValue ZeroDestination::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_amount, 0);
    return res;
}

void ZeroDestination::ReadJson(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (json[name_amount].getInt<CAmount>() != 0) throw ContractTermMismatch(move((lazy_name() += '.') += name_amount));
}

/*--------------------------------------------------------------------------------------------------------------------*/

const char* P2Witness::type = "p2witness";

P2Address::P2Address(ChainMode chain, const UniValue &json, const std::function<std::string()>& lazy_name): m_chain(chain)
{
    if (!json[name_type].isStr()) {
        throw ContractTermWrongValue(move((lazy_name() += '.') += name_type));
    }
    m_amount = json[name_amount].getInt<CAmount>();
    m_addr = json[name_addr].get_str();
}

UniValue P2Address::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, Type());
    res.pushKV(name_amount, m_amount);
    res.pushKV(name_addr, m_addr);

    return res;
}

void P2Address::ReadJson(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != Type()) throw ContractTermMismatch(move((lazy_name() += '.') += name_type));
    if (m_amount != json[name_amount].getInt<CAmount>()) throw ContractTermMismatch(move((lazy_name() += '.') += name_amount));
    if (m_addr != json[name_addr].get_str())  throw ContractTermMismatch(move((lazy_name() += '.') += name_addr));
}


std::shared_ptr<IContractDestination> P2Address::Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name)
{
    P2Address dest(chain, json, lazy_name);
    try {
        auto p2addr = Construct(chain, dest.m_amount, dest.m_addr);
        p2addr->ReadJson(json, lazy_name);
        return p2addr;
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(lazy_name()));
    }
}

std::shared_ptr<IContractDestination> P2Address::Construct(ChainMode chain, CAmount amount, std::string addr)
{
    Bech32 bech(BTC, chain);
    if (addr.starts_with(bech.GetHrp())) {
        auto [witver, data] = bech.Decode(addr);

        if (witver == 0)
            return std::make_shared<P2WPKH>(chain, amount, move(addr));
        if (witver == 1)
            return std::make_shared<P2TR>(chain, amount, move(addr));

        throw ContractTermWrongValue((std::ostringstream() << addr << " wrong witness ver: " << witver).str());
    }

    Base58 base58(chain);
    auto [addrtype, hash] = base58.Decode(addr);
    if (addrtype == PUB_KEY_HASH) {
        return std::make_shared<P2PKH>(chain, amount, move(addr));
    }

    throw ContractTermWrongValue(move(addr));
}

/*--------------------------------------------------------------------------------------------------------------------*/
const char* P2PKH::type = "p2pkh";

/*--------------------------------------------------------------------------------------------------------------------*/

const char* OpReturnDestination::type = "op_return";
const char* OpReturnDestination::name_data = "data";

OpReturnDestination::OpReturnDestination(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) {
        throw ContractTermWrongValue(move((lazy_name() += '.') += name_type));
    }

    if (!json.exists(name_amount)) throw ContractTermMissing(lazy_name() + '.' + name_amount);
    if (!json[name_amount].isNum()) throw ContractTermWrongFormat(lazy_name() + '.' + name_amount);

    if (json.exists(name_data)) {
        if(!json[name_data].isStr()) throw ContractTermWrongFormat(lazy_name() + '.' + name_data);
        try {
            m_data = unhex<bytevector>(json[name_data].getValStr());
        } catch(std::logic_error& e) {
            std::throw_with_nested(ContractTermWrongFormat(lazy_name() + '.' + name_data));
        }
        if (m_data.size() > 80)
            throw ContractTermWrongValue(lazy_name() + '.' + name_data + " is too large: " + std::to_string(m_data.size()));
    }

    m_amount = json[name_amount].getInt<CAmount>();
}

void OpReturnDestination::Data(bytevector data)
{
    if (data.size() > 80) {
        std::ostringstream buf;
        buf << type << "." << name_data << " is too large: " << m_data.size();
        throw ContractTermWrongValue(buf.str());
    }
    m_data = move(data);
}

CScript OpReturnDestination::PubKeyScript() const
{
    CScript pubKeyScript {OP_RETURN};

    if (!m_data.empty())
        pubKeyScript << m_data;

    return pubKeyScript;
}

UniValue OpReturnDestination::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, type);
    res.pushKV(name_amount, m_amount);
    res.pushKV(name_data, hex(m_data));

    return res;
}

void OpReturnDestination::ReadJson(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) throw ContractTermMismatch(lazy_name());
    if (m_amount != json[name_amount].getInt<CAmount>()) throw ContractTermMismatch(move((lazy_name() += '.') += name_amount));
    if (hex(m_data) != json[name_data].getValStr())  throw ContractTermMismatch(move((lazy_name() += '.') += name_data));
}

std::shared_ptr<IContractDestination> OpReturnDestination::Construct(ChainMode chain, const UniValue &json, const std::function<std::string()> &lazy_name)
{
    return std::make_shared<OpReturnDestination>(json, lazy_name);
}

/*--------------------------------------------------------------------------------------------------------------------*/

std::shared_ptr<ISigner> P2WPKH::LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const
{
    unsigned witver;
    bytevector pkhash;
    std::tie(witver, pkhash) = Bech().Decode(m_addr);
    if (witver != 0) throw ContractTermWrongValue(std::string(name_addr));

    KeyPair keypair = masterKey.Lookup(m_addr, key_filter_tag);
    EcdsaKeyPair ecdsa(masterKey.Secp256k1Context(), keypair.PrivKey());
    return std::make_shared<P2WPKHSigner>(move(ecdsa));
}

std::shared_ptr<ISigner> P2TR::LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const
{
    unsigned witver;
    bytevector pk;
    std::tie(witver, pk) = Bech().Decode(m_addr);
    if (witver != 1) throw ContractTermWrongValue(std::string(name_addr));

    KeyPair keypair = masterKey.Lookup(m_addr, key_filter_tag);
    SchnorrKeyPair schnorr(masterKey.Secp256k1Context(), keypair.PrivKey());
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

void UTXO::ReadJson(const UniValue &json, const std::function<std::string()> &lazy_name)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) {
        throw ContractTermWrongValue(move((lazy_name() += '.') += name_type));
    }
    m_txid = json[name_txid].get_str();
    m_nout = json[name_nout].getInt<uint32_t >();

    const UniValue& dest = json[name_destination];

    if (dest.isNull()) throw ContractTermWrongValue(move((lazy_name() += '.') += name_destination));

    m_destination = ContractDestinationFactory<P2Witness, ZeroDestination>::ReadJson(m_chain, dest, [&](){return (lazy_name() += '.') += name_destination; });
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string IContractBuilder::name_contract_type = "contract_type";
const std::string IContractBuilder::name_params = "params";
const std::string IContractBuilder::name_version = "protocol_version";
const std::string IContractBuilder::name_mining_fee_rate = "mining_fee_rate";
const std::string IContractBuilder::name_market_fee = "market_fee";
const std::string IContractBuilder::name_utxo = "utxo";
const std::string IContractBuilder::name_custom_fee = "custom_fee";
const std::string IContractBuilder::name_txid = "txid";
const std::string IContractBuilder::name_nout = "nout";
const std::string IContractDestination::name_amount = "amount";
const std::string IContractBuilder::name_pk = "pubkey";
const std::string IContractDestination::name_addr = "addr";
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
    if (!pk.verify(SchnorrKeyPair::GetStaticSecp256k1Context(), sig, sighash)) {
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

        if (!secp256k1_ec_pubkey_parse(SchnorrKeyPair::GetStaticSecp256k1Context(), &pubkey, witness[1].data(), 33)) throw SignatureError("pubkey");
        if (!secp256k1_ecdsa_signature_parse_der(SchnorrKeyPair::GetStaticSecp256k1Context(), &signature, witness[0].data(), witness[0].size() - 1)) throw SignatureError("signature format");
        if (!secp256k1_ecdsa_verify(SchnorrKeyPair::GetStaticSecp256k1Context(), &signature, sighash.data(), &pubkey)) throw SignatureError("sig");
    }
    else {
        throw std::runtime_error("not implemented witver: " + std::to_string(witver));
    }
}

CAmount IContractBuilder::GetNewInputMiningFee()
{
    return CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_KEYSPEND_VIN_VSIZE);
}

CAmount IContractBuilder::GetNewOutputMiningFee()
{
    return CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_VOUT_VSIZE);
}

void IContractBuilder::DeserializeContractAmount(const UniValue &val, std::optional<CAmount> &target, const std::function<std::string()> &lazy_name)
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

void IContractBuilder::DeserializeContractString(const UniValue& val, std::optional<std::string> &target, const std::function<std::string()> &lazy_name)
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


void IContractBuilder::DeserializeContractScriptPubkey(const UniValue &val, std::optional<xonly_pubkey> &pk, const std::function<std::string()> &lazy_name) {
    if (!val.isNull()) {
        xonly_pubkey new_pk;
        try {
            new_pk = unhex<xonly_pubkey>(val.get_str());
        }
        catch (std::logic_error& e) {
            std::throw_with_nested(ContractTermWrongFormat(lazy_name()));
        }

        if (pk) {
            if (*pk != new_pk) throw ContractTermMismatch(move((lazy_name() += " is already set: ") += hex(*pk)));
        } else {
            pk = move(new_pk);
        }
    }
}


} // utxord
