#include "runes.hpp"

#include "transaction.hpp"

#include <boost/container/flat_map.hpp>

#include <functional>
#include <sstream>

namespace utxord {


enum class RuneTag: uint8_t
{
    BODY = 0,
    FLAGS = 2,
    RUNE = 4,
    LIMIT = 6,
    TERM = 8,
    DEADLINE = 10,
    DEFAULT_OUTPUT = 12,
    BURN = 254,

    DIVISIBILITY = 1,
    SPACERS = 3,
    SYMBOL = 5,
    NOP = 255,
};

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

typedef boost::container::flat_map<RuneTag, std::function<void(RuneStone&, bytevector::const_iterator&, bytevector::const_iterator)>> tag_map_t;

tag_map_t tag_map {
    {RuneTag::FLAGS, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        if (*p == (uint8_t)RuneAction::ETCH || *p == (uint8_t)RuneAction::MINT || *p == (uint8_t)RuneAction::BURN)
            r.Action((RuneAction)*p++);
        else
            read_varint(p, end); // just to skip appropriate bytes
    }},
    {RuneTag::RUNE, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Rune(read_varint(p, end));
    }},
    {RuneTag::LIMIT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Limit(read_varint(p, end));
    }},
    {RuneTag::TERM, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Term(read_varint(p, end));
    }},
    {RuneTag::DEADLINE, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Deadline(read_varint(p, end));
    }},
    {RuneTag::DEFAULT_OUTPUT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.DefaultOutput(read_varint(p, end));
    }},
    {RuneTag::DIVISIBILITY, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Divisibility(read_varint(p, end));
    }},
    {RuneTag::SPACERS, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Spacers(read_varint(p, end));
    }},
    {RuneTag::SYMBOL, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.Symbol(read_varint(p, end));
    }},
};

}

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

        for (++p; p != data.end();) {
            uint128_t readid = read_varint(p, data.end());
            uint128_t amount = read_varint(p, data.end());
            uint128_t output = read_varint(p, data.end());

            uint128_t id = readid + 0x080;
            if (id < readid) id = std::numeric_limits<uint128_t>::max();

            assert(output <= std::numeric_limits<uint32_t>::max());

            m_dictionary.emplace_back(id, amount, output);
        }
    }
}

bytevector RuneStone::Pack() const
{
    bytevector res;
    bytevector buf;

    res.push_back((uint8_t)RuneTag::FLAGS);
    res.push_back((uint8_t)m_action);

    buf = write_varint(m_rune);
    res.push_back((uint8_t)RuneTag::RUNE);
    res.insert(res.end(), buf.begin(), buf.end());

    if (m_spacers.value_or(0)) {
        buf = write_varint(*m_spacers);
        res.push_back((uint8_t)RuneTag::SPACERS);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_divisibility.value_or(0)) {
        buf = write_varint(*m_divisibility);
        res.push_back((uint8_t)RuneTag::DIVISIBILITY);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_symbol) {
        buf = write_varint(*m_symbol);
        res.push_back((uint8_t)RuneTag::SYMBOL);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_deadline) {
        buf = write_varint(*m_deadline);
        res.push_back((uint8_t)RuneTag::DEADLINE);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_limit) {
        buf = write_varint(*m_limit);
        res.push_back((uint8_t)RuneTag::LIMIT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_term) {
        buf = write_varint(*m_term);
        res.push_back((uint8_t)RuneTag::TERM);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (m_default_output) {
        buf = write_varint(*m_default_output);
        res.push_back((uint8_t)RuneTag::DEFAULT_OUTPUT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (!m_dictionary.empty()) {
        res.push_back((uint8_t)RuneTag::BODY);
        for (const auto& entry: m_dictionary) {

        }
    }

    return res;
}

std::string RuneStone::RuneText() const
{
    std::string rune_text = DecodeRune(m_rune);
    if (m_spacers) {
        rune_text = AddSpaces(rune_text, *m_spacers);
    }
    return rune_text;
}

CScript RuneStone::PubKeyScript() const
{
    CScript pubKeyScript {OP_RETURN};

    pubKeyScript << ((m_chain == MAINNET) ? bytevector(RUNE_HEADER, RUNE_HEADER+4) : bytevector(RUNE_TEST_HEADER, RUNE_TEST_HEADER + 9));
    pubKeyScript << Pack();

    return pubKeyScript;
}

UniValue RuneStone::MakeJson() const
{
    return {};
}

void RuneStone::ReadJson(const UniValue &json)
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
                    res.emplace(chain, data);
                    break;
                }
            }
            else if (chain == MAINNET && op == 4 && std::equal(data.begin(), data.end(), RUNE_HEADER)) {
                if (out.scriptPubKey.GetOp(it, op, data)) {
                    res.emplace(chain, data);
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

}
