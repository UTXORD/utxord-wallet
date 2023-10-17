
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

#include "inscription_common.hpp"

namespace utxord {

namespace {

const std::string val_create_inscription("CreateInscription");
const std::string val_lasy_create_inscription("LasyCreateInscription");
const std::string val_create_collection("CreateCollection");

CScript MakeInscriptionScript(const xonly_pubkey& pk, const std::string& content_type, const bytevector& data,
                              const std::optional<std::string>& collection_id,
                              const std::optional<std::string>& metadata)
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
        script << COLLECTION_ID_TAG;
        script << SerializeInscriptionId(*collection_id);
    }

    if (metadata) {
        auto size = metadata->size();
        const auto* end = metadata->data() + size;
        for (const auto* p = metadata->data(); p < end; p += MAX_PUSH) {
            script << METADATA_TAG << bytevector(p,  ((p + MAX_PUSH) < end) ? (p + MAX_PUSH) : end);
        }
    }

    script << CONTENT_OP_TAG;
    auto pos = data.begin();
    for ( ; pos + MAX_PUSH < data.end(); pos += MAX_PUSH) {
        script << bytevector(pos, pos + MAX_PUSH);
    }
    if (pos != data.end()) {
        script << bytevector(pos, data.end());
    }

    script << OP_ENDIF;

    return script;
}

CScript MakeCollectionScript(const xonly_pubkey& pk, const std::string& collection_id)
{
    CScript script;
    script << pk;
    script << OP_CHECKSIG;
    script << OP_0;
    script << OP_IF;
    script << ORD_PARENT_TAG;
    script << COLLECTION_ID_TAG;
    script << SerializeInscriptionId(collection_id);
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

const uint32_t CreateInscriptionBuilder::m_protocol_version = 8;
const uint32_t CreateInscriptionBuilder::m_protocol_version_no_market_fee = 7;

const std::string CreateInscriptionBuilder::name_ord_amount = "ord_amount";
const std::string CreateInscriptionBuilder::name_utxo = "utxo";
const std::string CreateInscriptionBuilder::name_xtra_utxo = "xtra_utxo";
const std::string CreateInscriptionBuilder::name_collection = "collection";
const std::string CreateInscriptionBuilder::name_collection_id = "collection_id";
const std::string CreateInscriptionBuilder::name_metadata = "metadata";
const std::string CreateInscriptionBuilder::name_collection_mining_fee_sig = "collection_mining_fee_sig";
const std::string CreateInscriptionBuilder::name_content_type = "content_type";
const std::string CreateInscriptionBuilder::name_content = "content";
const std::string CreateInscriptionBuilder::name_inscribe_script_pk = "inscribe_script_pk";
const std::string CreateInscriptionBuilder::name_inscribe_int_pk = "inscribe_int_pk";
const std::string CreateInscriptionBuilder::name_inscribe_sig = "inscribe_sig";
const std::string CreateInscriptionBuilder::name_destination_addr = "destination_addr";
const std::string CreateInscriptionBuilder::name_change_addr = "change_addr";
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

void CreateInscriptionBuilder::AddUTXO(const std::string &txid, uint32_t nout,
                                                            const std::string& amount,
                                                            const std::string& addr)
{
    bech32().Decode(addr);
    m_utxo.emplace_back(std::string(txid), nout, ParseAmount(amount), addr);
}

void CreateInscriptionBuilder::AddToCollection(const std::string& collection_id,
                                                                    const std::string& utxo_txid, uint32_t utxo_nout, const std::string& utxo_amount,
                                                                    const std::string& collection_addr)
{
    CheckInscriptionId(collection_id);
    bech32().Decode(collection_addr);
    m_parent_collection_id = collection_id;
    m_collection_utxo = {utxo_txid, utxo_nout, ParseAmount(utxo_amount), collection_addr};
}


void CreateInscriptionBuilder::MetaData(const string &metadata)
{
    UniValue check_metadata;
    if (!check_metadata.read(metadata)) {
        throw ContractTermWrongValue(std::string(name_metadata) + " is not JSON");
    }

    m_metadata = metadata;
}

void CreateInscriptionBuilder::FundMiningFee(const std::string &txid, uint32_t nout,
                                             const std::string& amount,
                                             const std::string& addr)
{
    bech32().Decode(addr);
    m_xtra_utxo.emplace_back(std::string(txid), nout, ParseAmount(amount), addr);
}

void CreateInscriptionBuilder::Data(const std::string& content_type, const std::string &hex_data)
{
    m_content_type = content_type;
    m_content = unhex<bytevector>(hex_data);
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
    if (!m_destination_addr) {
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
    if (utxo_key.GetLocalPubKey() != get<1>(bech32().Decode(*utxo_it->m_addr))) throw ContractTermMismatch(name_utxo + '[' + std::to_string(n) + ']' + name_pk);

    if (!m_inscribe_int_pk) {
        core::ChannelKeys inscribe_internal_key = core::ChannelKeys();
        m_inscribe_int_sk = inscribe_internal_key.GetLocalPrivKey();
        m_inscribe_int_pk = inscribe_internal_key.GetLocalPubKey();
    }

    if (m_inscribe_script_pk) {
        if (*m_inscribe_script_pk != unhex<xonly_pubkey>(inscribe_script_pk)) throw ContractTermMismatch(std::string(name_inscribe_script_pk));
    }
    else {
        m_inscribe_script_pk = unhex<xonly_pubkey>(inscribe_script_pk);
    }

    CAmount utxo_amount = 0;
    std::vector<CTxOut> spending_outs;
    spending_outs.reserve(m_utxo.size());
    for (const auto& utxo: m_utxo) {
        utxo_amount += utxo.m_amount;
        spending_outs.emplace_back(utxo.m_amount, bech32().PubKeyScript(*utxo.m_addr));
    }

    CMutableTransaction funding_tx = MakeCommitTx();

    utxo_it->m_sig = utxo_key.SignTaprootTx(funding_tx, n, move(spending_outs), {});
}


const CScript& CreateInscriptionBuilder::GetInscriptionScript() const
{
    if (!mInscriptionScript) {
        mInscriptionScript = MakeInscriptionScript(*m_inscribe_script_pk, *m_content_type, *m_content, m_parent_collection_id, m_metadata);
    }
    return *mInscriptionScript;
}


void CreateInscriptionBuilder::SignCollection(const std::string& sk)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " undefined");
    //if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));
    if (!m_parent_collection_id) throw ContractStateError(name_collection_id + " undefined");
    if (!m_collection_utxo) throw ContractStateError(name_collection + " undefined");
    if (!m_collection_utxo->m_addr) throw ContractStateError(name_collection + '.' + name_pk + " undefined");

    core::ChannelKeys script_key(unhex<seckey>(sk));

    if (get<1>(bech32().Decode(*m_collection_utxo->m_addr)) != script_key.GetLocalPubKey()) throw ContractTermMismatch(name_collection + '.' + name_pk);

    CMutableTransaction genesis_tx = MakeGenesisTx(false);
    m_collection_utxo->m_sig = script_key.SignTaprootTx(genesis_tx, 1, GetGenesisTxSpends(), {});
}

void CreateInscriptionBuilder::SignInscription(const std::string& insribe_script_sk)
{
    if (!m_inscribe_script_pk) throw ContractStateError(name_inscribe_script_pk + " undefined");
    if (!m_inscribe_int_sk) throw ContractStateError(std::string("internal inscription key undefined: has commit tx been signed?"));

    core::ChannelKeys script_keypair(unhex<seckey>(insribe_script_sk));
    if (*m_inscribe_script_pk != script_keypair.GetLocalPubKey()) throw ContractTermMismatch(std::string(name_inscribe_script_pk));

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

    if (keypair.GetLocalPubKey() != get<1>(bech32().Decode(*xtra_it->m_addr))) throw ContractTermMismatch(name_xtra_utxo + '[' + std::to_string(n) + "]." + name_pk);

    CMutableTransaction genesis_tx = MakeGenesisTx(true);

    uint32_t n_in = n + (m_collection_utxo ? 2 : 1);
    xtra_it->m_sig = keypair.SignTaprootTx(genesis_tx, n_in, GetGenesisTxSpends(), {});
}

std::vector<std::string> CreateInscriptionBuilder::RawTransactions()
{
    if (!mCommitTx || !mGenesisTx) {
        RestoreTransactions();
    }

    std::string funding_tx_hex = EncodeHexTx(CTransaction(*mCommitTx));
    std::string genesis_tx_hex = EncodeHexTx(CTransaction(*mGenesisTx));

    return {move(funding_tx_hex), move(genesis_tx_hex)};
}

void CreateInscriptionBuilder::CheckContractTerms(InscribePhase phase) const
{
    if (m_type == COLLECTION) {
        throw std::runtime_error("Collection commit is not supported");
    }
    switch (phase) {
    case INSCRIPTION_SIGNATURE:
        if (m_collection_utxo) {
            if (!m_collection_utxo->m_sig) throw ContractTermMissing(name_collection + '.' + name_sig);
        }
        //no break
    case LASY_COLLECTION_INSCRIPTION_SIGNATURE:
        if (!m_ord_amount) throw ContractTermMissing(std::string(name_ord_amount));
        if (!m_mining_fee_rate) throw ContractTermMissing(std::string(name_mining_fee_rate));
        if (m_utxo.empty()) throw ContractTermMissing(std::string(name_utxo));
        {
            size_t n = 0;
            for (const auto &utxo: m_utxo) {
                if (!utxo.m_sig) throw ContractTermMissing(move((name_utxo + '[') += std::to_string(n) + "]." + name_sig));
                ++n;
            }
        }
        {
            size_t n = 0;
            for (const auto &utxo: m_xtra_utxo) {
                if (!utxo.m_sig) throw ContractTermMissing(move(((name_xtra_utxo + '[') += std::to_string(n) += "].") += name_sig));
                ++n;
            }
        }
        if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
        if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));
        if (!m_inscribe_script_sig) throw ContractTermMissing(std::string(name_inscribe_sig));
        if (m_collection_utxo && m_xtra_utxo.empty() && !m_collection_mining_fee_sig)
            throw ContractTermMissing(std::string(name_collection_mining_fee_sig));
        if (!m_destination_addr) throw ContractTermMissing(std::string(name_destination_addr));
        bech32().Decode(*m_destination_addr);

        if (m_change_addr) bech32().Decode(*m_change_addr);

//        if (!m_change_addr && CommitTx().vout.size() == (m_collection_utxo.has_value() ? 3 : 2))
//            throw ContractTermMissing(std::string(name_change_addr));

        //no break
    case LASY_COLLECTION_MARKET_TERMS:
        if (!m_content_type) throw ContractTermMissing(std::string(name_content_type));
        if (!m_content) throw ContractTermMissing(std::string(name_content));
        if (m_type == LASY_INSCRIPTION && !m_collection_utxo) throw ContractTermMissing(std::string(name_collection));
        if (m_collection_utxo) {
            if (!m_parent_collection_id) throw ContractTermMissing(std::string(name_collection_id));
            if (!m_collection_utxo->m_addr) throw ContractTermMissing(name_collection + '.' + name_pk);
        }
        //no break
    case MARKET_TERMS:
        if (!m_market_fee) throw ContractTermMissing(std::string(name_market_fee));
        if (*m_market_fee > 0 && !m_market_fee_addr) throw ContractTermMissing(std::string(name_market_fee));
    }
}

std::string CreateInscriptionBuilder::Serialize(InscribePhase phase) const
{
    CheckContractTerms(phase);

    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, (int)m_protocol_version);

    switch (phase) {
    case INSCRIPTION_SIGNATURE:
    case LASY_COLLECTION_INSCRIPTION_SIGNATURE:
        contract.pushKV(name_ord_amount, *m_ord_amount);
        contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);

        {
            UniValue utxo_arr(UniValue::VARR);
            for (const auto &utxo: m_utxo) {
                UniValue utxo_val(UniValue::VOBJ);
                utxo_val.pushKV(name_txid, utxo.m_txid);
                utxo_val.pushKV(name_nout, utxo.m_nout);
                utxo_val.pushKV(name_amount, utxo.m_amount);
                utxo_val.pushKV(name_sig, hex(*utxo.m_sig));

                utxo_arr.push_back(move(utxo_val));
            }
            contract.pushKV(name_utxo, utxo_arr);
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
            contract.pushKV(name_xtra_utxo, move(xtra_utxo_arr));
        }

        contract.pushKV(name_inscribe_script_pk, hex(*m_inscribe_script_pk));
        contract.pushKV(name_inscribe_int_pk, hex(*m_inscribe_int_pk));
        contract.pushKV(name_inscribe_sig, hex(*m_inscribe_script_sig));
        if (m_collection_utxo && m_xtra_utxo.empty()) {
            contract.pushKV(name_collection_mining_fee_sig, hex(*m_collection_mining_fee_sig));
        }

        contract.pushKV(name_destination_addr, *m_destination_addr);
        if (m_change_addr)
            contract.pushKV(name_change_addr, *m_change_addr);

        //no break
    case LASY_COLLECTION_MARKET_TERMS:
        contract.pushKV(name_content_type, *m_content_type);
        contract.pushKV(name_content, hex(*m_content));

        if (m_metadata) {
            contract.pushKV(name_metadata, *m_metadata);
        }

        if (m_collection_utxo) {
            UniValue collection_val(UniValue::VOBJ);
            collection_val.pushKV(name_txid, m_collection_utxo->m_txid);
            collection_val.pushKV(name_nout, m_collection_utxo->m_nout);
            collection_val.pushKV(name_amount, m_collection_utxo->m_amount);
            collection_val.pushKV(name_addr, *m_collection_utxo->m_addr);
            if (m_collection_utxo->m_sig) {
                collection_val.pushKV(name_sig, hex(*m_collection_utxo->m_sig));
            }
            collection_val.pushKV(name_collection_id, *m_parent_collection_id);
            contract.pushKV(name_collection, move(collection_val));
        }

    case MARKET_TERMS:
        contract.pushKV(name_market_fee, *m_market_fee);
        if (*m_market_fee > 0)
            contract.pushKV(name_market_fee_addr, *m_market_fee_addr);

    }

    UniValue dataRoot(UniValue::VOBJ);
    if (m_type == INSCRIPTION)
        dataRoot.pushKV(name_contract_type,  val_create_inscription);
    else
        dataRoot.pushKV(name_contract_type,  val_lasy_create_inscription);

    dataRoot.pushKV(name_params, move(contract));

    return dataRoot.write();
}

void CreateInscriptionBuilder::Deserialize(const std::string &data, InscribePhase phase)
{
    UniValue root;
    root.read(data);

    if (root[name_contract_type].get_str() == val_create_inscription) {
        if (m_type != INSCRIPTION) throw ContractTermMismatch (std::string(name_contract_type));
    } else if (root[name_contract_type].get_str() == val_lasy_create_inscription) {
        if (m_type != LASY_INSCRIPTION) throw ContractTermMismatch (std::string(name_contract_type));
    } else
        throw ContractProtocolError("CreateInscription contract does not match " + root[name_contract_type].getValStr());

    const UniValue& contract = root[name_params];

    if (contract[name_version].getInt<uint32_t>() != m_protocol_version)
        throw ContractProtocolError("Wrong " + root[name_contract_type].get_str() + " version: " + contract[name_version].getValStr());

    DeserializeContractAmount(contract[name_market_fee], m_market_fee, [&](){ return name_market_fee; });
    DeserializeContractString(contract[name_market_fee_addr], m_market_fee_addr, [&](){ return name_market_fee_addr; });
    DeserializeContractAmount(contract[name_ord_amount], m_ord_amount, [&](){ return name_ord_amount; });

    {
        const auto &val = contract[name_utxo];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_utxo));

            for (size_t n = 0; n < val.size(); ++n) {
                std::optional<Transfer> utxo;
                DeserializeContractTransfer(val[n], utxo, [&](){ return (name_utxo + '[') += std::to_string(n) += ']'; });
                if (!utxo->m_sig) throw ContractTermMissing(move(((name_utxo + '[') += std::to_string(n) += "].") += name_sig));
                m_utxo.emplace_back(move(*utxo));
            }
        }
    }
    {
        const auto &val = contract[name_collection];
        DeserializeContractTransfer(val, m_collection_utxo, [&](){ return name_collection; });
        if (m_collection_utxo) {
            DeserializeContractString(val[name_collection_id], m_parent_collection_id, [&]() { return move((name_collection + '.') += name_collection_id); });
            if (!m_parent_collection_id) throw ContractTermMissing(move((name_collection + '.') += name_collection_id));
        }
    }

    DeserializeContractString(contract[name_metadata], m_metadata, [&](){ return name_metadata; });
    if (m_metadata) {
        UniValue check_metadata;
        if (!check_metadata.read(*m_metadata)) {
            throw ContractTermWrongValue(name_metadata + " is not JSON");
        }
    }

    {
        const auto &val = contract[name_xtra_utxo];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractTermWrongFormat(std::string(name_xtra_utxo));

            for (size_t n = 0; n < val.size(); ++n) {
                std::optional<Transfer> utxo;
                DeserializeContractTransfer(val[n], utxo, [&](){ return (name_xtra_utxo + '[') += std::to_string(n) += ']'; });
                if (!utxo->m_sig) throw ContractTermMissing(move(((name_xtra_utxo + '[') += std::to_string(n) += "].") += name_sig));
                m_xtra_utxo.emplace_back(move(*utxo));
            }
        }
    }

    DeserializeContractAmount(contract[name_mining_fee_rate], m_mining_fee_rate, [&](){ return name_mining_fee_rate; });
    DeserializeContractString(contract[name_content_type], m_content_type, [&](){ return name_content_type; });
    DeserializeContractHexData(contract[name_content], m_content, [&](){ return name_content; });
    DeserializeContractHexData(contract[name_inscribe_script_pk], m_inscribe_script_pk, [&](){ return name_inscribe_script_pk; });
    DeserializeContractHexData(contract[name_inscribe_sig], m_inscribe_script_sig, [&](){ return name_inscribe_sig; });
    DeserializeContractHexData(contract[name_collection_mining_fee_sig], m_collection_mining_fee_sig, [&](){ return name_collection_mining_fee_sig; });
    DeserializeContractHexData(contract[name_inscribe_int_pk], m_inscribe_int_pk, [&](){ return name_inscribe_int_pk; });
    DeserializeContractString(contract[name_destination_addr], m_destination_addr, [&](){ return name_destination_addr; });
    DeserializeContractString(contract[name_change_addr], m_change_addr, [&](){ return name_change_addr; });

    CheckContractTerms(phase);
}

void CreateInscriptionBuilder::RestoreTransactions()
{
    if (!m_inscribe_script_pk) throw ContractTermMissing(std::string(name_inscribe_script_pk));
    if (!m_inscribe_int_pk) throw ContractTermMissing(std::string(name_inscribe_int_pk));
    if (!m_inscribe_script_sig) throw ContractTermMissing(std::string(name_inscribe_sig));

    if (!m_parent_collection_id && m_collection_utxo) throw ContractTermMissing(std::string(name_collection_id));
    if (!m_collection_utxo && m_parent_collection_id) throw ContractTermMissing(std::string(name_collection));
    if (m_collection_utxo) {
        if (!m_collection_utxo->m_addr) throw ContractTermMissing(std::string(name_collection) + '.' + name_pk);
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
        //pubkey_script << xonly_pubkey();
        throw ContractStateError("Inscribe keys are not set");
    }

    tx.vout.emplace_back(*m_ord_amount, pubkey_script);
    CAmount genesis_sum_fee = CalculateTxFee(*m_mining_fee_rate, CreateGenesisTxTemplate()) + *m_market_fee;

    if (m_parent_collection_id) {
        CAmount add_vsize = TAPROOT_KEYSPEND_VIN_VSIZE + TAPROOT_VOUT_VSIZE;
        if (m_xtra_utxo.empty()) {
            add_vsize += TAPROOT_KEYSPEND_VIN_VSIZE; // for mining fee compensation input
        }
        else {
            add_vsize += TAPROOT_KEYSPEND_VIN_VSIZE * m_xtra_utxo.size();
        }
        genesis_sum_fee += CFeeRate(*m_mining_fee_rate).GetFee(add_vsize);

        tx.vout.emplace_back(genesis_sum_fee, CScript() << 1 << m_inscribe_script_pk.value_or(xonly_pubkey()));
    }
    else {
        tx.vout.back().nValue += genesis_sum_fee;
    }

    if (m_change_addr) {
        try {
            tx.vout.emplace_back(0, bech32().PubKeyScript(*m_change_addr));
            CAmount change_amount = CalculateOutputAmount(total_funds - *m_ord_amount - genesis_sum_fee, *m_mining_fee_rate, tx);
            tx.vout.back().nValue = change_amount;
        }
        catch (const TransactionError &) {
            // Spend all the excessive funds to inscription or collection if less then dust
            tx.vout.pop_back();
            CAmount mining_fee = CalculateTxFee(*m_mining_fee_rate, tx);
            tx.vout.back().nValue += total_funds - *m_ord_amount - genesis_sum_fee - mining_fee;
            //CalculateOutputAmount(total_funds - genesis_sum_fee, *m_mining_fee_rate, result);
        }
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
        spending_outs.emplace_back(m_collection_utxo->m_amount, bech32().PubKeyScript(*m_collection_utxo->m_addr));
        if (m_xtra_utxo.empty()) {
            spending_outs.emplace_back(CommitTx().vout[1]);
        }
        else {
            for (const auto& utxo: m_xtra_utxo) {
                spending_outs.emplace_back(utxo.m_amount, bech32().PubKeyScript(*utxo.m_addr));
            }
        }
    }
    return spending_outs;
}

CMutableTransaction CreateInscriptionBuilder::MakeGenesisTx(bool for_inscribe_signature) const
{
    if (for_inscribe_signature && !m_inscribe_taproot_sk) {
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

        genesis_tx.vout.emplace(genesis_tx.vout.begin()+1, m_collection_utxo->m_amount, bech32().PubKeyScript(*m_collection_utxo->m_addr));

        if (m_xtra_utxo.empty()) {
            genesis_tx.vin.emplace_back(commit_tx.GetHash(), 1);
            genesis_tx.vin.back().scriptWitness.stack.emplace_back(m_collection_mining_fee_sig.value_or(signature()));
        }
    }
    else {
        genesis_tx.vout.front().nValue = CalculateOutputAmount(commit_tx.vout.front().nValue, *m_mining_fee_rate, genesis_tx) - *m_market_fee;
    }

    for (const auto &utxo: m_xtra_utxo) {
        genesis_tx.vin.emplace_back(uint256S(utxo.m_txid), utxo.m_nout);
        genesis_tx.vin.back().scriptWitness.stack.emplace_back(*utxo.m_sig);
    }

    for (const auto& out: genesis_tx.vout) {
        if (out.nValue < Dust(3000))
            throw ContractStateError(move((std::string("Funds not enough: out[") += std::to_string(&out - genesis_tx.vout.data()) += "]: ") += std::to_string(out.nValue)));
    }

    return genesis_tx;
}

CMutableTransaction CreateInscriptionBuilder::CreateGenesisTxTemplate() const {
    if (!m_content_type) throw ContractStateError(std::string(name_content_type) + " undefined");
    if (!m_content) throw ContractStateError(std::string(name_content) + " undefined");

    auto emptyKey = xonly_pubkey();

    CScript genesis_script = MakeInscriptionScript(m_inscribe_script_pk.value_or(emptyKey), *m_content_type, *m_content, m_parent_collection_id, m_metadata);
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

    tx.vout.emplace_back(0, m_destination_addr ? bech32().PubKeyScript(*m_destination_addr) : (CScript() << 1 << emptyKey));
    if (*m_market_fee > 0) {
        tx.vout.emplace_back(*m_market_fee, bech32().PubKeyScript(*m_market_fee_addr));
    }

    return tx;
}


std::string CreateInscriptionBuilder::MakeInscriptionId() const
{
    return MakeGenesisTx(false).GetHash().GetHex() + "i0";
}

std::string CreateInscriptionBuilder::GetMinFundingAmount(const std::string& params) const {
    if(!m_ord_amount) throw ContractStateError("ord amount is not set");
    if(!m_content_type) throw ContractTermMissing("content type is not set");
    if(!m_content) throw ContractTermMissing("content is not set");
    if(!m_market_fee) throw ContractTermMissing("market fee is not set");

    CAmount amount = *m_ord_amount + *m_market_fee + CalculateWholeFee(params);
    return FormatAmount(amount);
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
