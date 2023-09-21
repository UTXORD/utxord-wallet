#include "contract_builder.hpp"
#include "interpreter.h"
#include "feerate.h"
#include "script_merkle_tree.hpp"
#include "channel_keys.hpp"

namespace utxord {

const std::string ContractBuilder::name_contract_type = "contract_type";
const std::string ContractBuilder::name_params = "params";
const std::string ContractBuilder::name_version = "protocol_version";
const std::string ContractBuilder::name_mining_fee_rate = "mining_fee_rate";
const std::string ContractBuilder::name_txid = "txid";
const std::string ContractBuilder::name_nout = "nout";
const std::string ContractBuilder::name_amount = "amount";
const std::string ContractBuilder::name_pk = "pubkey";
const std::string ContractBuilder::name_sig = "sig";
const std::string ContractBuilder::name_market_fee = "market_fee";

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

        DeserializeContractHexData(val_pk, res.m_pubkey, [&]() { return (lazy_name() += '.') += name_pk; });
        DeserializeContractHexData(val_sig, res.m_sig, [&]() { return (lazy_name() += '.') += name_sig; });

        return res;
    }
    return {};
}


} // utxord
