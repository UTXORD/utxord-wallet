#include <ranges>

#include "core_io.h"
#include "policy.h"
#include "feerate.h"

#include "contract_builder_factory.hpp"
#include "trustless_swap_inscription.hpp"


namespace utxord {

using l15::core::ChannelKeys;
using l15::ScriptMerkleTree;
using l15::TreeBalanceType;
using l15::ParseAmount;
using l15::FormatAmount;
using l15::CalculateOutputAmount;

namespace {

const std::string val_trustless_swap_inscription("TrustlessSwapInscription");

}

const uint32_t TrustlessSwapInscriptionBuilder::s_protocol_version = 6;
const char* TrustlessSwapInscriptionBuilder::s_versions = "[6]";

const std::string TrustlessSwapInscriptionBuilder::name_ord_price = "ord_price";
const std::string TrustlessSwapInscriptionBuilder::name_ord_commit = "ord_commit";

const std::string TrustlessSwapInscriptionBuilder::name_market_script_pk = "market_script_pk";
const std::string TrustlessSwapInscriptionBuilder::name_ord_script_pk = "ord_script_pk";
const std::string TrustlessSwapInscriptionBuilder::name_ord_int_pk = "ord_int_pk";

const std::string TrustlessSwapInscriptionBuilder::name_ord_payoff_addr = "ord_payoff_addr";
const std::string TrustlessSwapInscriptionBuilder::name_funds_payoff_addr = "funds_payoff_addr";

const std::string TrustlessSwapInscriptionBuilder::name_funds = "funds";
const std::string TrustlessSwapInscriptionBuilder::name_swap_inputs = "swap_inputs";

CScript TrustlessSwapInscriptionBuilder::MakeMultiSigScript(const xonly_pubkey& pk1, const xonly_pubkey& pk2)
{
    CScript script;
    script << pk1 << OP_CHECKSIG;
    script << pk2 << OP_CHECKSIGADD;
    script << 2 << OP_NUMEQUAL;
    return script;
}

CScript TrustlessSwapInscriptionBuilder::OrdSwapScript() const
{
    if (!m_ord_script_pk) throw ContractStateError(name_ord_script_pk + " not defined");
    if (!m_market_script_pk) throw ContractStateError(name_market_script_pk + " not defined");

    return MakeMultiSigScript(m_ord_script_pk.value(), m_market_script_pk.value());
}

std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> TrustlessSwapInscriptionBuilder::OrdSwapTapRoot() const
{
    if (!m_ord_int_pk) throw ContractStateError(name_ord_int_pk + " not defined");

    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED, { OrdSwapScript() });
    return std::tuple_cat(ChannelKeys::AddTapTweak(m_ord_int_pk.value(), tap_tree.CalculateRoot()), std::make_tuple(tap_tree));
}


CMutableTransaction TrustlessSwapInscriptionBuilder::MakeSwapTx() const
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

const CMutableTransaction &TrustlessSwapInscriptionBuilder::GetOrdCommitTx() const
{
    if (!mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " not defined");

    if (!mOrdCommitTx) {
        mOrdCommitTx = mOrdCommitBuilder->MakeTx("");
    }
    return *mOrdCommitTx;
}

//CMutableTransaction SwapInscriptionBuilder::GetFundsCommitTxTemplate() const
//{
//    if (!mCommitBuilder) throw ContractStateError("Swap commit tx defined outside of the swap contract builder");
//
//    CMutableTransaction res = mCommitBuilder->MakeTx("");
//
//    while (res.vout.size() < 3) {
//        res.vout.emplace_back(0, bech32().PubKeyScript(bech32().Encode(xonly_pubkey())));
//    }
//
//    return res;
//}

const CMutableTransaction &TrustlessSwapInscriptionBuilder::GetFundsCommitTx() const
{
    if (!mCommitBuilder) throw ContractStateError("Funds committed outside of the swap contract builder");

    if (!mFundsCommitTx) {
        CAmount funds_required = CalculateWholeFee((m_market_fee->Amount() == 0 || m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) ? "" : "change") + *m_ord_price + m_market_fee->Amount();
        CAmount funds_provided = 0;
        for (const auto &input: mCommitBuilder->Inputs()) {
            funds_provided += input.output->Destination()->Amount();
        }

        if (funds_provided < funds_required) throw ContractFundsNotEnough(move((FormatAmount(funds_provided) += ", required: ") += FormatAmount(funds_required)));

        mFundsCommitTx = mCommitBuilder->MakeTx("");
    }
    return *mFundsCommitTx;
}


void TrustlessSwapInscriptionBuilder::SignOrdCommitment(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(TRUSTLESS_ORD_TERMS);
    if (!m_ord_script_pk) throw ContractTermMissing(std::string(name_ord_script_pk));
    if (!m_ord_int_pk) throw ContractTermMissing(std::string(name_ord_int_pk));

    mOrdCommitBuilder->Sign(master_key, key_filter);
}

void TrustlessSwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(TRUSTLESS_FUNDS_TERMS);

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

void TrustlessSwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &masterKey, const std::string& key_filter)
{
    CheckContractTerms(TRUSTLESS_ORD_TERMS);

    KeyPair keypair = masterKey.Lookup(*m_ord_script_pk, key_filter);
    ChannelKeys schnorr(masterKey.Secp256k1Context(), keypair.PrivKey());

    CMutableTransaction swap_tx(MakeSwapTx());

    std::vector<CTxOut> spend_outs = {CTxOut(m_swap_inputs.front().output->Destination()->Amount(), m_swap_inputs.front().output->Destination()->PubKeyScript())};

    signature sig = schnorr.SignTaprootTx(swap_tx, 0, move(spend_outs), OrdSwapScript(), SIGHASH_SINGLE|SIGHASH_ANYONECANPAY);
    m_swap_inputs.front().witness.Set(1, move(sig));

    if (mSwapTx) mSwapTx.reset();
}

void TrustlessSwapInscriptionBuilder::SignMarketSwap(const KeyRegistry &masterKey, const std::string& key_filter)
{
    CheckContractTerms(TRUSTLESS_FUNDS_SWAP_TERMS);

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

void TrustlessSwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(TRUSTLESS_FUNDS_SWAP_TERMS);

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

string TrustlessSwapInscriptionBuilder::OrdCommitRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetOrdCommitTx()));
    return res;
}

string TrustlessSwapInscriptionBuilder::FundsCommitRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetFundsCommitTx()));
    return res;
}

string TrustlessSwapInscriptionBuilder::OrdSwapRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetSwapTx()));
    return res;
}

UniValue TrustlessSwapInscriptionBuilder::MakeJson(uint32_t version, TrustlessSwapPhase phase) const
{
    if (version != s_protocol_version) throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    UniValue contract(UniValue::VOBJ);

    contract.pushKV(name_version, s_protocol_version);
    contract.pushKV(name_ord_price, *m_ord_price);

    if (phase == TRUSTLESS_ORD_TERMS) {
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
    }
    if (phase == TRUSTLESS_ORD_SWAP_SIG || phase == TRUSTLESS_ORD_TERMS) {
        contract.pushKV(name_market_script_pk, hex(*m_market_script_pk));
    }
    if (phase == TRUSTLESS_ORD_TERMS) {
        contract.pushKV(name_ord_commit, mOrdCommitBuilder->MakeJson(mOrdCommitBuilder->GetProtocolVersion(), TX_TERMS));
    }
    if (phase == TRUSTLESS_ORD_SWAP_SIG) {
        contract.pushKV(name_ord_commit, mOrdCommitBuilder->MakeJson(mOrdCommitBuilder->GetProtocolVersion(), TX_SIGNATURE));
    }
    if (phase == TRUSTLESS_FUNDS_SWAP_TERMS || phase == TRUSTLESS_FUNDS_SWAP_SIG) {
        contract.pushKV(name_market_script_pk, hex(*m_market_script_pk));
    }
    if (phase == TRUSTLESS_ORD_SWAP_SIG || phase == TRUSTLESS_FUNDS_SWAP_TERMS || phase == TRUSTLESS_FUNDS_SWAP_SIG) {
        contract.pushKV(name_ord_script_pk, hex(*m_ord_script_pk));
        contract.pushKV(name_ord_int_pk, hex(*m_ord_int_pk));
    }
    if (phase == TRUSTLESS_ORD_TERMS || phase == TRUSTLESS_ORD_SWAP_SIG || phase == TRUSTLESS_FUNDS_SWAP_TERMS || phase == TRUSTLESS_FUNDS_SWAP_SIG) {
        contract.pushKV(name_funds_payoff_addr, *m_funds_payoff_addr);
    }
    if (phase == TRUSTLESS_ORD_SWAP_SIG || phase == TRUSTLESS_FUNDS_SWAP_TERMS || phase == TRUSTLESS_FUNDS_SWAP_SIG) {
        UniValue input_arr(UniValue::VARR);
        for (const auto &input: m_swap_inputs) {
            input_arr.push_back(input.MakeJson());
        }
        contract.pushKV(name_swap_inputs, move(input_arr));
    }

    if (phase == TRUSTLESS_FUNDS_TERMS) {
        contract.pushKV(name_funds, mCommitBuilder->MakeJson(mCommitBuilder->GetProtocolVersion(), TX_TERMS));
    }
    else if (phase == TRUSTLESS_FUNDS_COMMIT_SIG) {
        contract.pushKV(name_funds, mCommitBuilder->MakeJson(mCommitBuilder->GetProtocolVersion(), TX_SIGNATURE));
    }

    if (phase == TRUSTLESS_FUNDS_TERMS || phase == TRUSTLESS_FUNDS_COMMIT_SIG || phase == TRUSTLESS_FUNDS_SWAP_TERMS || phase == TRUSTLESS_FUNDS_SWAP_SIG) {
        contract.pushKV(name_market_fee, m_market_fee->MakeJson());
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
        contract.pushKV(name_change_addr, *m_change_addr);
        contract.pushKV(name_ord_payoff_addr, *m_ord_payoff_addr);
        if (m_change_addr)
            contract.pushKV(name_change_addr, *m_change_addr);
    }

    return contract;
}

void TrustlessSwapInscriptionBuilder::CheckContractTerms(TrustlessSwapPhase phase) const
{
    if (!m_ord_price) throw ContractTermMissing(std::string(name_ord_price));
    if (*m_ord_price < l15::Dust(DUST_RELAY_TX_FEE)) throw ContractTermWrongValue(move((name_ord_price + ": ") += std::to_string(*m_ord_price)));

    switch (phase) {
    case TRUSTLESS_ORD_SWAP_SIG:
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
    case TRUSTLESS_ORD_TERMS:
        if (phase == TRUSTLESS_ORD_TERMS && !m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        if (!m_market_script_pk) throw ContractTermMissing(std::string (name_market_script_pk));
        if (!mOrdCommitBuilder) throw ContractTermMissing(name_ord_commit.c_str());
        if (!m_funds_payoff_addr) throw ContractTermMissing(std::string(name_funds_payoff_addr));
        break;
    case TRUSTLESS_FUNDS_COMMIT_SIG:
    case TRUSTLESS_FUNDS_TERMS:
        if (!mCommitBuilder) throw ContractTermMissing(std::string(name_funds));
        {
            CAmount req_amount = CalculateWholeFee((m_market_fee->Amount() >= l15::Dust(DUST_RELAY_TX_FEE) * 2) ? "" : "change") + *m_ord_price + m_market_fee->Amount();
            CAmount funds_amount = 0;
            for (const auto &input: mCommitBuilder->Inputs()) {
                if (phase == TRUSTLESS_FUNDS_COMMIT_SIG) {
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
    case TRUSTLESS_FUNDS_SWAP_SIG:
        for (const auto& input: m_swap_inputs) {
            if (!input.witness) throw ContractTermMissing(move(((name_swap_inputs + '[') += std::to_string(input.nin) += "].") += TxInput::name_witness));
        }
    case TRUSTLESS_FUNDS_SWAP_TERMS:
        if (mCommitBuilder)
            CheckContractTerms(TRUSTLESS_FUNDS_COMMIT_SIG);
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
    case TRUSTLESS_FUNDS_SWAP_SIG:
    case TRUSTLESS_ORD_SWAP_SIG:
    case TRUSTLESS_FUNDS_SWAP_TERMS:
        CheckOrdSwapSig();
    case TRUSTLESS_ORD_TERMS:
        break;
    case TRUSTLESS_FUNDS_COMMIT_SIG:
        CheckFundsCommitSig();
    case TRUSTLESS_FUNDS_TERMS:
        break;
    }
}


void TrustlessSwapInscriptionBuilder::ReadJson(const UniValue &contract, utxord::TrustlessSwapPhase phase)
{
    if (contract[name_version].getInt<uint32_t>() != s_protocol_version) {
        throw ContractProtocolError("Wrong " + val_trustless_swap_inscription + " contract version: " + contract[name_version].getValStr());
    }

    DeserializeContractAmount(contract[name_ord_price], m_ord_price, [](){ return name_ord_price; });
    {   const auto& val = contract[name_market_fee];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(std::string(name_market_fee));

            if (m_market_fee)
                m_market_fee->ReadJson(val);
            else
                m_market_fee = DestinationFactory::ReadJson(chain(), val);

        }
    }
    DeserializeContractString(contract[name_change_addr], m_change_addr, [&](){ return name_change_addr; });
    DeserializeContractString(contract[name_funds_payoff_addr], m_funds_payoff_addr, [](){ return name_funds_payoff_addr; });
    DeserializeContractScriptPubkey(contract[name_market_script_pk], m_market_script_pk, [](){ return name_market_script_pk; });
    DeserializeContractScriptPubkey(contract[name_ord_script_pk], m_ord_script_pk, [](){ return name_ord_script_pk; });
    DeserializeContractScriptPubkey(contract[name_ord_int_pk], m_ord_int_pk, [](){ return name_ord_int_pk; });

    {   const auto& val = contract[name_ord_commit];
        if (!val.isNull()) {
            mOrdCommitBuilder = std::make_shared<SimpleTransaction>(chain(), val);
        }
    }
    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            mCommitBuilder = std::make_shared<SimpleTransaction>(chain(), val);
        }
    }
    {   const auto& val = contract[name_swap_inputs];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_swap_inputs));
            uint32_t i = 0;
            for (const UniValue &input: val.getValues()) {
                if (i >= m_swap_inputs.size()) {
                    m_swap_inputs.emplace_back(chain(), m_swap_inputs.size(), input);
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


const CMutableTransaction &TrustlessSwapInscriptionBuilder::GetSwapTx() const
{
    if (!mSwapTx) {
        mSwapTx.emplace(MakeSwapTx());
    }
    return *mSwapTx;
}

void TrustlessSwapInscriptionBuilder::CommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " already defined");

    mOrdCommitBuilder = std::make_shared<SimpleTransaction>(chain());
    mOrdCommitBuilder->MiningFeeRate(GetMiningFeeRate());

    //mOrdCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), ParseAmount(amount), bech32().Encode(get<0>(OrdSwapTapRoot()))));

    try {
        mOrdCommitBuilder->AddInput(std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_ord_commit + "[ord]"));
    }
}

void TrustlessSwapInscriptionBuilder::FundCommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr, std::string change_addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (!mOrdCommitBuilder) throw ContractStateError(name_ord_commit + " not defined, call CommitOrdinal(...) first");

    try {
        mOrdCommitBuilder->AddInput(std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
        if (mOrdCommitBuilder->Outputs().size() > 2) {
            mOrdCommitBuilder->Outputs().pop_back();
        }
        mOrdCommitBuilder->AddOutput(std::make_shared<P2TR>(bech32().GetChainMode(), 0, move(change_addr)));
//        mOrdCommitBuilder->AddChangeOutput(change_addr);
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_ord_commit + '[' + std::to_string(mOrdCommitBuilder->Inputs().size()) + ']'));
    }
}

void TrustlessSwapInscriptionBuilder::CommitFunds(std::string txid, uint32_t nout, CAmount amount, std::string addr)
{
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");

    const CAmount dust = l15::Dust(DUST_RELAY_TX_FEE);

    if (!mCommitBuilder) {
        mCommitBuilder = std::make_shared<SimpleTransaction>(chain());
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
        mCommitBuilder->DropChangeOutput();

        mCommitBuilder->AddInput(std::make_shared<UTXO>(chain(), move(txid), nout, amount, addr));

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

            mCommitBuilder->AddChangeOutput(move(addr));
        }
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_funds + '[' + std::to_string(mCommitBuilder->Inputs().size()) + ']'));
    }
}

void TrustlessSwapInscriptionBuilder::BuildOrdCommit()
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

void TrustlessSwapInscriptionBuilder::OrdScriptPubKey(xonly_pubkey pk)
{
    m_ord_script_pk = move(pk);
    if (m_ord_int_pk) {
        BuildOrdCommit();
    }
}

void TrustlessSwapInscriptionBuilder::OrdIntPubKey(xonly_pubkey pk)
{
    m_ord_int_pk = move(pk);
    if (m_ord_script_pk) {
        BuildOrdCommit();
    }
}

void TrustlessSwapInscriptionBuilder::Brick1SwapUTXO(string txid, uint32_t nout, CAmount amount, std::string addr)
{
    if (mCommitBuilder) throw ContractStateError("Brick 1 swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), 0, std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void TrustlessSwapInscriptionBuilder::Brick2SwapUTXO(string txid, uint32_t nout, CAmount amount, std::string addr)
{
    if (mCommitBuilder) throw ContractStateError("Brick 2 swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), 1, std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void TrustlessSwapInscriptionBuilder::AddMainSwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr)
{
    if (mCommitBuilder) throw ContractStateError("Main swap tx input already defined");
    if (m_swap_inputs.empty()) throw ContractStateError(name_ord_commit + " is not defined");
    if (m_swap_inputs.size() == 1) {
        if (m_swap_inputs.front().nin != 0) throw ContractStateError(name_swap_inputs + " are inconsistent");
        m_swap_inputs.front().nin = 2;
    }

    m_swap_inputs.emplace_back(bech32(), m_swap_inputs.back().nin + 1, std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
    std::sort(m_swap_inputs.begin(), m_swap_inputs.end());
}

void TrustlessSwapInscriptionBuilder::CheckOrdSwapSig() const
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

CAmount TrustlessSwapInscriptionBuilder::CalculateSwapTxFee(bool change) const
{
    CAmount swap_vsize = TX_SWAP_BASE_VSIZE;
    if (m_market_fee->Amount() >= l15::Dust() && m_market_fee->Amount() < l15::Dust() * 2) swap_vsize += TAPROOT_VOUT_VSIZE;
    if (m_swap_inputs.size() > 4) swap_vsize += (m_swap_inputs.size() - 4) * TAPROOT_KEYSPEND_VIN_VSIZE;
    if (change) swap_vsize += TAPROOT_VOUT_VSIZE;
    return CFeeRate(*m_mining_fee_rate).GetFee(swap_vsize);
}

CAmount TrustlessSwapInscriptionBuilder::CalculateWholeFee(const std::string& params) const
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


CAmount TrustlessSwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const
{
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");

    CAmount res = *m_ord_price + m_market_fee->Amount() + CalculateWholeFee(params);
    if (m_market_fee->Amount() < l15::Dust() * 2)
        res += l15::Dust() * 2;

    return res;
}


CAmount TrustlessSwapInscriptionBuilder::GetMinSwapFundingAmount() const
{
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");

    CAmount res = *m_ord_price + CalculateSwapTxFee(false);
    if (m_market_fee->Amount() < l15::Dust() * 2)
        res += m_market_fee->Amount();

    return res;
}

void TrustlessSwapInscriptionBuilder::CheckFundsCommitSig() const
{
    if (!mCommitBuilder) throw ContractStateError("Funds committed outside of the swap contract builder");

    mCommitBuilder->CheckSig();
}

uint32_t TrustlessSwapInscriptionBuilder::TransactionCount(TrustlessSwapPhase phase) const
{
    switch (phase) {
    case TRUSTLESS_ORD_TERMS:
    case TRUSTLESS_ORD_SWAP_SIG:
        return 2;
    case TRUSTLESS_FUNDS_TERMS:
    case TRUSTLESS_FUNDS_COMMIT_SIG:
        return 1;
    case TRUSTLESS_FUNDS_SWAP_TERMS:
    case TRUSTLESS_FUNDS_SWAP_SIG:
        return 2;
    default:
        return 0;
    }
}

std::string TrustlessSwapInscriptionBuilder::RawTransaction(TrustlessSwapPhase phase, uint32_t n)
{
    switch (phase) {
    case TRUSTLESS_ORD_TERMS:
    case TRUSTLESS_ORD_SWAP_SIG:
        if (n == 0)
            return OrdCommitRawTransaction();
        else if (n == 1)
            return OrdSwapRawTransaction();
        else return {};
    case TRUSTLESS_FUNDS_TERMS:
    case TRUSTLESS_FUNDS_COMMIT_SIG:
        if (n == 0)
            return FundsCommitRawTransaction();
        else return {};
    case TRUSTLESS_FUNDS_SWAP_TERMS:
    case TRUSTLESS_FUNDS_SWAP_SIG:
        if (n == 0)
            return OrdCommitRawTransaction();
        else if (n == 1)
            return OrdSwapRawTransaction();
        else return {};
    default:
        return {};
    }
}

const std::string &TrustlessSwapInscriptionBuilder::GetContractName() const
{ return val_trustless_swap_inscription; }


} // namespace l15::utxord
