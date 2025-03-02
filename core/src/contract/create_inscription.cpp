
#include "nlohmann/json.hpp"
#include "wrapstream.hpp"
#include "base64.hpp"

#include "univalue.h"
#include "interpreter.h"
#include "feerate.h"
#include "policy.h"

#include "transaction.hpp"
#include "psbt.hpp"

#include "create_inscription.hpp"

#include <exception>
#include <ranges>
#include <deque>

#include "contract_builder_factory.hpp"
#include "runes.hpp"

#include "inscription_common.hpp"

namespace utxord {

namespace {

const std::string val_create_inscription("CreateInscription");

const char* MARKET_TERMS_STR = "MARKET_TERMS";
const char* LAZY_INSCRIPTION_MARKET_TERMS_STR = "LAZY_INSCRIPTION_MARKET_TERMS";
const char* LAZY_INSCRIPTION_SIGNATURE_STR = "LAZY_INSCRIPTION_SIGNATURE";
const char* INSCRIPTION_SIGNATURE_STR = "INSCRIPTION_SIGNATURE";

}

const uint32_t CreateInscriptionBuilder::s_protocol_version = 12;
const uint32_t CreateInscriptionBuilder::s_protocol_version_no_p2address = 11;
const uint32_t CreateInscriptionBuilder::s_protocol_version_no_custom_fee = 10;
const uint32_t CreateInscriptionBuilder::s_protocol_version_no_runes = 9;
const uint32_t CreateInscriptionBuilder::s_protocol_version_no_fixed_change = 8;
const char* CreateInscriptionBuilder::s_versions = "[8,9,10,11,12]";

const std::string CreateInscriptionBuilder::name_ord = "ord";
const std::string CreateInscriptionBuilder::name_ord_amount = "ord_amount";
const std::string CreateInscriptionBuilder::name_utxo = "utxo";
const std::string CreateInscriptionBuilder::name_collection = "collection";
const std::string CreateInscriptionBuilder::name_collection_id = "collection_id";
const std::string CreateInscriptionBuilder::name_collection_destination = "collection_destination";
const std::string CreateInscriptionBuilder::name_metadata = "metadata";
const std::string CreateInscriptionBuilder::name_rune_stone = "rune_stone";
const std::string CreateInscriptionBuilder::name_fund_mining_fee_int_pk = "fund_mining_fee_int_pk";
const std::string CreateInscriptionBuilder::name_fund_mining_fee_sig = "fund_mining_fee_sig";
const std::string CreateInscriptionBuilder::name_content_type = "content_type";
const std::string CreateInscriptionBuilder::name_content = "content";
const std::string CreateInscriptionBuilder::name_delegate = "delegate";
const std::string CreateInscriptionBuilder::name_inscribe_script_pk = "inscribe_script_pk";
const std::string CreateInscriptionBuilder::name_inscribe_int_pk = "inscribe_int_pk";
const std::string CreateInscriptionBuilder::name_inscribe_sig = "inscribe_sig";
const std::string CreateInscriptionBuilder::name_inscribe_script_market_pk = "inscribe_script_market_pk";
const std::string CreateInscriptionBuilder::name_destination_addr = "destination_addr";
const std::string CreateInscriptionBuilder::name_author_fee = "author_fee";
const std::string CreateInscriptionBuilder::name_fixed_change = "fixed_change";

const char * CreateInscriptionBuilder::PhaseString(InscribePhase phase)
{
    switch (phase) {
    case MARKET_TERMS:
        return MARKET_TERMS_STR;
    case LAZY_INSCRIPTION_MARKET_TERMS:
        return LAZY_INSCRIPTION_MARKET_TERMS_STR;
    case LAZY_INSCRIPTION_SIGNATURE:
        return LAZY_INSCRIPTION_SIGNATURE_STR;
    case INSCRIPTION_SIGNATURE:
        return INSCRIPTION_SIGNATURE_STR;
    }
    throw ContractTermWrongValue("InscribePhase: " + std::to_string(phase));
}

InscribePhase CreateInscriptionBuilder::ParsePhase(const std::string& str)
{
    if (str == MARKET_TERMS_STR) return MARKET_TERMS;
    if (str == LAZY_INSCRIPTION_MARKET_TERMS_STR) return LAZY_INSCRIPTION_MARKET_TERMS;
    if (str == LAZY_INSCRIPTION_SIGNATURE_STR) return LAZY_INSCRIPTION_SIGNATURE;
    if (str == INSCRIPTION_SIGNATURE_STR) return INSCRIPTION_SIGNATURE;
    throw ContractTermWrongValue(std::string(str));
}

const std::string& CreateInscriptionBuilder::GetContractName() const
{ return val_create_inscription; }

CScript CreateInscriptionBuilder::MakeInscriptionScript() const
{
    CScript script;
    script << m_inscribe_script_pk.value_or(xonly_pubkey());
    script << OP_CHECKSIG;
    if (m_type == LAZY_INSCRIPTION) {
        script << m_inscribe_script_market_pk.value_or(xonly_pubkey()) << OP_CHECKSIGADD;
        script << 2 << OP_NUMEQUAL;
    }
    script << OP_0;
    script << OP_IF;
    script << ORD_TAG;

    if (m_parent_collection_id) {
        script << COLLECTION_ID_TAG;
        script << SerializeInscriptionId(*m_parent_collection_id);
    }

    if (m_metadata) {
        for (auto pos = m_metadata->begin(); pos < m_metadata->end(); pos += MAX_PUSH) {
            script << METADATA_TAG << bytevector(pos, ((pos + MAX_PUSH) < m_metadata->end()) ? (pos + MAX_PUSH) : m_metadata->end());
        }
    }

    if (m_rune_stone)
        script << RUNE_TAG << m_rune_stone->Commit();

    if (m_delegate)
        script << DELEGATE_ID_TAG << SerializeInscriptionId(*m_delegate);

    if (m_content) {
        script << CONTENT_TYPE_TAG << bytevector(m_content_type->begin(), m_content_type->end());

        script << CONTENT_OP_TAG;

        for (auto pos = m_content->begin(); pos < m_content->end(); pos += MAX_PUSH) {
            script << bytevector(pos, ((pos + MAX_PUSH) < m_content->end()) ? (pos + MAX_PUSH) : m_content->end());
        }
    }

    script << OP_ENDIF;

    return script;
}


std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> CreateInscriptionBuilder::GenesisTapRoot() const
{
    if (!m_inscribe_int_pk) throw ContractStateError(name_inscribe_int_pk + " not defined");
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " not defined");
    if (m_type == LAZY_INSCRIPTION) {
        if (!m_parent_collection_id) throw ContractStateError(name_collection_id + " not defined");
        if (!m_inscribe_script_market_pk) throw ContractStateError(name_inscribe_script_pk + " not defined");
    }

    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED, { MakeInscriptionScript() });

    return std::tuple_cat(core::SchnorrKeyPair::AddTapTweak(KeyPair::GetStaticSecp256k1Context(), *m_inscribe_int_pk, tap_tree.CalculateRoot()), std::make_tuple(tap_tree));
}

std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> CreateInscriptionBuilder::FundMiningFeeTapRoot() const
{
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED,
                              { MakeMultiSigScript(m_inscribe_script_pk.value_or(xonly_pubkey()),
                                                  m_inscribe_script_market_pk.value_or(xonly_pubkey())) });

    return std::tuple_cat(core::SchnorrKeyPair::AddTapTweak(KeyPair::GetStaticSecp256k1Context(), m_fund_mining_fee_int_pk.value_or(xonly_pubkey()),
                                                            tap_tree.CalculateRoot()), std::make_tuple(tap_tree));
}


void CreateInscriptionBuilder::Collection(std::string collection_id, CAmount amount, std::string collection_addr)
{
    if (m_parent_collection_id) {
        if (m_parent_collection_id != collection_id) throw ContractTermMismatch(name_collection_id + ": " + *m_parent_collection_id);
    }
    else {
        CheckInscriptionId(collection_id);
        m_parent_collection_id = move(collection_id);
    }

    if (m_collection_destination) {
        if (m_collection_destination->Amount() != amount ||
            m_collection_destination->Address() != collection_addr)
            throw ContractTermMismatch(name_collection_destination.c_str());
    }
    else
        m_collection_destination = P2Address::Construct(chain(), amount, move(collection_addr));
}

void CreateInscriptionBuilder::OverrideCollectionAddress(std::string addr)
{
    if (!m_collection_destination) throw ContractStateError(name_collection_destination + " is needed to override collection address");
    m_collection_destination = P2Address::Construct(chain(), m_collection_destination->Amount(), move(addr));
}

void CreateInscriptionBuilder::MetaData(bytevector cbor)
{
    auto check_metadata = nlohmann::json::from_cbor(cbor);
    if (check_metadata.is_discarded())
        throw ContractTermWrongFormat(std::string(name_metadata));

    m_metadata = move(cbor);
}

void CreateInscriptionBuilder::Delegate(std::string inscription_id)
{
    CheckInscriptionId(inscription_id);
    m_delegate = move(inscription_id);
}

std::string CreateInscriptionBuilder::GetInscribeInternalPubKey() const
{
    if (m_inscribe_int_pk) {
        return hex(*m_inscribe_int_pk);
    }
    else
        throw ContractStateError(std::string(name_inscribe_int_pk) + " undefined");
}

void CreateInscriptionBuilder::SignCommit(const KeyRegistry& master_key, const std::string& key_filter)
{
    if (!m_inscribe_int_pk) throw ContractStateError(name_inscribe_int_pk + " not defined");
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " not defined");
    if (m_type == LAZY_INSCRIPTION) {
        if (!m_inscribe_script_market_pk) throw ContractStateError(name_inscribe_script_market_pk + " not defined");
        if (!m_fund_mining_fee_int_pk) throw ContractStateError(name_fund_mining_fee_int_pk + " not defined");
    }

    std::vector<CTxOut> spent_outs;
    spent_outs.reserve(m_inputs.size());
    for (const auto& input: m_inputs) {
        const auto& dest = input.output->Destination();
        spent_outs.emplace_back(dest->TxOutput());
    }

    CMutableTransaction tx = MakeCommitTx();

    for (auto& utxo: m_inputs) {
        auto signer = utxo.output->Destination()->LookupKey(master_key, key_filter);
        signer->SignInput(utxo, tx, spent_outs, SIGHASH_ALL);
    }
}

const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>& CreateInscriptionBuilder::GetInscriptionTapRoot() const
{
    if (!mInscriptionTaproot) {
        mInscriptionTaproot.emplace(GenesisTapRoot());
    }
    return *mInscriptionTaproot;
}

void CreateInscriptionBuilder::SignCollection(const KeyRegistry &master_key, const std::string& key_filter)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " undefined");
    //if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));
    if (!m_parent_collection_id) throw ContractStateError(name_collection_id + " undefined");
    if (!m_collection_input) throw ContractStateError(name_collection + " undefined");
    if (!m_collection_input->output) throw ContractStateError(name_collection + '.' + name_pk + " undefined");

    CMutableTransaction genesis_tx = MakeGenesisTx(MakeCommitTx());

    auto script_signer = m_collection_input->output->Destination()->LookupKey(master_key, key_filter);
    script_signer->SignInput(*m_collection_input, genesis_tx, GetGenesisTxSpends(), SIGHASH_ALL);
}

void CreateInscriptionBuilder::SignInscription(const KeyRegistry &master_key, const std::string& key_filter)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " not defined");
    if (m_type == LAZY_INSCRIPTION && !m_inscribe_script_market_pk) throw ContractStateError(name_inscribe_script_market_pk + " not defined");
    if (!m_inscribe_int_pk) throw ContractStateError(name_inscribe_int_pk + " not defined");

    auto inscribe_script_keypair = master_key.Lookup(*m_inscribe_script_pk, key_filter);
    core::SchnorrKeyPair script_keypair(inscribe_script_keypair.PrivKey());
    if (*m_inscribe_script_pk != script_keypair.GetPubKey()) throw ContractTermMismatch(std::string(name_inscribe_script_pk));

    CMutableTransaction genesis_tx = MakeGenesisTx(MakeCommitTx());

    m_inscribe_sig = script_keypair.SignTaprootTx(genesis_tx, 0, GetGenesisTxSpends(),
                                                  get<2>(GetInscriptionTapRoot()).GetScripts().front(),
                                                  m_type == LAZY_INSCRIPTION ? (SIGHASH_ANYONECANPAY | SIGHASH_SINGLE) : SIGHASH_DEFAULT);

    if (m_parent_collection_id) {
        if (m_type == LAZY_INSCRIPTION)
            m_fund_mining_fee_sig = script_keypair.SignTaprootTx(genesis_tx, 2, GetGenesisTxSpends(),
                MakeMultiSigScript(*m_inscribe_script_pk, *m_inscribe_script_market_pk), SIGHASH_ANYONECANPAY | SIGHASH_NONE);
        else
            m_fund_mining_fee_sig = script_keypair.SignTaprootTx(genesis_tx, 2, GetGenesisTxSpends(), {});
    }
}

void CreateInscriptionBuilder::MarketSignInscription(const KeyRegistry &master_key, const std::string& key_filter)
{
    if (m_type != LAZY_INSCRIPTION) throw ContractTermWrongValue(name_contract_type.c_str());
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " not defined");
    if (!m_inscribe_script_market_pk) throw ContractStateError(name_inscribe_script_market_pk + " not defined");
    if (!m_inscribe_int_pk) throw ContractStateError(name_inscribe_int_pk + " not defined");

    auto inscribe_script_keypair = master_key.Lookup(*m_inscribe_script_market_pk, key_filter);
    core::SchnorrKeyPair script_keypair(inscribe_script_keypair.PrivKey());
    if (*m_inscribe_script_market_pk != script_keypair.GetPubKey()) throw ContractTermMismatch(std::string(name_inscribe_script_market_pk));

    CMutableTransaction genesis_tx = MakeGenesisTx(MakeCommitTx());

    m_inscribe_market_sig = script_keypair.SignTaprootTx(genesis_tx, 0, GetGenesisTxSpends(), get<2>(GetInscriptionTapRoot()).GetScripts().front());
    if (m_parent_collection_id) {
        m_fund_mining_fee_market_sig = script_keypair.SignTaprootTx(genesis_tx, 2, GetGenesisTxSpends(),
                MakeMultiSigScript(*m_inscribe_script_pk, *m_inscribe_script_market_pk));
    }
}

l15::stringvector CreateInscriptionBuilder::RawTransactions() const
{
    if (!mCommitTx || !mGenesisTx) {
        RestoreTransactions();
    }

    std::string funding_tx_hex = EncodeHexTx(*mCommitTx);
    std::string genesis_tx_hex = EncodeHexTx(*mGenesisTx);

    return {move(funding_tx_hex), move(genesis_tx_hex)};
}

l15::stringvector CreateInscriptionBuilder::TransactionsPSBT() const
{
    if (!mCommitTx || !mGenesisTx) {
        RestoreTransactions();
    }

    l15::core::PSBT commitPsbt(*mCommitTx);
    std::ranges::transform(m_inputs, commitPsbt.inputs.begin(), [](const auto& in) {
        l15::core::PSBTInput res;
        res.witness_utxo = CTxOut(in.output->Amount(), in.output->Destination()->PubKeyScript());
        CScript scriptSig = in.output->Destination()->ScriptSig();
        if (!scriptSig.empty()) {
            res.redeem_script = move(scriptSig);
        }
        return res;
    });

    l15::core::PSBT genesisPsbt(*mGenesisTx);

    auto genSpends = GetGenesisTxSpends();
    std::ranges::transform(genSpends, genesisPsbt.inputs.begin(), [](const auto& in) {
        l15::core::PSBTInput res;
        res.witness_utxo = in;
        return res;
    });
    auto genTapRoot = GetInscriptionTapRoot();
    genesisPsbt.inputs.front().m_tap_internal_key = *m_inscribe_int_pk;
    genesisPsbt.inputs.front().m_tap_merkle_root = get<2>(genTapRoot).CalculateRoot();
    genesisPsbt.inputs.front().m_tap_scripts.emplace(get<2>(genTapRoot).GetScripts().front(), InscribeScriptControlBlock(genTapRoot));
    if (m_parent_collection_id) {
        if (m_type == LAZY_INSCRIPTION) {
            genesisPsbt.inputs.front().sighash_type = SIGHASH_ANYONECANPAY | SIGHASH_SINGLE;
            auto fundFeeTapRoot = FundMiningFeeTapRoot();
            genesisPsbt.inputs.back().m_tap_internal_key = *m_fund_mining_fee_int_pk;
            genesisPsbt.inputs.back().m_tap_merkle_root = get<2>(fundFeeTapRoot).CalculateRoot();
            genesisPsbt.inputs.back().m_tap_scripts.emplace(get<2>(fundFeeTapRoot).GetScripts().front(), FundMiningFeeControlBlock(fundFeeTapRoot));
            genesisPsbt.inputs.back().sighash_type = SIGHASH_ANYONECANPAY | SIGHASH_NONE;
        }
    }

    return {
        base64::encode(commitPsbt.Serialize<bytevector>()),
        base64::encode(genesisPsbt.Serialize<bytevector>()),
    };
}

void CreateInscriptionBuilder::ApplyPSBTSignature(const l15::stringvector& psbts)
{
    if (psbts.size() != 2) throw ContractTermWrongValue("psbt count: " + std::to_string(psbts.size()));

    core::PSBT commitPsbt(base64::decode<bytevector>(psbts.front()));

    if (commitPsbt.inputs.size() != m_inputs.size()) throw ContractTermMismatch(name_utxo + " count mismatch: " + std::to_string(commitPsbt.inputs.size()));
    for (auto [psbtInput, input, i]: std::ranges::zip_view(commitPsbt.inputs, m_inputs, std::ranges::iota_view(0))) {
        if (psbtInput.witness_utxo->nValue != input.output->Amount()) throw ContractTermMismatch(name_utxo + '[' + std::to_string(i) + "]." + IContractDestination::name_amount);
        if (psbtInput.witness_utxo->scriptPubKey != input.output->Destination()->PubKeyScript()) throw ContractTermMismatch(name_utxo + '[' + std::to_string(i) + "]." + IContractDestination::name_addr);

        if (psbtInput.final_script_witness) {
            for (size_t j = 0; j < psbtInput.final_script_witness->stack.size(); ++j) {
                input.witness.Set(j, move(psbtInput.final_script_witness->stack[j]));
            }
        }
        else if (psbtInput.m_tap_key_sig) {
            input.witness.Set(0, move(*psbtInput.m_tap_key_sig));
        }
        else if (!psbtInput.partial_sigs.empty()) {
            if (psbtInput.partial_sigs.size() > 1) throw ContractTermWrongValue("More than 1 signature");
            auto& [pk, sig] = psbtInput.partial_sigs.begin()->second;
            input.output->Destination()->SetSignature(input, bytevector(move(pk.as_vector())), move(sig));
        }
        else throw ContractTermMissing((std::ostringstream() << name_utxo << '['<< input.nin << "]." << name_sig).str());
    }

    core::PSBT genesisPsbt(base64::decode<bytevector>(psbts.back()));

    auto ordTR = GetInscriptionTapRoot();

    if (!genesisPsbt.inputs.empty()) {
        const auto& psbtOrdInput = genesisPsbt.inputs.front();

        if (psbtOrdInput.m_tap_internal_key != *m_inscribe_int_pk) throw ContractTermMismatch(std::string(name_inscribe_int_pk));
        if (psbtOrdInput.m_tap_merkle_root != get<2>(ordTR).CalculateRoot()) throw ContractTermMismatch(std::string(name_inscribe_int_pk));
        if (psbtOrdInput.m_tap_script_sigs.empty()) throw ContractTermMissing(std::string(name_inscribe_sig));
        if (psbtOrdInput.m_tap_script_sigs.size() != 1) throw ContractTermWrongValue(name_inscribe_sig + " has more than 1 value");

        if (psbtOrdInput.m_tap_scripts.size() != 1 ||
            psbtOrdInput.m_tap_scripts.begin()->first != get<2>(ordTR).GetScripts().front() ||
            psbtOrdInput.m_tap_scripts.begin()->second != InscribeScriptControlBlock(ordTR)) {
            throw ContractTermMismatch("Inscription script");
        }

        uint256 tap_leaf = l15::TapLeafHash(get<2>(ordTR).GetScripts().front());

        auto& [key_n_leaf, sig] = *(psbtOrdInput.m_tap_script_sigs.begin());
        if (get<0>(key_n_leaf) != *m_inscribe_script_pk) throw ContractTermMismatch(std::string(name_inscribe_script_pk));
        if (get<1>(key_n_leaf) != tap_leaf) throw ContractTermMismatch(std::string("inscribe script hash"));

        if (m_type == LAZY_INSCRIPTION) {
            if (sig.size() != 65) throw ContractTermWrongValue(name_inscribe_sig + ": no sighash flags");
            if (sig.back() != (SIGHASH_ANYONECANPAY | SIGHASH_SINGLE)) throw ContractTermWrongValue(name_inscribe_sig + ": wrong sighash flags: " + std::to_string(sig.back()));
        }

        m_inscribe_sig = move(sig);
    }

    if (m_parent_collection_id) {
        if (!m_collection_input) throw ContractStateError(name_collection + " not defined");

        if (genesisPsbt.inputs.size() != 3) throw ContractTermWrongValue("genesis tx inputs: " + std::to_string(genesisPsbt.inputs.size()));

        const auto& psbtFeeInput = genesisPsbt.inputs.back();
        auto fundTR = FundMiningFeeTapRoot();

        if (m_type == INSCRIPTION) {
            throw std::runtime_error("not implemented");
        }
        else if (m_type == LAZY_INSCRIPTION) {
            if (psbtFeeInput.m_tap_internal_key != *m_fund_mining_fee_int_pk) throw ContractTermMismatch(std::string(name_fund_mining_fee_int_pk));
            if (psbtFeeInput.m_tap_merkle_root != get<2>(fundTR).CalculateRoot()) throw ContractTermMismatch(std::string(name_fund_mining_fee_int_pk));
            if (psbtFeeInput.m_tap_script_sigs.empty()) throw ContractTermMissing(std::string(name_fund_mining_fee_sig));
            if (psbtFeeInput.m_tap_script_sigs.size() != 1) throw ContractTermWrongValue(name_fund_mining_fee_sig + " has more than 1 value");

            if (psbtFeeInput.m_tap_scripts.size() != 1 ||
                psbtFeeInput.m_tap_scripts.begin()->first != get<2>(fundTR).GetScripts().front() ||
                psbtFeeInput.m_tap_scripts.begin()->second != FundMiningFeeControlBlock(fundTR))
                throw ContractTermMismatch("Fund mining fee script");

            uint256 tap_leaf = l15::TapLeafHash(get<2>(fundTR).GetScripts().front());

            auto& [key_n_leaf, sig] = *(psbtFeeInput.m_tap_script_sigs.begin());
            if (get<0>(key_n_leaf) != *m_inscribe_script_pk) throw ContractTermMismatch(std::string(name_inscribe_script_pk));
            if (get<1>(key_n_leaf) != tap_leaf) throw ContractTermMismatch(std::string("fund mining fee script hash"));
            if (sig.size() != 65) throw ContractTermWrongValue(name_fund_mining_fee_sig + ": no sighash flags");
            if (sig.back() != (SIGHASH_ANYONECANPAY | SIGHASH_NONE)) throw ContractTermWrongValue(name_fund_mining_fee_sig + ": wrong sighash flags: " + std::to_string(sig.back()));

            m_fund_mining_fee_sig = move(sig);
        }
    }
    else {
        if (genesisPsbt.inputs.size() != 1) throw ContractTermWrongValue("genesis tx inputs: " + std::to_string(genesisPsbt.inputs.size()));
    }
}

void CreateInscriptionBuilder::CheckContractTerms(uint32_t version, InscribePhase phase) const
{
    switch (phase) {
    case INSCRIPTION_SIGNATURE:
        if (m_parent_collection_id) {
            if (!m_collection_input) throw ContractTermMissing(name_collection.c_str());
        }
        if (m_collection_input) {
            if (!m_collection_input->witness) throw ContractTermMissing(name_collection + '.' + TxInput::name_witness);
            if (!m_parent_collection_id) throw ContractTermMissing(name_collection_id.c_str());
            if (!m_collection_destination) throw ContractTermMissing(name_collection_destination.c_str());
            if (m_collection_destination->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError((std::ostringstream()
                    << name_collection_destination << '.' << IContractDestination::name_addr << ": "
                    << m_collection_destination->Address() << " is not supported with v. " << version).str());
        }
        //no break
    case LAZY_INSCRIPTION_SIGNATURE:
        if (m_content && !m_content_type) throw ContractTermMissing(name_content_type.c_str());
        if (m_content && m_delegate) throw ContractTermMismatch(name_content + " conflicts " + name_delegate);
        if (m_ord_destination) {
            if (m_ord_destination->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError((std::ostringstream()
                    << name_ord << '.' << IContractDestination::name_addr << ": "
                    << m_ord_destination->Address() << " is not supported with v. " << version).str());
        }
        else throw ContractTermMissing(std::string(name_ord));
        if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        if (m_inputs.empty()) throw ContractTermMissing(std::string(name_utxo));
        {
            for (const auto &input: m_inputs) {
                if (input.witness.size() == 0) throw ContractTermMissing((std::ostringstream() << name_utxo << '['<< input.nin << "]." << name_sig).str());
            }
        }
        if (!m_inscribe_script_pk) throw ContractTermMissing(name_author_fee.c_str());
        if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));
        if (m_type == LAZY_INSCRIPTION && !m_fund_mining_fee_int_pk) throw ContractTermMissing(name_fund_mining_fee_int_pk.c_str());

        if (m_change_addr) try {
            auto fakeChange = P2Address::Construct(chain(), 546, *m_change_addr);
            if (fakeChange->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_change_addr + ": " + *m_change_addr + " is not supported with v. " + std::to_string(version));
        } catch(const ContractProtocolError&) {
            std::rethrow_exception(std::current_exception());
        } catch(...) {
            std::throw_with_nested(ContractTermWrongValue(name_change_addr.c_str()));
        }
        if (m_fixed_change) {
            if (version <= s_protocol_version_no_fixed_change)
                throw ContractProtocolError(name_fixed_change + " is not supported with v. " + std::to_string(version));
            if (m_fixed_change->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_fixed_change + '.' + IContractDestination::name_addr + ": " + m_fixed_change->Address() + " is not supported with v. " + std::to_string(version));
        }

        if (!m_inscribe_sig) throw ContractTermMissing(name_inscribe_sig.c_str());
        if (m_parent_collection_id && !m_fund_mining_fee_sig)
            throw ContractTermMissing(name_fund_mining_fee_sig.c_str());
        //no break
    case LAZY_INSCRIPTION_MARKET_TERMS:
        if (m_type == LAZY_INSCRIPTION) {
            if (!m_author_fee) throw ContractTermMissing(name_author_fee.c_str());
            if (m_author_fee->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_author_fee + '.' + IContractDestination::name_addr + ": " + m_author_fee->Address() + " is not supported with v. " + std::to_string(version));
            if (!m_inscribe_script_market_pk) throw ContractTermMissing(name_author_fee.c_str());
            if (!m_parent_collection_id) throw ContractTermMissing(name_collection_id.c_str());
            if (!m_collection_destination) throw ContractTermMissing(name_collection.c_str());
            if (m_collection_destination->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError(name_collection_destination + '.' + IContractDestination::name_addr + ": " + m_collection_destination->Address() + " is not supported with v. " + std::to_string(version));
            if (m_collection_input) {
                if (m_collection_input->output->Destination()->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                    throw ContractProtocolError(name_collection + '.' + IContractDestination::name_addr + ": " + m_collection_input->output->Destination()->Address() + " is not supported with v. " + std::to_string(version));
            }
        }
        //no break
    case MARKET_TERMS:
        if (m_market_fee) {
            if (m_market_fee->Type() == P2Address::type && version <= s_protocol_version_no_p2address)
                throw ContractProtocolError((std::ostringstream()
                    << name_market_fee << '.' << IContractDestination::name_addr << ": "
                    << m_market_fee->Address() << " is not supported with v. " << version).str());
        }
        else throw ContractTermMissing(std::string(name_market_fee));
        if (!m_custom_fees.empty()) {
            if (version <= s_protocol_version_no_custom_fee)
                throw ContractProtocolError(name_custom_fee + " is not supported with v. " + std::to_string(version));
            if (version <= s_protocol_version_no_p2address) {
                for (uint32_t i = 0; const auto& fee: m_custom_fees) {
                    if (fee->Type() == P2Address::type)
                        throw ContractProtocolError((std::ostringstream()
                            << name_custom_fee << '[' << i << "]." << IContractDestination::name_addr << ": "
                            << fee->Address() << " is not supported with v. " << version).str());
                    ++i;
                }
            }
        }
    }
}

UniValue CreateInscriptionBuilder::MakeJson(uint32_t version, InscribePhase phase) const
{
    if (version != s_protocol_version &&
        version != s_protocol_version_no_p2address &&
        version != s_protocol_version_no_custom_fee &&
        version != s_protocol_version_no_runes &&
        version != s_protocol_version_no_fixed_change)
        throw ContractProtocolError("Wrong serialize version: " + std::to_string(version) + ". Allowed are " + s_versions);

    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, version);
    contract.pushKV(name_contract_phase, PhaseString(phase));

    // switch (phase) {
    // case INSCRIPTION_SIGNATURE:
    // case LAZY_INSCRIPTION_SIGNATURE:
    if (m_mining_fee_rate)
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);
        if (m_content_type)
            contract.pushKV(name_content_type, *m_content_type);
        if (m_content_type)
            contract.pushKV(name_content, hex(*m_content));
        if (m_metadata)
            contract.pushKV(name_metadata, hex(*m_metadata));
        {   UniValue utxo_arr(UniValue::VARR);
            for (const auto &input: m_inputs) {
                UniValue utxo_val = input.MakeJson();
                utxo_arr.push_back(move(utxo_val));
            }
            contract.pushKV(name_utxo, utxo_arr);
        }
        if (m_delegate)
            contract.pushKV(name_delegate, *m_delegate);
    if (m_inscribe_script_pk)
        contract.pushKV(name_inscribe_script_pk, hex(*m_inscribe_script_pk));
    if (m_inscribe_int_pk)
        contract.pushKV(name_inscribe_int_pk, hex(*m_inscribe_int_pk));
    if (m_inscribe_sig)
        contract.pushKV(name_inscribe_sig, hex(*m_inscribe_sig));
        if (m_fund_mining_fee_int_pk)
            contract.pushKV(name_fund_mining_fee_int_pk, hex(*m_fund_mining_fee_int_pk));
        if (m_fund_mining_fee_sig)
            contract.pushKV(name_fund_mining_fee_sig, hex(*m_fund_mining_fee_sig));
        if (m_change_addr)
            contract.pushKV(name_change_addr, *m_change_addr);

    if (version > s_protocol_version_no_runes) {
        if (m_ord_destination)
            contract.pushKV(name_ord, m_ord_destination->MakeJson());
            //if (m_collection_destination) contract.pushKV(name_collection_destination, m_collection_destination->MakeJson());
            if (m_rune_stone) contract.pushKV(name_rune_stone, m_rune_stone->MakeJson());
    }
    else {
        if (m_rune_stone) throw ContractProtocolError(name_rune_stone + " is not supported with v. " + std::to_string(version));
        if (m_ord_destination) {
            contract.pushKV(name_ord_amount, m_ord_destination->Amount());
            contract.pushKV(name_destination_addr, m_ord_destination->Address());
        }
    }
        if (version > s_protocol_version_no_fixed_change) {
            if (m_fixed_change) contract.pushKV(name_fixed_change, m_fixed_change->MakeJson());
        } else
            if (m_fixed_change) throw ContractProtocolError(name_fixed_change + " is not supported with v. " + std::to_string(version));

        //no break
    //case LAZY_INSCRIPTION_MARKET_TERMS:
        if (m_author_fee)
            contract.pushKV(name_author_fee, m_author_fee->MakeJson());
        if (m_collection_destination) {
            if (version > s_protocol_version_no_runes) {
                contract.pushKV(name_collection_destination, m_collection_destination->MakeJson());
            }
            if (m_collection_input) {
                UniValue collectionVal = m_collection_input->MakeJson();
                collectionVal.pushKV(name_collection_id, *m_parent_collection_id);
                contract.pushKV(name_collection, move(collectionVal));
            }
            else {
                TxInput fake_collection_input{chain(), 1, std::make_shared<UTXO>(chain(), uint256(0).GetHex(), 0, m_collection_destination)};
                UniValue collectionVal = fake_collection_input.MakeJson();
                collectionVal.pushKV(name_collection_id, *m_parent_collection_id);
                contract.pushKV(name_collection, move(collectionVal));
            }
        }
        if (m_type == LAZY_INSCRIPTION) {
            if (m_inscribe_script_market_pk)
                contract.pushKV(name_inscribe_script_market_pk, hex(*m_inscribe_script_market_pk));
        }

    //case MARKET_TERMS:
    if (m_market_fee)
        contract.pushKV(name_market_fee, m_market_fee->MakeJson());
    if (!m_custom_fees.empty()) {
            if (version > s_protocol_version_no_custom_fee) {
                UniValue customFeesVal(UniValue::VARR);
                for (const auto& fee: m_custom_fees) {
                    customFeesVal.push_back(fee->MakeJson());
                }
                contract.pushKV(name_custom_fee, move(customFeesVal));
            }
        }
    //}

    return contract;
}

void CreateInscriptionBuilder::ReadJson(const UniValue &contract, InscribePhase phase)
{
    if (m_type != INSCRIPTION && m_type != LAZY_INSCRIPTION) throw ContractTermMismatch (std::string(name_contract_type));

    uint32_t version = contract[name_version].getInt<uint32_t>();
    if (version != s_protocol_version &&
        version != s_protocol_version_no_p2address &&
        version != s_protocol_version_no_custom_fee &&
        version != s_protocol_version_no_runes &&
        version != s_protocol_version_no_fixed_change)
        throw ContractProtocolError("Wrong " + val_create_inscription + " contract version: " + contract[name_version].getValStr());

    std::unique_ptr<IContractDestination> collection_destiation;

    {   const auto& val = contract[name_market_fee];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(std::string(name_market_fee));

            if (m_market_fee)
                m_market_fee->ReadJson(val, [](){ return name_market_fee; });
            else
                m_market_fee = DestinationFactory::ReadJson(chain(), val, [](){ return name_market_fee; });

        }
    }
    {   const auto& val = contract[name_author_fee];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_author_fee.c_str());

            if (m_author_fee)
                m_author_fee->ReadJson(val, [](){return name_author_fee;});
            else
                m_author_fee = DestinationFactory::ReadJson(chain(), val, [](){ return name_author_fee; });
        }
    }
    {   const auto& vals = contract[name_custom_fee];
        if (!vals.isNull()) {
            if (!vals.isArray()) throw ContractTermWrongFormat(name_custom_fee.c_str());
            auto feeIt = m_custom_fees.begin();
            size_t i = 0;
            for (const UniValue &val: vals.getValues()) {
                if (feeIt == m_custom_fees.end()) {
                    m_custom_fees.emplace_back(NoZeroDestinationFactory::ReadJson(chain(), val, [i]{ return name_custom_fee + '[' + std::to_string(i) + ']'; }));
                    feeIt = m_custom_fees.end();
                }
                else {
                    (*feeIt)->ReadJson(val, [i]{ return name_custom_fee + '[' + std::to_string(i) + ']'; });
                    ++feeIt;
                }
                ++i;
            }
        }
    }

    {   const auto &val = contract[name_ord];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_ord.c_str());

            if (m_ord_destination)
                m_ord_destination->ReadJson(val, [](){ return name_ord; });
            else
                m_ord_destination = NoZeroDestinationFactory::ReadJson(chain(), val, [](){ return name_ord; });
        }
        else {
            std::optional<CAmount> amount;
            std::optional<std::string> addr;
            DeserializeContractAmount(contract[name_ord_amount], amount, [&](){ return name_ord_amount; });
            DeserializeContractString(contract[name_destination_addr], addr, [&](){ return name_destination_addr; });

            if (amount || addr) {
                UniValue ordVal(UniValue::VOBJ);
                ordVal.pushKV(IJsonSerializable::name_type, P2Witness::type);
                if (amount) ordVal.pushKV(IContractDestination::name_amount, *amount);
                if (addr) ordVal.pushKV(IContractDestination::name_addr, *addr);
                if (m_ord_destination) {
                    m_ord_destination->ReadJson(ordVal, [](){return name_ord;});
                }
                else {
                    m_ord_destination = NoZeroDestinationFactory::ReadJson(chain(), ordVal, [](){return name_ord;});
                }
            }
        }
    }

    {   const auto &val = contract[name_utxo];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));

            auto inputIt = m_inputs.begin();
            size_t i = 0;
            for (const UniValue &input: val.getValues()) {
                if (inputIt == m_inputs.end()) {
                    m_inputs.emplace_back(chain(), m_inputs.size(), input, [i]{ return name_utxo + '[' + std::to_string(i) + ']'; });
                    inputIt = m_inputs.end();
                }
                else {
                    inputIt->ReadJson(input, [i]{ return name_utxo + '[' + std::to_string(i) + ']'; });
                    ++inputIt;
                }
                i++;
            }
        }
    }
    {   const auto &val = contract[name_collection_destination];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_collection_destination.c_str());
            if (m_collection_destination)
                m_collection_destination->ReadJson(val, [](){ return name_collection_destination; });
            else
                m_collection_destination = NoZeroDestinationFactory::ReadJson(chain(), val, [](){ return name_collection_destination; });
        }
    }
    {   const auto &val = contract[name_collection];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_collection.c_str());
            DeserializeContractString(val[name_collection_id], m_parent_collection_id, [&]() { return move((name_collection + '.') += name_collection_id); });
            if (!m_parent_collection_id) throw ContractTermMissing(move((name_collection + '.') += name_collection_id));
            m_collection_input.emplace(chain(), 1, val, [](){ return name_collection; });

            if (version <= s_protocol_version_no_runes) {
                m_collection_destination = m_collection_input->output->Destination();
            }
        }
    }
    {   const auto &val = contract[name_metadata];
        if (!val.isNull()) {
            bytevector cbor = l15::unhex<bytevector>(val.get_str());
            auto check_metadata = nlohmann::json::from_cbor(cbor);
            if (check_metadata.is_discarded())
                throw ContractTermWrongFormat(std::string(name_metadata));

            m_metadata = move(cbor);;
        }
    }
    {   const auto &val = contract[name_rune_stone];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_rune_stone.c_str());

            if (m_rune_stone)
                m_rune_stone->ReadJson(val, [](){ return name_rune_stone; });
            else
                m_rune_stone = std::make_shared<RuneStoneDestination>(chain(), val, [](){ return name_rune_stone; });
        }
    }

    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [&](){ return name_mining_fee_rate; });
    DeserializeContractString(contract[name_content_type], m_content_type, [&](){ return name_content_type; });
    DeserializeContractHexData(contract[name_content], m_content, [&](){ return name_content; });
    DeserializeContractString(contract[name_delegate], m_delegate, [&](){ return name_delegate; });
    DeserializeContractHexData(contract[name_inscribe_script_pk], m_inscribe_script_pk, [&](){ return name_inscribe_script_pk; });
    DeserializeContractHexData(contract[name_inscribe_script_market_pk], m_inscribe_script_market_pk, [&](){ return name_inscribe_script_market_pk; });
    DeserializeContractHexData(contract[name_inscribe_sig], m_inscribe_sig, [&](){ return name_inscribe_sig; });
    DeserializeContractHexData(contract[name_fund_mining_fee_sig], m_fund_mining_fee_sig, [&](){ return name_fund_mining_fee_sig; });
    DeserializeContractHexData(contract[name_inscribe_int_pk], m_inscribe_int_pk, [&](){ return name_inscribe_int_pk; });
    DeserializeContractHexData(contract[name_fund_mining_fee_int_pk], m_fund_mining_fee_int_pk, [&](){ return name_fund_mining_fee_int_pk; });
    DeserializeContractString(contract[name_change_addr], m_change_addr, [&](){ return name_change_addr; });

    {   const auto &val = contract[name_fixed_change];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(name_fixed_change.c_str());

            if (m_fixed_change)
                m_fixed_change->ReadJson(val, [](){ return name_fixed_change; });
            else
                m_fixed_change = NoZeroDestinationFactory::ReadJson(chain(), val, [](){ return name_fixed_change; });
        }
    }
}

void CreateInscriptionBuilder::RestoreTransactions() const
{
    if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
    if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));

    if (!m_parent_collection_id && m_collection_input) throw ContractTermMissing(std::string(name_collection_id));
    //if (!m_collection_input && m_parent_collection_id) throw ContractTermMissing(std::string(name_collection));

    mGenesisTx = MakeGenesisTx(CommitTx());
}

const CMutableTransaction& CreateInscriptionBuilder::CommitTx() const
{
    if (!mCommitTx) {
        if (m_inputs.empty()) throw ContractTermMissing(std::string(name_utxo));

        mCommitTx = MakeCommitTx();
    }
    return *mCommitTx;
}

CMutableTransaction CreateInscriptionBuilder::MakeCommitTx() const
{
    CMutableTransaction tx;

    CAmount total_funds = 0;
    tx.vin.reserve(m_inputs.size());
    for(const auto& input: m_inputs) {
        tx.vin.emplace_back(Txid::FromUint256(uint256S(input.output->TxID())), input.output->NOut(), input.scriptSig);
        tx.vin.back().scriptWitness.stack = input.witness;
        if (tx.vin.back().scriptWitness.stack.empty()) {
            tx.vin.back().scriptWitness.stack = input.output->Destination()->DummyWitness();
        }
        if (tx.vin.back().scriptSig.empty()) {
            tx.vin.back().scriptSig = input.output->Destination()->DummyScriptSig();
        }
        total_funds += input.output->Destination()->Amount();
    }

    tx.vout.emplace_back(m_ord_destination->Amount(), CScript() << 1 << get<0>(GenesisTapRoot()));
    CAmount genesis_sum_fee = CalculateTxFee(*m_mining_fee_rate, CreateGenesisTxTemplate()) + m_market_fee->Amount();
    if (m_author_fee)
        genesis_sum_fee += m_author_fee->Amount();

    for (const auto& fee: m_custom_fees)
       genesis_sum_fee += fee->Amount();

    if (m_rune_stone)
        genesis_sum_fee += m_rune_stone->Amount();

    if (m_parent_collection_id) {
        CAmount add_vsize = TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE;
        if (m_type == LAZY_INSCRIPTION) {
            add_vsize += TAPROOT_MULTISIG_VIN_VSIZE + 1; // for mining fee compensation input (+1 for sighash flag)
            genesis_sum_fee += CFeeRate(*m_mining_fee_rate).GetFee(add_vsize);
            tx.vout.emplace_back(genesis_sum_fee, CScript() << 1 << get<0>(FundMiningFeeTapRoot()));
        }
        else {
            add_vsize += TAPROOT_KEYSPEND_VIN_VSIZE; // for mining fee compensation input
            genesis_sum_fee += CFeeRate(*m_mining_fee_rate).GetFee(add_vsize);

            tx.vout.emplace_back(genesis_sum_fee, CScript() << 1 << m_inscribe_script_pk.value_or(xonly_pubkey()));
        }
    }
    else {
        tx.vout.back().nValue += genesis_sum_fee;
    }

    CAmount fixed_change_amount = 0;
    if (m_fixed_change) {
        fixed_change_amount = m_fixed_change->Amount();
        tx.vout.emplace_back(m_fixed_change->TxOutput());
    }

    if (m_change_addr) {
        try {
            auto changeDest = P2Address::Construct(chain(), {}, *m_change_addr);
            tx.vout.emplace_back(changeDest->TxOutput());
            CAmount change_amount = total_funds - m_ord_destination->Amount() - fixed_change_amount - genesis_sum_fee - CalculateTxFee(*m_mining_fee_rate, tx);
            changeDest->Amount(change_amount);
            tx.vout.back().nValue = changeDest->Amount();
        }
        catch (const ContractTermWrongValue &) {
            // If less than dust then spend all the excessive funds to inscription, or collection, or add to "fixed" change
            tx.vout.pop_back();
            CAmount mining_fee = CalculateTxFee(*m_mining_fee_rate, tx);
            tx.vout.back().nValue += total_funds - m_ord_destination->Amount() - fixed_change_amount - genesis_sum_fee - mining_fee;
        }
    }

    return tx;
}

const CMutableTransaction& CreateInscriptionBuilder::GenesisTx() const
{
    if (!mGenesisTx) {
        if (!m_inscribe_sig) throw ContractStateError(std::string(name_inscribe_sig));
        if (m_collection_input && !m_collection_input->output) throw ContractStateError(name_collection + '.' + name_sig);
        if (m_collection_input && !m_collection_input->witness) throw ContractStateError(name_collection + '.' + TxInput::name_witness);
        if (m_collection_input && !m_fund_mining_fee_sig) throw ContractStateError(std::string(name_fund_mining_fee_sig));

        mGenesisTx = MakeGenesisTx(CommitTx());
    }
    return *mGenesisTx;
}

std::vector<CTxOut> CreateInscriptionBuilder::GetGenesisTxSpends() const
{
    std::vector<CTxOut> spending_outs;
    spending_outs.reserve(1 + (m_parent_collection_id ? 2 : 0));

    spending_outs.emplace_back(CommitTx().vout.front());
    if (m_parent_collection_id) {
        if (m_collection_input) {
            spending_outs.emplace_back(m_collection_input->output->Destination()->TxOutput());
        }
        else {
            // Just to stab collection prevout
            spending_outs.emplace_back(m_collection_destination->TxOutput());
        }
        if (CommitTx().vout.size() < 2) throw ContractStateError("fund_mining_fee output not found");
        spending_outs.emplace_back(CommitTx().vout[1]);
    }
    return spending_outs;
}

l15::bytevector CreateInscriptionBuilder::InscribeScriptControlBlock(const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>& tr) const
{
    std::vector<uint256> genesis_scriptpath = get<2>(tr).CalculateScriptPath(get<2>(tr).GetScripts().front());
    bytevector control_block;
    control_block.reserve(1 + m_inscribe_int_pk->size() + genesis_scriptpath.size() * uint256::size());
    control_block.emplace_back(static_cast<uint8_t>(0xc0 | get<1>(tr)));
    control_block.insert(control_block.end(), m_inscribe_int_pk->begin(), m_inscribe_int_pk->end());
    for (uint256 &branch_hash: genesis_scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());
    return control_block;
}

l15::bytevector CreateInscriptionBuilder::FundMiningFeeControlBlock(const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>& tr) const
{
    std::vector<uint256> scriptpath = get<2>(tr).CalculateScriptPath(get<2>(tr).GetScripts().front());
    bytevector control_block;
    control_block.reserve(1 + m_fund_mining_fee_int_pk->size() + scriptpath.size() * uint256::size());
    control_block.emplace_back(static_cast<uint8_t>(0xc0 | get<1>(tr)));
    control_block.insert(control_block.end(), m_fund_mining_fee_int_pk->begin(), m_fund_mining_fee_int_pk->end());

    for (uint256 &branch_hash: scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());
    return control_block;
}

CMutableTransaction CreateInscriptionBuilder::MakeGenesisTx(const CMutableTransaction& commit_tx) const
{
    CMutableTransaction tx;

    tx.vin.emplace_back(commit_tx.GetHash(), 0);
    tx.vout.emplace_back(m_ord_destination->Amount(), m_ord_destination->PubKeyScript());

    const auto &tr = GetInscriptionTapRoot();

    if (m_type == LAZY_INSCRIPTION) {
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_market_sig.value_or(signature()));
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_sig.value_or(signature()));
        tx.vin.front().scriptWitness.stack.back().resize(65);
    }
    else {
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_sig.value_or(signature()));
    }
    tx.vin.front().scriptWitness.stack.emplace_back(get<2>(tr).GetScripts().front().begin(), get<2>(tr).GetScripts().front().end());
    tx.vin.front().scriptWitness.stack.emplace_back(InscribeScriptControlBlock(tr));

    if (m_parent_collection_id) {
        if (m_collection_input) {
            tx.vin.emplace_back(Txid::FromUint256(uint256S(m_collection_input->output->TxID())), m_collection_input->output->NOut());
            tx.vin.back().scriptWitness.stack = m_collection_input->witness ? m_collection_input->witness : m_collection_input->output->Destination()->DummyWitness();
        }
        else {
            tx.vin.emplace_back(Txid(), 0);
        }
        tx.vin.emplace_back(tx.vin.front().prevout.hash, 1);

        tx.vout.emplace_back(m_collection_destination->TxOutput());

        if (m_type == LAZY_INSCRIPTION) {
            auto tr = FundMiningFeeTapRoot();
            tx.vin.back().scriptWitness.stack.emplace_back(m_fund_mining_fee_market_sig.value_or(signature()));
            tx.vin.back().scriptWitness.stack.emplace_back(m_fund_mining_fee_sig.value_or(signature()));
            tx.vin.back().scriptWitness.stack.back().resize(65);
            tx.vin.back().scriptWitness.stack.emplace_back(get<2>(tr).GetScripts().front().begin(), get<2>(tr).GetScripts().front().end());
            tx.vin.back().scriptWitness.stack.emplace_back(FundMiningFeeControlBlock(tr));
        }
        else {
            tx.vin.back().scriptWitness.stack.emplace_back(m_fund_mining_fee_sig.value_or(signature()));
        }
    }

    if (m_market_fee->Amount() > 0) {
        tx.vout.emplace_back(m_market_fee->TxOutput());
    }
    if (m_author_fee && m_author_fee->Amount() > 0) {
        tx.vout.emplace_back(m_author_fee->TxOutput());
    }
    for (const auto& fee: m_custom_fees) {
        tx.vout.emplace_back(fee->TxOutput());
    }
    if (m_rune_stone) {
        tx.vin.front().nSequence = 5;
        tx.vout.emplace_back(m_rune_stone->TxOutput());
    }

//    if (!m_parent_collection_id) {
//        auto mining_fee = CalculateTxFee(*m_mining_fee_rate, tx);
//        try {
//            auto fakeOrdDest = P2Address::Construct(chain(), commit_tx.vout.front().nValue - mining_fee, m_ord_destination->Address());
//        }
//        catch (ContractTermWrongValue& ) {
//            std::throw_with_nested(ContractFundsNotEnough("Ord output too small: " + std::to_string(commit_tx.vout.front().nValue - mining_fee)));
//        }
//    }

    return tx;
}

CMutableTransaction CreateInscriptionBuilder::CreateGenesisTxTemplate() const
{
    CMutableTransaction tx;

    tx.vin = {{Txid(), 0}};

    ScriptMerkleTree genesis_tap_tree(TreeBalanceType::WEIGHTED, { MakeInscriptionScript() });
    uint256 root = genesis_tap_tree.CalculateRoot();

    xonly_pubkey emptyKey;

    std::vector<uint256> genesis_scriptpath = genesis_tap_tree.CalculateScriptPath(genesis_tap_tree.GetScripts().front());
    bytevector control_block;
    control_block.reserve(1 + emptyKey.size() + genesis_scriptpath.size() * uint256::size());
    control_block.emplace_back(static_cast<uint8_t>(0xc0));
    control_block.insert(control_block.end(), emptyKey.begin(), emptyKey.end());

    for(uint256 &branch_hash : genesis_scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());

    if (m_type == LAZY_INSCRIPTION) {
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_market_sig.value_or(signature()));
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_sig.value_or(signature()));
        tx.vin.front().scriptWitness.stack.back().resize(65);
    }
    else {
        tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_sig.value_or(signature()));
    }
    tx.vin.front().scriptWitness.stack.emplace_back(genesis_tap_tree.GetScripts().front().begin(), genesis_tap_tree.GetScripts().front().end());
    tx.vin.front().scriptWitness.stack.emplace_back(move(control_block));

    tx.vout.emplace_back(0, m_ord_destination ? m_ord_destination->PubKeyScript() : (CScript() << 1 << emptyKey));
    if (m_market_fee->Amount() > 0) {
        tx.vout.emplace_back(m_market_fee->TxOutput());
    }
    if (m_author_fee && m_author_fee->Amount() > 0) {
        tx.vout.emplace_back(m_author_fee->TxOutput());
    }
    for (const auto& fee: m_custom_fees) {
        tx.vout.emplace_back(fee->TxOutput());
    }
    if (m_rune_stone) {
        tx.vout.emplace_back(m_rune_stone->TxOutput());
    }

    return tx;
}


std::string CreateInscriptionBuilder::MakeInscriptionId() const
{
    return MakeGenesisTx(MakeCommitTx()).GetHash().GetHex() + "i0";
}

CAmount CreateInscriptionBuilder::CalculateMissingAmount(std::string address)
{
    CAmount funds = 0;
    funds = std::accumulate(m_inputs.begin(), m_inputs.end(), funds, [](CAmount prev, const auto& input) { return prev + input.output->Amount(); });

    CAmount mining_fee = CalculateMiningFeeAmount();
    CAmount required_amount = mining_fee + m_ord_destination->Amount();

    // Do not need to take collection input/output amounts into this acccounting
    // if (m_parent_collection_id) {
    //     if (!m_collection_destination) throw ContractStateError(name_collection_destination + " not set");
    //     required_amount += m_collection_destination->Amount();
    // }

    if (m_fixed_change)
        required_amount += m_fixed_change->Amount();

    if (m_market_fee)
        required_amount += m_market_fee->Amount();

    if (m_author_fee)
        required_amount += m_author_fee->Amount();

    for (const auto& fee: m_custom_fees)
        required_amount += fee->Amount();

    if (m_rune_stone)
        required_amount += m_rune_stone->Amount();

    if (required_amount > funds) {
        if (address.empty())
            return required_amount - funds;

        auto add_source = P2Address::Construct(chain(), {}, address);
        AddInput(std::make_shared<UTXO>(chain(), uint256::ZERO.GetHex(), 0, add_source));
        required_amount += CalculateMiningFeeAmount() - mining_fee;
        m_inputs.pop_back();

        return std::max(required_amount - funds, add_source->Amount());
    }
    return 0;
}

CAmount CreateInscriptionBuilder::CalculateMiningFeeAmount() const
{
    if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
    if (!m_ord_destination) throw ContractTermMissing(std::string(name_ord));
    if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
    if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));

    if (!m_parent_collection_id && m_collection_input) throw ContractTermMissing(std::string(name_collection_id));
    //if (!m_collection_input && m_parent_collection_id) throw ContractTermMissing(std::string(name_collection));

    CMutableTransaction commitTx = MakeCommitTx();
    CAmount commit_fee = CalculateTxFee(*m_mining_fee_rate, commitTx);
    CAmount genesis_fee = CalculateTxFee(*m_mining_fee_rate, MakeGenesisTx(commitTx));

    return commit_fee + genesis_fee;
}


CAmount CreateInscriptionBuilder::GetMinFundingAmount(const std::string& params) const {
    if(!m_ord_destination) throw ContractStateError(std::string(name_ord_amount));
    if(!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
    if(m_type == LAZY_INSCRIPTION && !m_author_fee) throw ContractTermMissing(std::string(name_author_fee));

    CAmount amount = m_ord_destination->Amount() + m_market_fee->Amount() + CalculateWholeFee(params);
    if (m_author_fee)
        amount += m_author_fee->Amount();
    for (const auto& fee: m_custom_fees)
        amount += fee->Amount();
    if (m_rune_stone)
        amount += m_rune_stone->Amount();

    return amount;
}

std::string CreateInscriptionBuilder::GetGenesisTxMiningFee() const
{
    CAmount fee = CalculateTxFee(*m_mining_fee_rate, CreateGenesisTxTemplate());
    if (m_parent_collection_id) fee += CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_KEYSPEND_VIN_VSIZE*2 + TAPROOT_VOUT_VSIZE);
    return FormatAmount(fee);
}

CAmount CreateInscriptionBuilder::CalculateWholeFee(const std::string& params) const
{
    if (!m_mining_fee_rate) throw ContractStateError("mining fee rate is not set");

    bool change = false, collection = false, p2wpkh_utxo = false;

    std::istringstream ss(params);
    std::string param;
    while(std::getline(ss, param, ',')) {
        if (param == FEE_OPT_HAS_CHANGE) { change = true; continue; }
        if (param == FEE_OPT_HAS_COLLECTION) { collection = true; continue; }
        if (param == FEE_OPT_HAS_P2WPKH_INPUT) { p2wpkh_utxo = true; continue; }
        throw IllegalArgument(move(param));
    }

    CMutableTransaction genesisTxTpl = CreateGenesisTxTemplate();
    CAmount genesis_fee = CalculateTxFee(*m_mining_fee_rate, genesisTxTpl);

    CAmount genesis_vsize_add = 0;
    if (collection && !m_parent_collection_id) genesis_vsize_add += COLLECTION_SCRIPT_ADD_VSIZE;
    if (m_type == LAZY_INSCRIPTION) {
        genesis_vsize_add += TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_MULTISIG_VIN_VSIZE; // Collection in/out + mining fee in
    } else if (collection || m_parent_collection_id) {
        genesis_vsize_add += TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE; // Collection in/out + mining fee in
    }

    CAmount commit_vsize = TX_BASE_VSIZE + TAPROOT_VOUT_VSIZE;//MIN_TAPROOT_TX_VSIZE;
    if (p2wpkh_utxo) {
        commit_vsize += P2WPKH_VIN_VSIZE * (m_inputs.size() ? m_inputs.size() : 1);
    }
    else {
        commit_vsize += TAPROOT_KEYSPEND_VIN_VSIZE * (m_inputs.size() ? m_inputs.size() : 1);
    }
    if (change) commit_vsize += TAPROOT_VOUT_VSIZE;
    if (collection || m_parent_collection_id) {
            commit_vsize += TAPROOT_VOUT_VSIZE;
    }

    return genesis_fee + CFeeRate(*m_mining_fee_rate).GetFee(genesis_vsize_add + commit_vsize);
}

std::shared_ptr<IContractOutput> CreateInscriptionBuilder::InscriptionOutput() const
{
    return std::make_shared<UTXO>(chain(), GenesisTx().GetHash().GetHex(), 0, m_ord_destination);
}

std::shared_ptr<IContractOutput> CreateInscriptionBuilder::CollectionOutput() const
{
    if (m_collection_destination) {
        return std::make_shared<UTXO>(chain(), GenesisTx().GetHash().GetHex(), 1, m_collection_destination);
    }
    else
        return {};
}

std::shared_ptr<IContractOutput> CreateInscriptionBuilder::ChangeOutput() const
{
    std::shared_ptr<IContractOutput> res;
    if (m_change_addr) {
        CMutableTransaction commitTx = CommitTx();
        if (m_parent_collection_id && m_fixed_change) {
            if (commitTx.vout.size() == 4) {
                res = std::make_shared<UTXO>(chain(), commitTx.GetHash().GetHex(), 3, commitTx.vout[3].nValue, *m_change_addr);
            }
        }
        if (m_parent_collection_id || m_fixed_change) {
            if (commitTx.vout.size() == 3) {
                res = std::make_shared<UTXO>(chain(), commitTx.GetHash().GetHex(), 2, commitTx.vout[2].nValue, *m_change_addr);
            }
        }
        else {
            if (commitTx.vout.size() == 2) {
                res = std::make_shared<UTXO>(chain(), commitTx.GetHash().GetHex(), 1, commitTx.vout[1].nValue, *m_change_addr);
            }
        }
    }
    return res;
}

std::shared_ptr<IContractOutput> CreateInscriptionBuilder::FixedChangeOutput() const
{
    if (m_fixed_change) {
        return std::make_shared<UTXO>(chain(), MakeCommitTx().GetHash().GetHex(), m_parent_collection_id ? 2 : 1, m_fixed_change);
    }
    else
        return {};
}

std::string CreateInscriptionBuilder::RawTransaction(InscribePhase phase, uint32_t n) const
{
    if (n == 0) {
        return EncodeHexTx(MakeCommitTx());
    }
    else if (n == 1) {
        return EncodeHexTx(MakeGenesisTx(MakeCommitTx()));
    }
    else return {};
}

}
