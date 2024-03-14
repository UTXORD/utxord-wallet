#include "runes.hpp"

#include "transaction.hpp"

#include <boost/container/flat_map.hpp>

#include <functional>
#include <sstream>

namespace utxord {

template <typename I>
uint128_t read_varint(I& i, I end)
{
    uint128_t value = 0;
    unsigned factor = 128;

    for(; (*i & 0x080) != 0 && i != end; ++i) {
        value = value * factor + uint128_t(*i & 0x7f) + 1;
    }
    if (i == end) throw std::runtime_error("Wrong varint format");

    value = value * factor + *i;

    ++i;
    return value;
}

template <typename INT>
bytevector write_varint(INT n)
{
    bytevector varint;
    varint.reserve(sizeof n);

    for ( ;n > 0x7F; n = n / 0x80u - 1) {
        uint8_t little7bits = static_cast<uint8_t>(n % 0x80u);
        varint.push_back(varint.empty() ? little7bits : (little7bits | 0x80u));
    }
    varint.push_back(varint.empty() ? static_cast<uint8_t>(n) : (static_cast<uint8_t>(n) | 0x80u));
    return {varint.rbegin(), varint.rend()};
}


template uint128_t read_varint(bytevector::const_iterator&, bytevector::const_iterator);
template bytevector write_varint(uint128_t n);
template bytevector write_varint(uint64_t n);
template bytevector write_varint(uint32_t n);
template bytevector write_varint(uint8_t n);

namespace {

using namespace boost::multiprecision;

#ifndef DEBUG
using int136_t = number<cpp_int_backend<128, 136, signed_magnitude, unchecked, void> >;
#else
using int136_t = number<debug_adaptor<cpp_int_backend<128, 136, signed_magnitude, unchecked, void>>>;
#endif

const int136_t MAX_RUNE = static_cast<int136_t>(std::numeric_limits<uint128_t>::max());

const char *RUNE_TEST_HEADER = "RUNE_TEST";
const char *RUNE_HEADER = "RUNE";

}

tag_map_t RuneStone::tag_map = {
    {RuneTag::FLAGS, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.action_flags = read_varint(p, end);
    }},
    {RuneTag::RUNE, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.rune = read_varint(p, end);
    }},
    {RuneTag::LIMIT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.limit = read_varint(p, end);
    }},
    {RuneTag::TERM, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("term");
        r.term = static_cast<uint32_t>(v);
    }},
    {RuneTag::DEADLINE, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("deadline");
        r.deadline = static_cast<uint32_t>(v);
    }},
    {RuneTag::DEFAULT_OUTPUT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("default_output");
        r.default_output = static_cast<uint32_t>(v);
    }},
    {RuneTag::CLAIM, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("claim_id");
        r.claim_id = static_cast<uint64_t>(v);
    }},
    {RuneTag::DIVISIBILITY, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint8_t>::max()) throw std::overflow_error("divisibility");
        r.divisibility= static_cast<uint8_t>(v);
    }},
    {RuneTag::SPACERS, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("spacers");
        r.spacers = static_cast<uint32_t>(v);
    }},
    {RuneTag::SYMBOL, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<wchar_t>::max()) throw std::overflow_error("symbol");
        r.symbol = static_cast<wchar_t>(v);
    }},
};


void RuneStone::Unpack(const bytevector& data)
{
    auto p = data.begin();
    while(p != data.end() && *p != (uint8_t)RuneTag::BODY) {
        if (p+1 == data.end()) throw std::runtime_error("Wrong rune stone length: " + std::to_string(data.size()));

        RuneTag tag = (RuneTag)*p++;

        auto tag_it = tag_map.find(tag);

        if (tag_it != tag_map.end())
            tag_it->second(*this, p, data.end());
        else
            read_varint(p, data.end()); // just to skip unknown tag value
    }

    if (p != data.end()) {

        if (*p != (uint8_t)RuneTag::BODY) throw std::runtime_error("Wrong rune stone value after tags: " + std::to_string((int) *p));

        uint128_t id;
        for (++p; p != data.end();) {
            uint128_t readid = read_varint(p, data.end());
            uint128_t amount = read_varint(p, data.end());
            uint128_t output = read_varint(p, data.end());

            id += readid;
            //if (id > std::numeric_limits<uint128_t>)

            dictionary.emplace_back(id, amount, output);
        }
    }
}

bytevector RuneStone::Pack() const
{
    bytevector res;
    bytevector buf;

    if (rune) {
        buf = write_varint(*rune);
        res.push_back((uint8_t)RuneTag::RUNE);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (action_flags != 0) {
        buf = write_varint(action_flags);
        res.push_back((uint8_t)RuneTag::FLAGS);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (spacers.value_or(0)) {
        buf = write_varint(*spacers);
        res.push_back((uint8_t)RuneTag::SPACERS);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (divisibility.value_or(0)) {
        buf = write_varint(*divisibility);
        res.push_back((uint8_t)RuneTag::DIVISIBILITY);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (symbol) {
        buf = write_varint(*symbol);
        res.push_back((uint8_t)RuneTag::SYMBOL);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (deadline) {
        buf = write_varint(*deadline);
        res.push_back((uint8_t)RuneTag::DEADLINE);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (limit) {
        buf = write_varint(*limit);
        res.push_back((uint8_t)RuneTag::LIMIT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (term) {
        buf = write_varint(*term);
        res.push_back((uint8_t)RuneTag::TERM);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (default_output) {
        buf = write_varint(*default_output);
        res.push_back((uint8_t)RuneTag::DEFAULT_OUTPUT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (!dictionary.empty()) {
        uint128_t add_id = 0;
        res.push_back((uint8_t)RuneTag::BODY);
        for (const auto& entry: dictionary) {
            add_id = get<0>(entry) - add_id;
            buf = write_varint(add_id);
            res.insert(res.end(), buf.begin(), buf.end());

            buf = write_varint(get<1>(entry));
            res.insert(res.end(), buf.begin(), buf.end());

            buf = write_varint(get<2>(entry));
            res.insert(res.end(), buf.begin(), buf.end());
        }
    }

    return res;
}

std::string Rune::RuneText(const std::string space) const
{
    std::string rune_text = DecodeRune(m_rune);
    if (m_spacers) {
        rune_text = AddSpaces(rune_text, m_spacers, space);
    }
    return rune_text;
}

CScript RuneStoneDestination::PubKeyScript() const
{
    CScript pubKeyScript {OP_RETURN};

    pubKeyScript << ((m_chain == MAINNET) ? bytevector(RUNE_HEADER, RUNE_HEADER+4) : bytevector(RUNE_TEST_HEADER, RUNE_TEST_HEADER + 9));
    pubKeyScript << Pack();

    return pubKeyScript;
}

UniValue RuneStoneDestination::MakeJson() const
{
    return {};
}

void RuneStoneDestination::ReadJson(const UniValue &json)
{
}

std::optional<RuneStone> ParseRuneStone(const string &hex_tx, ChainMode chain)
{
    std::optional<RuneStone> res;
    auto tx = l15::core::Deserialize(hex_tx);

    for (const auto& out: tx.vout) {
        if (out.scriptPubKey.front() != OP_RETURN) continue;

        auto it = out.scriptPubKey.begin() + 1;

        opcodetype op;
        bytevector data;

        if (out.scriptPubKey.GetOp(it, op, data)) {
            if (chain != MAINNET && op == 9 && std::equal(data.begin(), data.end(), RUNE_TEST_HEADER)) {
                if (out.scriptPubKey.GetOp(it, op, data)) {
                    res.emplace();
                    res->Unpack(data);
                    break;
                }
            }
            else if (chain == MAINNET && op == 4 && std::equal(data.begin(), data.end(), RUNE_HEADER)) {
                if (out.scriptPubKey.GetOp(it, op, data)) {
                    res.emplace();
                    res->Unpack(data);
                    break;
                }
            }
        }
    }
    return res;
}

uint128_t EncodeRune(const std::string &text_rune)
{
    if (text_rune.empty()) throw std::runtime_error("cannot emcode empty string");

    int136_t n = -1;

    for (auto ch: text_rune ) {
        if (ch < 'A' || ch > 'Z') throw std::runtime_error("wrong letter to encode in rune: '" + std::to_string(ch) + "'");

        n = (n + 1) * 26;
        n += ch - 'A';

        if (n > MAX_RUNE) throw std::runtime_error("rune too large");
    }

    return uint128_t(n);
}

std::string DecodeRune(uint128_t n)
{
    std::ostringstream buf;
    for ( ; n > 25; n = n / 26 - 1) {
        uint128_t idx = n % 26;
        buf.put((char) ('A' + idx));
    }
    buf.put((char)('A' + n));
    return std::string(buf.view().rbegin(), buf.view().rend());
}

std::string AddSpaces(const std::string& text_rune, uint32_t spacers, const std::string& space)
{
    std::ostringstream buf;
    for (size_t i = 0; i < text_rune.length(); ++i) {
        buf.put(text_rune[i]);
        if (spacers & (1 << i))
            buf << space;
    }
    return buf.str();
}

std::string ExtractSpaces(const std::string& spaced_text_rune, uint32_t &spacers, const string &space)
{
    std::ostringstream text_rune;
    spacers = 0;
    bool lastischar = false;

    for (auto p = spaced_text_rune.begin(); p != spaced_text_rune.end(); ) {
        std::string_view pos_view {p, spaced_text_rune.end()};
        if (pos_view.starts_with(space)) {
            if (p == spaced_text_rune.begin()) throw std::runtime_error("leading space is not allowed");

            uint32_t space_flag = 1 << (text_rune.view().length() - 1);
            if (spacers & space_flag) throw std::runtime_error("double space at " + std::to_string(text_rune.view().length()));
            spacers |= space_flag;
            lastischar = false;
            p += space.length();
        }
        else if (*p >= 'A' && *p <= 'Z') {
            text_rune.put(*p);
            lastischar = true;
            ++p;
        }
        else throw std::runtime_error("invalid rune character");
    }

    if (!lastischar) throw std::runtime_error("trailing space is not allowed");

    return text_rune.str();
}

Rune::Rune(const std::string& rune_text, const std::string& space, std::optional<wchar_t> symbol)
    : m_spacers {}
    , m_rune(EncodeRune(ExtractSpaces(rune_text, m_spacers, space)))
    , m_symbol(symbol)
{ }

RuneStone Rune::Etch() const
{
    if (m_rune_id) throw std::runtime_error("already etched");

    RuneStone runeStone {
        .spacers = m_spacers,
        .rune = m_rune,
        .symbol = m_symbol,
        .divisibility = m_divisibility,
        .limit = m_limit_per_mint,
        .term = m_term,
        .deadline = m_deadline
    };

    runeStone.AddAction(RuneAction::ETCH);
    if (m_limit_per_mint || m_term || m_deadline) {
        runeStone.AddAction(RuneAction::MINT);
    }

    return runeStone;
}

RuneStone Rune::EtchAndMint(uint128_t amount, uint32_t nout) const
{
    if (m_rune_id) throw std::runtime_error("already etched");

    if ((m_limit_per_mint && amount == *m_limit_per_mint) || (!m_limit_per_mint && amount == std::numeric_limits<uint128_t>::max()))
        amount = 0;

    if (m_limit_per_mint && amount > *m_limit_per_mint) throw std::runtime_error("amount above mint limit");

    RuneStone runeStone {
        .spacers = m_spacers,
        .rune= m_rune,
        .symbol = m_symbol,
        .divisibility = m_divisibility,
        .limit = m_limit_per_mint,
        .term = m_term,
        .deadline = m_deadline
    };
    runeStone.AddAction(RuneAction::ETCH);
    if (m_limit_per_mint || m_term || m_deadline) {
        runeStone.AddAction(RuneAction::MINT);
    }

    runeStone.dictionary.emplace_back(0, amount, nout);

    return runeStone;
}

RuneStone Rune::Mint(uint128_t amount, uint32_t nout) const
{
    RuneStone runeStone;
    runeStone.claim_id = (((uint64_t)get<0>(*m_rune_id)) << 16) | (((uint64_t)get<1>(*m_rune_id)) & 0x0FFFFull);

    runeStone.dictionary.emplace_back(0, amount, nout);

    return runeStone;
}

void Rune::Read(const RuneStone &runestone)
{
}

}
