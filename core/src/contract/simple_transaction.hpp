#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "common.hpp"
#include "keypair.hpp"
#include "contract_builder.hpp"

namespace utxord {

class SimpleTransaction: public ContractBuilder, public IContractMultiOutput, public IJsonSerializable
{
public:
    static const std::string name_outputs;
    static const char* const type;
private:
    static const uint32_t m_protocol_version;

    std::vector<TxInput> m_inputs;
    std::vector<std::shared_ptr<IContractDestination>> m_outputs;

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

    CAmount CalculateWholeFee(const std::string& params) const override;
    std::string GetMinFundingAmount(const std::string& params) const override;

    void AddInput(std::shared_ptr<IContractOutput> prevout)
    { m_inputs.emplace_back(bech32(), m_inputs.size(), move(prevout)); }

    void AddOutput(std::shared_ptr<IContractDestination> destination)
    { m_outputs.emplace_back(move(destination)); }

    const std::vector<TxInput>& Inputs() const { return m_inputs; }
    std::vector<TxInput>& Inputs() { return m_inputs; }
    const std::vector<std::shared_ptr<IContractDestination>>& Outputs() const { return m_outputs; }
    std::vector<std::shared_ptr<IContractDestination>>& Outputs() { return m_outputs; }

    void AddChangeOutput(const std::string& addr);

    void Sign(const KeyRegistry& master_key, const std::string key_filter_tag);

    void CheckSig() const;

    std::vector<std::string> RawTransactions() const;

    UniValue MakeJson() const;
    void ReadJson(const UniValue& json);

    CMutableTransaction MakeTxTemplate() const;
    CMutableTransaction MakeTx() const;

    std::string TxID() const override
    { return MakeTx().GetHash().GetHex(); }

    std::vector<std::shared_ptr<IContractDestination>> Destinations() const override
    { return std::vector<std::shared_ptr<IContractDestination>>(m_outputs.begin(), m_outputs.end()); }

};

} // l15::utxord

