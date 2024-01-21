#include <ranges>

#include "swap_inscription.hpp"

#include "core_io.h"
#include "policy.h"
#include "feerate.h"

namespace utxord {

using l15::core::ChannelKeys;
using l15::ScriptMerkleTree;
using l15::TreeBalanceType;
using l15::ParseAmount;
using l15::FormatAmount;
using l15::CalculateOutputAmount;

namespace {

const std::string val_swap_inscription("SwapInscription");

}

const uint32_t SwapInscriptionBuilder::s_protocol_version = 6;
const char* SwapInscriptionBuilder::s_versions = "[6]";

const std::string SwapInscriptionBuilder::name_ord_price = "ord_price";
const std::string SwapInscriptionBuilder::name_ord_commit = "ord_commit";

const std::string SwapInscriptionBuilder::name_market_script_pk = "market_script_pk";
const std::string SwapInscriptionBuilder::name_ord_script_pk = "ord_script_pk";
const std::string SwapInscriptionBuilder::name_ord_int_pk = "ord_int_pk";

const std::string SwapInscriptionBuilder::name_ord_payoff_addr = "ord_payoff_addr";
const std::string SwapInscriptionBuilder::name_funds_payoff_addr = "funds_payoff_addr";

const std::string SwapInscriptionBuilder::name_funds = "funds";
const std::string SwapInscriptionBuilder::name_swap_inputs = "swap_inputs";

CScript SwapInscriptionBuilder::MakeMultiSigScript(const xonly_pubkey& pk1, const xonly_pubkey& pk2)
{
    CScript script;
    script << pk1 << OP_CHECKSIG;
    script << pk2 << OP_CHECKSIGADD;
    script << 2 << OP_NUMEQUAL;
    return script;
}

CScript SwapInscriptionBuilder::OrdSwapScript() const
{
    if (!m_ord_script_pk) throw ContractStateError(name_ord_script_pk + " not defined");
    if (!m_market_script_pk) throw ContractStateError(name_market_script_pk + " not defined");

    return MakeMultiSigScript(m_ord_script_pk.value(), m_market_script_pk.value());
}

std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> SwapInscriptionBuilder::OrdSwapTapRoot() const
{
    if (!m_ord_int_pk) throw ContractStateError(name_ord_int_pk + " not defined");

    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED, { OrdSwapScript() });
    return std::tuple_cat(ChannelKeys::AddTapTweak(m_ord_int_pk.value(), tap_tree.CalculateRoot()), std::make_tuple(tap_tree));
}


CMutableTransaction SwapInscriptionBuilder::MakeSwapTx() const
{
    CMutableTransaction swap_tx;
    swap_tx.vin.reserve(m_swap_inputs.size());

    for (const auto& input: m_swap_inputs) {
        swap_tx.vin.emplace_back(uint256S(input.output->TxID()), input.output->NOut());
        if (input.witness)
            swap_tx.vin.back().scriptWitness.stack = input.witness;
        else
            swap_tx.vin.back().scriptWitness.stack = input.output->Destination()->DummyWitness();
    }

    if (m_swap_inputs.size() == 1) {
        swap_tx.vout.emplace_back(*m_ord_price, bech32().PubKeyScript(*m_funds_payoff_addr));
    }
    else {
        swap_tx.vout.reserve(5);
        swap_tx.vout.emplace_back(m_swap_inputs[0].output->Destination()->Amount() + m_swap_inputs[1].output->Destination()->Amount(), CScript());
        swap_tx.vout.resize(3);
        if (m_market_fee->Amount() == swap_tx.vout.front().nValue) {
            swap_tx.vout[0].scriptPubKey = m_market_fee->PubKeyScript();
        }
        else {
            swap_tx.vout[0].scriptPubKey = bech32().PubKeyScript(*m_change_addr);
            if (m_market_fee->Amount() > 0) {
                swap_tx.vout.emplace_back(m_market_fee->Amount(), m_market_fee->PubKeyScript());
            }
        }
        swap_tx.vout[1].nValue = m_swap_inputs[2].output->Destination()->Amount(); // Ord amount remains the same
        swap_tx.vout[1].scriptPubKey = bech32().PubKeyScript(*m_ord_payoff_addr);

        swap_tx.vout[2].nValue = *m_ord_price;
        swap_tx.vout[2].scriptPubKey = bech32().PubKeyScript(*m_funds_payoff_addr);

        // Check if we need to add separate output for change if presented
        CAmount tx_fee = l15::CalculateTxFee(*m_mining_fee_rate, swap_tx);
        CAmount total_in = std::accumulate(m_swap_inputs.begin(), m_swap_inputs.end(), 0, [](CAmount part_sum, const auto &input) { return part_sum + input.output->Destination()->Amount(); });
        CAmount total_out = std::accumulate(swap_tx.vout.begin(), swap_tx.vout.end(), 0, [](CAmount part_sum, const auto &out) { return part_sum + out.nValue; });
        if (total_in - (tx_fee + TAPROOT_VOUT_VSIZE) - total_out >= l15::Dust()) {
            swap_tx.vout.emplace_back(0, bech32().PubKeyScript(*m_change_addr));

            tx_fee = l15::CalculateTxFee(*m_mining_fee_rate, swap_tx);
            swap_tx.vout.back().nValue = total_in - total_out - tx_fee;

            assert(swap_tx.vout.back().nValue >= l15::Dust());
        }
    }

    return swap_tx;
}

const CMutableTransaction &SwapInscriptionBuilder::GetOrdCommitTx() const
{
    if (!mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " not defined");

    if (!mOrdCommitTx) {
        mOrdCommitTx = mOrdCommitBuilder->MakeTx();
    }
    return *mOrdCommitTx;
}

CMutableTransaction SwapInscriptionBuilder::GetFundsCommitTxTemplate() const
{
    if (!mCommitBuilder) throw ContractStateError("Swap commit tx defined outside of the swap contract builder");

    CMutableTransaction res = mCommitBuilder->MakeTxTemplate();

    while (res.vout.size() < 3) {
        res.vout.emplace_back(0, bech32().PubKeyScript(bech32().Encode(xonly_pubkey())));
    }

    return res;
}

const CMutableTransaction &SwapInscriptionBuilder::GetFundsCommitTx() const
{
    if (!mCommitBuilder) throw ContractStateError("Funds committed outside of the swap contract builder");

    if (!mFundsCommitTx) {
        CAmount funds_required = CalculateWholeFee((m_market_fee->Amount() == 0 || m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) ? "" : "change") + *m_ord_price + m_market_fee->Amount();
        CAmount funds_provided = 0;
        for (const auto &input: mCommitBuilder->Inputs()) {
            funds_provided += input.output->Destination()->Amount();
        }

        if (funds_provided < funds_required) throw ContractFundsNotEnough(move((FormatAmount(funds_provided) += ", required: ") += FormatAmount(funds_required)));

        mFundsCommitTx = mCommitBuilder->MakeTx();
    }
    return *mFundsCommitTx;
}


void SwapInscriptionBuilder::SignOrdCommitment(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(ORD_TERMS);
    if (!m_ord_script_pk) throw ContractTermMissing(std::string(name_ord_script_pk));
    if (!m_ord_int_pk) throw ContractTermMissing(std::string(name_ord_int_pk));

    mOrdCommitBuilder->Sign(master_key, key_filter);
}

void SwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_TERMS);

    if (!mCommitBuilder) throw ContractStateError("Funds were committed outside of the swap contract builder");

    CAmount funds_required = CalculateWholeFee((m_market_fee->Amount() == 0 || m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) ? "" : "change") + *m_ord_price + m_market_fee->Amount();
    CAmount funds_provided = 0;
    for (const auto &input: mCommitBuilder->Inputs()) {
        funds_provided += input.output->Destination()->Amount();
    }

    if (funds_provided < funds_required) throw ContractFundsNotEnough(move((FormatAmount(funds_provided) += ", required: ") += FormatAmount(funds_required)));

    mCommitBuilder->Sign(master_key, key_filter);

    if (mFundsCommitTx) mFundsCommitTx.reset();
}

void SwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &masterKey, const std::string& key_filter)
{
    CheckContractTerms(ORD_TERMS);

    KeyPair keypair = masterKey.Lookup(*m_ord_script_pk, key_filter);
    ChannelKeys schnorr(masterKey.Secp256k1Context(), keypair.PrivKey());

    CMutableTransaction swap_tx(MakeSwapTx());

    std::vector<CTxOut> spend_outs = {CTxOut(m_swap_inputs.front().output->Destination()->Amount(), m_swap_inputs.front().output->Destination()->PubKeyScript())};

    signature sig = schnorr.SignTaprootTx(swap_tx, 0, move(spend_outs), OrdSwapScript(), SIGHASH_SINGLE|SIGHASH_ANYONECANPAY);
    m_swap_inputs.front().witness.Set(1, move(sig));

    if (mSwapTx) mSwapTx.reset();
}

void SwapInscriptionBuilder::SignMarketSwap(const KeyRegistry &masterKey, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_SWAP_TERMS);

    if (mCommitBuilder) {
        if (m_swap_inputs.size() != 1) throw ContractStateError(name_swap_inputs + " has inconsistent size: " + std::to_string(m_swap_inputs.size()));

        m_swap_inputs.front().nin = 2;
        m_swap_inputs.emplace_back(bech32(), 0, std::make_shared<ContractOutput>(mCommitBuilder, 0));
        m_swap_inputs.emplace_back(bech32(), 1, std::make_shared<ContractOutput>(mCommitBuilder, 1));
        m_swap_inputs.emplace_back(bech32(), 3, std::make_shared<ContractOutput>(mCommitBuilder, 2));
        std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
    }
    if (m_swap_inputs.size() < 4) throw ContractStateError(name_swap_inputs + " has inconsistent size: " + std::to_string(m_swap_inputs.size()));

    KeyPair keypair = masterKey.Lookup(*m_market_script_pk, key_filter);
    ChannelKeys schnorr(masterKey.Secp256k1Context(), keypair.PrivKey());

    CMutableTransaction swap_tx(MakeSwapTx());

    std::vector<CTxOut> spend_outs;
    spend_outs.reserve(m_swap_inputs.size());
    std::transform(m_swap_inputs.begin(), m_swap_inputs.end(), cex::smartinserter(spend_outs, spend_outs.end()), [](const auto& txin){ return CTxOut(txin.output->Destination()->Amount(), txin.output->Destination()->PubKeyScript()); });

    signature sig = schnorr.SignTaprootTx(swap_tx, 2, move(spend_outs), OrdSwapScript());
    m_swap_inputs[2].witness.Set(0, move(sig));

    if (mSwapTx) mSwapTx.reset();
}

void SwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_SWAP_TERMS);

    CMutableTransaction swap_tx(MakeSwapTx());
    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_swap_inputs.size());
    for(const auto& input: m_swap_inputs) {
        const auto& dest = input.output->Destination();
        spent_outs.emplace_back(dest->Amount(), dest->PubKeyScript());
    }

    for(auto& input: m_swap_inputs) {
        if (input.nin == 2) continue; // Skip ORD input

        auto dest = input.output->Destination();
        auto keypair = dest->LookupKey(master_key, key_filter);

        std::vector<bytevector> stack = keypair->Sign(swap_tx, input.nin, spent_outs, SIGHASH_ALL);

        for (size_t i = 0; i < stack.size(); ++i) {
            input.witness.Set(i, move(stack[i]));
        }
    }

    if (mSwapTx) mSwapTx.reset();
}

string SwapInscriptionBuilder::OrdCommitRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetOrdCommitTx()));
    return res;
}

string SwapInscriptionBuilder::FundsCommitRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetFundsCommitTx()));
    return res;
}

string SwapInscriptionBuilder::OrdSwapRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetSwapTx()));
    return res;
}

string SwapInscriptionBuilder::Serialize(uint32_t version, SwapPhase phase)
{
    if (version != s_protocol_version) throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    CheckContractTerms(phase);

    UniValue contract(UniValue::VOBJ);

    contract.pushKV(name_version, s_protocol_version);
    contract.pushKV(name_ord_price, *m_ord_price);

    if (phase == ORD_TERMS) {
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
    }
    if (phase == ORD_SWAP_SIG || phase == ORD_TERMS) {
        contract.pushKV(name_market_script_pk, hex(*m_market_script_pk));
        contract.pushKV(name_ord_commit, mOrdCommitBuilder->MakeJson());
    }
    if (phase == ORD_SWAP_SIG) {
        contract.pushKV(name_ord_commit, mOrdCommitBuilder->MakeJson());
    }
    if (phase == FUNDS_SWAP_TERMS || phase == FUNDS_SWAP_SIG) {
        contract.pushKV(name_market_script_pk, hex(*m_market_script_pk));
    }
    if (phase == ORD_SWAP_SIG || phase == FUNDS_SWAP_TERMS || phase == FUNDS_SWAP_SIG) {
        contract.pushKV(name_ord_script_pk, hex(*m_ord_script_pk));
        contract.pushKV(name_ord_int_pk, hex(*m_ord_int_pk));
    }
    if (phase == ORD_TERMS || phase == ORD_SWAP_SIG || phase == FUNDS_SWAP_TERMS || phase == FUNDS_SWAP_SIG) {
        contract.pushKV(name_funds_payoff_addr, *m_funds_payoff_addr);
    }
    if (phase == ORD_SWAP_SIG || phase == FUNDS_SWAP_TERMS || phase == FUNDS_SWAP_SIG) {
        UniValue input_arr(UniValue::VARR);
        for (const auto &input: m_swap_inputs) {
            input_arr.push_back(input.MakeJson());
        }
        contract.pushKV(name_swap_inputs, move(input_arr));
    }

    if (phase == FUNDS_TERMS || phase == FUNDS_COMMIT_SIG) {
        contract.pushKV(name_funds, mCommitBuilder->MakeJson());
    }
    if (phase == FUNDS_TERMS || phase == FUNDS_COMMIT_SIG || phase == FUNDS_SWAP_TERMS || phase == FUNDS_SWAP_SIG) {
        contract.pushKV(name_market_fee, m_market_fee->MakeJson());
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
        contract.pushKV(name_change_addr, *m_change_addr);
        contract.pushKV(name_ord_payoff_addr, *m_ord_payoff_addr);
        if (m_change_addr)
            contract.pushKV(name_change_addr, *m_change_addr);
    }

    UniValue dataRoot(UniValue::VOBJ);
    dataRoot.pushKV(name_contract_type, val_swap_inscription);
    dataRoot.pushKV(name_params, move(contract));

    return dataRoot.write();
}

void SwapInscriptionBuilder::CheckContractTerms(SwapPhase phase) const
{
    if (!m_ord_price) throw ContractTermMissing(std::string(name_ord_price));
    if (*m_ord_price < l15::Dust(DUST_RELAY_TX_FEE)) throw ContractTermWrongValue(move((name_ord_price + ": ") += std::to_string(*m_ord_price)));

    switch (phase) {
    case ORD_SWAP_SIG:
        if (m_swap_inputs.empty()) throw ContractTermMissing(name_swap_inputs + "[ord]");
        if (m_swap_inputs.size() != 1 && m_swap_inputs.size() < 4) throw ContractStateError(name_swap_inputs + " size: " + std::to_string(m_swap_inputs.size()));
        if (m_swap_inputs.size() == 1) {
            if (!m_swap_inputs.front().output->Destination()) throw ContractTermMissing(name_swap_inputs + "[ord]");
            if (m_swap_inputs.front().output->Destination()->Amount() < l15::Dust(DUST_RELAY_TX_FEE)) throw ContractTermWrongValue(
                        move((((name_swap_inputs + "[ord].") += name_amount) += ": ") += std::to_string(
                                m_swap_inputs.front().output->Destination()->Amount())));
            if (!m_swap_inputs.front().witness) throw ContractTermMissing(move((name_swap_inputs + "[ord].") += TxInput::name_witness));
        }
        else {
            if (!m_swap_inputs[2].output->Destination()) throw ContractTermMissing(name_swap_inputs + "[ord]");
            if (m_swap_inputs[2].output->Destination()->Amount() < l15::Dust(DUST_RELAY_TX_FEE)) throw ContractTermWrongValue(
                        move((((name_swap_inputs + "[ord].") += name_amount) += ": ") += std::to_string(
                                m_swap_inputs[2].output->Destination()->Amount())));
            if (!m_swap_inputs[2].witness) throw ContractTermMissing(move((name_swap_inputs + "[ord].") += TxInput::name_witness));
        }
        if (!m_ord_script_pk) throw ContractTermMissing(std::string (name_ord_script_pk));
        if (!m_ord_int_pk) throw ContractTermMissing(std::string (name_ord_int_pk));
        // no break;
    case ORD_TERMS:
        if (phase == ORD_TERMS && !m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        if (!m_market_script_pk) throw ContractTermMissing(std::string (name_market_script_pk));
        if (!mOrdCommitBuilder) throw ContractTermMissing(name_ord_commit.c_str());
        if (!m_funds_payoff_addr) throw ContractTermMissing(std::string(name_funds_payoff_addr));
        break;
    case FUNDS_COMMIT_SIG:
    case FUNDS_TERMS:
        if (!mCommitBuilder) throw ContractTermMissing(std::string(name_funds));
        {
            CAmount req_amount = CalculateWholeFee((m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) ? "" : "change") + *m_ord_price + m_market_fee->Amount();
            CAmount funds_amount = 0;
            for (const auto &input: mCommitBuilder->Inputs()) {
                if (phase == FUNDS_COMMIT_SIG) {
                    if (!input.witness) throw ContractTermMissing(move(((name_funds + '[') += std::to_string(input.nin) += "].") += TxInput::name_witness));
                }

                funds_amount += input.output->Destination()->Amount();
            }
            if (funds_amount < req_amount) throw ContractFundsNotEnough(FormatAmount(funds_amount) + ", required: " + FormatAmount(req_amount));
        }
        if (!m_ord_payoff_addr) throw ContractTermMissing(name_ord_payoff_addr.c_str());
        if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
        if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        if (!m_change_addr) throw ContractTermMissing(std::string(name_change_addr));
        break;
    case FUNDS_SWAP_SIG:
        for (const auto& input: m_swap_inputs) {
            if (!input.witness) throw ContractTermMissing(move(((name_swap_inputs + '[') += std::to_string(input.nin) += "].") += TxInput::name_witness));
        }
    case FUNDS_SWAP_TERMS:
        if (mCommitBuilder)
            CheckContractTerms(FUNDS_COMMIT_SIG);
        else {
            if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
            if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
            if (!m_change_addr) throw ContractTermMissing(std::string(name_change_addr));
            if (m_swap_inputs.size() < 4) {
                if (m_swap_inputs.empty()) throw ContractTermMissing(std::string(name_swap_inputs));
                if (m_swap_inputs.size() == 1) throw ContractTermMissing(name_swap_inputs + ": looks there is ord input only");
                throw ContractTermMissing(name_swap_inputs + " size: " + std::to_string(m_swap_inputs.size()));
            }
        }
        break;
    }

    switch (phase) {
    case FUNDS_SWAP_SIG:
    case ORD_SWAP_SIG:
    case FUNDS_SWAP_TERMS:
        CheckOrdSwapSig();
    case ORD_TERMS:
        break;
    case FUNDS_COMMIT_SIG:
        CheckFundsCommitSig();
    case FUNDS_TERMS:
        break;
    }
}


void SwapInscriptionBuilder::Deserialize(const string &data)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() != val_swap_inscription) {
        throw ContractProtocolError("SwapInscription contract does not match " + root[name_contract_type].getValStr());
    }

    const UniValue& contract = root[name_params];

//    if (contract[name_version].getInt<uint32_t>() == s_protocol_version_pubkey_v4 || contract[name_version].getInt<uint32_t>() == s_protocol_version_old_v3) {
//        Deserialize_v4(data);
//        return;
//    }
//    else
    if (contract[name_version].getInt<uint32_t>() != s_protocol_version) {
        throw ContractProtocolError("Wrong SwapInscription contract version: " + contract[name_version].getValStr());
    }

    DeserializeContractAmount(contract[name_ord_price], m_ord_price, [](){ return name_ord_price; });
    {   const auto& val = contract[name_market_fee];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(std::string(name_market_fee));

            if (m_market_fee)
                m_market_fee->ReadJson(bech32(), val, true);
            else
                m_market_fee = IContractDestination::ReadJson(bech32(), val, true);

        }
    }
    DeserializeContractString(contract[name_change_addr], m_change_addr, [&](){ return name_change_addr; });
    DeserializeContractString(contract[name_funds_payoff_addr], m_funds_payoff_addr, [](){ return name_funds_payoff_addr; });
    DeserializeContractScriptPubkey(contract[name_market_script_pk], m_market_script_pk, [](){ return name_market_script_pk; });
    DeserializeContractScriptPubkey(contract[name_ord_script_pk], m_ord_script_pk, [](){ return name_ord_script_pk; });
    DeserializeContractScriptPubkey(contract[name_ord_int_pk], m_ord_int_pk, [](){ return name_ord_int_pk; });

    {   const auto& val = contract[name_ord_commit];
        if (!val.isNull()) {
            mOrdCommitBuilder = std::make_shared<SimpleTransaction>(bech32(), val);
        }
    }
    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            mCommitBuilder = std::make_shared<SimpleTransaction>(bech32(), val);
        }
    }
    {   const auto& val = contract[name_swap_inputs];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_swap_inputs));
            uint32_t i = 0;
            for (const UniValue &input: val.getValues()) {
                if (i >= m_swap_inputs.size()) {
                    m_swap_inputs.emplace_back(bech32(), m_swap_inputs.size(), input);
                }
                else {
                    m_swap_inputs[i].ReadJson(input);
                }
                ++i;
            }
        }
    }

    DeserializeContractString(contract[name_ord_payoff_addr], m_ord_payoff_addr, [](){ return name_ord_payoff_addr; });
    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [](){ return name_mining_fee_rate; });
}


const CMutableTransaction &SwapInscriptionBuilder::GetSwapTx() const
{
    if (!mSwapTx) {
        mSwapTx.emplace(MakeSwapTx());
    }
    return *mSwapTx;
}

void SwapInscriptionBuilder::CommitOrdinal(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " already defined");

    mOrdCommitBuilder = std::make_shared<SimpleTransaction>(bech32());
    mOrdCommitBuilder->MiningFeeRate(GetMiningFeeRate());

    //mOrdCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), ParseAmount(amount), bech32().Encode(get<0>(OrdSwapTapRoot()))));

    try {
        mOrdCommitBuilder->AddInput(std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_ord_commit + "[ord]"));
    }
}

void SwapInscriptionBuilder::FundCommitOrdinal(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr, const std::string& change_addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (!mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " not defined, call CommitOrdinal(...) first");

    try {
        mOrdCommitBuilder->AddInput(std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
        if (mOrdCommitBuilder->Outputs().size() > 2) {
            mOrdCommitBuilder->Outputs().pop_back();
        }
        mOrdCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), 0, change_addr));
//        mOrdCommitBuilder->AddChangeOutput(change_addr);
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_ord_commit + '[' + std::to_string(mOrdCommitBuilder->Inputs().size()) + ']'));
    }
}

void SwapInscriptionBuilder::CommitFunds(const string &txid, uint32_t nout, const string &amount, const std::string& addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");

    const CAmount dust = l15::Dust(DUST_RELAY_TX_FEE);

    if (!mCommitBuilder) {
        mCommitBuilder = std::make_shared<SimpleTransaction>(bech32());
        mCommitBuilder->MiningFeeRate(GetMiningFeeRate());

        mCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32(), dust, addr));

        if (m_market_fee->Amount() >= dust * 2) {
            mCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32(), m_market_fee->Amount() - dust, addr));
        }
        else {
            mCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32(), dust, addr));
        }
    }
    try {
        // Remove the output with main funding if already exists
        if (mCommitBuilder->Outputs().size() > 2) {
            mCommitBuilder->Outputs().pop_back();
        }

        mCommitBuilder->AddInput(std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));

        CAmount commit_fee = mCommitBuilder->CalculateWholeFee("") + TAPROOT_VOUT_VSIZE;
        CAmount output_required = CalculateSwapTxFee(false) + *m_ord_price + m_market_fee->Amount();

        CAmount funds_provided = 0;
        for (const auto &input: mCommitBuilder->Inputs()) {
            funds_provided += input.output->Destination()->Amount();
        }

        if (funds_provided >= output_required + commit_fee) {
            CAmount change = funds_provided - output_required - commit_fee;

            if (m_market_fee->Amount() >= dust * 2) {
                mCommitBuilder->Outputs()[1]->Amount(m_market_fee->Amount() - dust);
            }
            else if (change >= dust * 2) {
                mCommitBuilder->Outputs()[1]->Amount(change - dust);
            }

            mCommitBuilder->AddChangeOutput(addr);
        }
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_funds + '[' + std::to_string(mCommitBuilder->Inputs().size()) + ']'));
    }
}

void SwapInscriptionBuilder::BuildOrdCommit()
{
    if (mOrdCommitBuilder->Outputs().size() != 1 && mOrdCommitBuilder->Outputs().back()->Amount() != 0) throw ContractStateError(name_ord_commit.c_str());
    if (!m_swap_inputs.empty()) throw ContractStateError(name_swap_inputs + " has size: " + std::to_string(m_swap_inputs.size()));

    std::string change_addr = mOrdCommitBuilder->Outputs().back()->Address();
    mOrdCommitBuilder->Outputs().pop_back();

    auto ordSwapTapRoot = OrdSwapTapRoot();
    const CScript& script = get<2>(ordSwapTapRoot).GetScripts().front();

    auto scriptpath = get<2>(ordSwapTapRoot).CalculateScriptPath(script);

    bytevector controlblock = {static_cast<uint8_t>(0xc0 | get<1>(ordSwapTapRoot))};
    controlblock.reserve(1 + m_ord_int_pk->size() + scriptpath.size() * uint256::size());
    controlblock.insert(controlblock.end(), m_ord_int_pk->begin(), m_ord_int_pk->end());

    std::for_each(scriptpath.begin(), scriptpath.end(), [&](uint256 &branchhash)
    {
        controlblock.insert(controlblock.end(), branchhash.begin(), branchhash.end());
    });

    mOrdCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), mOrdCommitBuilder->Inputs().front().output->Destination()->Amount(), bech32().Encode(get<0>(ordSwapTapRoot))));
    mOrdCommitBuilder->AddChangeOutput(change_addr);

    m_swap_inputs.emplace_back(bech32(), 0, std::make_shared<ContractOutput>(mOrdCommitBuilder, 0));
    m_swap_inputs.front().witness.Set(0, signature());
    m_swap_inputs.front().witness.Set(1, bytevector(65));
    m_swap_inputs.front().witness.Set(2, bytevector(script.begin(), script.end()));
    m_swap_inputs.front().witness.Set(3, move(controlblock));

}

void SwapInscriptionBuilder::OrdScriptPubKey(const std::string& pk)
{
    m_ord_script_pk = unhex<xonly_pubkey>(pk);
    if (m_ord_int_pk) {
        BuildOrdCommit();
    }
}

void SwapInscriptionBuilder::OrdIntPubKey(const std::string& pk)
{
    m_ord_int_pk = unhex<xonly_pubkey>(pk);
    if (m_ord_script_pk) {
        BuildOrdCommit();
    }
}

void SwapInscriptionBuilder::Brick1SwapUTXO(const string &txid, uint32_t nout, const string &amount, const std::string& addr)
{
    if (mCommitBuilder) throw ContractStateError("Brick 1 swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), 0, std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void SwapInscriptionBuilder::Brick2SwapUTXO(const string &txid, uint32_t nout, const string &amount, const std::string& addr)
{
    if (mCommitBuilder) throw ContractStateError("Brick 2 swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), 1, std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void SwapInscriptionBuilder::AddMainSwapUTXO(const std::string &txid, uint32_t nout, const std::string &amount, const std::string& addr)
{
    if (mCommitBuilder) throw ContractStateError("Main swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), m_swap_inputs.back().nin + 1, std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void SwapInscriptionBuilder::CheckOrdSwapSig() const
{
    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_swap_inputs.size());
    std::transform(m_swap_inputs.begin(), m_swap_inputs.end(), cex::smartinserter(spent_outs, spent_outs.end()), [](const auto& txin){ return CTxOut(txin.output->Destination()->Amount(), txin.output->Destination()->PubKeyScript()); });
    CScript ordSwapScript = OrdSwapScript();
    if (mSwapTx) {
        for (const auto &input: m_swap_inputs) {
            if ((m_swap_inputs.size() == 1 && input.nin == 0) || (m_swap_inputs.size() != 1 && input.nin == 2)) {
                if (!input.witness[0].empty() && !l15::IsZeroArray(input.witness[0])) {
                    VerifyTxSignature(*m_market_script_pk, input.witness[0], *mSwapTx, input.nin, spent_outs, ordSwapScript);
                }
                if (!input.witness[1].empty() && !l15::IsZeroArray(input.witness[1])) {
                    VerifyTxSignature(*m_ord_script_pk, input.witness[1], *mSwapTx, input.nin, spent_outs, ordSwapScript);
                }
            } else {
                if (input.witness && !input.witness[0].empty() && !l15::IsZeroArray(input.witness[0])) {
                    VerifyTxSignature(input.output->Destination()->Address(), input.witness, *mSwapTx, input.nin, spent_outs);
                }
            }
        }
    }
    else {
        CMutableTransaction swap_tx = MakeSwapTx();
        for (const auto &input: m_swap_inputs) {
            if ((m_swap_inputs.size() == 1 && input.nin == 0) || (m_swap_inputs.size() != 1 && input.nin == 2)) {
                if (!input.witness[0].empty() && !l15::IsZeroArray(input.witness[0])) {
                    VerifyTxSignature(*m_market_script_pk, input.witness[0], swap_tx, input.nin, spent_outs, ordSwapScript);
                }
                if (!input.witness[1].empty() && !l15::IsZeroArray(input.witness[1])) {
                    VerifyTxSignature(*m_ord_script_pk, input.witness[1], swap_tx, input.nin, spent_outs, ordSwapScript);
                }
            } else {
                if (input.witness && !input.witness[0].empty() && !l15::IsZeroArray(input.witness[0])) {
                    VerifyTxSignature(input.output->Destination()->Address(), input.witness, swap_tx, input.nin, spent_outs);
                }
            }
        }
    }
}

CAmount SwapInscriptionBuilder::CalculateSwapTxFee(bool change) const
{
    CAmount swap_vsize = TX_SWAP_BASE_VSIZE;
    if (m_market_fee->Amount() >= l15::Dust() && m_market_fee->Amount() < l15::Dust() * 2) swap_vsize += TAPROOT_VOUT_VSIZE;
    if (m_swap_inputs.size() > 4) swap_vsize += (m_swap_inputs.size() - 4) * TAPROOT_KEYSPEND_VIN_VSIZE;
    if (change) swap_vsize += TAPROOT_VOUT_VSIZE;
    return CFeeRate(*m_mining_fee_rate).GetFee(swap_vsize);
}

CAmount SwapInscriptionBuilder::CalculateWholeFee(const std::string& params) const
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");

    bool change = false, p2wpkh_utxo = false;

    std::istringstream ss(params);
    std::string param;
    while(std::getline(ss, param, ',')) {
        if (param == FEE_OPT_HAS_CHANGE) { change = true; continue; }
        else if (param == FEE_OPT_HAS_P2WPKH_INPUT) { p2wpkh_utxo = true; continue; }
        else throw l15::IllegalArgumentError(move(param));
    }

    CAmount commit_vsize;
    if (mCommitBuilder) {
        commit_vsize = mCommitBuilder->CalculateWholeFee("");
    }
    else {
        commit_vsize = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE * 3;
        if (p2wpkh_utxo) {
            commit_vsize += P2WPKH_VIN_VSIZE;
        }
        else {
            commit_vsize += TAPROOT_KEYSPEND_VIN_VSIZE;
        }
    }

    return CalculateSwapTxFee(m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) + CFeeRate(*m_mining_fee_rate).GetFee(commit_vsize);
}


std::string SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const
{
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");

    CAmount res = *m_ord_price + m_market_fee->Amount() + CalculateWholeFee(params);
    if (m_market_fee->Amount() < l15::Dust() * 2)
        res += l15::Dust() * 2;

    return FormatAmount(res);
}


std::string SwapInscriptionBuilder::GetMinSwapFundingAmount() const
{
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");

    CAmount res = *m_ord_price + CalculateSwapTxFee(false);
    if (m_market_fee->Amount() < l15::Dust() * 2)
        res += m_market_fee->Amount();

    return FormatAmount(res);
}

void SwapInscriptionBuilder::CheckFundsCommitSig() const
{
    if (!mCommitBuilder) throw ContractStateError("Funds committed outside of the swap contract builder");

    mCommitBuilder->CheckSig();
}

uint32_t SwapInscriptionBuilder::TransactionCount(SwapPhase phase) const
{
    switch (phase) {
    case ORD_TERMS:
    case ORD_SWAP_SIG:
        return 2;
    case FUNDS_TERMS:
    case FUNDS_COMMIT_SIG:
        return 1;
    case FUNDS_SWAP_TERMS:
    case FUNDS_SWAP_SIG:
        return 2;
    default:
        return 0;
    }
}

std::string SwapInscriptionBuilder::RawTransaction(SwapPhase phase, uint32_t n)
{
    switch (phase) {
    case ORD_TERMS:
    case ORD_SWAP_SIG:
        if (n == 0)
            return OrdCommitRawTransaction();
        else if (n == 1)
            return OrdSwapRawTransaction();
        else throw ContractStateError("Transaction unavailable: " + std::to_string(n));
    case FUNDS_TERMS:
    case FUNDS_COMMIT_SIG:
        if (n == 0)
            return FundsCommitRawTransaction();
        else throw ContractStateError("Transaction unavailable: " + std::to_string(n));
    case FUNDS_SWAP_TERMS:
    case FUNDS_SWAP_SIG:
        if (n == 0)
            return OrdCommitRawTransaction();
        else if (n == 1)
            return OrdSwapRawTransaction();
        else throw ContractStateError("Transaction unavailable: " + std::to_string(n));
    default:
        throw ContractStateError("Raw transaction data are not available");
    }
}
} // namespace l15::utxord
