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
        throw ContractTermWrongValue(std::string(ContractInput::name_witness));
    }

    size_t i = 0;
    for (const auto& v: stack.getValues()) {
        if (!v.isStr()) {
            throw ContractTermWrongValue("witness[" + std::to_string(i) + ']');
        }

        if (i < m_stack.size()) {
            if (m_stack[i] != unhex<bytevector>(v.get_str())) throw ContractTermMismatch(move(((ContractInput::name_witness + '[') += std::to_string(i)) += ']'));
        }
        else {
            m_stack.emplace_back(unhex<bytevector>(v.get_str()));
        }
        ++i;
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string ContractInput::name_witness = "witness";

UniValue ContractInput::MakeJson() const
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

void ContractInput::ReadJson(const UniValue &json)
{
    output = std::make_shared<UTXO>(bech32, json);

    {   const UniValue& val = json[name_witness];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongValue(name_witness.c_str());
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

const char* P2Witness::type = "p2witness";

UniValue P2Witness::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, type);
    res.pushKV(ContractBuilder::name_amount, m_amount);
    res.pushKV(ContractBuilder::name_addr, m_addr);

    return res;
}

void P2Witness::ReadJson(const UniValue &json)
{
    if (json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }
    m_amount = json[ContractBuilder::name_amount].getInt<CAmount>();
    m_addr = json[ContractBuilder::name_addr].get_str();
}


std::shared_ptr<IContractDestination> P2Witness::ReadJson(Bech32 bech, const UniValue& json)
{
    P2Witness dest(bech);
    dest.ReadJson(json);

    return Construct(bech, dest.m_amount, dest.m_addr);
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
    if (witver != 0) throw ContractTermWrongValue(std::string(ContractBuilder::name_addr));

    KeyPair keypair = masterKey.Lookup(m_addr, key_filter_tag);
    EcdsaKeypair ecdsa(masterKey.Secp256k1Context(), keypair.PrivKey());
    return std::make_shared<P2WPKHSigner>(move(ecdsa));
}

std::shared_ptr<ISigner> P2TR::LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const
{
    unsigned witver;
    bytevector pk;
    std::tie(witver, pk) = mBech.Decode(m_addr);
    if (witver != 1) throw ContractTermWrongValue(std::string(ContractBuilder::name_addr));

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
    res.pushKV(name_type, type);
    res.pushKV(name_txid, m_txid);
    res.pushKV(name_nout, m_nout);
    if (m_destination) res.pushKV(name_destination, m_destination->MakeJson());

    return res;
}

void UTXO::ReadJson(const UniValue &json)
{
    if (json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }
    m_txid = json[name_txid].get_str();
    m_nout = json[name_nout].getInt<uint32_t >();

    UniValue dest = json[name_destination];
    if (!dest.isNull()) {
        m_destination = P2Witness::ReadJson(mBech, dest);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string ContractBuilder::name_contract_type = "contract_type";
const std::string ContractBuilder::name_params = "params";
const std::string ContractBuilder::name_version = "protocol_version";
const std::string ContractBuilder::name_mining_fee_rate = "mining_fee_rate";
const std::string ContractBuilder::name_market_fee = "market_fee";
const std::string ContractBuilder::name_market_fee_addr = "market_fee_addr";
const char* ContractBuilder::name_utxo = "utxo";
const std::string ContractBuilder::name_txid = "txid";
const std::string ContractBuilder::name_nout = "nout";
const std::string ContractBuilder::name_amount = "amount";
const std::string ContractBuilder::name_pk = "pubkey";
const std::string ContractBuilder::name_addr = "addr";
const std::string ContractBuilder::name_sig = "sig";



CAmount ContractBuilder::CalculateWholeFee(const std::string& params) const {
    auto txs = GetTransactions();
    return std::accumulate(txs.begin(), txs.end(), CAmount(0), [](CAmount sum, const auto &tx) {
        return sum + l15::CalculateTxFee(tx.first, tx.second);
    });
}

void ContractBuilder::VerifyTxSignature(const xonly_pubkey& pk, const signature& sig, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut>&& spent_outputs, const CScript& spend_script)
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

void ContractBuilder::VerifyTxSignature(const std::string& addr, const std::vector<bytevector>& witness, const CMutableTransaction& tx, uint32_t nin, std::vector<CTxOut>&& spent_outputs) const
{
    uint32_t witver;
    bytevector keyid;

    std::tie(witver, keyid) = bech32().Decode(addr);
    if (witver == 1) {
        if (witness.size() != 1) throw SignatureError("witness stack size");
        if (witness[0].size() != 64 && witness[0].size() != 65) throw SignatureError("sig size");

        xonly_pubkey pk = move(keyid);
        signature sig = move(witness[0]);

        VerifyTxSignature(pk, sig, tx, nin, move(spent_outputs), {});

    }
    else if (witver == 0) {
        throw std::runtime_error("not implemented");
    }
}

std::string ContractBuilder::GetNewInputMiningFee()
{
    return FormatAmount(CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_KEYSPEND_VIN_VSIZE));
}

std::string ContractBuilder::GetNewOutputMiningFee()
{
    return FormatAmount(CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_VOUT_VSIZE));
}

void ContractBuilder::DeserializeContractAmount(const UniValue &val, std::optional<CAmount> &target, std::function<std::string()> lazy_name)
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

void ContractBuilder::DeserializeContractString(const UniValue& val, std::optional<std::string> &target, std::function<std::string()> lazy_name)
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

void ContractBuilder::DeserializeContractTransfer_w_pubkey(const UniValue& val, std::optional<Transfer>& transfer, std::function<std::string()> lazy_name)
{
    if (!val.isNull()) {
        if (!val.isObject()) throw ContractTermWrongFormat(lazy_name());

        const UniValue &val_txid = val[name_txid];
        const UniValue &val_nout = val[name_nout];
        const UniValue &val_amount = val[name_amount];
        const UniValue &val_pk = val[name_pk];
        const UniValue &val_sig = val[name_sig];

        if (val_txid.isNull())
            throw ContractTermMissing(move((lazy_name() += '.') += name_txid));
        if (!val_txid.isStr())
            throw ContractTermWrongValue(move((((lazy_name() += '.') += name_txid) += ": ") += val_txid.getValStr()));
        if (val_nout.isNull())
            throw ContractTermMissing(move((lazy_name() += '.') += name_nout));
        if (!val_nout.isNum())
            throw ContractTermWrongValue(move((((lazy_name() += '.') += name_nout) += ": ") += val_nout.getValStr()));

        std::optional<CAmount> amount;
        DeserializeContractAmount(val_amount, amount, [&]() { return (lazy_name() += '.') += name_amount; });

        if (!amount)
            throw ContractTermMissing(move((lazy_name() += '.') += name_amount));

        Transfer res = {val[name_txid].get_str(), val[name_nout].getInt<uint32_t>(), *amount};
        if (transfer) {
            if (transfer->m_txid != res.m_txid) throw ContractTermMismatch(move((lazy_name() += '.') += name_txid));
            if (transfer->m_nout != res.m_nout) throw ContractTermMismatch(move((lazy_name() += '.') += name_nout));
            if (transfer->m_amount != res.m_amount) throw ContractTermMismatch(move((lazy_name() += '.') += name_nout));

            DeserializeContractTaprootPubkey(val_pk, transfer->m_addr, [&]() { return (lazy_name() += '.') += name_pk; });
            DeserializeContractHexData(val_sig, transfer->m_sig, [&]() { return (lazy_name() += '.') += name_sig; });
        }
        else {
            DeserializeContractTaprootPubkey(val_pk, res.m_addr, [&]() { return (lazy_name() += '.') += name_pk; });
            DeserializeContractHexData(val_sig, res.m_sig, [&]() { return (lazy_name() += '.') += name_sig; });
            transfer.emplace(move(res));
        }
    }
}

void ContractBuilder::DeserializeContractTransfer(const UniValue& val, std::optional<Transfer>& transfer, std::function<std::string()> lazy_name)
{
    if (!val.isNull()) {
        if (!val.isObject()) throw ContractTermWrongFormat(lazy_name());

        const UniValue &val_txid = val[name_txid];
        const UniValue &val_nout = val[name_nout];
        const UniValue &val_amount = val[name_amount];
        const UniValue &val_addr = val[name_addr];
        const UniValue &val_sig = val[name_sig];

        if (val_txid.isNull())
            throw ContractTermMissing(move((lazy_name() += '.') += name_txid));
        if (!val_txid.isStr())
            throw ContractTermWrongValue(move((((lazy_name() += '.') += name_txid) += ": ") += val_txid.getValStr()));
        if (val_nout.isNull())
            throw ContractTermMissing(move((lazy_name() += '.') += name_nout));
        if (!val_nout.isNum())
            throw ContractTermWrongValue(move((((lazy_name() += '.') += name_nout) += ": ") += val_nout.getValStr()));

        std::optional<CAmount> amount;
        DeserializeContractAmount(val_amount, amount, [&]() { return (lazy_name() += '.') += name_amount; });

        if (!amount)
            throw ContractTermMissing(move((lazy_name() += '.') += name_amount));

        Transfer res = {val[name_txid].get_str(), val[name_nout].getInt<uint32_t>(), *amount};
        if (transfer) {
            if (transfer->m_txid != res.m_txid) throw ContractTermMismatch(move((lazy_name() += '.') += name_txid));
            if (transfer->m_nout != res.m_nout) throw ContractTermMismatch(move((lazy_name() += '.') += name_nout));
            if (transfer->m_amount != res.m_amount) throw ContractTermMismatch(move((lazy_name() += '.') += name_nout));

            DeserializeContractString(val_addr, transfer->m_addr, [&]() { return (lazy_name() += '.') += name_addr; });
            if (transfer->m_addr) bech32().Decode(*transfer->m_addr);

            DeserializeContractHexData(val_sig, transfer->m_sig, [&]() { return (lazy_name() += '.') += name_sig; });
        }
        else {
            DeserializeContractString(val_addr, res.m_addr, [&]() { return (lazy_name() += '.') += name_addr; });
            if (res.m_addr) bech32().Decode(*res.m_addr);

            DeserializeContractHexData(val_sig, res.m_sig, [&]() { return (lazy_name() += '.') += name_sig; });

            transfer.emplace(move(res));
        }
    }
    else if (transfer) {
        throw ContractTermMissing(lazy_name());
    }
}

std::shared_ptr<IContractDestination> ContractBuilder::ReadContractDestination(const UniValue & out) const
{
    if (!out.isObject()) {
        throw ContractTermWrongFormat("not a object");
    }

    {   const UniValue& type = out[IJsonSerializable::name_type];
        if (type.isNull()) throw ContractTermMissing(std::string(IJsonSerializable::name_type));
        if (!type.isStr()) throw ContractTermWrongFormat(std::string(IJsonSerializable::name_type));

        if (type.getValStr() == P2Witness::type) {
            return P2Witness::ReadJson(mBech, out);
        }
        // TODO: Other types like p2pkh will be here
        else throw ContractTermWrongValue("destination: " + type.getValStr());
    }
}

void ContractBuilder::DeserializeContractTaprootPubkey(const UniValue &val, std::optional<std::string> &addr, std::function<std::string()> lazy_name) {
    if (!val.isNull()) {
        std::string str;
        try {
            str = bech32().Encode(unhex<xonly_pubkey>(val.get_str()));
        }
        catch (...) {
            std::throw_with_nested(ContractTermWrongValue(lazy_name()));
        }

        if (addr) {
            if (*addr != str) throw ContractTermMismatch(move((lazy_name() += " is already set: ") += *addr));
        } else {
            addr = move(str);
        }
    }
}


} // utxord
