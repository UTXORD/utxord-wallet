#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "master_key.hpp"
#include "contract_builder.hpp"

namespace l15::utxord {

class SimpleTransaction: public ContractBuilder, public IContractOutput
{
public:
    static const std::string name_outputs;
    static const char* const type;
private:
    static const uint32_t m_protocol_version;

    std::vector<ContractInput> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;

    CMutableTransaction MakeTx() const;
public:
    SimpleTransaction() = default;
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    explicit SimpleTransaction(const UniValue& json)
    { SimpleTransaction::ReadJson(json); }

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    uint32_t GetProtocolVersion() const override
    { return 1; }

    CAmount GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(move(prevout)); }

    void AddOutput(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    void AddChangeOutput(const xonly_pubkey& pk);

    void Sign(const core::MasterKey& master_key);

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

