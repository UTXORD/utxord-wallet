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
    static const uint32_t s_protocol_version_no_p2address;
    static const uint32_t s_protocol_version_no_custom_fee;
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

private:
    void CheckContractTerms(uint32_t version, InscribePhase phase) const override;

    void RestoreTransactions() const;

    const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree>& GetInscriptionTapRoot() const;
public:
    std::vector<CTxOut> GetGenesisTxSpends() const;
private:
    CScript MakeInscriptionScript() const;

    l15::bytevector InscribeScriptControlBlock(const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> &tr) const;
    l15::bytevector FundMiningFeeControlBlock(const std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> &tr) const;
    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> GenesisTapRoot() const;
    std::tuple<xonly_pubkey, uint8_t, l15::ScriptMerkleTree> FundMiningFeeTapRoot() const;

    CMutableTransaction MakeCommitTx() const;
    CMutableTransaction MakeGenesisTx(const CMutableTransaction& commit_tx) const;

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

    static const char* PhaseString(InscribePhase phase);
    static InscribePhase ParsePhase(const std::string& p);

    const std::string& GetContractName() const override;
    uint32_t GetVersion() const override { return s_protocol_version; }
    UniValue MakeJson(uint32_t version, InscribePhase phase) const override;
    void ReadJson(const UniValue& json, InscribePhase phase) override;

    static const char* SupportedVersions() { return s_versions; }

    std::string GetContentType() const { return m_content_type.value_or(""); }
    std::string GetContent() const { return m_content ? l15::hex(m_content.value()) : std::string(); }
    std::string GetInscribeAddress() const { return m_ord_destination->Address(); }

    void OrdOutput(CAmount amount, std::string addr)
    { m_ord_destination = P2Address::Construct(chain(), amount, move(addr)); }

    void OrdOutputDestination(std::shared_ptr<IContractDestination> destination)
    { m_ord_destination = move(destination); }

    void AddUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr)
    { m_inputs.emplace_back(chain(), m_inputs.size(), std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr))); }

    void AddLegacyUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr, compressed_pubkey pk)
    { m_inputs.emplace_back(chain(), m_inputs.size(), std::make_shared<UTXO>(chain(), txid, nout, P2Legacy::Construct(chain(), amount, addr, pk))); }

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(chain(), m_inputs.size(), move(prevout)); }

    void ClearInputs()
    {
        m_inputs.clear();
        mCommitTx.reset();
        mGenesisTx.reset();
    }

    void Data(std::string content_type, bytevector data)
    {
        m_content_type = move(content_type);
        m_content = move(data);
    }
    void Delegate(std::string inscription_id);
    void MetaData(bytevector metadata);
    void Rune(std::shared_ptr<RuneStoneDestination> runeStone)
    { m_rune_stone = move(runeStone); }

    void InscribeScriptPubKey(xonly_pubkey pk)
    { m_inscribe_script_pk = move(pk); }

    void MarketInscribeScriptPubKey(xonly_pubkey pk)
    { m_inscribe_script_market_pk = move(pk); }

    void InscribeInternalPubKey(xonly_pubkey pk)
    { m_inscribe_int_pk = move(pk); }

    void FundMiningFeeInternalPubKey(xonly_pubkey pk)
    { m_fund_mining_fee_int_pk = move(pk); }

    void AuthorFee(CAmount amount, std::string addr)
    {
        if (amount > 0) {
            m_author_fee = P2Address::Construct(chain(), amount, move(addr));
        }
        else {
            m_author_fee = std::make_shared<ZeroDestination>();
        }
    }

    void FixedChange(CAmount amount, std::string addr)
    { m_fixed_change = P2Address::Construct(chain(), amount, move(addr)); }

    void AddCollectionInput(std::string collection_id, std::shared_ptr<IContractOutput> prevout)
    {
        Collection(move(collection_id), prevout->Amount(), prevout->Address());
        m_collection_input.emplace(chain(), 1, move(prevout));
    }

    void AddCollectionUTXO(std::string collection_id, std::string utxo_txid, uint32_t utxo_nout, CAmount amount, std::string collection_addr)
    { AddCollectionInput(move(collection_id), std::make_shared<UTXO>(chain(), move(utxo_txid), utxo_nout, amount, move(collection_addr))); }

    void Collection(std::string collection_id, CAmount amount, std::string collection_addr);

    void OverrideCollectionAddress(std::string addr);

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
    CAmount GetMinFundingAmount(const std::string& params) const override;

    CAmount CalculateMissingAmount(std::string address);
    CAmount CalculateMiningFeeAmount() const;

    l15::stringvector RawTransactions() const;
    l15::stringvector TransactionsPSBT() const;

    void ApplyPSBTSignature(const l15::stringvector& );

    uint32_t TransactionCount(InscribePhase phase) const
    { return 2; }

    std::string RawTransaction(InscribePhase phase, uint32_t n) const;

    std::shared_ptr<IContractOutput> InscriptionOutput() const;
    std::shared_ptr<IContractOutput> CollectionOutput() const;
    std::shared_ptr<IContractOutput> ChangeOutput() const;
    std::shared_ptr<IContractOutput> FixedChangeOutput() const;
};

} // utxord

