
#include <list>
#include <ranges>
#include <sstream>

#include "transaction.h"
#include "streams.h"
#include "version.h"


#include "inscription.hpp"
#include "inscription_common.hpp"
#include "transaction.hpp"


namespace utxord {

namespace {

opcodetype GetNextScriptData(const CScript &script, CScript::const_iterator &it, bytevector &data, const std::string errtag)
{
    opcodetype opcode;
    if (it < script.end()) {
        if (!script.GetOp(it, opcode, data))
            throw InscriptionFormatError(std::string(errtag));
    }
    else {
        throw InscriptionFormatError(std::string(errtag));
    }

    return opcode;
}

}


Inscription::Inscription(std::string inscription_id, std::list<std::pair<bytevector, bytevector>>&& inscr_data)
    : m_inscription_id(move(inscription_id))
{
    bytevector metadata;

    while(!inscr_data.empty()) {
        if (inscr_data.front().first == CONTENT_TYPE_TAG) {
            //if (!m_content_type.empty()) throw InscriptionFormatError("second CONTENT_TYPE tag");
            m_content_type.assign(inscr_data.front().second.begin(), inscr_data.front().second.end());
        }
        else if (inscr_data.front().first == CONTENT_TAG) {
            //if (!m_content.empty()) throw InscriptionFormatError("second CONTENT tag");
            m_content = move(inscr_data.front().second);
        }
        else if (inscr_data.front().first == COLLECTION_ID_TAG) {
            //if (!m_collection_id.empty()) throw InscriptionFormatError("second COLLECTION_ID tag");
            m_collection_id = DeserializeInscriptionId(inscr_data.front().second);
        }
        else if (inscr_data.front().first == ORD_SHIFT_TAG) {
            m_ord_shift = CScriptNum(inscr_data.front().second, false, sizeof(CAmount)).GetInt64();
            //if (!MoneyRange(ord_shift)) throw InscriptionFormatError("Ord shift is greater than whole Bitcoin supply")
        }
        else if (inscr_data.front().first == METADATA_TAG) {
            metadata.insert(metadata.end(), inscr_data.front().second.begin(), inscr_data.front().second.end());
        }

        inscr_data.pop_front();
    }

    m_metadata = move(metadata);
}


std::list<std::pair<bytevector, bytevector>> ParseEnvelopeScript(const CScript& script, CScript::const_iterator& it) {
    std::list<std::pair<bytevector, bytevector>> res;
    bytevector content;

    size_t i = 0;

    CScript::const_iterator end = script.end();
    opcodetype prev_opcode_2 = OP_INVALIDOPCODE;
    opcodetype prev_opcode = OP_INVALIDOPCODE;
    opcodetype opcode = OP_INVALIDOPCODE;
    bytevector data;
    bool has_ord_envelope = false;

    while (it < end && !has_ord_envelope) {
        prev_opcode_2 = prev_opcode;
        prev_opcode = opcode;

        if (!script.GetOp(it, opcode, data))
            throw TransactionError("wrong script");

        has_ord_envelope = (prev_opcode_2 == OP_0 &&
            prev_opcode == OP_IF &&
            opcode == ORD_TAG.size() &&
            data == ORD_TAG);
    }

    bool fetching_content = false;

    while (it < end) {
        opcode = GetNextScriptData(script, it, data, "inscription envelope");

        if (opcode == CONTENT_OP_TAG) {
            if (!fetching_content && !content.empty()) content.clear(); //throw InscriptionFormatError("second CONTENT tag");
            fetching_content = true;
        }
        else if (opcode == CONTENT_TYPE_OP_TAG || (opcode == CONTENT_TYPE_TAG.size() && data == CONTENT_TYPE_TAG)) {
            GetNextScriptData(script, it, data, "content type");
            res.emplace_back(CONTENT_TYPE_TAG, move(data));
            fetching_content = false;
        }
        else if (opcode == ORD_SHIFT_OP_TAG || (opcode == ORD_SHIFT_TAG.size() && data == ORD_SHIFT_TAG)) {
            GetNextScriptData(script, it, data, "ord shift");
            res.emplace_back(ORD_SHIFT_TAG, move(data));
            fetching_content = false;
        }
        else if (opcode == COLLECTION_ID_OP_TAG || (opcode == COLLECTION_ID_TAG.size() && data == COLLECTION_ID_TAG)) {
            GetNextScriptData(script, it, data, "collection id");
            res.emplace_back(COLLECTION_ID_TAG, move(data));
            fetching_content = false;
        }
        else if (opcode == METADATA_OP_TAG || (opcode == METADATA_TAG.size() && data == METADATA_TAG)) {
            GetNextScriptData(script, it, data, "meta-data");
            res.emplace_back(METADATA_TAG, move(data));
            fetching_content = false;
        }
        else if (opcode == OP_ENDIF) {
            if (!content.empty()) {
                res.emplace_back(CONTENT_TAG, move(content));
            }
            break;
        }
        else if (fetching_content) {
            content.insert(content.end(), data.begin(), data.end());
        }
        else {
            bytevector tag = data.empty() ? bytevector{(uint8_t)opcode} : move(data);
            GetNextScriptData(script, it, data, "unknown tag");
            res.emplace_back(move(tag), move(data));
            fetching_content = false;
        }
    }
    return res;
}


std::list<Inscription> ParseInscriptions(const string &hex_tx)
{
    std::list<Inscription> res;
    auto tx = core::Deserialize(hex_tx);

    for (const auto& in: tx.vin) {
        if (in.scriptWitness.stack.size() < 3) continue;

        const auto& witness_stack = in.scriptWitness.stack;
        CScript script(witness_stack[witness_stack.size() - 2].begin(), witness_stack[witness_stack.size() - 2].end());

        CScript::const_iterator it = script.begin();

        while (it != script.end()) {
            auto envelope_tags = ParseEnvelopeScript(script, it);
            if (!envelope_tags.empty()) {
                res.emplace_back(move((tx.GetHash().GetHex() + "i") += std::to_string(res.size())), move(envelope_tags));
            }
        }
    }

    return res;
}

} // utxord
