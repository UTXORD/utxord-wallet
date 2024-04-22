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
#include "script_merkle_tree.hpp"

namespace utxord {
class RuneStoneDestination;

enum InscribeType { INSCRIPTION, LAZY_INSCRIPTION };
enum InscribePhase { MARKET_TERMS, LAZY_INSCRIPTION_MARKET_TERMS, LAZY_INSCRIPTION_SIGNATURE, INSCRIPTION_SIGNATURE };

class CreateInscriptionBuilder: public utxord::ContractBuilder<utxord::InscribePhase>
{
    static const CAmount COLLECTION_SCRIPT_ADD_VSIZE = 18;
    static const CAmount COLLECTION_SCRIPT_VIN_VSIZE = 195;

    static const uint32_t s_protocol_version;
    static const uint32_t s_protocol_version_no_runes;
    static const uint32_t s_protocol_version_no_fixed_change;
    static const char* s_versions;

    InscribeType m_type;

    std::list<TxInput> m_inputs;
    std::shared_ptr<IContractDestination> m_ord_destination;
    std::shared_ptr<IContractDestination> m_collection_destination;
    std::shared_ptr<IContractDestination> m_author_fee;
    std::shared_ptr<RuneStoneDestination> m_rune_stone;

    std::shared_ptr<IContractDestination> m_fixed_change;

    std::optional<std::string> m_parent_collection_id;
    std::optional<TxInput> m_collection_input;

    std::optional<std::string> m_content_type;
    std::optional<bytevector> m_content;
    std::optional<std::string> m_delegate;

    std::optional<bytevector> m_metadata;

    std::optional<xonly_pubkey> m_inscribe_script_pk;
    std::optional<xonly_pubkey> m_inscribe_script_market_pk;
    std::optional<signature> m_inscribe_sig;
    std::optional<signature> m_inscribe_market_sig;

    std::optional<signature> m_fund_mining_fee_sig;
    std::optional<signature> m_fund_mining_fee_market_sig;

    std::optional<xonly_pubkey> m_inscribe_int_pk; //taproot
    std::optional<xonly_pubkey> m_fund_mining_fee_int_pk; //taproot

    mutable std::optional<std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>> mInscriptionTaproot;
    mutable std::optional<CMutableTransaction> mCommitTx;
    mutable std::optional<CMutableTransaction> mGenesisTx;
    mutable std::optional<CMutableTransaction> mCollectionCommitTx;

private:
    void CheckContractTerms(InscribePhase phase) const override;

    void RestoreTransactions() const;

    const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>& GetInscriptionTapRoot() const;
public:
    std::vector<CTxOut> GetGenesisTxSpends() const;
private:
    CScript MakeInscriptionScript() const;

    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> GenesisTapRoot() const;
    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> FundMiningFeeTapRoot() const;

    CMutableTransaction MakeCommitTx() const;
    CMutableTransaction MakeGenesisTx() const;

    CMutableTransaction CreateGenesisTxTemplate() const;

    const CMutableTransaction& CommitTx() const;
    const CMutableTransaction& GenesisTx() const;

public:
    static const std::string name_ord;
    static const std::string name_ord_amount;
    static const std::string name_utxo;
    static const std::string name_content_type;
    static const std::string name_content;
    static const std::string name_delegate;
    static const std::string name_collection;
    static const std::string name_collection_id;
    static const std::string name_collection_destination;
    static const std::string name_metadata;
    static const std::string name_rune_stone;
    static const std::string name_inscribe_script_pk;
    static const std::string name_inscribe_script_market_pk;
    static const std::string name_inscribe_int_pk;
    static const std::string name_inscribe_sig;
    static const std::string name_fund_mining_fee_int_pk;
    static const std::string name_fund_mining_fee_sig;
    static const std::string name_destination_addr;
    static const std::string name_author_fee;
    static const std::string name_fixed_change;

    //CreateInscriptionBuilder() : m_type(INSCRIPTION) {}
    CreateInscriptionBuilder(const CreateInscriptionBuilder&) = default;
    CreateInscriptionBuilder(CreateInscriptionBuilder&&) noexcept = default;

    explicit CreateInscriptionBuilder(ChainMode mode, InscribeType type) : ContractBuilder(mode), m_type(type) {}

    CreateInscriptionBuilder& operator=(const CreateInscriptionBuilder&) = default;
    CreateInscriptionBuilder& operator=(CreateInscriptionBuilder&&) noexcept = default;

    const std::string& GetContractName() const override;
    UniValue MakeJson(uint32_t version, InscribePhase phase) const override;
    void ReadJson(const UniValue& json, InscribePhase phase) override;

    static const char* SupportedVersions() { return s_versions; }

    std::string GetContentType() const { return m_content_type.value_or(""); }
    std::string GetContent() const { return m_content ? l15::hex(m_content.value()) : std::string(); }
    std::string GetInscribeAddress() const { return m_ord_destination->Address(); }

    std::string GetIntermediateSecKey() const;

    void OrdDestination(const std::string& amount, const std::string& addr)
    { m_ord_destination = P2Witness::Construct(chain(), l15::ParseAmount(amount), addr); }

    void AddUTXO(const std::string &txid, uint32_t nout, const std::string& amount, const std::string& addr);
    void Data(const std::string& content_type, const std::string& hex_data);
    void Delegate(const std::string& inscription_id);
    void MetaData(const std::string& metadata);
    void Rune(std::shared_ptr<RuneStoneDestination> runeStone);

    void InscribeScriptPubKey(const std::string& pk)
    { m_inscribe_script_pk = unhex<xonly_pubkey>(pk); }

    void MarketInscribeScriptPubKey(const std::string& pk)
    { m_inscribe_script_market_pk = unhex<xonly_pubkey>(pk); }

    void InscribeInternalPubKey(const std::string& pk)
    { m_inscribe_int_pk = unhex<xonly_pubkey>(pk); }

    void FundMiningFeeInternalPubKey(const std::string& pk)
    { m_fund_mining_fee_int_pk = unhex<xonly_pubkey>(pk); }

    void AuthorFee(const std::string& amount, const std::string& addr)
    {
        if (l15::ParseAmount(amount) > 0) {
            m_author_fee = P2Witness::Construct(chain(), l15::ParseAmount(amount), addr);
        }
        else {
            m_author_fee = std::make_shared<ZeroDestination>();
        }
    }

    void FixedChange(const std::string& amount, const std::string& addr)
    { m_fixed_change = P2Witness::Construct(chain(), l15::ParseAmount(amount), addr); }

    void AddToCollection(const std::string& collection_id,
                                              const std::string& utxo_txid, uint32_t utxo_nout, const std::string& amount,
                                              const std::string& collection_addr);

    void Collection(const std::string& collection_id, const std::string& amount, const std::string& collection_addr);

    void OverrideCollectionAddress(const std::string& addr);
    // { m_collection_address_override = addr; }

    std::string MakeInscriptionId() const;

    std::string GetInscribeScriptPubKey() const
    { return hex(m_inscribe_script_pk.value()); }

    std::string GetInscribeScriptSig() const
    { return hex(m_inscribe_sig.value()); }

    std::string GetInscribeInternalPubKey() const;

    std::string GetGenesisTxMiningFee() const;

    void SignCommit(const KeyRegistry &master_key, const std::string& key_filter);
    void SignInscription(const KeyRegistry &master_key, const std::string& key_filter);
    void MarketSignInscription(const KeyRegistry &master_key, const std::string& key_filter);

    void SignCollection(const KeyRegistry &master_key, const std::string& key_filter);

    CAmount CalculateWholeFee(const std::string& params) const override;
    std::string GetMinFundingAmount(const std::string& params) const override;

    std::vector<std::string> RawTransactions() const;

    uint32_t TransactionCount() const
    { return 2; }

    std::string RawTransaction(uint32_t n) const;

    std::string GetInscriptionLocation() const;
    std::string GetCollectionLocation() const;
    std::string GetChangeLocation() const;

};

} // utxord

