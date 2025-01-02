
#include <algorithm>

#include <smartinserter.hpp>

#include "univalue.h"

#include "policy.h"

#include "utils.hpp"

#include "simple_transaction.hpp"
#include "contract_builder_factory.hpp"
#include "runes.hpp"

namespace utxord {

namespace {

const std::string val_simple_transaction = "transaction";

const char* TX_TERMS_STR = "TX_TERMS";
const char* TX_SIGNATURE_STR = "TX_SIGNATURE";

}

const  std::string SimpleTransaction::name_outputs = "outputs";
const  std::string SimpleTransaction::name_rune_inputs = "rune_inputs";

const uint32_t SimpleTransaction::s_protocol_version = 4;
const uint32_t SimpleTransaction::s_protocol_version_no_p2address = 3;
const uint32_t SimpleTransaction::s_protocol_version_no_rune_transfer = 2;
const char* SimpleTransaction::s_versions = "[2,3,4]";


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
        tx.vout.emplace_back(out->TxOutput());
    }

    return tx;
}

void SimpleTransaction::AddRuneInput(std::shared_ptr<IContractOutput> prevout, RuneId runeid, uint128_t rune_amount)
{
    AddInput(move(prevout));
    m_rune_inputs.emplace(move(runeid), std::make_tuple(move(rune_amount), m_inputs.size()));
}

void SimpleTransaction::AddRuneUTXO(std::string txid, uint32_t nout, CAmount btc_amount, std::string addr, RuneId runeid, uint128_t rune_amount)
{
    AddRuneInput(std::make_shared<UTXO>(chain(), move(txid), nout, btc_amount, move(addr)), move(runeid), move(rune_amount));
}

void SimpleTransaction::AddRuneOutput(CAmount btc_amount, std::string addr, RuneId runeid, uint128_t rune_amount)
{
    AddRuneOutputDestination(P2Address::Construct(chain(), btc_amount, addr), move(runeid), move(rune_amount));
}

void SimpleTransaction::AddRuneOutputDestination(std::shared_ptr<IContractDestination> destination, RuneId runeid, uint128_t rune_amount)
{
    AddOutputDestination(move(destination));

    uint32_t transfer_nout = m_outputs.size() -1;

    std::shared_ptr<RuneStoneDestination> rune_stone;
    if (m_runestone_nout) {
        rune_stone = std::dynamic_pointer_cast<RuneStoneDestination>(m_outputs[*m_runestone_nout]);
        if (!rune_stone) throw ContractTermMismatch("Not RuneStone output: " + std::to_string(*m_runestone_nout));
    }
    else {
        rune_stone = std::make_shared<RuneStoneDestination>(chain());
        m_outputs.push_back(rune_stone);
        m_runestone_nout = m_outputs.size() - 1;
    }

    rune_stone->op_dictionary.emplace(move(runeid), std::make_tuple(move(rune_amount), transfer_nout));
}

void SimpleTransaction::BurnRune(RuneId runeid, uint128_t rune_amount)
{
    std::shared_ptr<RuneStoneDestination> rune_stone;
    if (m_runestone_nout) {
        rune_stone = std::dynamic_pointer_cast<RuneStoneDestination>(m_outputs[*m_runestone_nout]);
        if (!rune_stone) throw ContractStateError("Not a RuneStone output: " + std::to_string(*m_runestone_nout));
    }
    else {
        rune_stone = std::make_shared<RuneStoneDestination>(chain());
        m_outputs.push_back(rune_stone);
        m_runestone_nout = m_outputs.size() - 1;
    }

    rune_stone->op_dictionary.emplace(move(runeid), std::make_tuple(move(rune_amount), *m_runestone_nout));
}

void SimpleTransaction::AddChangeOutput(std::string addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");

    if (m_change_nout) {
        m_outputs.erase(m_outputs.begin() + *m_change_nout);
        m_change_nout.reset();
    }

    AddOutputDestination(P2Address::Construct(chain(), {}, move(addr)));

    CAmount required = GetMinFundingAmount("");

    CAmount total = std::accumulate(m_inputs.begin(), m_inputs.end(), 0, [](CAmount s, const auto& in) { return s + in.output->Destination()->Amount(); });

    if (total >= required) {
        m_outputs.back()->Amount(m_outputs.back()->Amount() + total - required);
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


void SimpleTransaction::PartialSign(const KeyRegistry &master_key, const string &key_filter_tag, uint32_t nin) {
    CMutableTransaction tx = MakeTx("");

    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    for(const auto& input: m_inputs) {
        spent_outs.emplace_back(input.output->Destination()->TxOutput());
    }

    auto inputIt = std::find_if(m_inputs.begin(), m_inputs.end(),[nin](const auto& in){ return in.nin == nin; });
    if (inputIt == m_inputs.end()) throw ContractTermMissing(name_utxo + '[' + std::to_string(nin) + ']');

    auto dest = inputIt->output->Destination();
    auto keypair = dest->LookupKey(master_key, key_filter_tag);

    std::vector<bytevector> stack = keypair->Sign(tx, nin, spent_outs, SIGHASH_ALL);

    for (size_t i = 0; i < stack.size(); ++i) {
        inputIt->witness.Set(i, move(stack[i]));
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
        spent_outs.emplace_back(input.output->Destination()->TxOutput());
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
    return {l15::EncodeHexTx(MakeTx("")) };
}

UniValue SimpleTransaction::MakeJson(uint32_t version, TxPhase phase) const
{
    if (version != s_protocol_version &&
        version != s_protocol_version_no_p2address &&
        version != s_protocol_version_no_rune_transfer) throw ContractProtocolError("Wrong contract version: " + std::to_string(version) + ". Allowed are " + s_versions);

    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, version);
    contract.pushKV(name_contract_phase, PhaseString(phase));
    contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);

    UniValue utxo_arr(UniValue::VARR);
    for (const auto& input: m_inputs) {
        if (input.output->Destination()->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
            throw ContractProtocolError(name_utxo + '[' + std::to_string(input.nin) + "]." + IContractDestination::name_addr + ": " + input.output->Destination()->Address() + " is not supported with v. " + std::to_string(version));
        UniValue spend = input.MakeJson();
        utxo_arr.push_back(move(spend));
    }
    contract.pushKV(name_utxo, move(utxo_arr));

    if (!m_rune_inputs.empty()) {
        if (version <= s_protocol_version_no_rune_transfer) throw ContractProtocolError(name_rune_inputs + " is not supported with v. " + std::to_string(version));
        contract.pushKV(name_rune_inputs, RuneStoneDestination::MakeOpDictionaryJson(m_rune_inputs));
    }

    UniValue out_arr(UniValue::VARR);
    for (const auto& out: m_outputs) {
        if (out->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
            throw ContractProtocolError(name_outputs + "[...]." + IContractDestination::name_addr + ": " + out->Address() + " is not supported with v. " + std::to_string(version));
        UniValue dest = out->MakeJson();
        out_arr.push_back(move(dest));
    }
    contract.pushKV(name_outputs, move(out_arr));
    return contract;
}

void SimpleTransaction::ReadJson(const UniValue& contract, TxPhase phase)
{
    uint32_t version = contract[name_version].getInt<uint32_t>();
    if (version != s_protocol_version &&
        version != s_protocol_version_no_p2address &&
        version != s_protocol_version_no_rune_transfer)
        throw ContractProtocolError("Wrong " + val_simple_transaction + " contract version: " + contract[name_version].getValStr());

    {   const auto &val = contract[name_mining_fee_rate];
        if (val.isNull()) throw ContractTermMissing(std::string(name_mining_fee_rate));
        m_mining_fee_rate = val.getInt<int64_t>();
    }

    {   const auto &val = contract[name_utxo];

        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_utxo));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));
        if (!m_inputs.empty() && m_inputs.size() != val.size()) throw ContractTermMismatch(name_utxo + " size");

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

    if (version > s_protocol_version_no_rune_transfer) {
        const auto &val = contract[name_rune_inputs];

        if (!(val.isNull() || val.empty())) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_rune_inputs));
            if (!m_rune_inputs.empty() && m_rune_inputs.size() != val.size()) throw ContractTermMismatch(std::string(name_rune_inputs) + " size");

            RuneStoneDestination::ReadOpDictionaryJson(val, m_rune_inputs, []{ return name_rune_inputs; });
        }
    }

    {   const auto &val = contract[name_outputs];
        if (val.isNull() || val.empty()) throw ContractTermMissing(std::string(name_outputs));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_outputs));
        if (!m_outputs.empty() && m_outputs.size() != val.size()) throw ContractTermMismatch(std::string(name_outputs) + " size");

        for (size_t i = 0; i < val.size(); ++i) {
            if (i == m_outputs.size()) {
                m_outputs.emplace_back(NoZeroDestinationFactory::ReadJson(chain(), val[i], [i](){ return (std::ostringstream() << name_outputs << '[' << i << ']').str();}));
                if (std::dynamic_pointer_cast<RuneStoneDestination>(m_outputs.back())) m_runestone_nout = m_outputs.size() - 1;
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

void SimpleTransaction::CheckContractTerms(uint32_t version, TxPhase phase) const
{
#ifndef DEBUG
    using int136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 136, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>;
#else
    using int136_t = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::cpp_int_backend<128, 136, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>>;
#endif

    if (m_runestone_nout || !m_rune_inputs.empty()) {
        std::shared_ptr<RuneStoneDestination> rune_stone;
        if (m_runestone_nout) {
            rune_stone = std::dynamic_pointer_cast<RuneStoneDestination>(m_outputs[*m_runestone_nout]);
            if (!rune_stone) throw ContractStateError("Not RuneStone output: " + std::to_string(*m_runestone_nout));
        }
        else throw ContractStateError("RuneStone");

        std::map<RuneId, int136_t> balances;

        std::for_each(m_rune_inputs.begin(), m_rune_inputs.end(), [&balances](const auto& rune_in){ balances[rune_in.first] += get<0>(rune_in.second); });
        std::for_each(rune_stone->op_dictionary.begin(), rune_stone->op_dictionary.end(), [&balances](const auto& rune_out) { balances[rune_out.first] -= get<0>(rune_out.second); });
        for (const auto& bal: balances) {
            if (bal.second != 0) throw ContractTermWrongValue((std::ostringstream() << "Non zero rune balance " << (std::string)bal.first << ": " << bal.second).str());
        }
    }

    if (phase == TX_SIGNATURE)
        CheckSig();
}

const char * SimpleTransaction::PhaseString(TxPhase phase)
{
    switch (phase) {
    case TX_TERMS:
        return TX_TERMS_STR;
    case TX_SIGNATURE:
        return TX_SIGNATURE_STR;
    }
    throw ContractTermWrongValue("TxPhase: " + std::to_string(phase));
}

TxPhase SimpleTransaction::ParsePhase(const std::string& str)
{
    if (str == TX_TERMS_STR) return TX_TERMS;
    if (str == TX_SIGNATURE_STR) return TX_SIGNATURE;
    throw ContractTermWrongValue(std::string(str));
}

CAmount SimpleTransaction::CalculateWholeFee(const string &params) const
{
    return l15::CalculateTxFee(*m_mining_fee_rate, MakeTx(params));
}

void SimpleTransaction::CheckSig() const
{
    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    std::transform(m_inputs.begin(), m_inputs.end(), cex::smartinserter(spent_outs, spent_outs.end()), [](const auto& txin){ return txin.output->Destination()->TxOutput(); });

    CMutableTransaction tx = MakeTx("");
    for (const auto &input: m_inputs) {
        VerifyTxSignature(input.output->Destination()->Address(), input.witness, tx, input.nin, spent_outs);
    }
}

} // l15::utxord

