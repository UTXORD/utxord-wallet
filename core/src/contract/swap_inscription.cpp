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

const uint32_t COMMIT_TIMEOUT = 12;


CScript MakeFundsSwapScript(const xonly_pubkey& pk_B, const xonly_pubkey& pk_M)
{
    CScript script;
    script << pk_B << OP_CHECKSIG;
    script << pk_M << OP_CHECKSIGADD;
    script << 2 << OP_NUMEQUAL;
    return script;
}

CScript MakeRelTimeLockScript(uint32_t blocks_to_lock, const xonly_pubkey& pk)
{
    CScript script;
    script << l15::GetCsvInBlocks(blocks_to_lock) << OP_CHECKSEQUENCEVERIFY << OP_DROP;
    script << pk << OP_CHECKSIG;
    return script;
}

}

const uint32_t SwapInscriptionBuilder::s_protocol_version = 5;
const uint32_t SwapInscriptionBuilder::s_protocol_version_pubkey_v4 = 4;
const uint32_t SwapInscriptionBuilder::s_protocol_version_old_v3 = 3;
const char* SwapInscriptionBuilder::s_versions = "[4,5]";

const std::string SwapInscriptionBuilder::name_ord_price = "ord_price";

const std::string SwapInscriptionBuilder::name_ord_mining_fee_rate = "ord_mining_fee_rate";

const std::string SwapInscriptionBuilder::name_swap_script_pk_A = "swap_script_pk_A";
const std::string SwapInscriptionBuilder::name_swap_script_pk_B = "swap_script_pk_B";
const std::string SwapInscriptionBuilder::name_swap_script_pk_M = "swap_script_pk_M";

const std::string SwapInscriptionBuilder::name_ord_input = "ord_utxo";
const std::string SwapInscriptionBuilder::name_ord_txid = "ord_txid";
const std::string SwapInscriptionBuilder::name_ord_nout = "ord_nout";
const std::string SwapInscriptionBuilder::name_ord_amount = "ord_amount";
const std::string SwapInscriptionBuilder::name_ord_pk = "ord_pk";
const std::string SwapInscriptionBuilder::name_ord_addr = "ord_addr";


const std::string SwapInscriptionBuilder::name_funds = "funds";
const std::string SwapInscriptionBuilder::name_funds_unspendable_key = "funds_unspendable_key_factor";
const std::string SwapInscriptionBuilder::name_funds_txid = "funds_txid";
const std::string SwapInscriptionBuilder::name_funds_nout = "funds_nout";
const std::string SwapInscriptionBuilder::name_funds_amount = "funds_amount";

const std::string SwapInscriptionBuilder::name_funds_commit_sig = "funds_commit_sig";

const std::string SwapInscriptionBuilder::name_ord_swap_sig_A = "ord_swap_sig_A";
const std::string SwapInscriptionBuilder::name_funds_swap_sig_B = "funds_swap_sig_B";
const std::string SwapInscriptionBuilder::name_funds_swap_sig_M = "funds_swap_sig_M";

const std::string SwapInscriptionBuilder::name_ordpayoff_unspendable_key_factor = "ordpayoff_unspendable_key_factor";
const std::string SwapInscriptionBuilder::name_ordpayoff_sig = "ordpayoff_sig";


std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> SwapInscriptionBuilder::FundsCommitTapRoot() const
{
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED,
                              { MakeFundsSwapScript(m_swap_script_pk_B.value(), m_swap_script_pk_M.value()),
                                MakeRelTimeLockScript(COMMIT_TIMEOUT, m_swap_script_pk_B.value())});

    return std::tuple_cat(ChannelKeys::AddTapTweak(ChannelKeys::CreateUnspendablePubKey(m_funds_unspendable_key_factor.value()),
                                                  tap_tree.CalculateRoot()), std::make_tuple(tap_tree));
}

std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> SwapInscriptionBuilder::FundsCommitTemplateTapRoot() const
{
    xonly_pubkey pubKey;
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED,
                              { MakeFundsSwapScript(pubKey, pubKey),
                                MakeRelTimeLockScript(COMMIT_TIMEOUT, pubKey)});

    return std::tuple_cat(std::pair<xonly_pubkey, uint8_t>(pubKey, 0), std::make_tuple(tap_tree));
}

CMutableTransaction SwapInscriptionBuilder::GetSwapTxTemplate() const {

    if (!mSwapTpl) {
        CMutableTransaction swapTpl;
        swapTpl.vin.reserve(2);
        swapTpl.vout.reserve(3);

        swapTpl.vout.emplace_back(0, CScript() << 1 << *m_swap_script_pk_M);
        swapTpl.vout.emplace_back(*m_ord_price, CScript() << 1 << xonly_pubkey());
        if (*m_market_fee != 0) {
            swapTpl.vout.emplace_back(*m_market_fee, CScript() << 1 << *m_swap_script_pk_M);
        }

        swapTpl.vin.emplace_back(COutPoint(uint256(), 0));
        swapTpl.vin.back().scriptWitness.stack.emplace_back(65);

        auto taproot = FundsCommitTemplateTapRoot();

        xonly_pubkey funds_unspendable_key;

        CScript &funds_swap_script = get<2>(taproot).GetScripts()[0];

        auto funds_scriptpath = get<2>(taproot).CalculateScriptPath(funds_swap_script);
        bytevector funds_control_block = {0};
        funds_control_block.reserve(1 + funds_unspendable_key.size() + funds_scriptpath.size() * uint256::size());
        funds_control_block.insert(funds_control_block.end(), funds_unspendable_key.begin(), funds_unspendable_key.end());
        for (uint256 &branch_hash: funds_scriptpath)
            funds_control_block.insert(funds_control_block.end(), branch_hash.begin(), branch_hash.end());

        swapTpl.vin.emplace_back(uint256(0), 0);
        swapTpl.vin.back().scriptWitness.stack.emplace_back(64);
        swapTpl.vin.back().scriptWitness.stack.emplace_back(64);

        swapTpl.vin.back().scriptWitness.stack.emplace_back(funds_swap_script.begin(), funds_swap_script.end());
        swapTpl.vin.back().scriptWitness.stack.emplace_back(move(funds_control_block));

        mSwapTpl = move(swapTpl);
    }
    return *mSwapTpl;
}

CMutableTransaction SwapInscriptionBuilder::MakeSwapTx(bool with_funds_in) const
{
    CMutableTransaction swap_tx = GetSwapTxTemplate();

    swap_tx.vin[0].prevout = COutPoint(uint256S(m_ord_input->output->TxID()), m_ord_input->output->NOut());
    if (m_ord_input->witness) {
        swap_tx.vin[0].scriptWitness.stack = m_ord_input->witness;
    }

    swap_tx.vout[0].nValue = m_ord_input->output->Destination()->Amount() + CFeeRate(*m_ord_mining_fee_rate).GetFee(MIN_TAPROOT_TX_VSIZE);
    swap_tx.vout[1].scriptPubKey = CScript() << 1 << *m_swap_script_pk_A;

    if (with_funds_in) {
        auto funds_commit_taproot = FundsCommitTapRoot();

        xonly_pubkey funds_unspendable_key = ChannelKeys::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

        CScript& funds_swap_script = get<2>(funds_commit_taproot).GetScripts()[0];

        auto funds_scriptpath = get<2>(funds_commit_taproot).CalculateScriptPath(funds_swap_script);
        bytevector funds_control_block = {static_cast<uint8_t>(0xc0 | get<1>(funds_commit_taproot))};
        funds_control_block.reserve(1 + funds_unspendable_key.size() + funds_scriptpath.size() * uint256::size());
        funds_control_block.insert(funds_control_block.end(), funds_unspendable_key.begin(), funds_unspendable_key.end());
        for(uint256 &branch_hash : funds_scriptpath)
            funds_control_block.insert(funds_control_block.end(), branch_hash.begin(), branch_hash.end());

        swap_tx.vin[1].prevout.hash = GetFundsCommitTx().GetHash();
        swap_tx.vin[1].prevout.n = 0;

        if (m_funds_swap_sig_M) {
            swap_tx.vin[1].scriptWitness.stack[0] = *m_funds_swap_sig_M;
        }
        if (m_funds_swap_sig_B) {
            swap_tx.vin[1].scriptWitness.stack[1] = *m_funds_swap_sig_B;
        }
        swap_tx.vin[1].scriptWitness.stack[2] = std::vector<unsigned char>(funds_swap_script.begin(), funds_swap_script.end());
        swap_tx.vin[1].scriptWitness.stack[3] = std::move(funds_control_block);
    }
    else {
        swap_tx.vin.pop_back();
    }
    return swap_tx;
}

void SwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    auto signer = m_ord_input->output->Destination()->LookupKey(master_key, key_filter);

    CMutableTransaction swap_tx(MakeSwapTx(false));

    auto stack = signer->Sign(swap_tx, 0, {CTxOut(m_ord_input->output->Destination()->Amount(), m_ord_input->output->Destination()->PubKeyScript())}, SIGHASH_ALL|SIGHASH_ANYONECANPAY);

    for (size_t i = 0; i < stack.size(); ++i)
        m_ord_input->witness.Set(i, stack[i]);
}

CMutableTransaction& SwapInscriptionBuilder::GetFundsCommitTxTemplate() const
{
    if (!mFundsCommitTpl) {
        auto commit_pubkeyscript = CScript() << 1 << get<0>(FundsCommitTemplateTapRoot());
        auto change_pubkeyscript = CScript() << 1 << xonly_pubkey();

        CMutableTransaction commit_tx;

        commit_tx.vout = {CTxOut(0, commit_pubkeyscript), CTxOut(0, change_pubkeyscript)};

        mFundsCommitTpl = move(commit_tx);
    }
    mFundsCommitTpl->vin.reserve(m_fund_inputs.size());

    if (m_fund_inputs.empty()) {
        mFundsCommitTpl->vin.emplace_back(uint256(), 0);
        mFundsCommitTpl->vin.back().scriptWitness.stack.emplace_back(64);
    }
    else {
        uint32_t n = 0;
        for (const auto &utxo: m_fund_inputs) {
            if (mFundsCommitTpl->vin.size() > n) {
                mFundsCommitTpl->vin[n].prevout = COutPoint(uint256S(utxo.output->TxID()), utxo.output->NOut());
                if (utxo.witness)
                    mFundsCommitTpl->vin[n].scriptWitness.stack = utxo.witness;
            }
            else {
                mFundsCommitTpl->vin.emplace_back(uint256S(utxo.output->TxID()), utxo.output->NOut());
                if (utxo.witness)
                    mFundsCommitTpl->vin[n].scriptWitness.stack = utxo.witness;
                else
                    mFundsCommitTpl->vin[n].scriptWitness.stack = utxo.output->Destination()->DummyWitness();
            }
            ++n;
        }
    }
    return *mFundsCommitTpl;
}

CMutableTransaction SwapInscriptionBuilder::MakeFundsCommitTx() const
{
    CMutableTransaction commit_tx = GetFundsCommitTxTemplate();

    CAmount funds_required = ParseAmount(GetMinFundingAmount(""));
    CAmount funds_provided = 0;
    for (const auto &utxo: m_fund_inputs) {
        funds_provided += utxo.output->Destination()->Amount();
    }

    CAmount change = funds_provided - funds_required;

    if(change > l15::Dust(3000)) {
        commit_tx.vout[1].scriptPubKey = (CScript() << 1 << *m_swap_script_pk_B);
        commit_tx.vout[1].nValue = change;
    } else {
        commit_tx.vout.pop_back();

        funds_required = ParseAmount(GetMinFundingAmount("")); // Calculate again since one output was removed
    }

    if(funds_provided < funds_required) {
        throw l15::TransactionError("funds amount too small");
    }

    commit_tx.vout[0].scriptPubKey = CScript() << 1 << get<0>(FundsCommitTapRoot());
    if (commit_tx.vout.size() == 2) { //has a change: commit just what required
        commit_tx.vout[0].nValue = CalculateOutputAmount(funds_required, *m_mining_fee_rate, commit_tx);
    }
    else { // has no change: commit whole amount from inputs
        commit_tx.vout[0].nValue = CalculateOutputAmount(funds_provided, *m_mining_fee_rate, commit_tx);
    }

    return commit_tx;
}

const CMutableTransaction &SwapInscriptionBuilder::GetFundsCommitTx() const
{
    if (!mFundsCommitTx) {
        mFundsCommitTx = MakeFundsCommitTx();
    }
    return *mFundsCommitTx;
}


void SwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_TERMS);

    m_funds_unspendable_key_factor = ChannelKeys::GetStrongRandomKey(master_key.Secp256k1Context());

    std::clog << "Signing fund inputs" << std::endl;

    CMutableTransaction commit_tx = MakeFundsCommitTx();

    std::vector<CTxOut> spent_outs;
    for (const auto& fund: m_fund_inputs) {
        spent_outs.emplace_back(fund.output->Destination()->Amount(), fund.output->Destination()->PubKeyScript());
    }

    for (auto& utxo: m_fund_inputs) {
        auto stack = utxo.output->Destination()->LookupKey(master_key, key_filter)->Sign(commit_tx, utxo.nin, spent_outs, SIGHASH_ALL);

        for (size_t i = 0; i < stack.size(); ++i) {
            utxo.witness.Set(i, move(stack[i]));
        }
    }
}

void SwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(MARKET_PAYOFF_SIG);

    auto keypair = master_key.Lookup(*m_swap_script_pk_B, key_filter);

    const CMutableTransaction& funds_commit = GetFundsCommitTx();
    CMutableTransaction swap_tx(MakeSwapTx(true));

    ChannelKeys key(keypair.PrivKey());
    m_funds_swap_sig_B = key.SignTaprootTx(swap_tx, 1, {CTxOut(m_ord_input->output->Destination()->Amount(), m_ord_input->output->Destination()->PubKeyScript()), funds_commit.vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
}

void SwapInscriptionBuilder::SignFundsPayBack(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_COMMIT_SIG);

    const CMutableTransaction& funds_commit = GetFundsCommitTx(); // Request it here in order to force reuired fields check

    auto keypair = master_key.Lookup(*m_swap_script_pk_B, key_filter);
    ChannelKeys key(keypair.PrivKey());

    auto commit_taproot = FundsCommitTapRoot();
    //auto commit_pubkeyscript = CScript() << 1 << get<0>(commit_taproot);
    auto payoff_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

    xonly_pubkey internal_unspendable_key = ChannelKeys::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

    CScript& payback_script = get<2>(commit_taproot).GetScripts()[1];

    auto commit_scriptpath = get<2>(commit_taproot).CalculateScriptPath(payback_script);
    bytevector control_block = {static_cast<uint8_t>(0xc0 | get<1>(commit_taproot))};
    control_block.reserve(1 + internal_unspendable_key.size() + commit_scriptpath.size() * uint256::size());
    control_block.insert(control_block.end(), internal_unspendable_key.begin(), internal_unspendable_key.end());
    for(uint256 &branch_hash : commit_scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());

    CMutableTransaction payback_tx;
    payback_tx.vin = {CTxIn(funds_commit.GetHash(), 0, {}, l15::GetCsvInBlocks(12))};
    payback_tx.vin.front().scriptWitness.stack.emplace_back(64);
    payback_tx.vin.front().scriptWitness.stack.emplace_back(payback_script.begin(), payback_script.end());
    payback_tx.vin.front().scriptWitness.stack.emplace_back(control_block);
    payback_tx.vout = {CTxOut(0, payoff_pubkeyscript)};
    payback_tx.vout.front().nValue = CalculateOutputAmount(funds_commit.vout[0].nValue, *m_mining_fee_rate, payback_tx);

    signature payback_sig = key.SignTaprootTx(payback_tx, 0, {funds_commit.vout[0]}, payback_script);
    payback_tx.vin.front().scriptWitness.stack.front() = move(payback_sig);

    mFundsPaybackTx = move(payback_tx);
}

void SwapInscriptionBuilder::MarketSignOrdPayoffTx(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(MARKET_PAYOFF_TERMS);

    auto keypair = master_key.Lookup(*m_swap_script_pk_M, key_filter);
    ChannelKeys key(keypair.PrivKey());

    CScript transfer_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

    CMutableTransaction swap_tx(MakeSwapTx(true));

    CMutableTransaction transfer_tx;

    transfer_tx.vin = {CTxIn(swap_tx.GetHash(), 0)};
    transfer_tx.vin.front().scriptWitness.stack.emplace_back(64);
    transfer_tx.vout = {CTxOut(swap_tx.vout[0].nValue, move(transfer_pubkeyscript))};
    transfer_tx.vout.front().nValue = CalculateOutputAmount(swap_tx.vout[0].nValue, *m_ord_mining_fee_rate, transfer_tx);

    m_ordpayoff_sig = key.SignTaprootTx(transfer_tx, 0, {swap_tx.vout[0]}, {});

    transfer_tx.vin.front().scriptWitness.stack.front() = *m_ordpayoff_sig;

    mOrdPayoffTx = move(transfer_tx);
}

void SwapInscriptionBuilder::MarketSignSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(FUNDS_SWAP_SIG);

    auto keypair = master_key.Lookup(*m_swap_script_pk_M, key_filter);
    ChannelKeys key(keypair.PrivKey());

    auto utxo_pubkeyscript = m_ord_input->output->Destination()->PubKeyScript();

    CMutableTransaction swap_tx(MakeSwapTx(true));

    m_funds_swap_sig_M = key.SignTaprootTx(swap_tx, 1, {CTxOut(m_ord_input->output->Destination()->Amount(), utxo_pubkeyscript), GetFundsCommitTx().vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));

    swap_tx.vin[1].scriptWitness.stack[0] = *m_funds_swap_sig_M;

    mSwapTx = move(swap_tx);

    PrecomputedTransactionData txdata;
    txdata.Init(*mSwapTx, {CTxOut(m_ord_input->output->Destination()->Amount(), utxo_pubkeyscript), GetFundsCommitTx().vout[0]}, /* force=*/ true);

    const CTxIn& ordTxin = mSwapTx->vin.at(0);
    MutableTransactionSignatureChecker TxOrdChecker(&(*mSwapTx), 0, m_ord_input->output->Destination()->Amount(), txdata, MissingDataBehavior::FAIL);
    bool ordPath = VerifyScript(ordTxin.scriptSig, utxo_pubkeyscript, &ordTxin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TxOrdChecker);
    if (!ordPath) {
        throw ContractError("Ord path swap error");
    }

    const CTxIn& txin = mSwapTx->vin.at(1);
    MutableTransactionSignatureChecker tx_checker(&(*mSwapTx), 1, GetFundsCommitTx().vout[0].nValue, txdata, MissingDataBehavior::FAIL);
    bool fundsPath = VerifyScript(txin.scriptSig, GetFundsCommitTx().vout[0].scriptPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, tx_checker);
    if (!fundsPath) {
        throw ContractError("Funds path swap error");
    }
}

string SwapInscriptionBuilder::FundsCommitRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetFundsCommitTx()));
    return res;
}

string SwapInscriptionBuilder::FundsPayBackRawTransaction() const
{
    if (!mFundsPaybackTx) {
        throw std::logic_error("FundsPayOff transaction data unavailable");
    }
    std::string res = EncodeHexTx(CTransaction(*mFundsPaybackTx));
    return res;
}

string SwapInscriptionBuilder::OrdSwapRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetSwapTx()));
    return res;
}

string SwapInscriptionBuilder::OrdPayoffRawTransaction() const
{
    std::string res = EncodeHexTx(CTransaction(GetPayoffTx()));
    return res;
}

string SwapInscriptionBuilder::Serialize(uint32_t version, SwapPhase phase)
{
    if (version != s_protocol_version) throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    CheckContractTerms(phase);

    UniValue contract(UniValue::VOBJ);

    contract.pushKV(name_version, s_protocol_version);
    contract.pushKV(name_ord_price, *m_ord_price);
    contract.pushKV(name_market_fee, *m_market_fee);
    contract.pushKV(name_swap_script_pk_M, hex(*m_swap_script_pk_M));
    contract.pushKV(name_ord_mining_fee_rate, *m_ord_mining_fee_rate);

    if (phase == ORD_SWAP_SIG || phase == MARKET_PAYOFF_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_ord_input, m_ord_input->MakeJson());
        contract.pushKV(name_swap_script_pk_A, hex(*m_swap_script_pk_A));
    }
//    if (phase == ORD_SWAP_SIG || phase == MARKET_PAYOFF_SIG || phase == MARKET_SWAP_SIG) {
//        contract.pushKV(name_ord_swap_sig_A, hex(*m_ord_swap_sig_A));
//    }

    if (phase == FUNDS_TERMS || phase == FUNDS_COMMIT_SIG || phase == MARKET_PAYOFF_SIG || phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
    }
    if (phase == FUNDS_COMMIT_SIG || phase == MARKET_PAYOFF_SIG || phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        UniValue funds(UniValue::VARR);
        for (const auto& utxo: m_fund_inputs) {
            funds.push_back(utxo.MakeJson());
        }
        contract.pushKV(name_funds, move(funds));

        contract.pushKV(name_funds_unspendable_key, hex(*m_funds_unspendable_key_factor));
        contract.pushKV(name_swap_script_pk_B, hex(*m_swap_script_pk_B));
    }

    if (phase == MARKET_PAYOFF_SIG) {
        contract.pushKV(name_ordpayoff_sig, hex(*m_ordpayoff_sig));
    }

    if (phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_funds_swap_sig_B, hex(*m_funds_swap_sig_B));
    }

    if (phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_funds_swap_sig_M, hex(*m_funds_swap_sig_M));
    }

    UniValue dataRoot(UniValue::VOBJ);
    dataRoot.pushKV(name_contract_type, val_swap_inscription);
    dataRoot.pushKV(name_params, move(contract));

    return dataRoot.write();
}

void SwapInscriptionBuilder::CheckContractTerms(SwapPhase phase) const
{
    //if (!m_ord_mining_fee_rate) throw ContractTermMissing(std::string(name_ord_mining_fee_rate));
    //if (m_ord_price <= 0) throw ContractTermMissing(std::string(name_ord_price));
    if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
    if (!m_swap_script_pk_M) throw ContractTermMissing(std::string(name_swap_script_pk_M));

    switch (phase) {
    case MARKET_SWAP_SIG:
        if (!m_funds_swap_sig_M) throw ContractTermMissing(std::string(name_funds_swap_sig_M));
        // no break;
    case FUNDS_SWAP_SIG:
        if (!m_funds_swap_sig_B) throw ContractTermMissing(std::string(name_funds_swap_sig_B));
        // no break;
    case MARKET_PAYOFF_SIG:
        if (!m_ordpayoff_sig) throw ContractTermMissing(std::string(name_ordpayoff_sig));
        // no break;
    case MARKET_PAYOFF_TERMS:
        CheckContractTerms(FUNDS_COMMIT_SIG);
    case ORD_SWAP_SIG:
        if (!m_ord_price) throw ContractTermMissing(std::string(name_ord_price));
        if (!m_ord_mining_fee_rate) throw ContractTermMissing(std::string(name_ord_mining_fee_rate));
        if (!m_ord_input) throw ContractTermMissing(std::string(name_ord_input));
        if (!m_ord_input->witness) throw ContractTermMissing(std::string(ContractInput::name_witness));
        if (!m_swap_script_pk_A) throw ContractTermMissing(std::string(name_swap_script_pk_A));
        // no break;
    case ORD_TERMS:
        break;
    case FUNDS_COMMIT_SIG:
        if (m_fund_inputs.empty()) throw ContractTermMissing(std::string(name_funds));
        {
            CAmount funds_amount = 0;
            size_t n = 0;
            for (const auto& utxo: m_fund_inputs) {
                funds_amount += utxo.output->Destination()->Amount();

                if (!utxo.witness) throw ContractTermMissing(move(((name_funds + '[') += std::to_string(n) += "].") += ContractInput::name_witness));

                ++n;
            }
            CAmount req_amount = ParseAmount(GetMinFundingAmount(""));
            if (funds_amount < req_amount) throw ContractFundsNotEnough(FormatAmount(funds_amount) + ", required: " + FormatAmount(req_amount));
        }
        if (!m_swap_script_pk_B) throw ContractTermMissing(std::string(name_swap_script_pk_B));
        if (!m_funds_unspendable_key_factor) throw ContractTermMissing(std::string(name_funds_unspendable_key));
        // no break;
    case FUNDS_TERMS:
        if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
        if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        break;
    }

    switch (phase) {
    case MARKET_SWAP_SIG:
    case FUNDS_SWAP_SIG:
        CheckFundsSwapSig();
    case MARKET_PAYOFF_SIG:
        CheckOrdPayoffSig();
    case MARKET_PAYOFF_TERMS:
    case ORD_SWAP_SIG:
        CheckOrdSwapSig();
    case ORD_TERMS:
        break;
    case FUNDS_COMMIT_SIG:
    case FUNDS_TERMS:
        break;
    }
}

void SwapInscriptionBuilder::Deserialize_v4(const string &data)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() != val_swap_inscription) {
        throw ContractProtocolError("SwapInscription contract does not match " + root[name_contract_type].getValStr());
    }

    const UniValue& contract = root[name_params];

    if (!(contract[name_version].getInt<uint32_t>() == s_protocol_version_pubkey_v4 || contract[name_version].getInt<uint32_t>() == s_protocol_version_old_v3)) {
        throw ContractProtocolError("Wrong SwapInscription contract version: " + contract[name_version].getValStr());
    }

    DeserializeContractAmount(contract[name_ord_price], m_ord_price, [&](){ return name_ord_price; });
    DeserializeContractAmount(contract[name_market_fee], m_market_fee, [&](){ return name_market_fee; });

    {   const auto& val = contract[name_swap_script_pk_A];
        if (!val.isNull()) {
            if (m_swap_script_pk_A) {
                if (*m_swap_script_pk_A != unhex<xonly_pubkey>(val.get_str())) throw ContractTermMismatch(std::string(name_swap_script_pk_A));
            }
            else m_swap_script_pk_A = unhex<xonly_pubkey>(val.get_str());
        }
    }

    DeserializeContractAmount(contract[name_ord_mining_fee_rate], m_ord_mining_fee_rate, [](){ return name_ord_mining_fee_rate; });

    {   const auto& val = contract[name_swap_script_pk_B];
        if (!val.isNull()) {
            if (m_swap_script_pk_B) {
                if (*m_swap_script_pk_B != unhex<xonly_pubkey>(val.get_str())) throw ContractTermMismatch(std::string(name_swap_script_pk_B));
            }
            else m_swap_script_pk_B = unhex<xonly_pubkey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_swap_script_pk_M];
        if (!val.isNull()) {
            if (m_swap_script_pk_M) {
                if (*m_swap_script_pk_M != unhex<xonly_pubkey>(val.get_str())) throw ContractTermMismatch(std::string(name_swap_script_pk_M));
            }
            else m_swap_script_pk_M = unhex<xonly_pubkey>(val.get_str());
        }
    }

    std::optional<CAmount> ord_amount;
    std::optional<std::string> ord_txid;
    std::optional<uint32_t> ord_nout;
    std::optional<std::string> ord_addr;
    DeserializeContractAmount(contract[name_ord_amount], ord_amount, [&](){ return name_ord_amount; });
    DeserializeContractString(contract[name_ord_txid], ord_txid, [](){ return name_ord_txid; });

    {   const auto& val = contract[name_ord_nout];
        if (!val.isNull()) {
            ord_nout = val.getInt<uint32_t>();
        }
    }
    DeserializeContractTaprootPubkey(contract[name_ord_pk], ord_addr, [](){ return name_ord_pk; });

    if (ord_txid && ord_nout && ord_amount && ord_addr) {
        m_ord_input.emplace(bech32(), 0, std::make_shared<UTXO>(bech32(), move(*ord_txid), *ord_nout, *ord_amount, *ord_addr));
    }
    if (m_ord_input) {
        std::optional<signature> ord_swap_sig_A;
        DeserializeContractHexData(contract[name_ord_swap_sig_A], ord_swap_sig_A, [](){ return name_ord_swap_sig_A; });
        if (ord_swap_sig_A) {
            m_ord_input->witness.Set(0, *ord_swap_sig_A);
        }

    }
    {   const auto& val = contract[name_funds_unspendable_key];
        if (!val.isNull()) {
            if (m_funds_unspendable_key_factor) {
                if (*m_funds_unspendable_key_factor != unhex<seckey>(val.get_str())) throw ContractTermMismatch(std::string(name_funds_unspendable_key));
            }
            else m_funds_unspendable_key_factor = unhex<seckey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_funds));
            if (!m_fund_inputs.empty() && m_fund_inputs.size() != val.size()) throw ContractTermMismatch(std::string(name_funds) + " size");

            auto utxo_it = m_fund_inputs.begin();
            for (size_t i = 0; i < val.size(); ++i) {
                const auto &txid_val = val[i][name_funds_txid];
                //std::format("{}[{}].{}", name_funds, i, name_funds_txid)
                if (txid_val.isNull())
                    throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_txid).str());

                const auto &nout_val = val[i][name_funds_nout];
                //std::format("{}[{}].{}", name_funds, i, name_funds_nout)
                if (nout_val.isNull())
                    throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_nout).str());

                const auto &amount_val = val[i][name_funds_amount];
                // std::format("{}[{}].{}", name_funds, i, name_funds_amount)
                if (amount_val.isNull())
                    throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_amount).str());

                const auto &sig_val = val[i][name_funds_commit_sig];
                if (sig_val.isNull())
                    throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_commit_sig).str());

                if (i == m_fund_inputs.size()) {
                    m_fund_inputs.emplace_back(bech32(), i,  std::make_shared<UTXO>(bech32(),
                                         std::string(txid_val.get_str()),
                                         nout_val.getInt<uint32_t>(),
                                         amount_val.isStr() ? ParseAmount(amount_val.getValStr()) : amount_val.getInt<CAmount>(),
                                         std::string{}));

                    m_fund_inputs.back().witness.Set(0,unhex<signature>(sig_val.get_str()));
                }
                else {
                    // std::format("{}[{}].{}", name_funds, i, name_funds_txid)
                    if (utxo_it->output->TxID() != txid_val.get_str())
                        throw ContractTermMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_txid).str());
                    // std::format("{}[{}].{}", name_funds, i, name_funds_nout)
                    if (utxo_it->output->NOut() != nout_val.getInt<uint32_t>())
                        throw ContractTermMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_nout).str());
                    // std::format("{}[{}].{}", name_funds, i, name_funds_amount)
                    if (utxo_it->output->Destination()->Amount() != (amount_val.isStr() ? ParseAmount(amount_val.getValStr()) : amount_val.getInt<CAmount>()))
                        throw ContractTermMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_amount).str());
                    if (utxo_it->witness) {
                        // std::format("{}[{}].{}", name_funds, i, name_funds_commit_sig
                        if (utxo_it->witness[0] != unhex<signature>(sig_val.get_str()))
                            throw ContractTermMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_commit_sig).str());
                    }
                    else utxo_it->witness.Set(0, unhex<signature>(sig_val.get_str()));
                }
                ++utxo_it;
            }
        }
    }

    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [](){ return name_mining_fee_rate; });
    DeserializeContractHexData(contract[name_funds_swap_sig_B], m_funds_swap_sig_B, [](){ return name_funds_swap_sig_B; });
    DeserializeContractHexData(contract[name_funds_swap_sig_M], m_funds_swap_sig_M, [](){ return name_funds_swap_sig_M; });
    DeserializeContractHexData(contract[name_ordpayoff_sig], m_ordpayoff_sig, [](){ return name_ordpayoff_sig; });
}
void SwapInscriptionBuilder::Deserialize(const string &data)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() != val_swap_inscription) {
        throw ContractProtocolError("SwapInscription contract does not match " + root[name_contract_type].getValStr());
    }

    const UniValue& contract = root[name_params];

    if (contract[name_version].getInt<uint32_t>() == s_protocol_version_pubkey_v4 || contract[name_version].getInt<uint32_t>() == s_protocol_version_old_v3) {
        Deserialize_v4(data);
        return;
    }
    else if (contract[name_version].getInt<uint32_t>() != s_protocol_version) {
        throw ContractProtocolError("Wrong SwapInscription contract version: " + contract[name_version].getValStr());
    }

    {   const auto& val = contract[name_ord_input];
        if (!val.isNull()) {
            if (m_ord_input) {
                m_ord_input->ReadJson(val);
            }
            else {
                m_ord_input.emplace(bech32(), 0, val);
            }
            if (!m_ord_input->witness) throw ContractTermMissing(move((name_ord_input + '.') += ContractInput::name_witness));
        }
    }

    DeserializeContractAmount(contract[name_ord_price], m_ord_price, [](){ return name_ord_price; });
    DeserializeContractAmount(contract[name_market_fee], m_market_fee, [](){ return name_market_fee; });
    DeserializeContractAmount(contract[name_ord_mining_fee_rate], m_ord_mining_fee_rate, [](){ return name_ord_mining_fee_rate; });
    DeserializeContractHexData(contract[name_swap_script_pk_A], m_swap_script_pk_A, [](){ return name_swap_script_pk_A; });
    DeserializeContractHexData(contract[name_swap_script_pk_B], m_swap_script_pk_B, [](){ return name_swap_script_pk_B; });
    DeserializeContractHexData(contract[name_swap_script_pk_M], m_swap_script_pk_M, [](){ return name_swap_script_pk_M; });
    DeserializeContractHexData(contract[name_funds_unspendable_key], m_funds_unspendable_key_factor, [](){ return name_funds_unspendable_key; });

    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_funds));
            if (!m_fund_inputs.empty() && m_fund_inputs.size() != val.size()) throw ContractTermMismatch(std::string(name_funds) + " size");

            for (size_t i = 0; i < val.size(); ++i) {
                if (i == m_fund_inputs.size()) {
                    std::optional<ContractInput> input = ContractInput(bech32(), i, val[i]);

                    if (!input->witness) throw ContractTermMissing(move(((name_funds + '[') += std::to_string(i) += ']') += ContractInput::name_witness));

                    m_fund_inputs.emplace_back(move(*input));
                }
                else {
                    m_fund_inputs[i].ReadJson(val[i]);
                    if (!m_fund_inputs[i].witness) throw ContractTermMissing(move(((name_funds + '[') += std::to_string(i) += ']') += ContractInput::name_witness));
                }
            }
        }
    }

    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [](){ return name_mining_fee_rate; });
    DeserializeContractHexData(contract[name_funds_swap_sig_B], m_funds_swap_sig_B, [](){ return name_funds_swap_sig_B; });
    DeserializeContractHexData(contract[name_funds_swap_sig_M], m_funds_swap_sig_M, [](){ return name_funds_swap_sig_M; });
    DeserializeContractHexData(contract[name_ordpayoff_sig], m_ordpayoff_sig, [](){ return name_ordpayoff_sig; });
}


const CMutableTransaction &SwapInscriptionBuilder::GetSwapTx() const
{
    if (!mSwapTx) {
        mSwapTx.emplace(MakeSwapTx(true));
    }
    return *mSwapTx;
}

const CMutableTransaction &SwapInscriptionBuilder::GetPayoffTx() const
{
    if (!mOrdPayoffTx) {

        CScript transfer_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

        CMutableTransaction swap_tx(MakeSwapTx(true));

        CMutableTransaction transfer_tx = CreatePayoffTxTemplate();

        transfer_tx.vin[0].prevout.hash = swap_tx.GetHash();
        transfer_tx.vin[0].prevout.n = 0;
        transfer_tx.vin[0].scriptWitness.stack[0] = *m_ordpayoff_sig;

        transfer_tx.vout[0].scriptPubKey = move(transfer_pubkeyscript);
        transfer_tx.vout[0].nValue = CalculateOutputAmount(swap_tx.vout[0].nValue, *m_ord_mining_fee_rate, transfer_tx);

        mOrdPayoffTx = move(transfer_tx);
    }
    return *mOrdPayoffTx;
}

std::vector<std::pair<CAmount,CMutableTransaction>> SwapInscriptionBuilder::GetTransactions() const {
    return {
        { *m_ord_mining_fee_rate, CreatePayoffTxTemplate() },
        { *m_mining_fee_rate, GetSwapTxTemplate() },
        { *m_mining_fee_rate, GetFundsCommitTxTemplate() }
    };
}

void SwapInscriptionBuilder::OrdUTXO(const string &txid, uint32_t nout, const string &amount, const std::string& addr)
{
    try {
        m_ord_input.emplace(bech32(), 0, std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_ord_input.c_str()));
    }
}

void SwapInscriptionBuilder::AddFundsUTXO(const string &txid, uint32_t nout, const string &amount, const std::string& addr)
{
    try {
        m_fund_inputs.emplace_back(bech32(), m_fund_inputs.size(), std::make_shared<UTXO>(bech32(), txid, nout, ParseAmount(amount), addr));
    }
    catch(...) {
        std::throw_with_nested(ContractTermWrongValue(name_funds + '[' + std::to_string(m_fund_inputs.size()) + ']'));
    }
}

CMutableTransaction SwapInscriptionBuilder::CreatePayoffTxTemplate() const {
    CMutableTransaction result;
    CScript pubKeyScript = CScript() << 1 << xonly_pubkey();

    result.vin = {CTxIn(uint256(0), 0)};
    result.vin.front().scriptWitness.stack.push_back(signature());
    result.vout = {CTxOut(0, move(pubKeyScript))};
    result.vout.front().nValue = 0;

    return result;
}


void SwapInscriptionBuilder::CheckOrdSwapSig() const
{
    bool has_funds_sig = m_funds_unspendable_key_factor && m_funds_swap_sig_B && m_funds_swap_sig_M;

    std::vector<CTxOut> spent_outs = {CTxOut(m_ord_input->output->Destination()->Amount(), m_ord_input->output->Destination()->PubKeyScript())};
    if (has_funds_sig) {
        spent_outs.emplace_back(GetFundsCommitTx().vout.front());
    }

    if (mSwapTx) {
        VerifyTxSignature(m_ord_input->output->Destination()->Address(), m_ord_input->witness, *mSwapTx, 0, move(spent_outs));
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(has_funds_sig));
        VerifyTxSignature(m_ord_input->output->Destination()->Address(), m_ord_input->witness, swap_tx, 0, move(spent_outs));
    }
}

std::string SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const
{
    return FormatAmount(*m_ord_price + *m_market_fee + CalculateWholeFee(params));
}

void SwapInscriptionBuilder::CheckFundsSwapSig() const
{
    std::vector<CTxOut> spent_outs = {CTxOut(m_ord_input->output->Destination()->Amount(), m_ord_input->output->Destination()->PubKeyScript()), GetFundsCommitTx().vout.front()};

    if (mSwapTx) {
        VerifyTxSignature(*m_swap_script_pk_B, *m_funds_swap_sig_B, *mSwapTx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(true));
        VerifyTxSignature(*m_swap_script_pk_B, *m_funds_swap_sig_B, swap_tx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
}

void SwapInscriptionBuilder::CheckOrdPayoffSig() const
{
    if (mSwapTx) {
        VerifyTxSignature(*m_swap_script_pk_M, *m_ordpayoff_sig, GetPayoffTx(), 0, {mSwapTx->vout.front()}, {});
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(true));
        VerifyTxSignature(*m_swap_script_pk_M, *m_ordpayoff_sig, GetPayoffTx(), 0, {swap_tx.vout.front()}, {});
    }
}


} // namespace l15::utxord
