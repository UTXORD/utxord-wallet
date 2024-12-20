#include <ranges>

#include "smartinserter.hpp"

#include "policy.h"
#include "feerate.h"

#include "contract_builder_factory.hpp"
#include "swap_inscription.hpp"

namespace utxord {

using l15::core::SchnorrKeyPair;
using l15::ScriptMerkleTree;
using l15::TreeBalanceType;
using l15::ParseAmount;
using l15::FormatAmount;
using l15::CalculateOutputAmount;
using l15::EncodeHexTx;

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

const uint32_t SwapInscriptionBuilder::s_protocol_version = 6;
const uint32_t SwapInscriptionBuilder::s_protocol_version_no_p2address = 5;
const char* SwapInscriptionBuilder::s_versions = "[5,6]";

const std::string SwapInscriptionBuilder::name_ord_price = "ord_price";

const std::string SwapInscriptionBuilder::name_ord_mining_fee_rate = "ord_mining_fee_rate";

const std::string SwapInscriptionBuilder::name_swap_script_pk_A = "swap_script_pk_A";
const std::string SwapInscriptionBuilder::name_swap_script_pk_B = "swap_script_pk_B";
const std::string SwapInscriptionBuilder::name_swap_script_pk_M = "swap_script_pk_M";

const std::string SwapInscriptionBuilder::name_ord_payoff_addr = "ord_payoff_addr";
const std::string SwapInscriptionBuilder::name_funds_payoff_addr = "funds_payoff_addr";

const std::string SwapInscriptionBuilder::name_ord_input = "ord_utxo";
const std::string SwapInscriptionBuilder::name_ord_txid = "ord_txid";
const std::string SwapInscriptionBuilder::name_ord_nout = "ord_nout";
const std::string SwapInscriptionBuilder::name_ord_amount = "ord_amount";
const std::string SwapInscriptionBuilder::name_ord_pk = "ord_pk";


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
const std::string SwapInscriptionBuilder::name_ord_payoff_sig = "ordpayoff_sig";


const std::string& SwapInscriptionBuilder::GetContractName() const
{ return val_swap_inscription; }

CAmount SwapInscriptionBuilder::CalculateWholeFee(const std::string& params) const {
    if (!m_ord_mining_fee_rate) throw ContractStateError(name_ord_mining_fee_rate + " not defined");
    if (!m_mining_fee_rate) throw ContractStateError(name_mining_fee_rate + " not defined");

    bool change = false, p2wpkh_utxo = false;

    std::istringstream ss(params);
    std::string param;
    while(std::getline(ss, param, ',')) {
        if (param == "change") { change = true; continue; }
        if (param == "p2wpkh_utxo") { p2wpkh_utxo = true; continue; }
        throw l15::IllegalArgument(move(param));
    }

    CAmount fee =  l15::CalculateTxFee(*m_mining_fee_rate, GetFundsCommitTxTemplate(p2wpkh_utxo))
         + l15::CalculateTxFee(*m_mining_fee_rate, GetSwapTxTemplate())
         + l15::CalculateTxFee(*m_ord_mining_fee_rate, GetFundsCommitTxTemplate());

    if (change) fee += CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_VOUT_VSIZE);

    return fee;
}

std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> SwapInscriptionBuilder::FundsCommitTapRoot() const
{
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED,
                              { MakeFundsSwapScript(m_swap_script_pk_B.value(), m_swap_script_pk_M.value()),
                                MakeRelTimeLockScript(COMMIT_TIMEOUT, m_swap_script_pk_B.value())});

    return std::tuple_cat(SchnorrKeyPair::AddTapTweak(KeyPair::GetStaticSecp256k1Context(), SchnorrKeyPair::CreateUnspendablePubKey(m_funds_unspendable_key_factor.value()),
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
        if (m_market_fee->Amount() != 0) {
            swapTpl.vout.emplace_back(m_market_fee->TxOutput());
        }

        swapTpl.vin.emplace_back(COutPoint(Txid(), 0));
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

        swapTpl.vin.emplace_back(Txid(), 0);
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
    if (!m_ord_input) throw ContractStateError(name_ord_input + " not defined");
    if (!m_funds_payoff_addr) throw ContractStateError(name_funds_payoff_addr + " not defined");
    if (!m_funds_unspendable_key_factor) throw ContractStateError(name_funds_unspendable_key + " not defined");

    CMutableTransaction swap_tx = GetSwapTxTemplate();

    swap_tx.vin[0].prevout = COutPoint(Txid::FromUint256(uint256S(m_ord_input->output->TxID())), m_ord_input->output->NOut());
    if (m_ord_input->witness) {
        swap_tx.vin[0].scriptWitness.stack = m_ord_input->witness;
    }

    swap_tx.vout[0].nValue = m_ord_input->output->Destination()->Amount() + CFeeRate(*m_ord_mining_fee_rate).GetFee(MIN_TAPROOT_TX_VSIZE);

    swap_tx.vout[1].scriptPubKey = P2Address::Construct(chain(), 0, *m_funds_payoff_addr)->PubKeyScript();

    if (with_funds_in) {
        auto funds_commit_taproot = FundsCommitTapRoot();

        xonly_pubkey funds_unspendable_key = SchnorrKeyPair::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

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

    auto stack = signer->Sign(swap_tx, 0, {m_ord_input->output->Destination()->TxOutput()}, SIGHASH_ALL|SIGHASH_ANYONECANPAY);

    for (size_t i = 0; i < stack.size(); ++i)
        m_ord_input->witness.Set(i, stack[i]);
}

CMutableTransaction SwapInscriptionBuilder::GetFundsCommitTxTemplate(bool segwit_in) const
{
    CMutableTransaction commitTpl;

    auto commit_pubkeyscript = CScript() << 1 << get<0>(FundsCommitTemplateTapRoot());
    auto change_pubkeyscript = CScript() << 1 << xonly_pubkey();

    commitTpl.vout = {CTxOut(0, commit_pubkeyscript), CTxOut(0, change_pubkeyscript)};

    commitTpl.vin.reserve(m_fund_inputs.size());

    if (m_fund_inputs.empty()) {
        commitTpl.vin.emplace_back(Txid(), 0);
        if (segwit_in) {
            commitTpl.vin.back().scriptWitness.stack.emplace_back(71);
            commitTpl.vin.back().scriptWitness.stack.emplace_back(33);
        }
        else {
            commitTpl.vin.back().scriptWitness.stack.emplace_back(64);
        }
    }
    else {
        for (const auto &utxo: m_fund_inputs) {
            if (commitTpl.vin.size() > utxo.nin) {
                commitTpl.vin[utxo.nin].prevout = COutPoint(Txid::FromUint256(uint256S(utxo.output->TxID())), utxo.output->NOut());
                if (utxo.witness)
                    commitTpl.vin[utxo.nin].scriptWitness.stack = utxo.witness;
            }
            else {
                if (utxo.nin > commitTpl.vin.size()) throw ContractError(name_funds + " are inconsistent");

                commitTpl.vin.emplace_back(Txid::FromUint256(uint256S(utxo.output->TxID())), utxo.output->NOut());

                if (utxo.witness)
                    commitTpl.vin[utxo.nin].scriptWitness.stack = utxo.witness;
                else
                    commitTpl.vin[utxo.nin].scriptWitness.stack = utxo.output->Destination()->DummyWitness();
            }
        }
    }
    return commitTpl;
}

CMutableTransaction SwapInscriptionBuilder::MakeFundsCommitTx() const
{
    CMutableTransaction commit_tx = GetFundsCommitTxTemplate();

    CAmount funds_required = GetMinFundingAmount("");
    CAmount funds_provided = 0;
    for (const auto &utxo: m_fund_inputs) {
        funds_provided += utxo.output->Destination()->Amount();
    }

    CAmount change = funds_provided - funds_required;

    if(change > l15::Dust(3000) && m_change_addr) {
        commit_tx.vout[1] = P2Address::Construct(chain(), change, *m_change_addr)->TxOutput();
    } else {
        commit_tx.vout.pop_back();

        funds_required = GetMinFundingAmount(""); // Calculate again since one output was removed
    }

    if(funds_provided < funds_required) {
        throw ContractFundsNotEnough("funds");
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
    CheckContractTerms(s_protocol_version, FUNDS_TERMS);

    m_funds_unspendable_key_factor = SchnorrKeyPair::GetStrongRandomKey(master_key.Secp256k1Context());

    CMutableTransaction commit_tx = MakeFundsCommitTx();

    std::vector<CTxOut> spent_outs;
    for (const auto& fund: m_fund_inputs) {
        spent_outs.emplace_back(fund.output->Destination()->TxOutput());
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
    CheckContractTerms(s_protocol_version, MARKET_PAYOFF_SIG);

    auto keypair = master_key.Lookup(*m_swap_script_pk_B, key_filter);

    const CMutableTransaction& funds_commit = GetFundsCommitTx();
    CMutableTransaction swap_tx(MakeSwapTx(true));

    SchnorrKeyPair key(keypair.PrivKey());
    m_funds_swap_sig_B = key.SignTaprootTx(swap_tx, 1, {m_ord_input->output->Destination()->TxOutput(), funds_commit.vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
}

void SwapInscriptionBuilder::SignFundsPayBack(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(s_protocol_version, FUNDS_COMMIT_SIG);

    const CMutableTransaction& funds_commit = GetFundsCommitTx(); // Request it here in order to force reuired fields check

    auto keypair = master_key.Lookup(*m_swap_script_pk_B, key_filter);
    SchnorrKeyPair key(keypair.PrivKey());

    auto commit_taproot = FundsCommitTapRoot();
    //auto commit_pubkeyscript = CScript() << 1 << get<0>(commit_taproot);
    auto payoff_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

    xonly_pubkey internal_unspendable_key = SchnorrKeyPair::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

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
    CheckContractTerms(s_protocol_version, MARKET_PAYOFF_TERMS);

    auto keypair = master_key.Lookup(*m_swap_script_pk_M, key_filter);
    SchnorrKeyPair key(keypair.PrivKey());

    CMutableTransaction swap_tx(MakeSwapTx(true));

    CMutableTransaction transfer_tx = CreatePayoffTxTemplate();

    transfer_tx.vin[0].prevout.hash = swap_tx.GetHash();
    transfer_tx.vin[0].prevout.n = 0;

    transfer_tx.vout[0] = P2Address::Construct(chain(), m_ord_input->output->Destination()->Amount(), *m_ord_payoff_addr)->TxOutput();

    m_ord_payoff_sig = key.SignTaprootTx(transfer_tx, 0, {swap_tx.vout[0]}, {});

    transfer_tx.vin[0].scriptWitness.stack[0] = *m_ord_payoff_sig;

    mOrdPayoffTx = move(transfer_tx);
}

void SwapInscriptionBuilder::MarketSignSwap(const KeyRegistry &master_key, const std::string& key_filter)
{
    CheckContractTerms(s_protocol_version, FUNDS_SWAP_SIG);

    auto keypair = master_key.Lookup(*m_swap_script_pk_M, key_filter);
    SchnorrKeyPair key(keypair.PrivKey());

    auto utxo_pubkeyscript = m_ord_input->output->Destination()->PubKeyScript();

    CMutableTransaction swap_tx(MakeSwapTx(true));

    m_funds_swap_sig_M = key.SignTaprootTx(swap_tx, 1, {m_ord_input->output->Destination()->TxOutput(), GetFundsCommitTx().vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));

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
    std::string res = EncodeHexTx(GetFundsCommitTx());
    return res;
}

string SwapInscriptionBuilder::FundsPayBackRawTransaction() const
{
    if (!mFundsPaybackTx) {
        throw ContractStateError("FundsPayOff transaction data unavailable");
    }
    std::string res = EncodeHexTx(*mFundsPaybackTx);
    return res;
}

string SwapInscriptionBuilder::OrdSwapRawTransaction() const
{
    std::string res = EncodeHexTx(GetSwapTx());
    return res;
}

string SwapInscriptionBuilder::OrdPayoffRawTransaction() const
{
    std::string res = EncodeHexTx(GetPayoffTx());
    return res;
}

UniValue SwapInscriptionBuilder::MakeJson(uint32_t version, SwapPhase phase) const
{
    if (version != s_protocol_version && version != s_protocol_version_no_p2address) throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    UniValue contract(UniValue::VOBJ);

    contract.pushKV(name_version, version);
    contract.pushKV(name_ord_price, *m_ord_price);
    contract.pushKV(name_market_fee, m_market_fee->MakeJson());
    contract.pushKV(name_swap_script_pk_M, hex(*m_swap_script_pk_M));
    contract.pushKV(name_ord_mining_fee_rate, *m_ord_mining_fee_rate);

    if (phase == ORD_SWAP_SIG || phase == MARKET_PAYOFF_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_ord_input, m_ord_input->MakeJson());
        contract.pushKV(name_funds_payoff_addr, *m_funds_payoff_addr);
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
        contract.pushKV(name_ord_payoff_addr, *m_ord_payoff_addr);
        if (m_change_addr)
            contract.pushKV(name_change_addr, *m_change_addr);

        contract.pushKV(name_funds_unspendable_key, hex(*m_funds_unspendable_key_factor));
        contract.pushKV(name_swap_script_pk_B, hex(*m_swap_script_pk_B));
    }

    if (phase == MARKET_PAYOFF_SIG) {
        contract.pushKV(name_ord_payoff_sig, hex(*m_ord_payoff_sig));
    }

    if (phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_funds_swap_sig_B, hex(*m_funds_swap_sig_B));
    }

    if (phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_funds_swap_sig_M, hex(*m_funds_swap_sig_M));
    }

    return contract;
}

void SwapInscriptionBuilder::CheckContractTerms(uint32_t version, SwapPhase phase) const
{
    if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
    if (m_market_fee->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
        throw ContractProtocolError(name_market_fee + '.' + IContractDestination::name_addr + ": " + m_market_fee->Address() + " is not supported with v. " + std::to_string(version));
    if (!m_swap_script_pk_M) throw ContractTermMissing(std::string(name_swap_script_pk_M));

    switch (phase) {
    case MARKET_SWAP_SIG:
        if (!m_funds_swap_sig_M) throw ContractTermMissing(std::string(name_funds_swap_sig_M));
        // no break;
    case FUNDS_SWAP_SIG:
        if (!m_funds_swap_sig_B) throw ContractTermMissing(std::string(name_funds_swap_sig_B));
        // no break;
    case MARKET_PAYOFF_SIG:
        if (!m_ord_payoff_sig) throw ContractTermMissing(std::string(name_ord_payoff_sig));
        // no break;
    case MARKET_PAYOFF_TERMS:
        CheckContractTerms(version, FUNDS_COMMIT_SIG);
    case ORD_SWAP_SIG:
        if (!m_ord_price) throw ContractTermMissing(std::string(name_ord_price));
        if (!m_ord_mining_fee_rate) throw ContractTermMissing(std::string(name_ord_mining_fee_rate));
        if (!m_ord_input) throw ContractTermMissing(std::string(name_ord_input));
        if (m_ord_input->output->Destination()->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
            throw ContractProtocolError(name_ord_input + '.' + IContractDestination::name_addr + ": " + m_ord_input->output->Destination()->Address() + " is not supported with v. " + std::to_string(version));
        if (!m_ord_input->witness) throw ContractTermMissing(std::string(TxInput::name_witness));
        if (m_funds_payoff_addr) {
            auto fakePayOff = P2Address::Construct(chain(), *m_ord_price, *m_funds_payoff_addr);
            if (fakePayOff->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_funds_payoff_addr + ": " + *m_funds_payoff_addr + " is not supported with v. " + std::to_string(version));
        }
        else throw ContractTermMissing(std::string(name_funds_payoff_addr));
        // no break;
    case ORD_TERMS:
        break;
    case FUNDS_COMMIT_SIG:
        if (m_fund_inputs.empty()) throw ContractTermMissing(std::string(name_funds));
        {
            CAmount funds_amount = 0;
            for (const auto& utxo: m_fund_inputs) {
                if (!utxo.witness) throw ContractTermMissing(move(((name_funds + '[') += std::to_string(utxo.nin) += "].") += TxInput::name_witness));
                funds_amount += utxo.output->Destination()->Amount();
            }
            CAmount req_amount = GetMinFundingAmount("");
            if (funds_amount < req_amount) throw ContractFundsNotEnough(FormatAmount(funds_amount) + ", required: " + FormatAmount(req_amount));
        }
        if (!m_swap_script_pk_B) throw ContractTermMissing(std::string(name_swap_script_pk_B));
        if (m_ord_payoff_addr) {
            auto fakePayOff = P2Address::Construct(chain(), 546, *m_ord_payoff_addr);
            if (fakePayOff->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_ord_payoff_addr + ": " + *m_ord_payoff_addr + " is not supported with v. " + std::to_string(version));

        }
        else throw ContractTermMissing(name_ord_payoff_addr.c_str());
        if (!m_funds_unspendable_key_factor) throw ContractTermMissing(std::string(name_funds_unspendable_key));
        // no break;
    case FUNDS_TERMS:
        if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
        if (m_market_fee->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
            throw ContractProtocolError(name_market_fee + '.' + IContractDestination::name_addr + ": " + m_market_fee->Address() + " is not supported with v. " + std::to_string(version));
        if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        break;
    }

    switch (phase) {
    case MARKET_SWAP_SIG:
        CheckMarketSwapSig();
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
        CheckFundsCommitSig();
    case FUNDS_TERMS:
        break;
    }
}

void SwapInscriptionBuilder::ReadJson(const UniValue& contract, SwapPhase phase)
{
//    if (contract[name_version].getInt<uint32_t>() == s_protocol_version_pubkey_v4 || contract[name_version].getInt<uint32_t>() == s_protocol_version_old_v3) {
//        ReadJson_v4(contract, phase);
//        return;
//    }
//    else
    if (contract[name_version].getInt<uint32_t>() != s_protocol_version &&
        contract[name_version].getInt<uint32_t>() != s_protocol_version_no_p2address) {
        throw ContractProtocolError("Wrong SwapInscription contract version: " + contract[name_version].getValStr());
    }

    {   const auto& val = contract[name_ord_input];
        if (!val.isNull()) {
            if (m_ord_input) {
                m_ord_input->ReadJson(val, [](){ return name_ord_input; });
            }
            else {
                m_ord_input.emplace(chain(), 0, val, [](){ return name_ord_input; });
            }
            if (!m_ord_input->witness) throw ContractTermMissing(move((name_ord_input + '.') += TxInput::name_witness));
        }
    }

    DeserializeContractAmount(contract[name_ord_price], m_ord_price, [](){ return name_ord_price; });
    {   const auto& val = contract[name_market_fee];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(std::string(name_market_fee));

            if (m_market_fee)
                m_market_fee->ReadJson(val, [](){ return name_market_fee; });
            else
                m_market_fee = DestinationFactory::ReadJson(chain(), val, [](){ return name_market_fee; });

        }
    }
    DeserializeContractAmount(contract[name_ord_mining_fee_rate], m_ord_mining_fee_rate, [](){ return name_ord_mining_fee_rate; });
    DeserializeContractString(contract[name_change_addr], m_change_addr, [&](){ return name_change_addr; });
    DeserializeContractString(contract[name_funds_payoff_addr], m_funds_payoff_addr, [](){ return name_funds_payoff_addr; });
    DeserializeContractHexData(contract[name_swap_script_pk_B], m_swap_script_pk_B, [](){ return name_swap_script_pk_B; });
    DeserializeContractHexData(contract[name_swap_script_pk_M], m_swap_script_pk_M, [](){ return name_swap_script_pk_M; });
    DeserializeContractHexData(contract[name_funds_unspendable_key], m_funds_unspendable_key_factor, [](){ return name_funds_unspendable_key; });

    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_funds));
            if (!m_fund_inputs.empty() && m_fund_inputs.size() != val.size()) throw ContractTermMismatch(std::string(name_funds) + " size");

            for (size_t i = 0; i < val.size(); ++i) {
                if (i == m_fund_inputs.size()) {
                    std::optional<TxInput> input = TxInput(chain(), i, val[i], [i](){ return (std::ostringstream() << name_funds << '[' << i << ']').str(); });

                    if (!input->witness) throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << TxInput::name_witness).str());

                    m_fund_inputs.emplace_back(move(*input));
                }
                else {
                    m_fund_inputs[i].ReadJson(val[i], [i](){ return (std::ostringstream() << name_funds << '[' << i << ']').str(); });
                    if (!m_fund_inputs[i].witness) throw ContractTermMissing((std::ostringstream() << name_funds << '[' << i << "]." << TxInput::name_witness).str());
                }
            }
        }
    }

    DeserializeContractString(contract[name_ord_payoff_addr], m_ord_payoff_addr, [](){ return name_ord_payoff_addr; });
    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [](){ return name_mining_fee_rate; });
    DeserializeContractHexData(contract[name_funds_swap_sig_B], m_funds_swap_sig_B, [](){ return name_funds_swap_sig_B; });
    DeserializeContractHexData(contract[name_funds_swap_sig_M], m_funds_swap_sig_M, [](){ return name_funds_swap_sig_M; });
    DeserializeContractHexData(contract[name_ord_payoff_sig], m_ord_payoff_sig, [](){ return name_ord_payoff_sig; });
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

        CMutableTransaction swap_tx(MakeSwapTx(true));

        CMutableTransaction transfer_tx = CreatePayoffTxTemplate();

        transfer_tx.vin[0].prevout.hash = swap_tx.GetHash();
        transfer_tx.vin[0].prevout.n = 0;
        transfer_tx.vin[0].scriptWitness.stack[0] = *m_ord_payoff_sig;

        transfer_tx.vout[0] = P2Address::Construct(chain(), m_ord_input->output->Destination()->Amount(), *m_ord_payoff_addr)->TxOutput();

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

void SwapInscriptionBuilder::OrdUTXO(string txid, uint32_t nout, CAmount amount, std::string addr)
{
    m_ord_input.emplace(chain(), 0, std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
}

void SwapInscriptionBuilder::AddFundsUTXO(string txid, uint32_t nout, CAmount amount, std::string addr)
{
    uint32_t i = m_fund_inputs.size();
    m_fund_inputs.emplace_back(chain(), m_fund_inputs.size(), std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr)));
}

CMutableTransaction SwapInscriptionBuilder::CreatePayoffTxTemplate() const {
    CMutableTransaction result;

    result.vin = {{}};
    result.vin.front().scriptWitness.stack.push_back(signature());
    result.vout = {CTxOut(0, CScript() << 1 << xonly_pubkey())};
    result.vout.front().nValue = 0;

    return result;
}


void SwapInscriptionBuilder::CheckOrdSwapSig() const
{
    bool has_funds_sig = m_funds_unspendable_key_factor && m_funds_swap_sig_B && m_funds_swap_sig_M;

    std::vector<CTxOut> spent_outs = {m_ord_input->output->Destination()->TxOutput()};
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

CAmount SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const
{
    if (!m_ord_price) throw ContractStateError(name_ord_price + " not defined");
    if (!m_market_fee) throw ContractStateError(name_market_fee + " not defined");
    return *m_ord_price + m_market_fee->Amount() + CalculateWholeFee(params);
}

void SwapInscriptionBuilder::CheckFundsCommitSig() const
{
    std::vector<CTxOut> spent_outs;//
    std::ranges::transform(m_fund_inputs, cex::smartinserter(spent_outs, spent_outs.end()),
                           [](const TxInput& in){ return in.output->Destination()->TxOutput(); });

    if (mFundsCommitTx) {
        for (const auto& in: m_fund_inputs) {
            VerifyTxSignature(in.output->Destination()->Address(), in.witness, *mFundsCommitTx, in.nin, std::vector<CTxOut>(spent_outs));
        }
    }
    else {
        CMutableTransaction swap_tx(MakeFundsCommitTx());
        for (const auto& in: m_fund_inputs) {
            VerifyTxSignature(in.output->Destination()->Address(), in.witness, swap_tx, in.nin, std::vector<CTxOut>(spent_outs));
        }
    }
}

void SwapInscriptionBuilder::CheckFundsSwapSig() const
{
    std::vector<CTxOut> spent_outs = {m_ord_input->output->Destination()->TxOutput(), GetFundsCommitTx().vout.front()};

    if (mSwapTx) {
        VerifyTxSignature(*m_swap_script_pk_B, *m_funds_swap_sig_B, *mSwapTx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(true));
        VerifyTxSignature(*m_swap_script_pk_B, *m_funds_swap_sig_B, swap_tx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
}

void SwapInscriptionBuilder::CheckMarketSwapSig() const
{
    std::vector<CTxOut> spent_outs = {m_ord_input->output->Destination()->TxOutput(), GetFundsCommitTx().vout.front()};

    if (mSwapTx) {
        VerifyTxSignature(*m_swap_script_pk_M, *m_funds_swap_sig_M, *mSwapTx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(true));
        VerifyTxSignature(*m_swap_script_pk_M, *m_funds_swap_sig_M, swap_tx, 1, move(spent_outs), MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
    }
}

void SwapInscriptionBuilder::CheckOrdPayoffSig() const
{
    if (mSwapTx) {
        VerifyTxSignature(*m_swap_script_pk_M, *m_ord_payoff_sig, GetPayoffTx(), 0, {mSwapTx->vout.front()}, {});
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(true));
        VerifyTxSignature(*m_swap_script_pk_M, *m_ord_payoff_sig, GetPayoffTx(), 0, {swap_tx.vout.front()}, {});
    }
}

uint32_t SwapInscriptionBuilder::TransactionCount(SwapPhase phase) const
{
    switch (phase) {
    case FUNDS_COMMIT_SIG:
    case ORD_SWAP_SIG:
        return 1;
    case MARKET_SWAP_SIG:
    case FUNDS_SWAP_SIG:
        return 3;
//    case FUNDS_TERMS:
//    case ORD_TERMS:
//    case MARKET_PAYOFF_SIG:
//    case MARKET_PAYOFF_TERMS:
    default:
        return 0;
    }
}

std::string SwapInscriptionBuilder::RawTransaction(SwapPhase phase, uint32_t n) const
{
    switch (phase) {
    case ORD_SWAP_SIG:
        if (n == 0) {
            return EncodeHexTx(MakeSwapTx(false));
        }
        else throw ContractStateError("Transaction unavailable: " + std::to_string(n));
    case FUNDS_COMMIT_SIG:
        if (n == 0) {
            return FundsCommitRawTransaction();
        }
        else throw ContractStateError("Transaction unavailable: " + std::to_string(n));
    case MARKET_SWAP_SIG:
    case FUNDS_SWAP_SIG:
        switch(n) {
        case 0:
            return FundsCommitRawTransaction();
        case 1:
            return OrdSwapRawTransaction();
        case 2:
            return OrdPayoffRawTransaction();
        default:
            throw ContractStateError("Transaction unavailable: " + std::to_string(n));
        }
//    case FUNDS_TERMS:
//    case ORD_TERMS:
//    case MARKET_PAYOFF_SIG:
//    case MARKET_PAYOFF_TERMS:
    default:
        return {};
    }
}

std::shared_ptr<IContractOutput> SwapInscriptionBuilder::InscriptionOutput() const
{
    auto tx = GetPayoffTx();
    return std::make_shared<UTXO>(chain(), tx.GetHash().GetHex(), 0, tx.vout.front().nValue, *m_ord_payoff_addr);
}

std::shared_ptr<IContractOutput> SwapInscriptionBuilder::FundsOutput() const
{
    auto tx = GetSwapTx();
    return std::make_shared<UTXO>(chain(), tx.GetHash().GetHex(), 1, tx.vout[1].nValue, *m_funds_payoff_addr);
}

std::shared_ptr<IContractOutput> SwapInscriptionBuilder::ChangeOutput() const
{
    std::shared_ptr<IContractOutput> res;
    if (m_change_addr) {
        auto commitTx = GetFundsCommitTx();
        if (commitTx.vout.size() > 1) {
            res = std::make_shared<UTXO>(chain(), commitTx.GetHash().GetHex(), 1, commitTx.vout[1].nValue, *m_change_addr);
        }
    }
    return res;
}

} // namespace l15::utxord
