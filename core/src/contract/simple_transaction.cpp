
#include "univalue.h"

#include "core_io.h"

#include "utils.hpp"
#include "simple_transaction.hpp"

namespace utxord {

const  std::string SimpleTransaction::name_outputs = "outputs";

const char* const SimpleTransaction::type = "transaction";
const uint32_t SimpleTransaction::m_protocol_version = 1;

CMutableTransaction SimpleTransaction::MakeTx() const
{
    CMutableTransaction tx;

    for(const auto& input: m_inputs) {
        tx.vin.emplace_back(uint256S(input.output->TxID()), input.output->NOut());
        tx.vin.back().scriptWitness.stack = input.witness;
        if (tx.vin.back().scriptWitness.stack.empty()) {
            tx.vin.back().scriptWitness.stack = input.output->Destination()->DummyWitness();
        }
    }

    for(const auto& out: m_outputs) {
        tx.vout.emplace_back(out->Amount(), out->PubKeyScript());
    }

    return tx;
}

void SimpleTransaction::AddChangeOutput(const std::string& addr)
{
    if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));

    unsigned v;
    bytevector arg;
    std::tie(v, arg) = bech32().Decode(addr);
    if (v == 0) {
        AddOutput(std::make_shared<P2WPKH>(bech32().GetChainMode(), 0, addr));
    }
    else if (v == 1) {
        AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), 0, addr));
    }
    else {
        throw ContractTermWrongValue("");
    }

    CAmount required = l15::ParseAmount(GetMinFundingAmount(""));

    CAmount total = std::accumulate(m_inputs.begin(), m_inputs.end(), 0, [](CAmount s, const auto& in) { return s + in.output->Destination()->Amount(); });

    if (required > total) throw ContractStateError("inputs too small");
    CAmount change = total - required;

    if (change > l15::Dust(*m_mining_fee_rate)) {
        m_outputs.back()->Amount(change);
    }
    else {
       m_outputs.pop_back();
    }
}

void SimpleTransaction::Sign(const KeyRegistry &master_key, const std::string key_filter_tag)
{
    CMutableTransaction tx = MakeTx();

    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    for(auto& input: m_inputs) {
        const auto& dest = input.output->Destination();
        spent_outs.emplace_back(dest->Amount(), dest->PubKeyScript());
    }

    for(auto& input: m_inputs) {
        auto& dest = input.output->Destination();
        auto keypair = dest->LookupKey(master_key, key_filter_tag);

        std::vector<bytevector> stack = keypair->Sign(tx, input.nin, spent_outs, SIGHASH_ALL);

        for (size_t i = 0; i < stack.size(); ++i) {
            input.witness.Set(i, move(stack[i]));
        }
    }
}

std::vector<std::string> SimpleTransaction::RawTransactions() const
{
    return { EncodeHexTx(CTransaction(MakeTx())) };
}

UniValue SimpleTransaction::MakeJson() const
{
    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_type, type);
    contract.pushKV(name_version, (int)m_protocol_version);
    contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);

    UniValue utxo_arr(UniValue::VARR);
    for (const auto& input: m_inputs) {
        UniValue spend = input.MakeJson();
        utxo_arr.push_back(move(spend));
    }
    contract.pushKV(name_utxo, move(utxo_arr));

    UniValue out_arr(UniValue::VARR);
    for (const auto& out: m_outputs) {
        UniValue dest = out->MakeJson();
        out_arr.push_back(move(dest));
    }
    contract.pushKV(name_outputs, move(out_arr));
    return contract;
}

void SimpleTransaction::ReadJson(const UniValue &contract)
{
    if (!contract.isObject()) {
        throw ContractTermWrongFormat("not an object");
    }
    if (contract[name_type].get_str() != type) {
        throw ContractProtocolError("transaction contract does not match " + contract[name_type].getValStr());
    }
    if (contract[name_version].getInt<int>() != m_protocol_version) {
        throw ContractProtocolError(std::string("Wrong ") + type + " version: " + contract[name_version].getValStr());
    }

    {   const auto &val = contract[name_mining_fee_rate];
        if (val.isNull()) throw ContractTermMissing(std::string(name_mining_fee_rate));
        m_mining_fee_rate = val.getInt<int64_t>();
    }

    {   const auto &val = contract[name_utxo];

        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_utxo));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));

        for (const UniValue &input: val.getValues()) {
            m_inputs.emplace_back(bech32(), m_inputs.size(), input);
        }
    }

    {   const auto &val = contract[name_outputs];
        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_outputs));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_outputs));

        for (const UniValue &out: val.getValues()) {
            m_outputs.emplace_back(ReadContractDestination(out));
        }
    }
}

std::string SimpleTransaction::GetMinFundingAmount(const std::string& params) const
{
    CAmount total_out = std::accumulate(m_outputs.begin(), m_outputs.end(), 0, [](CAmount s, const auto& d) { return s + d->Amount(); });
    return l15::FormatAmount(l15::CalculateTxFee(*m_mining_fee_rate, MakeTx()) + total_out);
}

} // l15::utxord

