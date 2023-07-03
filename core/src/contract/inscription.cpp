
#include <list>

#include "transaction.h"
#include "streams.h"
#include "version.h"


#include "inscription.hpp"
#include "inscription_common.hpp"


namespace l15::utxord {

namespace {

opcodetype GetNextScriptData(const CScript& script, CScript::const_iterator& it, bytevector& data, const std::string errtag) {
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

std::list<std::pair<bytevector, bytevector>> ParseEnvelopeScript(const CScript& script) {
    std::list<std::pair<bytevector, bytevector>> res;
    bytevector content;

    size_t i = 0;

    CScript::const_iterator it = script.begin();
    CScript::const_iterator end = script.end();
    opcodetype prev_opcode_2 = OP_INVALIDOPCODE;
    opcodetype prev_opcode = OP_INVALIDOPCODE;
    opcodetype opcode = OP_INVALIDOPCODE;
    bytevector data;
    bool has_ord_envelope = false;
    bool has_ord_parent_envelope = false;

    while (it < end && !has_ord_envelope && !has_ord_parent_envelope) {
        prev_opcode_2 = prev_opcode;
        prev_opcode = opcode;

        if (!script.GetOp(it, opcode, data))
            throw TransactionError("script");

        has_ord_envelope = (prev_opcode_2 == OP_0 &&
            prev_opcode == OP_IF &&
            opcode == ORD_TAG.size() &&
            data == ORD_TAG);

        has_ord_parent_envelope = (prev_opcode_2 == OP_0 &&
            prev_opcode == OP_IF &&
            opcode == ORD_PARENT_TAG.size() &&
            data == ORD_PARENT_TAG);
    }

    bool fetching_content = false;

    while (it < end) {
        opcode = GetNextScriptData(script, it, data, "inscription envelope");

        if (opcode == CONTENT_TAG) {
            if (!fetching_content && !content.empty()) throw InscriptionFormatError("second CONTENT tag");
            fetching_content = true;
        }
        else if ((opcode == CONTENT_TYPE_TAG.size() && data == CONTENT_TYPE_TAG)) {
            GetNextScriptData(script, it, data, "content type");
            res.emplace_back(CONTENT_TYPE_TAG, move(data));
        }
        else if (opcode == COLLECTION_ID_TAG.size() && data == COLLECTION_ID_TAG) {
            GetNextScriptData(script, it, data, "collection id");
            res.emplace_back(COLLECTION_ID_TAG, move(data));
        }
        else if (opcode == OP_ENDIF) {
            if (!content.empty()) {
                res.emplace_back(CONTENT_ALIAS_TAG, move(content));
            }
            break;
        }
        else if (fetching_content) {
            content.insert(content.end(), data.begin(), data.end());
        }
    }
    return res;
}

}


Inscription::Inscription(const std::string &hex_tx)
{
    CDataStream stream(unhex<bytevector>(hex_tx), SER_NETWORK, PROTOCOL_VERSION);

    CMutableTransaction tx;

    try {
        stream >> tx;
    }
    catch (const std::exception& e) {
        std::throw_with_nested(TransactionError("TX parse error"));
    }

    if (tx.vin[0].scriptWitness.stack.size() < 3) throw InscriptionFormatError("no witness script");

    const auto& witness_stack = tx.vin[0].scriptWitness.stack;
    CScript script(witness_stack[witness_stack.size() - 2].begin(), witness_stack[witness_stack.size() - 2].end());

    auto inscr_data = ParseEnvelopeScript(script);
    while(!inscr_data.empty()) {
        if (inscr_data.front().first == CONTENT_TYPE_TAG) {
            if (!m_content_type.empty()) throw InscriptionFormatError("second CONTENT_TYPE tag");
            m_content_type.assign(inscr_data.front().second.begin(), inscr_data.front().second.end());
            inscr_data.pop_front();
        }
        else if (inscr_data.front().first == CONTENT_ALIAS_TAG) {
            if (!m_content.empty()) throw InscriptionFormatError("second CONTENT tag");
            m_content = move(inscr_data.front().second);
            inscr_data.pop_front();
        }
        else if (inscr_data.front().first == COLLECTION_ID_TAG) {
            if (!m_collection_id.empty()) throw InscriptionFormatError("second COLLECTION_ID tag");

            std::string collection_id(inscr_data.front().second.begin(), inscr_data.front().second.end());
            CheckCollectionId(collection_id);
            m_collection_id = move(collection_id);
            inscr_data.pop_front();
        }
        else {
            // just skip unknown tag
            inscr_data.pop_front();
        }
    }

    if (m_content.empty()) throw InscriptionError("no content");

    if (m_collection_id.empty() && tx.vin.size() > 1 && tx.vin[1].scriptWitness.stack.size() >= 3) {
        const auto &witness_stack = tx.vin[1].scriptWitness.stack;
        CScript script(witness_stack[witness_stack.size() - 2].begin(), witness_stack[witness_stack.size() - 2].end());
        std::string collection_id;

        auto inscr_data = ParseEnvelopeScript(script);
        while(!inscr_data.empty()) {
            if (inscr_data.front().first == CONTENT_TYPE_TAG || inscr_data.front().first == CONTENT_ALIAS_TAG) {
                collection_id.clear();
                break;
            }
            else if (inscr_data.front().first == COLLECTION_ID_TAG) {
                collection_id.assign(inscr_data.front().second.begin(), inscr_data.front().second.end());
                CheckCollectionId(collection_id);
                inscr_data.pop_front();
            }
            else {
                // just skip unknown tag
                inscr_data.pop_front();
            }
        }
        if (!collection_id.empty()) {
            m_collection_id = move(collection_id);
        }
    }

    m_inscription_id = tx.GetHash().GetHex() + "i0";
}


} // utxord
