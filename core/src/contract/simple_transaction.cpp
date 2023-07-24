
#include "univalue.h"

#include "core_io.h"

#include "utils.hpp"
#include "simple_transaction.hpp"

namespace l15::utxord {

const  std::string SimpleTransaction::name_outputs = "outputs";

const char* const SimpleTransaction::type = "transaction";
const uint32_t SimpleTransaction::m_protocol_version = 1;

CMutableTransaction SimpleTransaction::MakeTx() const
{
    CMutableTransaction tx;

    for(const auto& input: m_inputs) {
        tx.vin.emplace_back(uint256S(input->TxID()), input->NOut());
        tx.vin.back().scriptWitness.stack = input->Destination()->Witness();
        if (tx.vin.back().scriptWitness.stack.empty()) {
            tx.vin.back().scriptWitness.stack.emplace_back(signature());
        }
    }

    for(const auto& out: m_outputs) {
        tx.vout.emplace_back(out->Amount(), out->DestinationPubKeyScript());
    }

    return tx;
}

void SimpleTransaction::AddChangeOutput(const l15::xonly_pubkey &pk)
{
    if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));

    AddOutput(std::make_shared<P2TR>(0, pk));
    CAmount required = GetMinFundingAmount("");

    CAmount total = std::accumulate(m_inputs.begin(), m_inputs.end(), 0, [](CAmount s, const auto& in) { return s + in->Destination()->Amount(); });

    if (required > total) throw ContractStateError("inputs too small");
    CAmount change = total - required;

    if (change > Dust(*m_mining_fee_rate)) {
        m_outputs.back()->Amount(change);
    }
    else {
       m_outputs.pop_back();
    };
}

void SimpleTransaction::Sign(const core::MasterKey &master_key)
{
    CMutableTransaction tx = MakeTx();

    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    for(auto& input: m_inputs) {
        const auto& dest = input->Destination();
        spent_outs.emplace_back(dest->Amount(), dest->DestinationPubKeyScript());
    }

    uint32_t nin = 0;
    for(auto& input: m_inputs) {
        auto& dest = input->Destination();
        try {
            core::ChannelKeys keypair = dest->LookupKeyPair(master_key, TAPROOT_OUTPUT);
            dest->Witness().Set(0, keypair.SignTaprootTx(tx, nin, spent_outs, {}));
        }
        catch(const KeyError& e) {
            core::ChannelKeys keypair = dest->LookupKeyPair(master_key, TAPROOT_DEFAULT);
            dest->Witness().Set(0, keypair.SignTaprootTx(tx, nin, spent_outs, {}));
        }
        ++nin;
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
        // Do not serialize underlying contract as transaction input: copy it as UTXO and write UTXO related values only
        // Lazy mode copy of an UTXO state is used to allow early-set of not completed contract as the input at any time
        UTXO utxo(*input);
        UniValue spend = utxo.MakeJson();
        utxo_arr.push_back(move(spend));
    }
    contract.pushKV(name_utxo, utxo_arr);

    UniValue out_arr(UniValue::VARR);
    for (const auto& out: m_outputs) {
        UniValue dest = out->MakeJson();
        out_arr.push_back(dest);
    }
    contract.pushKV(name_outputs, out_arr);
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

        for (const UniValue &utxo: val.getValues()) {
            m_inputs.emplace_back(std::make_shared<UTXO>(utxo));
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

CAmount SimpleTransaction::GetMinFundingAmount(const string &params) const
{
    CAmount total_out = std::accumulate(m_outputs.begin(), m_outputs.end(), 0, [](CAmount s, const auto& d) { return s + d->Amount(); });
    return l15::CalculateTxFee(*m_mining_fee_rate, MakeTx()) + total_out;
}

} // l15::utxord

