#pragma once

#include "contract_builder.hpp"
#include "simple_transaction.hpp"

#include <optional>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>

namespace utxord {

#ifndef DEBUG
using boost::multiprecision::uint128_t;
#else
namespace bmp = boost::multiprecision;
using uint128_t = bmp::number<bmp::debug_adaptor<bmp::cpp_int_backend<128, 128, bmp::unsigned_magnitude, bmp::unchecked, void> >>;
#endif

const uint8_t MAX_DIVISIBILITY = 38;
const uint128_t MAX_LIMIT = uint128_t(std::numeric_limits<uint64_t>::max()) + 1;
const uint32_t MAX_SPACERS = 0x7fff;

template <typename I>
uint128_t read_varint(I& i, I end);
template <typename INT>
bytevector write_varint(INT n);

uint128_t EncodeRune(const std::string& text_rune);
std::string DecodeRune(uint128_t rune);

std::string AddSpaces(const std::string& text_rune, uint32_t spacers, const std::string& space = "•");
std::string ExtractSpaces(const std::string &spaced_text_rune, uint32_t& spacers, const std::string& space = "•");

enum class RuneAction: uint8_t
{
    ETCH = 0,
    MINT = 1,
    BURN = 127
};

class RuneStone: public IContractDestination {
    ChainMode m_chain;
    CAmount m_amount = 0;

    std::optional<uint32_t> m_spacers;
    uint128_t m_rune;
    RuneAction m_action = RuneAction::BURN;
    std::optional<uint8_t> m_divisibility;
    std::optional<uint128_t> m_limit;
    std::optional<uint32_t> m_term;
    std::optional<uint32_t> m_deadline;
    std::optional<uint32_t> m_default_output;
    std::optional<wchar_t> m_symbol; // unicode index

    std::list<std::tuple<uint128_t, uint128_t, uint32_t>> m_dictionary;

    bytevector Pack() const;
    void Unpack(const bytevector& data);
public:
    explicit RuneStone(ChainMode chain, const std::string& rune_text) :
            m_chain(chain), m_spacers(0),
            m_rune(EncodeRune(ExtractSpaces(rune_text, *m_spacers, " "))) {}

    explicit RuneStone(ChainMode chain, const bytevector& data) : m_chain(chain)
    { RuneStone::Unpack(data); }

    explicit RuneStone(ChainMode chain, const UniValue& json) : m_chain(chain)
    { RuneStone::ReadJson(json); }

    void Amount(CAmount amount) override { m_amount = amount; }
    CAmount Amount() const final { return m_amount; }

    void Action(RuneAction action) { m_action = action; }
    RuneAction Action() const { return m_action; }

    void Rune(uint128_t rune) {m_rune = move(rune); }
    const uint128_t& Rune() const { return m_rune; }
    std::string RuneText() const;

    void Spacers(auto v) { if (v <= MAX_SPACERS) m_spacers.emplace((uint32_t)v); }

    void Divisibility(auto v) { if (v <= MAX_DIVISIBILITY) m_divisibility = (uint8_t)v; }
    uint8_t Divisibility() const { return m_divisibility.value_or(0); }

    void Limit(auto v) { if (v <= MAX_LIMIT) m_limit.emplace(uint128_t(v)); }
    void Limit(uint128_t v) { if (v <= MAX_LIMIT) m_limit.emplace(move(v)); }
    const std::optional<uint128_t>& Limit() const { return m_limit; }

    void Term(auto v) { if (v <= std::numeric_limits<uint32_t>::max()) m_term.emplace((uint32_t)v); }
    const std::optional<uint32_t>& Term() const { return m_term; }

    void Deadline(auto v) { if (v <= std::numeric_limits<uint32_t>::max()) m_deadline.emplace((uint32_t)v); }
    const std::optional<uint32_t>& Deadline() const { return m_deadline; }

    void DefaultOutput(auto v) { if (v <= std::numeric_limits<uint32_t>::max()) m_default_output.emplace((uint32_t)v); }
    const std::optional<uint32_t>& DefaultOutput() const { return m_default_output; }

    void Symbol(auto v) { if (v <= std::numeric_limits<wchar_t>::max()) m_symbol.emplace((wchar_t)v); }
    const std::optional<wchar_t>& Symbol() const { return m_symbol; }

    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override;
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("rune stone destination cannot have a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("rune stone destination cannot provide a signer"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;

};

enum RunePhase { RUNE_ETCH_SIGNATURE, RUNE_MINT_SIGNATURE };

std::optional<RuneStone> ParseRuneStone(const std::string& hex_tx, ChainMode chain);

}
