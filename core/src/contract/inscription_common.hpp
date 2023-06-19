#pragma once

#include "common.hpp"
#include "contract_error.hpp"

namespace l15::utxord {

const size_t chunk_size = 520;
const bytevector ORD_TAG {'o', 'r', 'd'};
const bytevector ORD_PARENT_TAG {'o', 'r', 'p'};
const opcodetype CONTENT_TAG {OP_0};
const bytevector CONTENT_ALIAS_TAG {'\0'};
const bytevector CONTENT_TYPE_TAG {'\1'};
const bytevector COLLECTION_ID_TAG {'\3'};

inline void CheckCollectionId(const std::string& collection_id)
{
    if (collection_id[64] != 'i') throw ContractTermWrongValue("collection id: " + collection_id);
    try {
        unhex<bytevector>(collection_id.substr(0, 64));
        std::stoul(collection_id.substr(65));
    }
    catch (const std::exception& e) {
        std::throw_with_nested(ContractTermWrongValue("collection id: " + collection_id));
    }
}

}
