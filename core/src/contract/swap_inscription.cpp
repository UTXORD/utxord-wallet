#include <ranges>
//#include <format>

#include "swap_inscription.hpp"

#include "univalue.h"

#include "core_io.h"
#include "policy.h"
#include "feerate.h"

#include "channel_keys.hpp"

namespace l15::utxord {

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
    script << GetCsvInBlocks(blocks_to_lock) << OP_CHECKSEQUENCEVERIFY << OP_DROP;
    script << pk << OP_CHECKSIG;
    return script;
}

}

const uint32_t SwapInscriptionBuilder::m_protocol_version = 3;

const std::string SwapInscriptionBuilder::name_ord_price = "ord_price";
const std::string SwapInscriptionBuilder::name_market_fee = "market_fee";

const std::string SwapInscriptionBuilder::name_ord_mining_fee_rate = "ord_mining_fee_rate";

const std::string SwapInscriptionBuilder::name_swap_script_pk_A = "swap_script_pk_A";
const std::string SwapInscriptionBuilder::name_swap_script_pk_B = "swap_script_pk_B";
const std::string SwapInscriptionBuilder::name_swap_script_pk_M = "swap_script_pk_M";

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
const std::string SwapInscriptionBuilder::name_ordpayoff_sig = "ordpayoff_sig";

SwapInscriptionBuilder::SwapInscriptionBuilder(const string &ord_price, const string &market_fee)
        : ContractBuilder(), m_ord_price(ParseAmount(ord_price)), m_market_fee(ParseAmount(market_fee)) {};



std::tuple<xonly_pubkey, uint8_t, ScriptMerkleTree> SwapInscriptionBuilder::FundsCommitTapRoot() const
{
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED,
                              { MakeFundsSwapScript(m_swap_script_pk_B.value(), m_swap_script_pk_M.value()),
                                MakeRelTimeLockScript(COMMIT_TIMEOUT, m_swap_script_pk_B.value())});

    return std::tuple_cat(core::ChannelKeys::AddTapTweak(core::ChannelKeys::CreateUnspendablePubKey(m_funds_unspendable_key_factor.value()),
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
        swapTpl.vout.emplace_back(m_ord_price, CScript() << 1 << xonly_pubkey());
        swapTpl.vout.emplace_back(*m_market_fee, CScript() << 1 << *m_swap_script_pk_M);

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

    swap_tx.vin[0].prevout = COutPoint(uint256S(*m_ord_txid), *m_ord_nout);
    if (m_ord_swap_sig_A) {
        swap_tx.vin[0].scriptWitness.stack[0] = *m_ord_swap_sig_A;
    }

    swap_tx.vout[0].nValue = *m_ord_amount + CFeeRate(*m_ord_mining_fee_rate).GetFee(MIN_TAPROOT_TX_VSIZE);
    swap_tx.vout[1].scriptPubKey = CScript() << 1 << *m_swap_script_pk_A;

    if (with_funds_in) {
        auto funds_commit_taproot = FundsCommitTapRoot();

        xonly_pubkey funds_unspendable_key = core::ChannelKeys::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

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

void SwapInscriptionBuilder::SignOrdSwap(const std::string& sk)
{
    core::ChannelKeys keypair(unhex<seckey>(sk));

    m_ord_pk = keypair.GetLocalPubKey();

    auto utxo_pubkeyscript = CScript() << 1 << (keypair.GetLocalPubKey());

    CMutableTransaction swap_tx(MakeSwapTx(false));

    m_ord_swap_sig_A = keypair.SignTaprootTx(swap_tx, 0, {CTxOut(*m_ord_amount, utxo_pubkeyscript)}, {}, SIGHASH_ALL|SIGHASH_ANYONECANPAY);
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
    mFundsCommitTpl->vin.reserve(m_funds.size());

    if (m_funds.empty()) {
        mFundsCommitTpl->vin.emplace_back(uint256(), 0);
        mFundsCommitTpl->vin.back().scriptWitness.stack.emplace_back(64);
    }
    else {
        uint32_t n = 0;
        for (const auto &utxo: m_funds) {
            if (mFundsCommitTpl->vin.size() > n) {
                mFundsCommitTpl->vin[n].prevout = COutPoint(uint256S(utxo.m_txid), utxo.m_nout);
                if (utxo.m_sig)
                    mFundsCommitTpl->vin[n].scriptWitness.stack[0] = *utxo.m_sig;
            }
            else {
                mFundsCommitTpl->vin.emplace_back(uint256S(utxo.m_txid), utxo.m_nout);
                if (utxo.m_sig)
                    mFundsCommitTpl->vin[n].scriptWitness.stack.push_back(*utxo.m_sig);
                else
                    mFundsCommitTpl->vin[n].scriptWitness.stack.emplace_back(64);
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
    for (const auto &utxo: m_funds) {
        funds_provided += utxo.m_amount;
    }

    CAmount change = funds_provided - funds_required;

    if(change > Dust(3000)) {
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


void SwapInscriptionBuilder::SignFundsCommitment(uint32_t n, const std::string& sk)
{
    CheckContractTerms(FUNDS_TERMS);

    if (n >= m_funds.size())
        throw ContractTermWrongValue((std::ostringstream() << name_funds << '[' << n << "]: " << n << ">= size: " << m_funds.size()).str());

    auto funds_it = m_funds.begin();
    std::advance(funds_it, n);

    core::ChannelKeys keypair(unhex<seckey>(sk));

    if (!funds_it->m_pubkey)
        throw ContractTermMissing((std::ostringstream() << name_funds << '[' << n << "].pubkey").str());
    if (keypair.GetLocalPubKey() != funds_it->m_pubkey)
        throw ContractValueMismatch((std::ostringstream() << name_funds << '[' << n << "].pubkey").str());

    if (!m_funds_unspendable_key_factor)
        m_funds_unspendable_key_factor = core::ChannelKeys::GetStrongRandomKey(keypair.Secp256k1Context());

    std::clog << "Signing input#" << n << std::endl;

    CMutableTransaction commit_tx = MakeFundsCommitTx();

    std::vector<CTxOut> spent_outs;
    for (const auto& utxo: m_funds) {
        spent_outs.emplace_back(utxo.m_amount, CScript() << 1 << *utxo.m_pubkey);
    }

    funds_it->m_sig = keypair.SignTaprootTx(commit_tx, n, std::vector<CTxOut>(spent_outs), {});
    commit_tx.vin[n].scriptWitness.stack[0] = *funds_it->m_sig;
}

void SwapInscriptionBuilder::SignFundsSwap(const std::string& sk)
{
    CheckContractTerms(MARKET_PAYOFF_SIG);

    core::ChannelKeys keypair(unhex<seckey>(sk));

    if (keypair.GetLocalPubKey() != *m_swap_script_pk_B) {
        throw ContractError("Swap PubKey does not match the secret");
    }

    auto utxo_pubkeyscript = CScript() << 1 << (*m_ord_pk);

    const CMutableTransaction& funds_commit = GetFundsCommitTx();
    CMutableTransaction swap_tx(MakeSwapTx(true));

    m_funds_swap_sig_B = keypair.SignTaprootTx(swap_tx, 1, {CTxOut(*m_ord_amount, utxo_pubkeyscript), funds_commit.vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));
}

void SwapInscriptionBuilder::SignFundsPayBack(const std::string& sk)
{
    const CMutableTransaction& funds_commit = GetFundsCommitTx(); // Request it here in order to force reuired fields check

    core::ChannelKeys keypair(unhex<seckey>(sk));

    auto commit_taproot = FundsCommitTapRoot();
    //auto commit_pubkeyscript = CScript() << 1 << get<0>(commit_taproot);
    auto payoff_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

    xonly_pubkey internal_unspendable_key = core::ChannelKeys::CreateUnspendablePubKey(*m_funds_unspendable_key_factor);

    CScript& payback_script = get<2>(commit_taproot).GetScripts()[1];

    auto commit_scriptpath = get<2>(commit_taproot).CalculateScriptPath(payback_script);
    bytevector control_block = {static_cast<uint8_t>(0xc0 | get<1>(commit_taproot))};
    control_block.reserve(1 + internal_unspendable_key.size() + commit_scriptpath.size() * uint256::size());
    control_block.insert(control_block.end(), internal_unspendable_key.begin(), internal_unspendable_key.end());
    for(uint256 &branch_hash : commit_scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());

    CMutableTransaction payback_tx;
    payback_tx.vin = {CTxIn(funds_commit.GetHash(), 0, {}, GetCsvInBlocks(12))};
    payback_tx.vin.front().scriptWitness.stack.emplace_back(64);
    payback_tx.vin.front().scriptWitness.stack.emplace_back(payback_script.begin(), payback_script.end());
    payback_tx.vin.front().scriptWitness.stack.emplace_back(control_block);
    payback_tx.vout = {CTxOut(0, payoff_pubkeyscript)};
    payback_tx.vout.front().nValue = CalculateOutputAmount(funds_commit.vout[0].nValue, *m_mining_fee_rate, payback_tx);

    signature payback_sig = keypair.SignTaprootTx(payback_tx, 0, {funds_commit.vout[0]}, payback_script);
    payback_tx.vin.front().scriptWitness.stack.front() = move(payback_sig);

    mFundsPaybackTx = move(payback_tx);
}

void SwapInscriptionBuilder::MarketSignOrdPayoffTx(const std::string& sk)
{
    CheckContractTerms(MARKET_PAYOFF_TERMS);

    core::ChannelKeys keypair(unhex<seckey>(sk));
    if (keypair.GetLocalPubKey() != *m_swap_script_pk_M) {
        throw ContractError("Swap PubKey does not match the secret");
    }

    CScript transfer_pubkeyscript = CScript() << 1 << *m_swap_script_pk_B;

    CMutableTransaction swap_tx(MakeSwapTx(true));

    CMutableTransaction transfer_tx;

    transfer_tx.vin = {CTxIn(swap_tx.GetHash(), 0)};
    transfer_tx.vin.front().scriptWitness.stack.emplace_back(64);
    transfer_tx.vout = {CTxOut(swap_tx.vout[0].nValue, move(transfer_pubkeyscript))};
    transfer_tx.vout.front().nValue = CalculateOutputAmount(swap_tx.vout[0].nValue, *m_ord_mining_fee_rate, transfer_tx);

    m_ordpayoff_sig = keypair.SignTaprootTx(transfer_tx, 0, {swap_tx.vout[0]}, {});

    transfer_tx.vin.front().scriptWitness.stack.front() = *m_ordpayoff_sig;

    mOrdPayoffTx = move(transfer_tx);
}

void SwapInscriptionBuilder::MarketSignSwap(const std::string& sk)
{
    CheckContractTerms(FUNDS_SWAP_SIG);

    core::ChannelKeys keypair(unhex<seckey>(sk));

    if (keypair.GetLocalPubKey() != m_swap_script_pk_M.value()) {
        throw ContractError("Swap PubKey does not match the secret");
    }

    auto utxo_pubkeyscript = CScript() << 1 << *m_ord_pk;

    CMutableTransaction swap_tx(MakeSwapTx(true));

    m_funds_swap_sig_M = keypair.SignTaprootTx(swap_tx, 1, {CTxOut(*m_ord_amount, utxo_pubkeyscript), GetFundsCommitTx().vout[0]}, MakeFundsSwapScript(*m_swap_script_pk_B, *m_swap_script_pk_M));

    swap_tx.vin[1].scriptWitness.stack[0] = *m_funds_swap_sig_M;

    mSwapTx = move(swap_tx);

    PrecomputedTransactionData txdata;
    txdata.Init(*mSwapTx, {CTxOut(*m_ord_amount, utxo_pubkeyscript), GetFundsCommitTx().vout[0]}, /* force=*/ true);

    const CTxIn& ordTxin = mSwapTx->vin.at(0);
    MutableTransactionSignatureChecker TxOrdChecker(&(*mSwapTx), 0, *m_ord_amount, txdata, MissingDataBehavior::FAIL);
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

string SwapInscriptionBuilder::FundsPayBackRawTransaction()
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

string SwapInscriptionBuilder::Serialize(SwapPhase phase)
{
    CheckContractTerms(phase);

    UniValue contract(UniValue::VOBJ);

    contract.pushKV(name_version, m_protocol_version);
    contract.pushKV(name_ord_price, UniValue(FormatAmount(m_ord_price)));
    contract.pushKV(name_swap_script_pk_M, hex(*m_swap_script_pk_M));
    contract.pushKV(name_ord_mining_fee_rate, FormatAmount(*m_ord_mining_fee_rate));

    if (phase == ORD_SWAP_SIG || phase == MARKET_PAYOFF_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_ord_txid, *m_ord_txid);
        contract.pushKV(name_ord_nout, *m_ord_nout);
        contract.pushKV(name_ord_amount, FormatAmount(*m_ord_amount));
        contract.pushKV(name_ord_pk, hex(*m_ord_pk));
        contract.pushKV(name_swap_script_pk_A, hex(*m_swap_script_pk_A));
    }
    if (phase == ORD_SWAP_SIG || phase == MARKET_PAYOFF_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_ord_swap_sig_A, hex(*m_ord_swap_sig_A));
    }

    if (phase == FUNDS_TERMS || phase == FUNDS_COMMIT_SIG || phase == MARKET_PAYOFF_SIG || phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        contract.pushKV(name_market_fee, UniValue(FormatAmount(*m_market_fee)));
        contract.pushKV(name_mining_fee_rate, UniValue(FormatAmount(*m_mining_fee_rate)));
    }
    if (phase == FUNDS_COMMIT_SIG || phase == MARKET_PAYOFF_SIG || phase == FUNDS_SWAP_SIG || phase == MARKET_SWAP_SIG) {
        UniValue funds(UniValue::VARR);
        for (const auto& utxo: m_funds) {
            UniValue utxo_val(UniValue::VOBJ);

            utxo_val.pushKV(name_funds_txid, utxo.m_txid);
            utxo_val.pushKV(name_funds_nout, utxo.m_nout);
            utxo_val.pushKV(name_funds_amount, FormatAmount(utxo.m_amount));
            utxo_val.pushKV(name_funds_commit_sig, hex(*utxo.m_sig));

            funds.push_back(move(utxo_val));
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
    if (!m_ord_mining_fee_rate) throw ContractTermMissing(std::string(name_ord_mining_fee_rate));
    if (m_ord_price <= 0) throw ContractTermMissing(std::string(name_ord_price));
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
        if (!m_ord_swap_sig_A) throw ContractTermMissing(std::string(name_ord_swap_sig_A));
        if (!m_ord_pk) throw ContractTermMissing(std::string(name_ord_pk));
        if (!m_ord_amount) throw ContractTermMissing(std::string(name_ord_amount));
        if (!m_ord_txid) throw ContractTermMissing(std::string(name_ord_txid));
        if (!m_ord_nout) throw ContractTermMissing(std::string(name_ord_nout));
        if (!m_swap_script_pk_A) throw ContractTermMissing(std::string(name_swap_script_pk_A));
        // no break;
    case ORD_TERMS:
        break;
    case FUNDS_COMMIT_SIG:
        if (m_funds.empty()) throw ContractTermMissing(std::string(name_funds));
        {
            CAmount funds_amount = 0;
            size_t n = 0;
            for (const auto& utxo: m_funds) {
                funds_amount += utxo.m_amount;

                if (!utxo.m_sig) throw ContractTermMissing(name_funds + '[' + std::to_string(n) + "]." +  name_funds_commit_sig);

                ++n;
            }
            if (funds_amount < ParseAmount(GetMinFundingAmount(""))) throw ContractTermMissing("Funds UTXO amount not enough");
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

void SwapInscriptionBuilder::Deserialize(const string &data)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() != val_swap_inscription) {
        throw ContractProtocolError("SwapInscription contract does not match " + root[name_contract_type].getValStr());
    }

    const UniValue& contract = root[name_params];

    if (contract[name_version].getInt<uint32_t>() != m_protocol_version) {
        throw ContractProtocolError("Wrong SwapInscription contract version: " + contract[name_version].getValStr());
    }

    if (m_ord_price) {
        if (m_ord_price != ParseAmount(contract[name_ord_price].getValStr())) throw ContractValueMismatch(std::string(name_ord_price));
    } else
        m_ord_price = ParseAmount(contract[name_ord_price].getValStr());

    {   const auto &val = contract[name_market_fee];
        if (!val.isNull()) {
            if (m_market_fee) {
                if (*m_market_fee != ParseAmount(val.getValStr())) throw ContractValueMismatch(std::string(name_market_fee));
            }
            m_market_fee = ParseAmount(val.getValStr());
        }
    }
    {   const auto& val = contract[name_swap_script_pk_A];
        if (!val.isNull()) {
            if (m_swap_script_pk_A) {
                if (*m_swap_script_pk_A != unhex<xonly_pubkey>(val.get_str())) throw ContractValueMismatch(std::string(name_swap_script_pk_A));
            }
            else m_swap_script_pk_A = unhex<xonly_pubkey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_ord_mining_fee_rate];
        if (!val.isNull()) {
            if (m_ord_mining_fee_rate) {
                if (*m_ord_mining_fee_rate != ParseAmount(val.getValStr())) throw ContractValueMismatch(std::string(name_ord_mining_fee_rate));
            }
            else m_ord_mining_fee_rate = ParseAmount(val.getValStr());
        }
    }
    {   const auto& val = contract[name_swap_script_pk_B];
        if (!val.isNull()) {
            if (m_swap_script_pk_B) {
                if (*m_swap_script_pk_B != unhex<xonly_pubkey>(val.get_str())) throw ContractValueMismatch(std::string(name_swap_script_pk_B));
            }
            else m_swap_script_pk_B = unhex<xonly_pubkey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_swap_script_pk_M];
        if (!val.isNull()) {
            if (m_swap_script_pk_M) {
                if (*m_swap_script_pk_M != unhex<xonly_pubkey>(val.get_str())) throw ContractValueMismatch(std::string(name_swap_script_pk_M));
            }
            else m_swap_script_pk_M = unhex<xonly_pubkey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_ord_txid];
        if (!val.isNull()) {
            if (m_ord_txid) {
                if (*m_ord_txid != val.get_str()) throw ContractValueMismatch(std::string(name_ord_txid));
            }
            else m_ord_txid = val.get_str();
        }
    }
    {   const auto& val = contract[name_ord_nout];
        if (!val.isNull()) {
            if (m_ord_nout) {
                if (*m_ord_nout != val.getInt<uint32_t>()) throw ContractValueMismatch(std::string(name_ord_nout));
            }
            else m_ord_nout = val.getInt<uint32_t>();
        }
    }
    {   const auto& val = contract[name_ord_amount];
        if (!val.isNull()) {
            if (m_ord_amount) {
                if (*m_ord_amount != ParseAmount(val.getValStr())) throw ContractValueMismatch(std::string(name_ord_amount));
            }
            else m_ord_amount = ParseAmount(val.getValStr());
        }
    }
    {   const auto& val = contract[name_ord_pk];
        if (!val.isNull()) {
            if (m_ord_pk) {
                if (*m_ord_pk != unhex<xonly_pubkey>(val.get_str())) throw ContractValueMismatch(std::string(name_ord_txid));
            }
            else m_ord_pk = unhex<xonly_pubkey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_funds_unspendable_key];
        if (!val.isNull()) {
            if (m_funds_unspendable_key_factor) {
                if (*m_funds_unspendable_key_factor != unhex<seckey>(val.get_str())) throw ContractValueMismatch(std::string(name_funds_unspendable_key));
            }
            else m_funds_unspendable_key_factor = unhex<seckey>(val.get_str());
        }
    }
    {   const auto& val = contract[name_funds];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_funds));
            if (!m_funds.empty() && m_funds.size() != val.size()) throw ContractValueMismatch(std::string(name_funds) + " size");

            auto utxo_it = m_funds.begin();
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

                if (i == m_funds.size()) {
                    m_funds.emplace_back(txid_val.get_str(),
                                         nout_val.getInt<uint32_t>(),
                                         ParseAmount(amount_val.getValStr()),
                                         xonly_pubkey(),
                                         unhex<signature>(sig_val.get_str()));
                }
                else {
                    // std::format("{}[{}].{}", name_funds, i, name_funds_txid)
                    if (utxo_it->m_txid != txid_val.get_str())
                        throw ContractValueMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_txid).str());
                    // std::format("{}[{}].{}", name_funds, i, name_funds_nout)
                    if (utxo_it->m_nout != nout_val.getInt<uint32_t>())
                        throw ContractValueMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_nout).str());
                    // std::format("{}[{}].{}", name_funds, i, name_funds_amount)
                    if (utxo_it->m_amount != ParseAmount(amount_val.getValStr()))
                        throw ContractValueMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_amount).str());
                    if (utxo_it->m_sig) {
                        // std::format("{}[{}].{}", name_funds, i, name_funds_commit_sig
                        if (*utxo_it->m_sig != unhex<signature>(sig_val.get_str()))
                            throw ContractValueMismatch((std::ostringstream() << name_funds << '[' << i << "]." << name_funds_commit_sig).str());
                    }
                    else utxo_it->m_sig = unhex<signature>(sig_val.get_str());

                    ++utxo_it;
                }
            }
        }
    }
    {   const auto& val = contract[name_mining_fee_rate];
        if (!val.isNull()) {
            if (m_mining_fee_rate) {
                if (*m_mining_fee_rate != ParseAmount(val.getValStr())) throw ContractError(std::string(name_mining_fee_rate));
            }
            else m_mining_fee_rate = ParseAmount(val.getValStr());
        }
    }
    {   const auto& val = contract[name_ord_swap_sig_A];
        if (!val.isNull()) {
            if (m_ord_swap_sig_A) {
                if (*m_ord_swap_sig_A != unhex<signature>(val.get_str())) throw ContractError(std::string(name_ord_swap_sig_A));
            }
            else m_ord_swap_sig_A = unhex<signature>(val.get_str());
        }
    }
    {   const auto& val = contract[name_funds_swap_sig_B];
        if (!val.isNull()) {
            if (m_funds_swap_sig_B) {
                if (*m_funds_swap_sig_B != unhex<signature>(val.get_str())) throw ContractError(std::string(name_funds_swap_sig_B));
            }
            else m_funds_swap_sig_B = unhex<signature>(val.get_str());
        }
    }
    {   const auto& val = contract[name_funds_swap_sig_M];
        if (!val.isNull()) {
            if (m_funds_swap_sig_M) {
                if (*m_funds_swap_sig_M != unhex<signature>(val.get_str())) throw ContractError(std::string(name_funds_swap_sig_M));
            }
            else m_funds_swap_sig_M = unhex<signature>(val.get_str());
        }
    }
    {   const auto& val = contract[name_ordpayoff_sig];
        if (!val.isNull()) {
            if (m_ordpayoff_sig) {
                if (*m_ordpayoff_sig != unhex<signature>(val.get_str())) throw ContractError(std::string(name_ordpayoff_sig));
            }
            else m_ordpayoff_sig = unhex<signature>(val.get_str());
        }
    }
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

SwapInscriptionBuilder &SwapInscriptionBuilder::OrdUTXO(const string &txid, uint32_t nout, const string &amount)
{
    m_ord_txid = txid;
    m_ord_nout = nout;
    m_ord_amount = ParseAmount(amount);
    return *this;
}

SwapInscriptionBuilder &SwapInscriptionBuilder::AddFundsUTXO(const string &txid, uint32_t nout, const string &amount, const std::string& pk)
{
    m_funds.emplace_back(txid, nout, ParseAmount(amount), unhex<xonly_pubkey>(pk));
    return *this;
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

    auto ord_scriptpubkey = CScript() << 1 << *m_ord_pk;

    std::vector<CTxOut> spent_outs = {CTxOut(*m_ord_amount, ord_scriptpubkey)};
    if (has_funds_sig) {
        spent_outs.emplace_back(GetFundsCommitTx().vout.front());
    }

    if (mSwapTx) {
        VerifyTxSignature(*m_ord_pk, *m_ord_swap_sig_A, *mSwapTx, 0, move(spent_outs), {});
    }
    else {
        CMutableTransaction swap_tx(MakeSwapTx(has_funds_sig));
        VerifyTxSignature(*m_ord_pk, *m_ord_swap_sig_A, swap_tx, 0, move(spent_outs), {});
    }
}

std::string SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const
{
    return FormatAmount(m_ord_price + *m_market_fee + CalculateWholeFee(params));
}

void SwapInscriptionBuilder::CheckFundsSwapSig() const
{
    auto ord_scriptpubkey = CScript() << 1 << *m_ord_pk;
    std::vector<CTxOut> spent_outs = {CTxOut(*m_ord_amount, ord_scriptpubkey), GetFundsCommitTx().vout.front()};

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
