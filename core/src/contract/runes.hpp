#pragma once

#include "contract_builder.hpp"
//#include "simple_transaction.hpp"

#include <optional>
#include <tuple>

namespace utxord {

const uint8_t MAX_DIVISIBILITY = 38;
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
    PREMINE_AMOUNT = 6,
    MINT_CAP = 8,
    PER_MINT_AMOUNT = 10,
    MINT_HEIGHT_START = 12,
    MINT_HEIGHT_END = 14,
    MINT_OFFSET_START = 16,
    MINT_OFFSET_END = 18,
    MINT = 20,
    POINTER = 22, //DEFAULT_OUTPUT
    CENOTAPH = 126,

    DIVISIBILITY = 1,
    SPACERS = 3,
    SYMBOL = 5,
    NOP = 127,
};

typedef boost::container::flat_map<RuneTag, std::function<void(struct RuneStone&, bytevector::const_iterator&, bytevector::const_iterator)>> tag_map_t;

enum class RuneAction: uint8_t
{
    ETCH = 0,
    TERMS = 1,
    BURN = 127
};

struct RuneId: IJsonSerializable
{
    static const char* name_chain_height;
    static const char* name_tx_index;

    uint64_t chain_height = 0;
    uint32_t tx_index = 0;

    RuneId() = default;
    RuneId(const uint64_t& h, uint32_t tx) : chain_height(h), tx_index(tx) {}
    RuneId(const RuneId& ) = default;
    RuneId(RuneId&&) noexcept = default;
    RuneId& operator=(const RuneId&) = default;
    RuneId& operator=(RuneId&&) = default;
    bool operator<(const RuneId& r) const
    { return (chain_height == r.chain_height) ? (tx_index < r.tx_index) : (chain_height < r.chain_height); }
    bool operator==(const RuneId& r) const
    { return (chain_height == r.chain_height) && (tx_index < r.tx_index); }
    RuneId operator-(const RuneId& r) const
    { return (chain_height == r.chain_height) ? RuneId{0 , (tx_index - r.tx_index)} : RuneId{(chain_height - r.chain_height), tx_index}; }
    RuneId operator+(const RuneId& r) const
    { return (r.chain_height == 0) ? RuneId{chain_height, (tx_index + r.tx_index)} : RuneId{(chain_height + r.chain_height), tx_index}; }
    RuneId& operator+=(const RuneId& r)
    {
        if (r.chain_height == 0)
            tx_index += r.tx_index;
        else {
            chain_height += r.chain_height;
            tx_index = r.tx_index;
        }
        return *this;
    }
    operator std::string () const;

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()>& lazy_name) override;
};


struct RuneStone
{
    uint128_t action_flags;

    std::optional<uint32_t> spacers;
    std::optional<uint128_t> rune;
    std::optional<wchar_t> symbol; // unicode index
    std::optional<uint8_t> divisibility;
    std::optional<uint128_t> premine_amount;

    // Mint Terms
    std::optional<uint128_t> mint_cap;
    std::optional<uint128_t> per_mint_amount;
    std::optional<uint64_t> mint_height_start;
    std::optional<uint64_t> mint_height_end;
    std::optional<uint64_t> mint_height_offset_start;
    std::optional<uint64_t> mint_height_offset_end;

    std::optional<RuneId> mint_rune_id;

    std::optional<uint32_t> default_output;

    std::multimap<RuneId, std::tuple<uint128_t, uint32_t>> op_dictionary; // rune_id -> {rune_amount, nout}

    void AddAction(RuneAction action) { action_flags |= (uint128_t(1) << (uint8_t)action); }

    bytevector Pack() const;
    void Unpack(const bytevector& data);

    static tag_map_t tag_map;
};

class RuneStoneDestination: public IContractDestination, public RuneStone
{
public:
    static const char* type;

    static const char* name_rune;
    static const char* name_symbol;
    static const char* name_spacers;
    static const char* name_divisibility;
    static const char* name_premine_amount;
    static const char* name_mint_cap;
    static const char* name_per_mint_amount;
    static const char* name_mint_height_start;
    static const char* name_mint_height_end;
    static const char* name_mint_height_offset_start;
    static const char* name_mint_height_offset_end;

    static const char* name_mint_rune_id;
    static const char* name_default_output;
    static const char* name_flags;
    static const char* name_op_dictionary;

private:
    ChainMode m_chain;
    CAmount m_amount = 0;

public:
    explicit RuneStoneDestination(ChainMode chain) : RuneStone(), m_chain(chain) {}
    RuneStoneDestination(ChainMode chain, RuneStone runeStone) : RuneStone(move(runeStone)), m_chain(chain) {}
    RuneStoneDestination(const RuneStoneDestination& ) = default;
    RuneStoneDestination(RuneStoneDestination&& ) noexcept = default;

    RuneStoneDestination& operator= (const RuneStoneDestination& ) = default;
    RuneStoneDestination& operator= (RuneStoneDestination&& ) noexcept = default;

    explicit RuneStoneDestination(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name) : m_chain(chain)
    { RuneStoneDestination::ReadJson(json, lazy_name); }

    bytevector Commit() const;

    const char* Type() const override
    { return type; }

    void Amount(CAmount amount) override { m_amount = amount; }
    CAmount Amount() const final { return m_amount; }

    std::string Address() const override { return {}; }
    CScript PubKeyScript() const override;
    CScript DummyScriptSig() const override { return {}; }
    std::vector<bytevector> DummyWitness() const override { throw std::domain_error("rune stone destination cannot have a witness"); }
    std::shared_ptr<ISigner> LookupKey(const KeyRegistry& masterKey, const std::string& key_filter_tag) const override
    { throw std::domain_error("rune stone destination cannot provide a signer"); }

    UniValue MakeJson() const override;
    void ReadJson(const UniValue& json, const std::function<std::string()> &lazy_name) override;

    static UniValue MakeOpDictionaryJson(const std::multimap<RuneId, std::tuple<uint128_t, uint32_t>>&);
    static void ReadOpDictionaryJson(const UniValue& json, std::multimap<RuneId, std::tuple<uint128_t, uint32_t>>& op_dictinary, const std::function<std::string()> &lazy_name);

    static std::shared_ptr<IContractDestination> Construct(ChainMode chain, const UniValue& json, const std::function<std::string()>& lazy_name);

};

class Rune
{
    uint32_t m_spacers;
    uint128_t m_rune;
    std::optional<wchar_t> m_symbol; // unicode index
    uint8_t m_divisibility;

    std::optional<utxord::RuneId> m_rune_id; // block height of etching tx and rune stone index in the block

    //Mint terms
    std::optional<uint128_t> m_mint_cap;
    std::optional<uint128_t> m_amount_per_mint;
    std::optional<uint64_t> m_mint_height_start;
    std::optional<uint64_t> m_mint_height_end;
    std::optional<uint64_t> m_mint_height_offset_start;
    std::optional<uint64_t> m_mint_height_offset_end;
public:
    explicit Rune(const std::string &rune_text, const std::string &space, std::optional<wchar_t> symbol = {});

    std::string RuneText(const std::string& space = " ") const;

    void Divisibility(auto v) { if (v <= MAX_DIVISIBILITY) m_divisibility = (uint8_t)v; }
    uint8_t Divisibility() const { return m_divisibility; }

    void RuneId(uint64_t chain_h, uint32_t tx_index)
    { m_rune_id.emplace(chain_h, tx_index); }

    const auto& RuneId() const
    { return m_rune_id; }

    const std::optional<uint128_t>& MintCap() const { return m_mint_cap; }
    std::optional<uint128_t>& MintCap() { return m_mint_cap; }

    const std::optional<uint128_t>& AmountPerMint() const { return m_amount_per_mint; }
    std::optional<uint128_t>& AmountPerMint() { return m_amount_per_mint; }

    const std::optional<uint64_t>& MintHeightStart() const { return m_mint_height_start; }
    std::optional<uint64_t>& MintHeightStart() { return m_mint_height_start; }

    const std::optional<uint64_t>& MintHeightEnd() const { return m_mint_height_end; }
    std::optional<uint64_t>& MintHeightEnd() { return m_mint_height_end; }

    const std::optional<uint64_t>& MintHeightOffsetStart() const { return m_mint_height_offset_start; }
    std::optional<uint64_t>& MintHeightOffsetStart() { return m_mint_height_offset_start; }

    const std::optional<uint64_t>& MintHeightOffsetEnd() const { return m_mint_height_offset_end; }
    std::optional<uint64_t>& MintHeightOffsetEnd() { return m_mint_height_offset_end; }

    RuneStone Etch() const;
    RuneStone EtchAndMint(uint128_t amount, uint32_t nout) const;
    RuneStone Mint(uint32_t nout) const;

    void Read(const RuneStone& runestone);
};


std::optional<RuneStone> ParseRuneStone(const std::string& hex_tx, ChainMode chain);

}
