#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <optional>
#include <memory>
#include <list>

#include "univalue.h"

#include "common.hpp"
#include "contract_builder.hpp"

namespace utxord {

class Collection
{
public:
    static std::string GetCollectionTapRootPubKey(const std::string& collection_id,
                                                  const std::string& script_pk,
                                                  const std::string& internal_pk);
};


enum InscribeType { INSCRIPTION, LASY_INSCRIPTION, COLLECTION };
enum InscribePhase { MARKET_TERMS, LASY_COLLECTION_MARKET_TERMS, LASY_COLLECTION_INSCRIPTION_SIGNATURE, INSCRIPTION_SIGNATURE };

class CreateInscriptionBuilder: public ContractBuilder
{
    static const std::string FEE_OPT_HAS_CHANGE;
    static const std::string FEE_OPT_HAS_COLLECTION;
    static const std::string FEE_OPT_HAS_XTRA_UTXO;

    static const CAmount COLLECTION_SCRIPT_ADD_VSIZE = 18;
    static const CAmount COLLECTION_SCRIPT_VIN_VSIZE = 195;

    static const uint32_t m_protocol_version;
    static const uint32_t m_protocol_version_no_market_fee;

    InscribeType m_type;
    std::optional<CAmount> m_ord_amount;

    std::list<Transfer> m_utxo;
    std::list<Transfer> m_xtra_utxo;

    std::optional<std::string> m_parent_collection_id;
    std::optional<Transfer> m_collection_utxo;

    std::optional<std::string> m_content_type;
    std::optional<bytevector> m_content;

    std::optional<std::string> m_metadata;

    std::optional<xonly_pubkey> m_inscribe_script_pk;
    std::optional<signature> m_inscribe_script_sig;

    std::optional<signature> m_collection_mining_fee_sig;

    mutable std::optional<seckey> m_inscribe_taproot_sk; // needed in case of a fallback scenario to return funds
    std::optional<seckey> m_inscribe_int_sk; //taproot
    std::optional<xonly_pubkey> m_inscribe_int_pk; //taproot

    std::optional<xonly_pubkey> m_destination_pk;
    std::optional<xonly_pubkey> m_change_pk;

    mutable std::optional<CScript> mInscriptionScript;
    mutable std::optional<CMutableTransaction> mCommitTx;
    mutable std::optional<CMutableTransaction> mGenesisTx;
    mutable std::optional<CMutableTransaction> mCollectionCommitTx;

private:
    void CheckBuildArgs() const;
    void CheckContractTerms(InscribePhase phase) const;

    void RestoreTransactions();

    const CScript& GetInscriptionScript() const;
    std::vector<CTxOut> GetGenesisTxSpends() const;
    CAmount CalculateWholeFee(const std::string& params) const override;

    CMutableTransaction MakeCommitTx() const;
    CMutableTransaction MakeGenesisTx(bool for_inscribe_signature) const;

    CMutableTransaction CreateGenesisTxTemplate() const;

    const CMutableTransaction& CommitTx() const;
    const CMutableTransaction& GenesisTx() const;

public:
    static const std::string name_ord_amount;
    static const std::string name_utxo;
    static const std::string name_xtra_utxo;
    static const std::string name_content_type;
    static const std::string name_content;
    static const std::string name_collection;
    static const std::string name_collection_id;
    static const std::string name_metadata;
    static const std::string name_collection_mining_fee_sig;
    static const std::string name_inscribe_script_pk;
    static const std::string name_inscribe_int_pk;
    static const std::string name_inscribe_sig;
    static const std::string name_destination_pk;
    static const std::string name_market_fee_pk;
    static const std::string name_change_pk;
//    static const std::string name_parent_collection_script_pk;
//    static const std::string name_parent_collection_int_pk;
//    static const std::string name_parent_collection_out_pk;
//    static const std::string name_collection_script_pk;
//    static const std::string name_collection_int_pk;
//    static const std::string name_collection_commit_sig;
//    static const std::string name_collection_out_pk;

    CreateInscriptionBuilder() : m_type(INSCRIPTION) {}
    CreateInscriptionBuilder(const CreateInscriptionBuilder&) = default;
    CreateInscriptionBuilder(CreateInscriptionBuilder&&) noexcept = default;

    explicit CreateInscriptionBuilder(InscribeType type) : m_type(type) {}

    CreateInscriptionBuilder& operator=(const CreateInscriptionBuilder&) = default;
    CreateInscriptionBuilder& operator=(CreateInscriptionBuilder&&) noexcept = default;

    uint32_t GetProtocolVersion() const override { return m_protocol_version; }

    const std::string& GetContentType() const { return *m_content_type; }
    std::string GetContent() const { return l15::hex(m_content.value()); }
    std::string GetDestinationPubKey() const { return l15::hex(m_destination_pk.value()); }

    std::string GetIntermediateSecKey() const { return l15::hex(m_inscribe_taproot_sk.value()); }

    void OrdAmount(const std::string& amount)
    { m_ord_amount = ParseAmount(amount); }

    void AddUTXO(const std::string &txid, uint32_t nout, const std::string& amount, const std::string& pk);
    void Data(const std::string& content_type, const std::string& hex_data);

    void MetaData(const std::string& metadata);

    void InscribePubKey(const std::string& pk)
    { m_destination_pk = unhex<xonly_pubkey>(pk); }

    void ChangePubKey(const std::string& pk)
    { m_change_pk = unhex<xonly_pubkey>(pk); }

    //void CollectionCommitPubKeys(const std::string& script_pk, const std::string& int_pk);
    void AddToCollection(const std::string& collection_id,
                                              const std::string& utxo_txid, uint32_t utxo_nout, const std::string& utxo_amount,
                                              const std::string& collection_pk);
    void FundMiningFee(const std::string &txid, uint32_t nout, const std::string& amount, const std::string& pk);

    std::string MakeInscriptionId() const;

    std::string getIntermediateTaprootSK() const
    { return hex(m_inscribe_taproot_sk.value()); }

    std::string GetInscribeScriptPubKey() const
    { return hex(m_inscribe_script_pk.value()); }

    std::string GetInscribeScriptSig() const
    { return hex(m_inscribe_script_sig.value()); }

    std::string GetInscribeInternalPubKey() const;

    std::string GetGenesisTxMiningFee() const;

    void SignCommit(uint32_t n, const std::string& sk, const std::string& inscribe_script_pk);
    void SignInscription(const std::string& insribe_script_sk);

    void SignCollection(const std::string& script_sk);
    void SignFundMiningFee(uint32_t n, const std::string& sk);

    std::string GetMinFundingAmount(const std::string& params) const override;

    std::vector<std::string> RawTransactions();

    std::string RawTransaction(uint32_t n)
    { return RawTransactions()[n]; }

    std::string Serialize(InscribePhase phase) const;
    void Deserialize(const std::string& data, InscribePhase phase);

};

} // utxord

