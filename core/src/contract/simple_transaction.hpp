#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "master_key.hpp"
#include "contract_builder.hpp"

namespace utxord {

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
    explicit SimpleTransaction(Bech32 bech) : ContractBuilder(bech) {}
    //explicit SimpleTransaction(ChainMode m) : SimpleTransaction(Bech32(m)) {}
    SimpleTransaction(const SimpleTransaction&) = default;
    SimpleTransaction(SimpleTransaction&&) noexcept = default;

    explicit SimpleTransaction(Bech32 bech, const UniValue& json) : ContractBuilder(bech)
    { SimpleTransaction::ReadJson(json); }

    ~SimpleTransaction() override = default;

    SimpleTransaction& operator=(const SimpleTransaction&) = default;
    SimpleTransaction& operator=(SimpleTransaction&&) = default;

    uint32_t GetProtocolVersion() const
    { return m_protocol_version; }

    std::string GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(bech32(), m_inputs.size(), move(prevout)); }

    void AddOutput(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    void AddChangeOutput(const std::string& addr);

    void Sign(const l15::core::MasterKey& master_key);

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

