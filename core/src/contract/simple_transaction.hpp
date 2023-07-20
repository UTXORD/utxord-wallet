#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "master_key.hpp"
#include "contract_builder.hpp"

namespace l15::utxord {

class SimpleTransaction: public ContractBuilder, IJsonSerializable
{
    static const uint32_t m_protocol_version;

    std::vector<std::shared_ptr<IContractOutput>> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;

    CMutableTransaction MakeTx() const;
public:
    SimpleTransaction() = default;
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    uint32_t GetProtocolVersion() const override
    { return 1; }

    std::string GetMinFundingAmount(const std::string& params) const override
    { return FormatAmount(546); }

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(move(prevout)); }

    void AddDestination(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    void Sign(const core::MasterKey& master_key);

    std::vector<std::string> RawTransactions() const;

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;

};

} // l15::utxord

