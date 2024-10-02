
#include "univalue.h"

#include "policy.h"
#include "core_io.h"

#include "utils.hpp"

#include "simple_transaction.hpp"
#include "contract_builder_factory.hpp"

namespace utxord {

namespace {

const std::string val_simple_transaction = "transaction";

}

const  std::string SimpleTransaction::name_outputs = "outputs";

const uint32_t SimpleTransaction::s_protocol_version = 2;
const char* SimpleTransaction::s_versions = "[2]";


const std::string& SimpleTransaction::GetContractName() const
{ return val_simple_transaction; }

CMutableTransaction SimpleTransaction::MakeTx(const std::string& params) const
{
    CMutableTransaction tx;

    if (m_inputs.empty()) {
        tx.vin.emplace_back(Txid(), 0);
        if (params.find("p2wpkh_utxo"))
            tx.vin.back().scriptWitness.stack = P2WPKH(chain(), 0, "").DummyWitness();
        else
            tx.vin.back().scriptWitness.stack = P2TR(chain(), 0, "").DummyWitness();
    }
    else {
        for (const auto &input: m_inputs) {
            tx.vin.emplace_back(Txid::FromUint256(uint256S(input.output->TxID())), input.output->NOut());
            tx.vin.back().scriptWitness.stack = input.witness;
            if (tx.vin.back().scriptWitness.stack.empty()) {
                tx.vin.back().scriptWitness.stack = input.output->Destination()->DummyWitness();
            }
        }
    }

    for(const auto& out: m_outputs) {
        tx.vout.emplace_back(out->Amount(), out->PubKeyScript());
    }

    return tx;
}

void SimpleTransaction::AddChangeOutput(std::string addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");

    if (m_change_nout) {
        m_outputs.erase(m_outputs.begin() + *m_change_nout);
        m_change_nout.reset();
    }

    unsigned v;
    bytevector arg;
    std::tie(v, arg) = bech32().Decode(addr);
    if (v == 0) {
        AddOutput(std::make_shared<P2WPKH>(bech32().GetChainMode(), 0, move(addr)));
    }
    else if (v == 1) {
        AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), 0, move(addr)));
    }
    else {
        throw ContractTermWrongValue(name_change_addr.c_str());
    }

    CAmount required = GetMinFundingAmount("");

    CAmount total = std::accumulate(m_inputs.begin(), m_inputs.end(), 0, [](CAmount s, const auto& in) { return s + in.output->Destination()->Amount(); });

    //if (required > total) throw ContractStateError("inputs too small");
    CAmount change = total - required;

    if (change >= l15::Dust(DUST_RELAY_TX_FEE)) {
        m_outputs.back()->Amount(change);
        m_change_nout = m_outputs.size() - 1;
    }
    else {
       m_outputs.pop_back();
    }
}

void SimpleTransaction::DropChangeOutput()
{
    if (m_change_nout) {
        m_outputs.erase(m_outputs.begin() + *m_change_nout);
        m_change_nout.reset();
    }
}


void SimpleTransaction::Sign(const KeyRegistry &master_key, const std::string& key_filter_tag)
{
    if (m_inputs.empty()) throw ContractStateError(std::string(name_utxo) + " not defined");
    if (m_outputs.empty()) throw ContractStateError(name_outputs + " not defined");

    CMutableTransaction tx = MakeTx("");

    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    for(const auto& input: m_inputs) {
        const auto& dest = input.output->Destination();
        spent_outs.emplace_back(dest->Amount(), dest->PubKeyScript());
    }

    for(auto& input: m_inputs) {
        auto dest = input.output->Destination();
        auto keypair = dest->LookupKey(master_key, key_filter_tag);

        std::vector<bytevector> stack = keypair->Sign(tx, input.nin, spent_outs, SIGHASH_ALL);

        for (size_t i = 0; i < stack.size(); ++i) {
            input.witness.Set(i, move(stack[i]));
        }
    }
}

std::vector<std::string> SimpleTransaction::RawTransactions() const
{
    return { EncodeHexTx(CTransaction(MakeTx(""))) };
}

UniValue SimpleTransaction::MakeJson(uint32_t version, TxPhase phase) const
{
    if (version != s_protocol_version) throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, (int)s_protocol_version);
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

void SimpleTransaction::ReadJson(const UniValue& contract, TxPhase phase)
{
    if (contract[name_version].getInt<int>() != s_protocol_version)
        throw ContractProtocolError("Wrong " + val_simple_transaction + " contract version: " + contract[name_version].getValStr());

    {   const auto &val = contract[name_mining_fee_rate];
        if (val.isNull()) throw ContractTermMissing(std::string(name_mining_fee_rate));
        m_mining_fee_rate = val.getInt<int64_t>();
    }

    {   const auto &val = contract[name_utxo];

        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_utxo));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));
        if (!m_inputs.empty() && m_inputs.size() != val.size()) throw ContractTermMismatch(std::string(name_utxo) + " size");

        m_inputs.reserve(val.size());

        for (size_t i = 0; i < val.size(); ++i) {
            if (i == m_inputs.size()) {
                m_inputs.emplace_back(chain(), m_inputs.size(), val[i], [i](){ return (std::ostringstream() << name_utxo << '[' << i << ']').str();});
            }
            else {
                m_inputs[i].ReadJson(val[i], [i](){ return (std::ostringstream() << name_utxo << '[' << i << ']').str();});
            }
        }
    }

    {   const auto &val = contract[name_outputs];
        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_outputs));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_outputs));
        if (!m_outputs.empty() && m_outputs.size() != val.size()) throw ContractTermMismatch(std::string(name_outputs) + " size");

        for (size_t i = 0; i < val.size(); ++i) {
            if (i == m_outputs.size()) {
                m_outputs.emplace_back(NoZeroDestinationFactory::ReadJson(chain(), val[i], [i](){ return (std::ostringstream() << name_outputs << '[' << i << ']').str();}));
            }
            else {
                m_outputs[i]->ReadJson(val[i], [i](){ return (std::ostringstream() << name_outputs << '[' << i << ']').str();});
            }
        }
    }
}

CAmount SimpleTransaction::GetMinFundingAmount(const std::string& params) const
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");

    CAmount total_out = std::accumulate(m_outputs.begin(), m_outputs.end(), 0, [](CAmount s, const auto& d) { return s + d->Amount(); });
    return l15::CalculateTxFee(*m_mining_fee_rate, MakeTx(params)) + total_out;
}

void SimpleTransaction::CheckContractTerms(TxPhase phase) const
{
    if (phase == TX_SIGNATURE)
        CheckSig();
}

CAmount SimpleTransaction::CalculateWholeFee(const string &params) const
{
    return l15::CalculateTxFee(*m_mining_fee_rate, MakeTx(params));
}

void SimpleTransaction::CheckSig() const
{
    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    std::transform(m_inputs.begin(), m_inputs.end(), cex::smartinserter(spent_outs, spent_outs.end()), [](const auto& txin){ return CTxOut(txin.output->Destination()->Amount(), txin.output->Destination()->PubKeyScript()); });

    CMutableTransaction tx = MakeTx("");
    for (const auto &input: m_inputs) {
        VerifyTxSignature(input.output->Destination()->Address(), input.witness, tx, input.nin, spent_outs);
    }
}

} // l15::utxord

