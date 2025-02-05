#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "keyregistry.hpp"
#include "contract_builder.hpp"
#include "runes.hpp"

namespace utxord {

enum TxPhase {TX_TERMS, TX_SIGNATURE};

class SimpleTransaction: public utxord::ContractBuilder<utxord::TxPhase>, public IContractMultiOutput
{
public:
    static const std::string name_outputs;
    static const std::string name_rune_inputs;
private:
    static const uint32_t s_protocol_version;
    static const uint32_t s_protocol_version_no_p2address_sign;
    static const uint32_t s_protocol_version_no_p2address;
    static const uint32_t s_protocol_version_no_rune_transfer;
    static const char* s_versions;

    std::vector<TxInput> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;
    std::optional<uint32_t> m_change_nout;
    std::optional<uint32_t> m_runestone_nout;

    std::multimap<RuneId, std::tuple<uint128_t, uint32_t>> m_rune_inputs; // rune_id -> {rune_amount, nin}

public:
    explicit SimpleTransaction(ChainMode chain) : ContractBuilder(chain) {}
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    explicit SimpleTransaction(ChainMode chain, const UniValue& json) : ContractBuilder(chain)
    { SimpleTransaction::ReadJson(json, TX_TERMS); }

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    uint32_t GetVersion() const override
    { return s_protocol_version; }

    static const char* SupportedVersions()
    { return s_versions; }

    static const char* PhaseString(TxPhase phase);
    static TxPhase ParsePhase(const std::string& p);

    CAmount CalculateWholeFee(const std::string& params) const override;
    CAmount GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    {
        if (!prevout) throw ContractTermWrongValue(name_utxo + '[' + std::to_string(m_inputs.size()) + ']');
        m_inputs.emplace_back(chain(), m_inputs.size(), move(prevout));
    }

    void AddUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr)
    { AddInput(std::make_shared<UTXO>(chain(), move(txid), nout, amount, move(addr))); }

    void AddRuneInput(std::shared_ptr<IContractOutput> prevout, RuneId runeid, uint128_t rune_amount);
    void AddRuneUTXO(std::string txid, uint32_t nout, CAmount btc_amount, std::string addr, RuneId runeid, uint128_t rune_amount);

    void AddOutput(CAmount amount, std::string addr)
    { AddOutputDestination(P2Address::Construct(chain(), amount, addr)); }

    void AddOutputDestination(std::shared_ptr<IContractDestination> destination)
    {
        if (!destination) throw ContractTermWrongValue(name_outputs + '[' + std::to_string(m_outputs.size()) + ']');
        m_outputs.emplace_back(move(destination));
    }

    void AddRuneOutputDestination(std::shared_ptr<IContractDestination> destination, RuneId runeid, uint128_t rune_amount);
    void AddRuneOutput(CAmount btc_amount, std::string addr, RuneId runeid, uint128_t rune_amount);
    void BurnRune(RuneId runeid, uint128_t rune_amount);

    const std::vector<TxInput>& Inputs() const { return m_inputs; }
    std::vector<TxInput>& Inputs() { return m_inputs; }
    std::vector<std::shared_ptr<IContractOutput>> Outputs() const
    {
        std::vector<std::shared_ptr<IContractOutput>> outputs(m_outputs.size());
        for (auto [dest, i]: std::ranges::views::zip(m_outputs, std::ranges::views::iota(0))) {
            outputs[i] = std::make_shared<UTXO>(chain(), TxID(), i, dest);
        }
        return outputs;
    }

    void AddChangeOutput(std::string addr);
    void DropChangeOutput();

    void Sign(const KeyRegistry& master_key, const std::string& key_filter_tag);
    void PartialSign(const KeyRegistry& master_key, const std::string& key_filter_tag, uint32_t nin);

    void CheckSig() const;

    CMutableTransaction MakeTx(const std::string& params) const;

    std::vector<std::string> RawTransactions() const;

    const std::string& GetContractName() const override;
    void CheckContractTerms(uint32_t version, TxPhase phase) const override;
    UniValue MakeJson(uint32_t version, TxPhase phase) const override;
    void ReadJson(const UniValue& json, TxPhase phase) override;

    std::string TxID() const override
    { return MakeTx("").GetHash().GetHex(); }

    uint32_t CountDestinations() const override
    { return m_outputs.size(); }

    const std::vector<std::shared_ptr<IContractDestination>>& Destinations() const override
    { return m_outputs; }

    std::shared_ptr<IContractOutput> ChangeOutput() const
    {
        return m_change_nout
            ? std::make_shared<UTXO>(chain(), TxID(), *m_change_nout, m_outputs[*m_change_nout])
            : std::shared_ptr<IContractOutput>();
    }

    std::shared_ptr<IContractOutput> RuneStoneOutput() const
    {
        return m_runestone_nout
            ? std::make_shared<UTXO>(chain(), TxID(), *m_runestone_nout, m_outputs[*m_runestone_nout])
            : std::shared_ptr<IContractOutput>();
    }

};

} // l15::utxord

