#pragma once

#include "contract_builder.hpp"
#include "simple_transaction.hpp"

#include <optional>
#include <tuple>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
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

enum class RuneTag: uint8_t
{
    BODY = 0,
    FLAGS = 2,
    RUNE = 4,
    LIMIT = 6,
    TERM = 8,
    DEADLINE = 10,
    DEFAULT_OUTPUT = 12,
    CLAIM = 14,
    BURN = 126,

    DIVISIBILITY = 1,
    SPACERS = 3,
    SYMBOL = 5,
    NOP = 127,
};

typedef boost::container::flat_map<RuneTag, std::function<void(struct RuneStone&, bytevector::const_iterator&, bytevector::const_iterator)>> tag_map_t;

enum class RuneAction: uint8_t
{
    ETCH = 0,
    MINT = 1,
    BURN = 127
};

struct RuneStone
{
    std::optional<uint32_t> spacers;
    std::optional<uint128_t> rune;
    std::optional<wchar_t> symbol; // unicode index
    std::optional<uint8_t> divisibility;
    std::optional<uint128_t> limit;
    std::optional<uint32_t> term;
    std::optional<uint32_t> deadline;
    std::optional<uint32_t> default_output;
    std::optional<uint64_t> claim_id;

    uint128_t action_flags;

    std::list<std::tuple<uint128_t, uint128_t, uint32_t>> dictionary; // id, rune amount, nout


    void AddAction(RuneAction action) { action_flags |= (uint128_t(1) << (uint8_t)action); }

    bytevector Pack() const;
    void Unpack(const bytevector& data);

    static tag_map_t tag_map;
};

class RuneStoneDestination: public IContractDestination, public RuneStone
{
    ChainMode m_chain;
    CAmount m_amount = 0;

public:
    RuneStoneDestination(ChainMode chain, RuneStone runeStone) : RuneStone(move(runeStone)), m_chain(chain) {}
    RuneStoneDestination(const RuneStoneDestination& ) = default;
    RuneStoneDestination(RuneStoneDestination&& ) noexcept = default;

    RuneStoneDestination& operator= (const RuneStoneDestination& ) = default;
    RuneStoneDestination& operator= (RuneStoneDestination&& ) noexcept = default;

    explicit RuneStoneDestination(ChainMode chain, const UniValue& json) : m_chain(chain)
    { ReadJson(json); }

    void Amount(CAmount amount) override { m_amount = amount; }
    CAmount Amount() const final { return m_amount; }

    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override;
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("rune stone destination cannot have a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("rune stone destination cannot provide a signer"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json) override;

};

class Rune
{
    uint32_t m_spacers;
    uint128_t m_rune;
    std::optional<wchar_t> m_symbol; // unicode index
    uint8_t m_divisibility;

    std::optional<std::tuple<uint32_t, uint32_t>> m_rune_id; // block height of etching tx and rune stone index in the block

    //Mint options
    std::optional<uint32_t> m_deadline; // (??) timestamp of minting deadline
    std::optional<uint32_t> m_term; // (??) count of blocks since etching block while minting is open
    std::optional<uint128_t> m_limit_per_mint; // limit per mint
public:
    explicit Rune(const std::string &rune_text, const std::string &space, std::optional<wchar_t> symbol = {});

    std::string RuneText(const std::string space = " ") const;

    void Divisibility(auto v) { if (v <= MAX_DIVISIBILITY) m_divisibility = (uint8_t)v; }
    uint8_t Divisibility() const { return m_divisibility; }

    void LimitPerMint(auto v) { if (v <= MAX_LIMIT) m_limit_per_mint.emplace(uint128_t(v)); }
    void LimitPerMint(uint128_t v) { if (v <= MAX_LIMIT) m_limit_per_mint.emplace(move(v)); }
    const std::optional<uint128_t>& LimitPerMint() const { return m_limit_per_mint; }

    void Term(auto v) { if (v <= std::numeric_limits<uint32_t>::max()) m_term.emplace((uint32_t)v); }
    const std::optional<uint32_t>& Term() const { return m_term; }

    void Deadline(auto v) { if (v <= std::numeric_limits<uint32_t>::max()) m_deadline.emplace((uint32_t)v); }
    const std::optional<uint32_t>& Deadline() const { return m_deadline; }


    RuneStone Etch() const;

    RuneStone EtchAndMint(uint128_t amount, uint32_t nout) const;

    RuneStone Mint(uint128_t amount, uint32_t nout) const;

    void Read(const RuneStone& runestone);
};


std::optional<RuneStone> ParseRuneStone(const std::string& hex_tx, ChainMode chain);

}
