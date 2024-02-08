#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "keypair.hpp"
#include "contract_builder.hpp"

namespace utxord {

class SimpleTransaction: public ContractBuilder, public IContractOutput
{
public:
    static const std::string name_outputs;
    static const char* const type;
private:
    static const uint32_t s_protocol_version;
    static const char* s_versions;

    std::vector<ContractInput> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;

    CMutableTransaction MakeTx() const;
public:
    explicit SimpleTransaction(Bech32 bech) : ContractBuilder(bech) {}
    //explicit SimpleTransaction(ChainMode m) : SimpleTransaction(Bech32(m)) {}
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    explicit SimpleTransaction(Bech32 bech, const UniValue& json) : ContractBuilder(bech)
    { SimpleTransaction::ReadJson(json); }

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    static uint32_t GetProtocolVersion()
    { return s_protocol_version; }

    static const char* SupportedVersions()
    { return s_versions; }

    std::string GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(bech32(), m_inputs.size(), move(prevout)); }

    void AddOutput(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    const std::vector<ContractInput>& Inputs() const { return m_inputs; }
    std::vector<ContractInput>& Inputs() { return m_inputs; }
    const std::vector<std::shared_ptr<IContractDestination>> Outputs() const { return m_outputs; }
    std::vector<std::shared_ptr<IContractDestination>> Outputs() { return m_outputs; }


    void AddChangeOutput(const std::string& addr);

    void Sign(const KeyRegistry& master_key, const std::string key_filter_tag);

    std::vector<std::string> RawTransactions() const;

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;

    std::string TxID() const override
    { return MakeTx().GetHash().GetHex(); }
    uint32_t NOut() const override
    { return 0; }

    const std::shared_ptr<IContractDestination>& Destination() const override
    { return const_cast<SimpleTransaction*>(this)->Destination(); }

    std::shared_ptr<IContractDestination>& Destination() override
    { return m_outputs[NOut()]; }
};

} // l15::utxord

