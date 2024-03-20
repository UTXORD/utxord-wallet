#include "runes.hpp"

#include <create_inscription.hpp>

#include "transaction.hpp"
#include "contract_error.hpp"

#include <boost/container/flat_map.hpp>

#include <functional>
#include <sstream>

namespace utxord {

template <typename I>
uint128_t read_varint(I& i, I end)
{
    uint128_t value = 0;

    for(; (*i & 0x080) != 0 && i != end; ++i) {
        value = (value * 128) | uint128_t(*i & 0x7f);
    }
    if (i == end) throw std::runtime_error("Wrong varint format");

    value = (value * 128) | uint128_t(*i);

    ++i;
    return value;
}

template <typename INT>
bytevector write_varint(INT n)
{
    bytevector varint;
    varint.reserve(sizeof n);

    // for ( ;n > 0x7F; n = n / 0x80u - 1) {
    //     uint8_t little7bits = static_cast<uint8_t>(n % 0x80u);
    //     varint.push_back(varint.empty() ? little7bits : (little7bits | 0x80u));
    // }
    // varint.push_back(varint.empty() ? static_cast<uint8_t>(n) : (static_cast<uint8_t>(n) | 0x80u));
    // return {varint.rbegin(), varint.rend()};
    for ( ;n > 0x7F; n /= 128) {
        uint8_t little7bits = static_cast<uint8_t>(n % 128);
        varint.push_back(little7bits | 0x80u);
    }
    varint.push_back(static_cast<uint8_t>(n));
    return varint;
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

const char* RuneId::name_chain_height = "chain_height";
const char* RuneId::name_tx_index = "tx_index";

const char* RuneStoneDestination::type = "RuneStone";

const char* RuneStoneDestination::name_rune = "rune";
const char* RuneStoneDestination::name_symbol = "symbol";
const char* RuneStoneDestination::name_spacers = "spacers";
const char* RuneStoneDestination::name_divisibility = "divisibility";
const char* RuneStoneDestination::name_premine_amount = "premine_amount";
const char* RuneStoneDestination::name_mint_cap = "mint_cap";
const char* RuneStoneDestination::name_per_mint_amount = "per_mint_amount";
const char* RuneStoneDestination::name_mint_height_start = "mint_height_start";
const char* RuneStoneDestination::name_mint_height_end = "mint_height_end";
const char* RuneStoneDestination::name_mint_height_offset_start = "mint_height_offset_start";
const char* RuneStoneDestination::name_mint_height_offset_end = "mint_height_offset_end";
const char* RuneStoneDestination::name_default_output = "default_output";
const char* RuneStoneDestination::name_mint_rune_id = "mint_rune_id";
const char* RuneStoneDestination::name_flags = "flags";
const char* RuneStoneDestination::name_op_dictionary = "op_dictionary";


tag_map_t RuneStone::tag_map = {
    {RuneTag::FLAGS, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.action_flags = read_varint(p, end);
    }},
    {RuneTag::RUNE, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.rune = read_varint(p, end);
    }},
    {RuneTag::PREMINE_AMOUNT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.premine_amount = read_varint(p, end);
    }},
    {RuneTag::MINT_CAP, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.mint_cap = read_varint(p, end);
    }},
    {RuneTag::PER_MINT_AMOUNT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        r.per_mint_amount = read_varint(p, end);
    }},
    {RuneTag::MINT_HEIGHT_START, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("mint_height_start");
        r.mint_height_start = static_cast<uint64_t>(v);
    }},
    {RuneTag::MINT_HEIGHT_END, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("mint_height_end");
        r.mint_height_end = static_cast<uint64_t>(v);
    }},
    {RuneTag::MINT_OFFSET_START, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("mint_height_offset_start");
        r.mint_height_offset_start = static_cast<uint64_t>(v);
    }},
    {RuneTag::MINT_OFFSET_END, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("mint_height_offset_end");
        r.mint_height_offset_end = static_cast<uint64_t>(v);
    }},
    {RuneTag::POINTER, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("default_output");
        r.default_output = static_cast<uint32_t>(v);
    }},
    {RuneTag::MINT, [](RuneStone& r, bytevector::const_iterator& p, bytevector::const_iterator end){
        uint128_t v = read_varint(p, end);
        if (r.mint_rune_id) {
            if (v > std::numeric_limits<uint32_t>::max()) throw std::overflow_error("mint_rune_id.tx_index");
            r.mint_rune_id->tx_index = static_cast<uint32_t>(v);
        }
        else {
            if (v > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("mint_rune_id.chain_height");
            r.mint_rune_id.emplace(RuneId{static_cast<uint64_t>(v)});
        }
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

bytevector RuneStoneDestination::Commit() const
{
    if (!rune) throw ContractTermMissing(name_rune);

    bytevector res;
    res.reserve(16);

    for(uint128_t r = *rune; r > 0; r /= 256)
        res.push_back(static_cast<uint8_t>(r % 256));

    return res;
}


void RuneStone::Unpack(const bytevector& data)
{
    auto p = data.begin();
    while(p != data.end() && *p != (uint8_t)RuneTag::BODY) {
        if (p+1 == data.end()) throw ContractFormatError("RuneStone length: " + std::to_string(data.size()));

        RuneTag tag = (RuneTag)*p++;

        auto tag_it = tag_map.find(tag);

        if (tag_it != tag_map.end())
            try {
                tag_it->second(*this, p, data.end());
            }
            catch(...) {
                std::throw_with_nested(ContractFormatError("RuneStone tag " + std::to_string((int)tag)));
            }
        else
            try {
                read_varint(p, data.end()); // just to skip unknown tag value
            }
            catch(...){}
    }

    if (p != data.end()) {

        if (*p != (uint8_t)RuneTag::BODY) throw ContractFormatError("Wrong RuneStone value after tags: " + std::to_string((int) *p));

        RuneId id;
        for (++p; p != data.end();) {
            try {
                uint128_t chain_height = read_varint(p, data.end());
                uint128_t tx_number = read_varint(p, data.end());
                uint128_t amount = read_varint(p, data.end());
                uint128_t output = read_varint(p, data.end());


                id += {static_cast<uint32_t>(chain_height), static_cast<uint32_t>(tx_number)};

                op_dictionary[id] = {amount, static_cast<uint32_t>(output)};
            }
            catch(...) {
                std::throw_with_nested(ContractFormatError("RuneStone dictionary"));
            }
        }
    }
}

bytevector RuneStone::Pack() const
{
    bytevector res;
    bytevector buf;

    if (action_flags != 0) {
        buf = write_varint(action_flags);
        res.push_back((uint8_t)RuneTag::FLAGS);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (rune) {
        buf = write_varint(*rune);
        res.push_back((uint8_t)RuneTag::RUNE);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (spacers.value_or(0)) {
        buf = write_varint(*spacers);
        res.push_back((uint8_t)RuneTag::SPACERS);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (symbol) {
        buf = write_varint(*symbol);
        res.push_back((uint8_t)RuneTag::SYMBOL);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (divisibility.value_or(0)) {
        buf = write_varint(*divisibility);
        res.push_back((uint8_t)RuneTag::DIVISIBILITY);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (premine_amount) {
        buf = write_varint(*premine_amount);
        res.push_back((uint8_t)RuneTag::PREMINE_AMOUNT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_cap) {
        buf = write_varint(*mint_cap);
        res.push_back((uint8_t)RuneTag::MINT_CAP);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (per_mint_amount) {
        buf = write_varint(*per_mint_amount);
        res.push_back((uint8_t)RuneTag::PER_MINT_AMOUNT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_height_start) {
        buf = write_varint(*mint_height_start);
        res.push_back((uint8_t)RuneTag::MINT_HEIGHT_START);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_height_end) {
        buf = write_varint(*mint_height_end);
        res.push_back((uint8_t)RuneTag::MINT_HEIGHT_END);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_height_offset_start) {
        buf = write_varint(*mint_height_offset_start);
        res.push_back((uint8_t)RuneTag::MINT_OFFSET_START);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_height_offset_end) {
        buf = write_varint(*mint_height_offset_end);
        res.push_back((uint8_t)RuneTag::MINT_OFFSET_END);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (mint_rune_id) {
        buf = write_varint(mint_rune_id->chain_height);
        res.push_back((uint8_t)RuneTag::MINT);
        res.insert(res.end(), buf.begin(), buf.end());

        buf = write_varint(mint_rune_id->tx_index);
        res.push_back((uint8_t)RuneTag::MINT);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (default_output) {
        buf = write_varint(*default_output);
        res.push_back((uint8_t)RuneTag::POINTER);
        res.insert(res.end(), buf.begin(), buf.end());
    }

    if (!op_dictionary.empty()) {
        RuneId id;
        res.push_back((uint8_t)RuneTag::BODY);
        for (const auto& entry: op_dictionary) {
            id = entry.first - id;
            buf = write_varint(id.chain_height);
            res.insert(res.end(), buf.begin(), buf.end());

            buf = write_varint(id.tx_index);
            res.insert(res.end(), buf.begin(), buf.end());

            buf = write_varint(get<0>(entry.second));
            res.insert(res.end(), buf.begin(), buf.end());

            buf = write_varint(get<1>(entry.second));
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
    pubKeyScript << OP_13;
    //pubKeyScript << ((m_chain == MAINNET) ? bytevector(RUNE_HEADER, RUNE_HEADER+4) : bytevector(RUNE_TEST_HEADER, RUNE_TEST_HEADER + 9));
    pubKeyScript << Pack();

    return pubKeyScript;
}

UniValue RuneStoneDestination::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_type, type);
    res.pushKV(IContractBuilder::name_amount, m_amount);
    res.pushKV(name_flags, (std::ostringstream() << action_flags).view());

    if (rune) res.pushKV(name_rune, (std::ostringstream() << *rune).view());
    if (symbol) res.pushKV(name_symbol, (uint32_t)*symbol);
    if (spacers) res.pushKV(name_spacers, *spacers);
    if (divisibility) res.pushKV(name_divisibility, *divisibility);
    if (premine_amount) res.pushKV(name_premine_amount, (std::ostringstream() << *premine_amount).view());
    if (mint_cap) res.pushKV(name_mint_cap, (std::ostringstream() << *mint_cap).view());
    if (per_mint_amount) res.pushKV(name_per_mint_amount, (std::ostringstream() << *per_mint_amount).view());
    if (mint_height_start) res.pushKV(name_mint_height_start, *mint_height_start);
    if (mint_height_end) res.pushKV(name_mint_height_end, *mint_height_end);
    if (mint_height_offset_start) res.pushKV(name_mint_height_offset_start, *mint_height_offset_start);
    if (mint_height_offset_end) res.pushKV(name_mint_height_offset_end, *mint_height_offset_end);
    if (mint_rune_id) res.pushKV(name_mint_rune_id, mint_rune_id->MakeJson());
    if (default_output) res.pushKV(name_default_output, *default_output);

    if (!op_dictionary.empty()) {
        UniValue opsVal(UniValue::VARR);
        for (const auto& op: op_dictionary) {
            UniValue opVal = op.first.MakeJson();
            opVal.pushKV(IContractBuilder::name_amount, (std::ostringstream() << get<0>(op.second)).view());
            opVal.pushKV(IContractBuilder::name_nout, get<1>(op.second));

            opsVal.push_back(move(opVal));
        }
        res.pushKV(name_op_dictionary, move(opsVal));
    }

    return res;
}

void RuneStoneDestination::ReadJson(const UniValue &json)
{
    if (!json[name_type].isStr() || json[name_type].get_str() != type) {
        throw ContractTermWrongValue(std::string(name_type));
    }

    m_amount = json[IContractBuilder::name_amount].getInt<CAmount>();

    {   const UniValue& val = json[name_flags];
        if (val.isNull()) throw ContractTermMissing(name_flags);
            uint128_t r;
            std::istringstream(val.getValStr()) >> r;
            if (action_flags) {
                if (action_flags != r) throw ContractTermMismatch(name_flags);
            }
            else action_flags = move(r);

    }
    {   const UniValue& val = json[name_rune];
        if (!val.isNull()) {
            uint128_t r;
            std::istringstream(val.getValStr()) >> r;
            if (rune) {
                if (*rune != r) throw ContractTermMismatch(name_rune);
            }
            else rune = move(r);
        }
    }
    {   const UniValue& val = json[name_symbol];
        if (!val.isNull()) {
            if (symbol) {
                if (*symbol != val.getInt<uint32_t>()) throw ContractTermMismatch(name_symbol);
            }
            else symbol = val.getInt<uint32_t>();
        }
    }
    {   const UniValue& val = json[name_spacers];
        if (!val.isNull()) {
            if (spacers) {
                if (*spacers != val.getInt<uint32_t>()) throw ContractTermMismatch(name_spacers);
            }
            else spacers = val.getInt<uint32_t>();
        }
    }
    {   const UniValue& val = json[name_divisibility];
        if (!val.isNull()) {
            if (divisibility) {
                if (*divisibility != val.getInt<uint8_t>()) throw ContractTermMismatch(name_divisibility);
            }
            else divisibility = val.getInt<uint8_t>();
        }
    }
    {   const UniValue& val = json[name_premine_amount];
        if (!val.isNull()) {
            uint128_t r;
            std::istringstream(val.getValStr()) >> r;
            if (premine_amount) {
                if (*premine_amount != r) throw ContractTermMismatch(name_premine_amount);
            }
            else premine_amount = move(r);
        }
    }
    {   const UniValue& val = json[name_mint_cap];
        if (!val.isNull()) {
            uint128_t r;
            std::istringstream(val.getValStr()) >> r;
            if (mint_cap) {
                if (*mint_cap != r) throw ContractTermMismatch(name_mint_cap);
            }
            else mint_cap = move(r);
        }
    }
    {   const UniValue& val = json[name_per_mint_amount];
        if (!val.isNull()) {
            uint128_t r;
            std::istringstream(val.getValStr()) >> r;
            if (per_mint_amount) {
                if (*per_mint_amount != r) throw ContractTermMismatch(name_per_mint_amount);
            }
            else per_mint_amount = move(r);
        }
    }
    {   const UniValue& val = json[name_mint_height_start];
        if (!val.isNull()) {
            if (mint_height_start) {
                if (*mint_height_start != val.getInt<uint64_t>()) throw ContractTermMismatch(name_mint_height_start);
            }
            else mint_height_start = val.getInt<uint64_t>();
        }
    }
    {   const UniValue& val = json[name_mint_height_end];
        if (!val.isNull()) {
            if (mint_height_end) {
                if (*mint_height_end != val.getInt<uint64_t>()) throw ContractTermMismatch(name_mint_height_end);
            }
            else mint_height_end = val.getInt<uint64_t>();
        }
    }
    {   const UniValue& val = json[name_mint_height_offset_start];
        if (!val.isNull()) {
            if (mint_height_offset_start) {
                if (*mint_height_offset_start != val.getInt<uint64_t>()) throw ContractTermMismatch(name_mint_height_offset_start);
            }
            else mint_height_offset_start = val.getInt<uint64_t>();
        }
    }
    {   const UniValue& val = json[name_mint_height_offset_end];
        if (!val.isNull()) {
            if (mint_height_offset_end) {
                if (*mint_height_offset_end != val.getInt<uint64_t>()) throw ContractTermMismatch(name_mint_height_offset_end);
            }
            else mint_height_offset_end = val.getInt<uint64_t>();
        }
    }
    {   const UniValue& val = json[name_default_output];
        if (!val.isNull()) {
            if (default_output) {
                if (*default_output != val.getInt<uint32_t>()) throw ContractTermMismatch(name_default_output);
            }
            else default_output = val.getInt<uint32_t>();
        }
    }
    {   const UniValue& val = json[name_mint_rune_id];
        if (!val.isNull()) {
            if (mint_rune_id) {
                RuneId id;
                id.ReadJson(val, []{return name_mint_rune_id;});
                if (*mint_rune_id != id) throw ContractTermMismatch(name_mint_rune_id);
            }
            else {
                mint_rune_id.emplace(RuneId());
                mint_rune_id->ReadJson(val, []{return name_mint_rune_id;});
            }
        }
    }
    {   const UniValue& val = json[name_op_dictionary];
        if (!val.isNull()) {
            if (!val.isArray()) throw ContractFormatError(name_op_dictionary);
            if (!op_dictionary.empty() && (op_dictionary.size() != val.size())) throw ContractTermMismatch(name_op_dictionary);
            bool dict_empty = op_dictionary.empty();

            auto dictIt = op_dictionary.begin();
            size_t i = 0;
            for (const UniValue& opVal: val.getValues()) {
                RuneId id;
                id.ReadJson(opVal, [=]{return std::string(name_op_dictionary) + '[' + std::to_string(i) + ']';});

                if (opVal[IContractBuilder::name_amount].isNull()) throw ContractTermMissing(std::string(name_op_dictionary) + '[' + std::to_string(i) + "]." + IContractBuilder::name_amount);
                if (opVal[IContractBuilder::name_nout].isNull()) throw ContractTermMissing(std::string(name_op_dictionary) + '[' + std::to_string(i) + "]." + IContractBuilder::name_nout);

                uint128_t amount;
                std::istringstream(opVal[IContractBuilder::name_amount].getValStr()) >> amount;

                if (dict_empty) {
                    op_dictionary[id] = {move(amount), opVal[IContractBuilder::name_nout].getInt<uint32_t>()};
                }
                else {
                    if (dictIt->first != id) throw ContractTermMismatch(std::string(name_op_dictionary) + '[' + std::to_string(i) + "].rune_id");
                    if (get<0>(dictIt->second) != amount) throw ContractTermMismatch(std::string(name_op_dictionary) + '[' + std::to_string(i) + "]." + IContractBuilder::name_amount);
                    if (get<1>(dictIt->second) != opVal[IContractBuilder::name_nout].getInt<uint32_t>()) throw ContractTermMismatch(std::string(name_op_dictionary) + '[' + std::to_string(i) + "]." + IContractBuilder::name_nout);
                    ++dictIt;
                }

                ++i;
            }
        }
    }
}

std::shared_ptr<IContractDestination> RuneStoneDestination::Construct(ChainMode chain, const UniValue &json)
{
    return std::make_shared<RuneStoneDestination>(chain, json);
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
    if (text_rune.empty()) throw ContractFormatError("cannot encode empty string as Rune");

    int136_t n = -1;

    for (auto ch: text_rune ) {
        if (ch < 'A' || ch > 'Z') throw ContractFormatError("wrong letter to encode as Rune: '" + std::to_string(ch) + "'");

        n = (n + 1) * 26;
        n += ch - 'A';

        if (n > MAX_RUNE) throw ContractFormatError("Rune is too large");
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
            if (p == spaced_text_rune.begin()) throw ContractFormatError("leading space is not allowed for Rune text");

            uint32_t space_flag = 1 << (text_rune.view().length() - 1);
            if (spacers & space_flag) throw ContractFormatError("double space for Rune text at " + std::to_string(text_rune.view().length()));
            spacers |= space_flag;
            lastischar = false;
            p += space.length();
        }
        else if (*p >= 'A' && *p <= 'Z') {
            text_rune.put(*p);
            lastischar = true;
            ++p;
        }
        else throw ContractFormatError("invalid Rune text character: " + std::to_string(*p));
    }

    if (!lastischar) throw ContractFormatError("trailing space is not allowed for Rune text");

    return text_rune.str();
}

Rune::Rune(const std::string& rune_text, const std::string& space, std::optional<wchar_t> symbol)
    : m_spacers {}
    , m_rune(EncodeRune(ExtractSpaces(rune_text, m_spacers, space)))
    , m_symbol(symbol)
{ }

UniValue RuneId::MakeJson() const
{
    UniValue res(UniValue::VOBJ);
    res.pushKV(name_chain_height, chain_height);
    res.pushKV(name_tx_index, tx_index);
    return res;
}

RuneId& RuneId::ReadJson(const UniValue &json, std::function<std::string()> lazy_name)
{
    if (!json.isObject()) throw ContractTermWrongValue(lazy_name());

    UniValue chainHeightVal = json[name_chain_height];
    if (chainHeightVal.isNull()) throw ContractTermMissing(lazy_name() + '.' + name_chain_height);
    if (!chainHeightVal.isNum()) throw ContractTermWrongFormat(lazy_name() + '.' + name_chain_height);

    UniValue txIndexVal = json[name_tx_index];
    if (txIndexVal.isNull()) throw ContractTermMissing(lazy_name() + '.' + name_tx_index);
    if (!txIndexVal.isNum()) throw ContractTermWrongFormat(lazy_name() + '.' + name_tx_index);

    chain_height = chainHeightVal.getInt<uint32_t>();
    tx_index = txIndexVal.getInt<uint32_t>();

    return *this;
}

RuneStone Rune::Etch() const
{
    if (m_rune_id) {
        std::ostringstream err;
        err << "Rune is already etched: " << m_rune_id->chain_height << ':' << m_rune_id->tx_index;
        throw ContractStateError(err.str());
    }

    RuneStone runeStone {
        .spacers = m_spacers,
        .rune = m_rune,
        .symbol = m_symbol,
        .divisibility = m_divisibility,
        .mint_cap = m_mint_cap,
        .per_mint_amount = m_amount_per_mint,
        .mint_height_start = m_mint_height_start,
        .mint_height_end = m_mint_height_end,
        .mint_height_offset_start = m_mint_height_offset_start,
        .mint_height_offset_end = m_mint_height_offset_end

    };

    runeStone.AddAction(RuneAction::ETCH);
    if (m_mint_cap || m_amount_per_mint || m_mint_height_start || m_mint_height_end || m_mint_height_offset_start || m_mint_height_offset_end) {
        runeStone.AddAction(RuneAction::TERMS);
    }

    return runeStone;
}

RuneStone Rune::EtchAndMint(uint128_t amount, uint32_t nout) const
{
    if (m_rune_id) {
        std::ostringstream err;
        err << "Rune is already etched: " << m_rune_id->chain_height << ':' << m_rune_id->tx_index;
        throw ContractStateError(err.str());
    }

    //if (m_amount_per_mint && amount > *m_amount_per_mint) throw std::runtime_error("amount above mint limit");

    RuneStone runeStone {
            .spacers = m_spacers,
            .rune = m_rune,
            .symbol = m_symbol,
            .divisibility = m_divisibility,
            .premine_amount = amount,
            .mint_cap = m_mint_cap,
            .per_mint_amount = m_amount_per_mint,
            .mint_height_start = m_mint_height_start,
            .mint_height_end = m_mint_height_end,
            .mint_height_offset_start = m_mint_height_offset_start,
            .mint_height_offset_end = m_mint_height_offset_end,
            .default_output = nout,

    };

    runeStone.AddAction(RuneAction::ETCH);
    if (m_mint_cap || m_amount_per_mint || m_mint_height_start || m_mint_height_end || m_mint_height_offset_start || m_mint_height_offset_end) {
        runeStone.AddAction(RuneAction::TERMS);
    }

    return runeStone;
}

RuneStone Rune::Mint(uint32_t nout) const
{
    if (!m_rune_id) throw ContractStateError(RuneStoneDestination::name_mint_rune_id);
    if (!m_amount_per_mint) throw ContractStateError(RuneStoneDestination::name_per_mint_amount);
    if (!m_mint_cap) throw ContractStateError(RuneStoneDestination::name_mint_cap);

    RuneStone runeStone;
    runeStone.mint_rune_id = m_rune_id;
    runeStone.default_output = nout;

    return runeStone;
}

void Rune::Read(const RuneStone &runestone)
{
}

}
