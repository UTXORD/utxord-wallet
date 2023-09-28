#include "interpreter.h"
#include "feerate.h"
#include "script_merkle_tree.hpp"
#include "channel_keys.hpp"
#include "contract_builder.hpp"
#include "simple_transaction.hpp"

#include <execution>
#include <atomic>
#include <limits>

namespace utxord {

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
        throw ContractTermWrongValue("witness");
    }

    size_t i = 0;
    for (const auto& v: stack.getValues()) {
        if (!v.isStr()) {
            throw ContractTermWrongValue("witness[" + std::to_string(i) + ']');
        }

        m_stack.emplace_back(unhex<bytevector>(v.get_str()));
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string ContractInput::name_witness = "witness";

UniValue ContractInput::MakeJson() const
{
    // Do not serialize underlying contract as transaction input: copy it as UTXO and write UTXO related values only
    // Lazy mode copy of an UTXO state is used to allow early-set of not completed contract as the input at any time
    UTXO utxo_out(*output);
    UniValue json = utxo_out.MakeJson();
    json.pushKV(name_witness, witness.MakeJson());

    return json;
}

void ContractInput::ReadJson(const UniValue &json)
{
    output = std::make_shared<UTXO>(json);

    {   const UniValue& val = json[name_witness];
        if (val.isNull()) throw ContractTermMissing(name_witness.c_str());
        if (!val.isArray()) throw ContractTermWrongValue(name_witness.c_str());
        witness.ReadJson(val);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

const std::string P2TR::name_amount = "amount";
const std::string P2TR::name_pk = "pubkey";

const char* P2TR::type = "p2tr";

UniValue P2TR::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, type);
    res.pushKV(name_amount, m_amount);
    res.pushKV(name_pk, hex(m_pk));

    return res;
}

void P2TR::ReadJson(const UniValue &json)
{
    if (json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }
    m_amount = json[name_amount].getInt<CAmount>();
    m_pk = unhex<xonly_pubkey>(json[name_pk].get_str());
}

/*--------------------------------------------------------------------------------------------------------------------*/

core::ChannelKeys P2TR::LookupKeyPair(const core::MasterKey &masterKey, utxord::OutputType outType) const
{
    core::MasterKey masterCopy(masterKey);
    masterCopy.DeriveSelf(core::MasterKey::BIP32_HARDENED_KEY_LIMIT + core::MasterKey::BIP86_TAPROOT_ACCOUNT);
    masterCopy.DeriveSelf(core::MasterKey::BIP32_HARDENED_KEY_LIMIT);

    switch (outType) {
    case TAPROOT_DEFAULT:
    case TAPROOT_DEFAULT_SCRIPT:
        masterCopy.DeriveSelf(core::MasterKey::BIP32_HARDENED_KEY_LIMIT);
        break;
    case TAPROOT_OUTPUT:
    case TAPROOT_SCRIPT:
        masterCopy.DeriveSelf(core::MasterKey::BIP32_HARDENED_KEY_LIMIT + 1);
        break;
    case INSCRIPTION_OUTPUT:
        masterCopy.DeriveSelf(core::MasterKey::BIP32_HARDENED_KEY_LIMIT + 2);
        break;
    }

    masterCopy.DeriveSelf(0);

#ifdef _LIBCPP_HAS_PARALLEL_ALGORITHMS
    std::atomic_uint32_t res_index = std::numeric_limits<uint32_t>::max();
    const uint32_t step = 64;
    uint32_t indexes[step];
    for (uint32_t key_index = 0; key_index < 65536/*core::MasterKey::BIP32_HARDENED_KEY_LIMIT*/; key_index += step) {
        std::iota(indexes, indexes + step, key_index);
        std::for_each(std::execution::par_unseq, indexes, indexes+step, [&](const auto& k){
            core::ChannelKeys keypair = masterCopy.Derive(std::vector<uint32_t >{k}, (outType == TAPROOT_DEFAULT_SCRIPT || outType == TAPROOT_SCRIPT) ? core::SUPPRESS : core::FORCE);
            if (keypair.GetLocalPubKey() == m_pk) {
                res_index = k;
            }
        });
        if (res_index != std::numeric_limits<uint32_t>::max()) {
            return masterCopy.Derive(std::vector<uint32_t >{res_index}, (outType == TAPROOT_DEFAULT_SCRIPT || outType == TAPROOT_SCRIPT) ? core::SUPPRESS : core::FORCE);
        }
    }
#else
    for (uint32_t key_index = 0; key_index < 65536/*core::MasterKey::BIP32_HARDENED_KEY_LIMIT*/; ++key_index) {
        core::ChannelKeys keypair = masterCopy.Derive(std::vector<uint32_t >{key_index}, (outType == TAPROOT_DEFAULT_SCRIPT || outType == TAPROOT_SCRIPT) ? core::SUPPRESS : core::FORCE);
        if (keypair.GetLocalPubKey() == m_pk) {
            return keypair;
        }
    }
#endif
    throw KeyError("derivation lookup");
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
        m_destination = std::make_shared<P2TR>();
        m_destination->ReadJson(dest);
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

ContractBuilder::ContractBuilder(IBech32::ChainMode chain_mode) {
    switch (chain_mode) {
    case IBech32::REGTEST:
        mBech.reset(new Bech32<IBech32::REGTEST>());
        break;
    case IBech32::TESTNET:
        mBech.reset(new Bech32<IBech32::TESTNET>());
        break;
    case IBech32::MAINNET:
        mBech.reset(new Bech32<IBech32::MAINNET>());
        break;
    default:
        throw std::runtime_error("Wrong chain mode: " + std::to_string(chain_mode));
    }
}


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
        execdata.m_tapleaf_hash = TapLeafHash(spend_script);
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
    if (!pk.verify(core::ChannelKeys::GetStaticSecp256k1Context(), sig, sighash)) {
        throw SignatureError("sig");
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

std::optional<Transfer> ContractBuilder::DeserializeContractTransfer(const UniValue& val, std::function<std::string()> lazy_name)
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

        DeserializeContractTaprootPubkey(val_pk, res.m_addr, [&]() { return (lazy_name() += '.') += name_pk; });
        DeserializeContractHexData(val_sig, res.m_sig, [&]() { return (lazy_name() += '.') += name_sig; });

        return res;
    }
    return {};
}

std::shared_ptr<IContractDestination> ContractBuilder::ReadContractDestination(const UniValue & out) const
{
    if (!out.isObject()) {
        throw ContractTermWrongFormat("not a object");
    }

    {   const UniValue& type = out[IJsonSerializable::name_type];
        if (type.isNull()) throw ContractTermMissing(std::string(IJsonSerializable::name_type));
        if (!type.isStr()) throw ContractTermWrongFormat(std::string(IJsonSerializable::name_type));

        if (type.getValStr() == P2TR::type) {
            return std::make_shared<P2TR>(out);
        }
        // TODO: Other types like p2wpkh will be here
        else throw ContractTermWrongValue("destination: " + type.getValStr());
    }
}

void ContractBuilder::DeserializeContractTaprootPubkey(const UniValue &val, std::optional<std::string> &addr, std::function<std::string()> lazy_name) {
    if (!val.isNull()) {
        std::string str;
        try {
            str = mBech->Encode(unhex<xonly_pubkey>(val.get_str()));
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
