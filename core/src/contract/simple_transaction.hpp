#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "keyregistry.hpp"
#include "contract_builder.hpp"

namespace utxord {

enum TxPhase {TX_TERMS, TX_SIGNATURE};

class SimpleTransaction: public utxord::ContractBuilder<utxord::TxPhase>, public IContractMultiOutput
{
public:
    static const std::string name_outputs;
private:
    static const uint32_t s_protocol_version;
    static const char* s_versions;

    std::vector<TxInput> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;
    std::optional<uint32_t> m_change_nout;

public:
    explicit SimpleTransaction(ChainMode chain) : ContractBuilder(chain) {}
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    explicit SimpleTransaction(ChainMode chain, const UniValue& json) : ContractBuilder(chain)
    { SimpleTransaction::ReadJson(json, TX_TERMS); }

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    static uint32_t GetProtocolVersion()
    { return s_protocol_version; }

    static const char* SupportedVersions()
    { return s_versions; }

    CAmount CalculateWholeFee(const std::string& params) const override;
    CAmount GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(bech32(), m_inputs.size(), move(prevout)); }

    void AddOutput(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    const std::vector<TxInput>& Inputs() const { return m_inputs; }
    std::vector<TxInput>& Inputs() { return m_inputs; }
    const std::vector<std::shared_ptr<IContractDestination>>& Outputs() const { return m_outputs; }
    std::vector<std::shared_ptr<IContractDestination>>& Outputs() { return m_outputs; }

    void AddChangeOutput(std::string addr);
    void DropChangeOutput();

    void Sign(const KeyRegistry& master_key, const std::string& key_filter_tag);

    void CheckSig() const;

    CMutableTransaction MakeTx(const std::string& params) const;

    std::vector<std::string> RawTransactions() const;

    const std::string& GetContractName() const override;
    void CheckContractTerms(TxPhase phase) const override;
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
};

} // l15::utxord

