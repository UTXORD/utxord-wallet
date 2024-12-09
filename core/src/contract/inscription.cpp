
#include <list>
#include <ranges>
#include <sstream>

#include "transaction.h"
#include "streams.h"


#include "inscription.hpp"
#include "inscription_common.hpp"
#include "transaction.hpp"


namespace utxord {

namespace {

opcodetype GetNextScriptData(const CScript &script, CScript::const_iterator &it, bytevector &data, const std::string& errtag, bool force_push = false)
{
    opcodetype opcode;
    if (it < script.end()) {
        if (script.GetOp(it, opcode, data)) {
            if (force_push && opcode > OP_PUSHDATA4) {
                if (opcode == OP_ENDIF) throw EnvelopeEnd();
                throw InscriptionFormatError("Wrong OP_CODE: " + GetOpName(opcode));
            }
        }
        else throw InscriptionFormatError(std::string(errtag));
    }
    else throw InscriptionFormatError(std::string(errtag));

    return opcode;
}

}


Inscription::Inscription(std::string inscription_id, std::list<std::pair<bytevector, bytevector>>&& inscr_data)
    : m_inscription_id(move(inscription_id))
{
    bytevector metadata;

    while(!inscr_data.empty()) {
        if (inscr_data.front().first == CONTENT_TYPE_TAG) {
            m_content_type.assign(inscr_data.front().second.begin(), inscr_data.front().second.end());
        }
        else if (inscr_data.front().first == CONTENT_TAG) {
            m_content = move(inscr_data.front().second);
        }
        else if (inscr_data.front().first == COLLECTION_ID_TAG) {
            m_collection_id = DeserializeInscriptionId(inscr_data.front().second);
        }
        else if (inscr_data.front().first == ORD_SHIFT_TAG) {
            m_ord_shift = CScriptNum(inscr_data.front().second, false, sizeof(CAmount)).GetInt64();
            //if (!MoneyRange(ord_shift)) throw InscriptionFormatError("Ord shift is greater than whole Bitcoin supply")
        }
        else if (inscr_data.front().first == METADATA_TAG) {
            metadata.insert(metadata.end(), inscr_data.front().second.begin(), inscr_data.front().second.end());
        }
        else if (inscr_data.front().first == CONTENT_ENCODING_TAG) {
            m_content_encoding.assign(inscr_data.front().second.begin(), inscr_data.front().second.end());
        }
        else if (inscr_data.front().first == DELEGATE_ID_TAG) {
            m_delegate_id = DeserializeInscriptionId(inscr_data.front().second);
        }
        else if (inscr_data.front().first == RUNE_TAG) {
            m_rune_commitment = move(inscr_data.front().second);
        }

        inscr_data.pop_front();
    }

    m_metadata = move(metadata);
}


std::list<std::pair<bytevector, bytevector>> ParseEnvelopeScript(const CScript& script, CScript::const_iterator& it) {
    std::list<std::pair<bytevector, bytevector>> res;
    bytevector content;

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

    if (!has_ord_envelope) throw TransactionError("No inscription");

    bool fetching_content = false;

    while (it < end) {
        try {
            opcode = GetNextScriptData(script, it, data, "inscription envelope", fetching_content);

            if (opcode == OP_ENDIF) {
                break;
            }
            if (fetching_content) {
                content.insert(content.end(), data.begin(), data.end());
            }
            else if (opcode == CONTENT_OP_TAG) {
                content.clear();
                fetching_content = true;
            }
            else if (opcode == CONTENT_TYPE_OP_TAG || (opcode == CONTENT_TYPE_TAG.size() && data == CONTENT_TYPE_TAG)) {
                GetNextScriptData(script, it, data, "content type", true);
                res.emplace_back(CONTENT_TYPE_TAG, move(data));
            }
            else if (opcode == ORD_SHIFT_OP_TAG || (opcode == ORD_SHIFT_TAG.size() && data == ORD_SHIFT_TAG)) {
                GetNextScriptData(script, it, data, "ord shift", true);
                res.emplace_back(ORD_SHIFT_TAG, move(data));
            }
            else if (opcode == COLLECTION_ID_OP_TAG || (opcode == COLLECTION_ID_TAG.size() && data == COLLECTION_ID_TAG)) {
                GetNextScriptData(script, it, data, "collection id", true);
                res.emplace_back(COLLECTION_ID_TAG, move(data));
            }
            else if (opcode == METADATA_OP_TAG || (opcode == METADATA_TAG.size() && data == METADATA_TAG)) {
                GetNextScriptData(script, it, data, "meta-data", true);
                res.emplace_back(METADATA_TAG, move(data));
            }
            else if (opcode == CONTENT_ENCODING_OP_TAG || (opcode == CONTENT_ENCODING_TAG.size() && data == CONTENT_ENCODING_TAG)) {
                GetNextScriptData(script, it, data, "content encoding", true);
                res.emplace_back(CONTENT_ENCODING_TAG, move(data));
            }
            else if (opcode == DELEGATE_ID_OP_TAG || (opcode == DELEGATE_ID_TAG.size() && data == DELEGATE_ID_TAG)) {
                GetNextScriptData(script, it, data, "delegate id", true);
                res.emplace_back(DELEGATE_ID_TAG, move(data));
            }
            else {
                bytevector tag = data.empty() ? bytevector{(uint8_t)opcode} : move(data);
                GetNextScriptData(script, it, data, "unknown tag", true);
                res.emplace_back(move(tag), move(data));
            }
        }
        catch (EnvelopeEnd&) {break;}
        catch (...) { }
    }

    if (!content.empty()) {
        res.emplace_back(CONTENT_TAG, move(content));
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
            try {
                auto envelope_tags = ParseEnvelopeScript(script, it);
                res.emplace_back(move((tx.GetHash().GetHex() + "i") += std::to_string(res.size())), move(envelope_tags));
            } catch(...) { /*Ignore errors but skip adding inscription*/}
        }
    }

    return res;
}

} // utxord
