
#include <ranges>
#include <exception>

#include "univalue.h"

#include "serialize.h"
#include "interpreter.h"
#include "core_io.h"
#include "feerate.h"
#include "streams.h"

#include "create_inscription.hpp"
#include "script_merkle_tree.hpp"
#include "channel_keys.hpp"

#include "inscription_common.hpp"

namespace l15::utxord {

namespace {

const std::string val_create_inscription("CreateInscription");
const std::string val_create_collection("CreateCollection");


CScript MakeInscriptionScript(const xonly_pubkey& pk, const std::string& content_type, const bytevector& data, const std::optional<std::string>& collection_id = {})
{
    CScript script;
    script << pk;
    script << OP_CHECKSIG;
    script << OP_0;
    script << OP_IF;
    script << ORD_TAG;
    script << CONTENT_TYPE_TAG;
    script << bytevector(content_type.begin(), content_type.end());

    if (collection_id) {
        CheckCollectionId(*collection_id);
        script << COLLECTION_ID_TAG;
        script << bytevector(collection_id->begin(), collection_id->end());
    }

    script << CONTENT_TAG;
    auto pos = data.begin();
    for ( ; pos + chunk_size < data.end(); pos += chunk_size) {
        script << bytevector(pos, pos + chunk_size);
    }
    if (pos != data.end()) {
        script << bytevector(pos, data.end());
    }


    script << OP_ENDIF;

    return script;
}

CScript MakeCollectionScript(const xonly_pubkey& pk, const std::string& collection_id)
{
    CheckCollectionId(collection_id);

    CScript script;
    script << pk;
    script << OP_CHECKSIG;
    script << OP_0;
    script << OP_IF;
    script << ORD_PARENT_TAG;
    script << COLLECTION_ID_TAG;
    script << bytevector(collection_id.begin(), collection_id.end());
    script << OP_ENDIF;

    return script;
}

std::pair<xonly_pubkey, uint8_t> MakeCollectionTapRootPubKey(const string &collection_id, const xonly_pubkey &script_pk, const xonly_pubkey &internal_pk)
{
    ScriptMerkleTree tap_tree(TreeBalanceType::WEIGHTED, {MakeCollectionScript(script_pk, collection_id)});
    uint256 root = tap_tree.CalculateRoot();

    return core::ChannelKeys::AddTapTweak(internal_pk, root);
}

}

std::string Collection::GetCollectionTapRootPubKey(const string &collection_id, const string &script_pk, const string &internal_pk)
{
    auto taproot = MakeCollectionTapRootPubKey(collection_id, unhex<xonly_pubkey>(script_pk), unhex<xonly_pubkey>(internal_pk));
    return hex(taproot.first);
}

const uint32_t CreateInscriptionBuilder::m_protocol_version = 7;

const std::string CreateInscriptionBuilder::name_ord_amount = "ord_amount";
const std::string CreateInscriptionBuilder::name_utxo = "utxo";
const std::string CreateInscriptionBuilder::name_xtra_utxo = "xtra_utxo";
const std::string CreateInscriptionBuilder::name_collection = "collection";
const std::string CreateInscriptionBuilder::name_collection_id = "collection_id";
const std::string CreateInscriptionBuilder::name_collection_mining_fee_sig = "collection_mining_fee_sig";
const std::string CreateInscriptionBuilder::name_content_type = "content_type";
const std::string CreateInscriptionBuilder::name_content = "content";
const std::string CreateInscriptionBuilder::name_inscribe_script_pk = "inscribe_script_pk";
const std::string CreateInscriptionBuilder::name_inscribe_int_pk = "inscribe_int_pk";
const std::string CreateInscriptionBuilder::name_inscribe_sig = "inscribe_sig";
const std::string CreateInscriptionBuilder::name_destination_pk = "destination_pk";
const std::string CreateInscriptionBuilder::name_change_pk = "change_pk";
//const std::string CreateInscriptionBuilder::name_parent_collection_script_pk = "parent_collection_script_pk";
//const std::string CreateInscriptionBuilder::name_parent_collection_int_pk = "parent_collection_int_pk";
//const std::string CreateInscriptionBuilder::name_parent_collection_out_pk = "parent_collection_out_pk";
//const std::string CreateInscriptionBuilder::name_collection_script_pk = "collection_script_pk";
//const std::string CreateInscriptionBuilder::name_collection_int_pk = "collection_int_pk";
//const std::string CreateInscriptionBuilder::name_collection_commit_sig = "collection_commit_sig";
//const std::string CreateInscriptionBuilder::name_collection_out_pk = "collection_out_pk";

const std::string CreateInscriptionBuilder::FEE_OPT_HAS_CHANGE = "change";
const std::string CreateInscriptionBuilder::FEE_OPT_HAS_COLLECTION = "collection";
const std::string CreateInscriptionBuilder::FEE_OPT_HAS_XTRA_UTXO = "extra_utxo";

CreateInscriptionBuilder &CreateInscriptionBuilder::AddUTXO(const std::string &txid, uint32_t nout,
                                                            const std::string& amount,
                                                            const std::string& pk)
{
    m_utxo.emplace_back(std::string(txid), nout, ParseAmount(amount), unhex<xonly_pubkey>(pk));
    return *this;
}

CreateInscriptionBuilder& CreateInscriptionBuilder::AddToCollection(const std::string& collection_id,
                                                                    const std::string& utxo_txid, uint32_t utxo_nout, const std::string& utxo_amount,
                                                                    const std::string& collection_pk)
{
    CheckCollectionId(collection_id);
    m_parent_collection_id = collection_id;
    m_collection_utxo = {utxo_txid, utxo_nout, ParseAmount(utxo_amount), unhex<xonly_pubkey>(collection_pk)};
    return *this;
}

CreateInscriptionBuilder &CreateInscriptionBuilder::AddFundMiningFee(const std::string &txid, uint32_t nout,
                                                                     const std::string& amount,
                                                                     const std::string& pk)
{
    m_xtra_utxo.emplace_back(std::string(txid), nout, ParseAmount(amount), unhex<xonly_pubkey>(pk));
    return *this;
}

CreateInscriptionBuilder &CreateInscriptionBuilder::MiningFeeRate(const std::string &rate)
{
    SetMiningFeeRate(rate);
    return *this;
}

CreateInscriptionBuilder &CreateInscriptionBuilder::Data(const std::string& content_type, const std::string &hex_data)
{
    m_content_type = content_type;
    m_content = unhex<bytevector>(hex_data);
    return *this;
}

CreateInscriptionBuilder &CreateInscriptionBuilder::InscribePubKey(const std::string &pk)
{
    m_destination_pk = unhex<xonly_pubkey>(pk);
    return *this;
}

CreateInscriptionBuilder &CreateInscriptionBuilder::ChangePubKey(const std::string &pk)
{
    m_change_pk = unhex<xonly_pubkey>(pk);
    return *this;
}


std::string CreateInscriptionBuilder::GetInscribeInternalPubKey() const
{
    if (m_inscribe_int_pk) {
        return hex(*m_inscribe_int_pk);
    }
    else
        throw ContractStateError(std::string(name_inscribe_int_pk) + " undefined");
}

void CreateInscriptionBuilder::CheckBuildArgs() const
{
    if (!m_destination_pk) {
        throw ContractTermMissing("destination pubkey");
    }
    if (!m_content) {
        throw ContractTermMissing("content");
    }
    if (!m_content_type) {
        throw ContractTermMissing("content-type");
    }
    if (m_utxo.empty()) {
        throw ContractTermMissing("UTXO");
    }
    if (!m_mining_fee_rate) {
        throw ContractTermMissing("mining fee rate");
    }
}

void CreateInscriptionBuilder::SignCommit(uint32_t n, const std::string& sk, const std::string& inscribe_script_pk)
{
    if (n >= m_utxo.size()) throw ContractTermMissing(name_utxo + '[' + std::to_string(n) + ']');
    CheckBuildArgs();

    auto utxo_it = m_utxo.begin();
    std::advance(utxo_it, n);
    core::ChannelKeys utxo_key(unhex<seckey>(sk));
    if (utxo_key.GetLocalPubKey() != utxo_it->m_pubkey) throw ContractValueMismatch(name_utxo + '[' + std::to_string(n) + ']' + name_pk);

    if (!m_inscribe_int_pk) {
        core::ChannelKeys inscribe_internal_key = core::ChannelKeys();
        m_inscribe_int_sk = inscribe_internal_key.GetLocalPrivKey();
        m_inscribe_int_pk = inscribe_internal_key.GetLocalPubKey();
    }

    if (m_inscribe_script_pk) {
        if (*m_inscribe_script_pk != unhex<xonly_pubkey>(inscribe_script_pk)) throw ContractValueMismatch(std::string(name_inscribe_script_pk));
    }
    else {
        m_inscribe_script_pk = unhex<xonly_pubkey>(inscribe_script_pk);
    }

    CAmount utxo_amount = 0;
    std::vector<CTxOut> spending_outs;
    spending_outs.reserve(m_utxo.size());
    for (const auto& utxo: m_utxo) {
        utxo_amount += utxo.m_amount;
        spending_outs.emplace_back(utxo.m_amount, CScript() << 1 << *utxo.m_pubkey);
    }

    CMutableTransaction funding_tx = MakeCommitTx();

    utxo_it->m_sig = utxo_key.SignTaprootTx(funding_tx, n, move(spending_outs), {});
}



const CScript& CreateInscriptionBuilder::GetInscriptionScript() const
{
    if (!mInscriptionScript) {
        mInscriptionScript = MakeInscriptionScript(*m_inscribe_script_pk, *m_content_type, *m_content, m_parent_collection_id);
    }
    return *mInscriptionScript;
}


void CreateInscriptionBuilder::SignCollection(const std::string& sk)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " undefined");
    if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));
    if (!m_parent_collection_id) throw ContractStateError(name_collection_id + " undefined");
    if (!m_collection_utxo) throw ContractStateError(name_collection + " undefined");
    if (!m_collection_utxo->m_pubkey) throw ContractStateError(name_collection + '.' + name_pk + " undefined");

    core::ChannelKeys script_key(unhex<seckey>(sk));

    if (*m_collection_utxo->m_pubkey != script_key.GetLocalPubKey()) throw ContractValueMismatch(name_collection + '.' + name_pk);

    CMutableTransaction genesis_tx = MakeGenesisTx(true);
    m_collection_utxo->m_sig = script_key.SignTaprootTx(genesis_tx, 1, GetGenesisTxSpends(), {});
}

void CreateInscriptionBuilder::SignInscription(const std::string& insribe_script_sk)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " undefined");
    if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));

    core::ChannelKeys script_keypair(unhex<seckey>(insribe_script_sk));
    if (*m_inscribe_script_pk != script_keypair.GetLocalPubKey()) throw ContractValueMismatch(std::string(name_inscribe_script_pk));

    CMutableTransaction genesis_tx = MakeGenesisTx(true);

    m_inscribe_script_sig = script_keypair.SignTaprootTx(genesis_tx, 0, GetGenesisTxSpends(), GetInscriptionScript());
    if (m_collection_utxo && m_xtra_utxo.empty()) {
        m_collection_mining_fee_sig = script_keypair.SignTaprootTx(genesis_tx, 2, GetGenesisTxSpends(), {});
    }
}


void CreateInscriptionBuilder::SignFundMiningFee(uint32_t n, const string &sk)
{
    if (n >= m_xtra_utxo.size()) throw ContractTermMissing(name_xtra_utxo + '[' + std::to_string(n) + ']');
    if (!m_inscribe_script_pk) throw ContractStateError(std::string(name_inscribe_script_pk) + " undefined");
    if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));

    core::ChannelKeys keypair(unhex<seckey>(sk));
    auto xtra_it = m_xtra_utxo.begin();
    std::advance(xtra_it, n);

    if (keypair.GetLocalPubKey() != *xtra_it->m_pubkey) throw ContractValueMismatch(name_xtra_utxo + '[' + std::to_string(n) + "]." + name_pk);

    CMutableTransaction genesis_tx = MakeGenesisTx(true);

    uint32_t n_in = n + (m_collection_utxo ? 2 : 1);
    xtra_it->m_sig = keypair.SignTaprootTx(genesis_tx, n_in, GetGenesisTxSpends(), {});
}

std::vector<std::string> CreateInscriptionBuilder::RawTransactions() const
{
    if (!mCommitTx || !mGenesisTx) {
        throw ContractStateError("Transaction data unavailable");
    }

    std::string funding_tx_hex = EncodeHexTx(CTransaction(*mCommitTx));
    std::string genesis_tx_hex = EncodeHexTx(CTransaction(*mGenesisTx));

    return {move(funding_tx_hex), move(genesis_tx_hex)};
}

void CreateInscriptionBuilder::CheckContractTerms() const
{
    if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
    if (!m_content_type) throw ContractTermMissing(std::string(name_content_type));
    if (!m_content) throw ContractTermMissing(std::string(name_content));

    if (m_utxo.empty()) throw ContractTermMissing(std::string(name_utxo));
    size_t n = 0;
    for (const auto& utxo: m_utxo) {
        if (!utxo.m_sig) throw ContractTermMissing(name_utxo + '[' + std::to_string(n) + "]." + name_sig);
        ++n;
    }

    if (m_collection_utxo) {
        if (!m_collection_utxo->m_pubkey) throw ContractTermMissing(name_collection + '.' + name_pk);
        if (!m_collection_utxo->m_sig) throw ContractTermMissing(name_collection + '.' + name_sig);
        if (!m_parent_collection_id) throw ContractTermMissing(std::string(name_collection_id));
    }

    n = 0;
    for (const auto& utxo: m_xtra_utxo) {
        if (!utxo.m_sig) throw ContractTermMissing(name_xtra_utxo + '[' + std::to_string(n) + "]." + name_sig);
        ++n;
    }

    if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
    if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));
    if (!m_inscribe_script_sig) throw ContractTermMissing(std::string(name_inscribe_sig));
    if (m_collection_utxo && m_xtra_utxo.empty() && !m_collection_mining_fee_sig) throw ContractTermMissing(std::string(name_collection_mining_fee_sig));
    if (!m_destination_pk) throw ContractTermMissing(std::string(name_destination_pk));

    if (!m_change_pk && CommitTx().vout.size() == (m_collection_utxo.has_value() ? 3 : 2)) throw ContractTermMissing(std::string(name_change_pk));

    if (m_type == COLLECTION) {
        throw std::runtime_error("Collection commit is not supported now");
    }
}

std::string CreateInscriptionBuilder::Serialize() const
{
    CheckContractTerms();

    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, (int)m_protocol_version);
    contract.pushKV(name_ord_amount, m_ord_amount);
    contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);

    UniValue utxo_arr(UniValue::VARR);
    for (const auto& utxo: m_utxo) {
        UniValue utxo_val(UniValue::VOBJ);
        utxo_val.pushKV(name_txid, utxo.m_txid);
        utxo_val.pushKV(name_nout, utxo.m_nout);
        utxo_val.pushKV(name_amount, utxo.m_amount);
        utxo_val.pushKV(name_sig, hex(*utxo.m_sig));

        utxo_arr.push_back(move(utxo_val));
    }
    contract.pushKV(name_utxo, utxo_arr);

    if (m_collection_utxo) {
        UniValue collection_val(UniValue::VOBJ);
        collection_val.pushKV(name_txid, m_collection_utxo->m_txid);
        collection_val.pushKV(name_nout, m_collection_utxo->m_nout);
        collection_val.pushKV(name_amount, m_collection_utxo->m_amount);
        collection_val.pushKV(name_pk, hex(*m_collection_utxo->m_pubkey));
        collection_val.pushKV(name_sig, hex(*m_collection_utxo->m_sig));
        collection_val.pushKV(name_collection_id, *m_parent_collection_id);
        contract.pushKV(name_collection, move(collection_val));
    }

    if (!m_xtra_utxo.empty()) {
        UniValue xtra_utxo_arr(UniValue::VARR);
        for (const auto &utxo: m_xtra_utxo) {
            UniValue utxo_val(UniValue::VOBJ);
            utxo_val.pushKV(name_txid, utxo.m_txid);
            utxo_val.pushKV(name_nout, utxo.m_nout);
            utxo_val.pushKV(name_amount, utxo.m_amount);
            utxo_val.pushKV(name_sig, hex(*utxo.m_sig));

            xtra_utxo_arr.push_back(move(utxo_val));
        }
        contract.pushKV(name_xtra_utxo, xtra_utxo_arr);
    }

    contract.pushKV(name_content_type, *m_content_type);
    contract.pushKV(name_content, hex(*m_content));

    contract.pushKV(name_inscribe_script_pk, hex(*m_inscribe_script_pk));
    contract.pushKV(name_inscribe_int_pk, hex(*m_inscribe_int_pk));
    contract.pushKV(name_inscribe_sig, hex(*m_inscribe_script_sig));
    if (m_collection_utxo && m_xtra_utxo.empty()) {
        contract.pushKV(name_collection_mining_fee_sig, hex(*m_collection_mining_fee_sig));
    }

    contract.pushKV(name_destination_pk, hex(*m_destination_pk));
    if (m_change_pk)
        contract.pushKV(name_change_pk, hex(*m_change_pk));

    if (m_type == COLLECTION) {
        throw std::runtime_error("Collection commit is not supported now");
    }

    UniValue dataRoot(UniValue::VOBJ);
    dataRoot.pushKV(name_contract_type, m_type == INSCRIPTION ? val_create_inscription : val_create_collection);
    dataRoot.pushKV(name_params, move(contract));

    return dataRoot.write();
}

void CreateInscriptionBuilder::Deserialize(const std::string &data)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() == val_create_inscription)
        m_type = INSCRIPTION;
    else if (root[name_contract_type].get_str() == val_create_collection)
        m_type = COLLECTION;
    else
        throw ContractProtocolError("CreateInscription contract does not match " + root[name_contract_type].getValStr());

    const UniValue& contract = root[name_params];

    if (contract[name_version].getInt<uint32_t>() != m_protocol_version) {
        throw ContractProtocolError("Wrong " + root[name_contract_type].get_str() + " version: " + contract[name_version].getValStr());
    }

    {   const auto &val = contract[name_ord_amount];
        if (val.isNull()) throw ContractTermMissing(std::string(name_ord_amount));
        if (!val.isNum() || val.getInt<CAmount>() != m_ord_amount)
            throw ContractTermWrongValue(std::string(name_ord_amount) + ": " + contract[name_ord_amount].getValStr() + ", awaited: " + std::to_string(m_ord_amount));
    }

    {   const auto &val = contract[name_utxo];

        if (val.isNull()) throw ContractTermMissing(std::string(name_utxo));
        if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));
        if (val.empty()) throw ContractTermMissing(std::string(name_utxo));

        for (size_t n = 0; n < val.size(); ++n) {
            const UniValue &utxo = val[n];

            if (!utxo.exists(name_txid))
                throw ContractTermMissing(std::string(name_utxo) + '[' + std::to_string(n) + "]." + name_txid);
            if (!utxo.exists(name_nout))
                throw ContractTermMissing(std::string(name_utxo) + '[' + std::to_string(n) + "]." + name_nout);
            if (!utxo.exists(name_amount))
                throw ContractTermMissing(std::string(name_utxo) + '[' + std::to_string(n) + "]." + name_amount);
            if (!utxo.exists(name_sig))
                throw ContractTermMissing(std::string(name_utxo) + '[' + std::to_string(n) + "]." + name_sig);

            std::string txid = utxo[name_txid].get_str();
            uint32_t nout = utxo[name_nout].getInt<uint32_t>();
            CAmount amount = utxo[name_amount].getInt<CAmount>();
            signature sig = unhex<signature>(utxo[name_sig].get_str());

            m_utxo.emplace_back(move(txid), nout, amount);
            m_utxo.back().m_sig = move(sig);
        }
    }
    {   const auto &val = contract[name_collection];
        if (!val.isNull()) {
            if (!val.isObject()) throw ContractTermWrongFormat(std::string(name_collection));

            if (!val.exists(name_txid))
                throw ContractTermMissing(name_collection + '.' + name_txid);
            if (!val.exists(name_nout))
                throw ContractTermMissing(name_collection + '.' + name_nout);
            if (!val.exists(name_amount))
                throw ContractTermMissing(name_collection + '.' + name_amount);
            if (!val.exists(name_pk))
                throw ContractTermMissing(name_collection + '.' + name_pk);
            if (!val.exists(name_sig))
                throw ContractTermMissing(name_collection + '.' + name_sig);
            if (!val.exists(name_collection_id))
                throw ContractTermMissing(name_collection + '.' + name_collection_id);

            m_parent_collection_id = val[name_collection_id].get_str();

            std::string txid = val[name_txid].get_str();
            uint32_t nout = val[name_nout].getInt<uint32_t>();
            CAmount amount = val[name_amount].getInt<CAmount>();
            xonly_pubkey pk = unhex<signature>(val[name_pk].get_str());
            signature sig = unhex<signature>(val[name_sig].get_str());

            m_collection_utxo = {move(txid), nout, amount, move(pk), move(sig)};

        }
    }
    {   const auto &val = contract[name_xtra_utxo];

        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_xtra_utxo));

            for (size_t n = 0; n < val.size(); ++n) {
                const UniValue &utxo = val[n];

                if (!utxo.exists(name_txid))
                    throw ContractTermMissing(std::string(name_xtra_utxo) + '[' + std::to_string(n) + "]." + name_txid);
                if (!utxo.exists(name_nout))
                    throw ContractTermMissing(std::string(name_xtra_utxo) + '[' + std::to_string(n) + "]." + name_nout);
                if (!utxo.exists(name_amount))
                    throw ContractTermMissing(std::string(name_xtra_utxo) + '[' + std::to_string(n) + "]." + name_amount);
                if (!utxo.exists(name_sig))
                    throw ContractTermMissing(std::string(name_xtra_utxo) + '[' + std::to_string(n) + "]." + name_sig);

                std::string txid = utxo[name_txid].get_str();
                uint32_t nout = utxo[name_nout].getInt<uint32_t>();
                CAmount amount = utxo[name_amount].getInt<CAmount>();
                signature sig = unhex<signature>(utxo[name_sig].get_str());

                m_xtra_utxo.emplace_back(move(txid), nout, amount);
                m_xtra_utxo.back().m_sig = move(sig);
            }
        }
    }
    {   const auto &val = contract[name_mining_fee_rate];
        if (val.isNull()) throw ContractTermMissing(std::string(name_mining_fee_rate));
        m_mining_fee_rate = val.getInt<int64_t>();
    }
    {   const auto &val = contract[name_content_type];
        if (val.isNull()) throw ContractTermMissing(std::string(name_content_type));
        m_content_type = val.get_str();
    }
    {   const auto &val = contract[name_content];
        if (val.isNull()) throw ContractTermMissing(std::string(name_content));
        m_content = unhex<bytevector>(val.get_str());
    }
    {   const auto &val = contract[name_inscribe_script_pk];
        if (val.isNull()) throw ContractTermMissing(std::string(name_inscribe_script_pk));
        m_inscribe_script_pk = unhex<xonly_pubkey>(val.get_str());
    }
    {   const auto &val = contract[name_inscribe_sig];
        if (val.isNull()) throw ContractTermMissing(std::string(name_inscribe_sig));
        m_inscribe_script_sig = unhex<signature>(val.get_str());
    }
    {   const auto &val = contract[name_collection_mining_fee_sig];
        if (!val.isNull()) {
            m_collection_mining_fee_sig = unhex<signature>(val.get_str());
        }
    }
    {   const auto &val = contract[name_inscribe_int_pk];
        if (val.isNull()) throw ContractTermMissing(std::string(name_inscribe_int_pk));
        m_inscribe_int_pk = unhex<xonly_pubkey>(val.get_str());
    }
    {   const auto &val = contract[name_destination_pk];
        if (val.isNull()) throw ContractTermMissing(std::string(name_destination_pk));
        m_destination_pk = unhex<xonly_pubkey>(val.get_str());
    }
    {   const auto &val = contract[name_change_pk];
        if (!val.isNull()) {
            m_change_pk = unhex<xonly_pubkey>(val.get_str());
        }
    }
    if (m_type == COLLECTION) {
        throw std::runtime_error("Collection commit is not supported now");
    }
    RestoreTransactions();
}

void CreateInscriptionBuilder::RestoreTransactions()
{
    if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
    if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));
    if (!m_inscribe_script_sig) throw ContractTermMissing(std::string(name_inscribe_sig));

    if (!m_parent_collection_id && m_collection_utxo) throw ContractTermMissing(std::string(name_collection_id));
    if (!m_collection_utxo && m_parent_collection_id) throw ContractTermMissing(std::string(name_collection));
    if (m_collection_utxo) {
        if (!m_collection_utxo->m_pubkey) throw ContractTermMissing(std::string(name_collection) + '.' + name_pk);
        if (!m_collection_utxo->m_sig) throw ContractTermMissing(std::string(name_collection) + '.' + name_sig);
        if (m_xtra_utxo.empty() && !m_collection_mining_fee_sig) throw ContractTermMissing(std::string(name_collection_mining_fee_sig));
    }

    mGenesisTx = MakeGenesisTx(false);
}

const CMutableTransaction& CreateInscriptionBuilder::CommitTx() const
{
    if (!mCommitTx) {
        if (m_utxo.empty()) throw ContractTermMissing(std::string(name_utxo));
        uint32_t n = 0;
        for (const auto& utxo: m_utxo) {
            if (!utxo.m_sig) throw ContractStateError(std::string(name_utxo) + '[' + std::to_string(n) + "]." + name_sig);
            ++n;
        }

        mCommitTx = MakeCommitTx();
    }
    return *mCommitTx;
}

CMutableTransaction CreateInscriptionBuilder::MakeCommitTx() const {

    CMutableTransaction tx;

    CAmount total_funds = 0;
    tx.vin.reserve(m_utxo.size());
    for (const auto &utxo: m_utxo) {
        tx.vin.emplace_back(uint256S(utxo.m_txid), utxo.m_nout);
        tx.vin.back().scriptWitness.stack.emplace_back(utxo.m_sig.value_or(signature()));
        total_funds += utxo.m_amount;
    }

    CScript pubkey_script;
    pubkey_script << 1;
    if (m_inscribe_taproot_sk) {
        core::ChannelKeys taproot_keypair(*m_inscribe_taproot_sk);
        pubkey_script << taproot_keypair.GetLocalPubKey();
    }
    else if (m_inscribe_int_pk && m_inscribe_script_pk) {
        ScriptMerkleTree genesis_tap_tree(TreeBalanceType::WEIGHTED, {GetInscriptionScript()});
        uint256 root = genesis_tap_tree.CalculateRoot();
        auto taproot = core::ChannelKeys::AddTapTweak(*m_inscribe_int_pk, root);
        pubkey_script << get<0>(taproot);
    }
    else {
        pubkey_script << xonly_pubkey();
    }


    tx.vout.emplace_back(m_ord_amount, pubkey_script);
    CAmount genesis_fee = CalculateTxFee(*m_mining_fee_rate, CreateGenesisTxTemplate());

    if (m_parent_collection_id) {
        CAmount add_vsize = TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE;
        if (m_xtra_utxo.empty()) {
            add_vsize += TAPROOT_KEYSPEND_VIN_VSIZE; // for mining fee compensation input
        }
        else {
            add_vsize += TAPROOT_KEYSPEND_VIN_VSIZE * m_xtra_utxo.size();
        }
        genesis_fee += CFeeRate(*m_mining_fee_rate).GetFee(add_vsize);

        tx.vout.emplace_back(genesis_fee, CScript() << 1 << m_inscribe_script_pk.value_or(xonly_pubkey()));
    }
    else {
        tx.vout.back().nValue += genesis_fee;
    }

    try {
        tx.vout.emplace_back(0, CScript() << 1 << m_change_pk.value_or(xonly_pubkey()));
        CAmount change_amount = CalculateOutputAmount(total_funds - m_ord_amount - genesis_fee, *m_mining_fee_rate, tx);
        tx.vout.back().nValue = change_amount;
    }
    catch(const TransactionError& ) {
        // Spend all the excessive funds to inscription or collection if less then dust
        tx.vout.pop_back();
        CAmount mining_fee = CalculateTxFee(*m_mining_fee_rate, tx);
        tx.vout.back().nValue += total_funds - m_ord_amount - genesis_fee - mining_fee;
                //CalculateOutputAmount(total_funds - genesis_fee, *m_mining_fee_rate, result);
    }

    return tx;
}

const CMutableTransaction& CreateInscriptionBuilder::GenesisTx() const
{
    if (!mGenesisTx) {
        if (!m_inscribe_script_sig) throw ContractStateError(std::string(name_inscribe_sig));
        if (m_collection_utxo && !m_collection_utxo->m_sig) throw ContractStateError(name_collection + '.' + name_sig);
        if (m_collection_utxo && !m_collection_mining_fee_sig) throw ContractStateError(std::string(name_collection_mining_fee_sig));

        mGenesisTx = MakeGenesisTx(false);
    }
    return *mGenesisTx;
}

std::vector<CTxOut> CreateInscriptionBuilder::GetGenesisTxSpends() const
{
    std::vector<CTxOut> spending_outs;
    spending_outs.reserve(1 + (m_collection_utxo ? (m_xtra_utxo.empty() ? 2 : m_xtra_utxo.size() + 1) : 0));

    spending_outs.emplace_back(CommitTx().vout.front());
    if (m_collection_utxo) {
        spending_outs.emplace_back(m_collection_utxo->m_amount, CScript() << 1 << *m_collection_utxo->m_pubkey);
        if (m_xtra_utxo.empty()) {
            spending_outs.emplace_back(CommitTx().vout[1]);
        }
        else {
            for (const auto& utxo: m_xtra_utxo) {
                spending_outs.emplace_back(utxo.m_amount, CScript() << 1 << *utxo.m_pubkey);
            }
        }
    }
    return spending_outs;
}

CMutableTransaction CreateInscriptionBuilder::MakeGenesisTx(bool to_sign) const
{
    if (to_sign && !m_inscribe_taproot_sk) {
        ScriptMerkleTree genesis_tap_tree(TreeBalanceType::WEIGHTED, {GetInscriptionScript()});
        uint256 root = genesis_tap_tree.CalculateRoot();

        core::ChannelKeys inscribe_internal_key(*m_inscribe_int_sk);
        auto taproot = inscribe_internal_key.NewKeyAddTapTweak(root);
        m_inscribe_taproot_sk.emplace(taproot.first.GetLocalPrivKey());
    }

    const CMutableTransaction& commit_tx = CommitTx();

    CMutableTransaction genesis_tx = CreateGenesisTxTemplate();

    genesis_tx.vin[0].prevout.hash = commit_tx.GetHash();

    if (m_collection_utxo) {
        genesis_tx.vout.front().nValue = commit_tx.vout.front().nValue;

        genesis_tx.vin.emplace_back(uint256S(m_collection_utxo->m_txid), m_collection_utxo->m_nout);
        genesis_tx.vin.back().scriptWitness.stack.emplace_back(m_collection_utxo->m_sig.value_or(signature()));

        genesis_tx.vout.emplace_back(m_collection_utxo->m_amount, CScript() << 1 << *m_collection_utxo->m_pubkey);

        if (m_xtra_utxo.empty()) {
            genesis_tx.vin.emplace_back(commit_tx.GetHash(), 1);
            genesis_tx.vin.back().scriptWitness.stack.emplace_back(m_collection_mining_fee_sig.value_or(signature()));
        }
    }
    else {
        genesis_tx.vout.front().nValue = CalculateOutputAmount(commit_tx.vout.front().nValue, *m_mining_fee_rate, genesis_tx);
    }

    for (const auto &utxo: m_xtra_utxo) {
        genesis_tx.vin.emplace_back(uint256S(utxo.m_txid), utxo.m_nout);
        genesis_tx.vin.back().scriptWitness.stack.emplace_back(*utxo.m_sig);
    }

    return genesis_tx;
}

CMutableTransaction CreateInscriptionBuilder::CreateGenesisTxTemplate() const {
    if (!m_content_type) throw ContractStateError(std::string(name_content_type) + " undefined");
    if (!m_content) throw ContractStateError(std::string(name_content) + " undefined");

    auto emptyKey = xonly_pubkey();

    CScript genesis_script = MakeInscriptionScript(m_inscribe_script_pk.value_or(emptyKey), *m_content_type, *m_content, m_parent_collection_id);
    ScriptMerkleTree genesis_tap_tree(TreeBalanceType::WEIGHTED, {genesis_script});
    uint256 root = genesis_tap_tree.CalculateRoot();

    uint8_t taproot_parity = 0;
    if (m_inscribe_int_pk) {
        auto taproot = core::ChannelKeys::AddTapTweak(*m_inscribe_int_pk, root);
        taproot_parity = taproot.second;
    }

    std::vector<uint256> genesis_scriptpath = genesis_tap_tree.CalculateScriptPath(genesis_script);
    bytevector control_block = {static_cast<uint8_t>(0xc0 | taproot_parity)};
    control_block.reserve(1 + emptyKey.size() + genesis_scriptpath.size() * uint256::size());
    if (m_inscribe_int_pk) {
        control_block.insert(control_block.end(), m_inscribe_int_pk->begin(), m_inscribe_int_pk->end());
    }
    else {
        control_block.insert(control_block.end(), emptyKey.begin(), emptyKey.end());
    }

    for(uint256 &branch_hash : genesis_scriptpath)
        control_block.insert(control_block.end(), branch_hash.begin(), branch_hash.end());

    CMutableTransaction tx;

    tx.vin = {{uint256(0), 0}};
    tx.vin.front().scriptWitness.stack.emplace_back(m_inscribe_script_sig.value_or(signature()));
    tx.vin.front().scriptWitness.stack.emplace_back(genesis_script.begin(), genesis_script.end());
    tx.vin.front().scriptWitness.stack.emplace_back(move(control_block));

    tx.vout.emplace_back(0, CScript() << 1 << m_destination_pk.value_or(emptyKey));

    return tx;
}


std::string CreateInscriptionBuilder::MakeInscriptionId() const
{
    return MakeGenesisTx(false).GetHash().GetHex() + "i0";
}

std::string CreateInscriptionBuilder::GetMinFundingAmount(const std::string& params) const {
    if(!m_content_type) throw l15::TransactionError("content type is empty");
    if(!m_content) throw l15::TransactionError("content is empty");

    return FormatAmount(m_ord_amount + CalculateWholeFee(params));
}

std::string CreateInscriptionBuilder::GetGenesisTxMiningFee() const
{
    CAmount fee = CalculateTxFee(*m_mining_fee_rate, CreateGenesisTxTemplate());
    if (m_parent_collection_id) fee += CFeeRate(*m_mining_fee_rate).GetFee(TAPROOT_KEYSPEND_VIN_VSIZE*2 + TAPROOT_VOUT_VSIZE);
    return FormatAmount(fee);
}

CAmount CreateInscriptionBuilder::CalculateWholeFee(const std::string& params) const
{
    if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));

    bool change = false, collection = false, xtra_utxo = false;

    std::istringstream ss(params);
    std::string param;
    while(std::getline(ss, param, ',')) {
        if (param == FEE_OPT_HAS_CHANGE) { change = true; continue; }
        else if (param == FEE_OPT_HAS_COLLECTION) { collection = true; continue; }
        else if (param == FEE_OPT_HAS_XTRA_UTXO) { xtra_utxo = true; continue; }
        else throw IllegalArgumentError(move(param));
    }

    CMutableTransaction genesisTxTpl = CreateGenesisTxTemplate();
    CAmount genesis_fee = CalculateTxFee(*m_mining_fee_rate, genesisTxTpl);

    CAmount genesis_vsize_add = 0;
    if (collection && !m_parent_collection_id) genesis_vsize_add += COLLECTION_SCRIPT_ADD_VSIZE;
    if (collection || m_parent_collection_id) genesis_vsize_add += TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE + TAPROOT_KEYSPEND_VIN_VSIZE; // Collection in/out + mining fee in
    if (m_xtra_utxo.size() > 1) genesis_vsize_add += TAPROOT_KEYSPEND_VIN_VSIZE * (m_xtra_utxo.size() - 1);

    CAmount commit_vsize = MIN_TAPROOT_TX_VSIZE;
    if (m_utxo.size() > 1) {
        commit_vsize += TAPROOT_KEYSPEND_VIN_VSIZE * (m_utxo.size() - 1); // Additional UTXOs
    }
    if (change) commit_vsize += TAPROOT_VOUT_VSIZE;
    if (collection || m_parent_collection_id) {
        if (!xtra_utxo && m_xtra_utxo.empty()) {
            commit_vsize += TAPROOT_VOUT_VSIZE;
        }
    }

    return genesis_fee + CFeeRate(*m_mining_fee_rate).GetFee(genesis_vsize_add + commit_vsize);
}

}
