#pragma once

#include "common.hpp"
#include "contract_error.hpp"

namespace utxord {

class InscriptionError : public l15::Error {
protected:
    InscriptionError() : l15::Error() {}

public:
    explicit InscriptionError(std::string&& details) : l15::Error(move(details)) {}
    ~InscriptionError() override = default;

    const char* what() const noexcept override
    { return "InscriptionError"; }
};

class InscriptionFormatError : public InscriptionError {
public:
    explicit InscriptionFormatError(std::string&& details) : InscriptionError(move(details)) {}
    ~InscriptionFormatError() override = default;

    const char* what() const noexcept override
    { return "InscriptionFormatError"; }
};


using namespace l15;

const size_t MAX_PUSH = 520;
const bytevector ORD_TAG {'o', 'r', 'd'};
const opcodetype CONTENT_OP_TAG {OP_0};
const bytevector CONTENT_TAG {'\x00'};
const opcodetype CONTENT_TYPE_OP_TAG {OP_1};
const bytevector CONTENT_TYPE_TAG {'\x01'};
const opcodetype ORD_SHIFT_OP_TAG {OP_2};
const bytevector ORD_SHIFT_TAG {'\x02'};
const opcodetype COLLECTION_ID_OP_TAG {OP_3};
const bytevector COLLECTION_ID_TAG {'\x03'};
const opcodetype METADATA_OP_TAG {OP_5};
const bytevector METADATA_TAG {'\x05'};
const opcodetype CONTENT_ENCODING_OP_TAG {OP_9};
const bytevector CONTENT_ENCODING_TAG {'\x09'};
const opcodetype DELEGATE_ID_OP_TAG {OP_11};
const bytevector DELEGATE_ID_TAG {'\x0b'};
const opcodetype RUNE_OP_TAG {OP_13};
const bytevector RUNE_TAG {'\x0d'};

inline void CheckInscriptionId(const std::string& inscription_id)
{
    if (inscription_id.length() < 66) throw InscriptionFormatError("inscription id: " + inscription_id);
    if (inscription_id[64] != 'i') throw InscriptionFormatError("inscription id: " + inscription_id);
    try {
        unhex<bytevector>(inscription_id.substr(0, 64));
        std::stoul(inscription_id.substr(65));
    }
    catch (const std::exception& e) {
        std::throw_with_nested(InscriptionError("inscription id: " + inscription_id));
    }
}

inline bytevector SerializeInscriptionId(const std::string& inscription_id)
{
    if (inscription_id.length() < 66) throw InscriptionFormatError("inscription id: " + inscription_id);
    if (inscription_id[64] != 'i') throw InscriptionFormatError("inscription id: " + inscription_id);
    try {
        uint32_t in = std::stoul(inscription_id.substr(65));
        if (in > 255) throw InscriptionFormatError("inscription id input > 255");

        uint256 txid = uint256S(inscription_id.substr(0, 64));
        bytevector res;
        res.reserve(33);

        res.assign(txid.begin(), txid.end());

        if (in > 0) {
            res.push_back(static_cast<uint8_t>(in));
        }
        return res;
    }
    catch (const InscriptionError& e) {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception& e) {
        std::throw_with_nested(InscriptionError("inscription id: " + inscription_id));
    }
}

inline std::string DeserializeInscriptionId(const bytevector& data)
{
    if (data.size() > 65) { // raw string format
        std::string id(data.begin(), data.end());
        CheckInscriptionId(id);
        return id;
    }

    if (data.size() < 32 || data.size() > 33) throw InscriptionFormatError("inscription id binary: " + hex(data));
    try {
        uint32_t in = 0;
        if (data.size() == 33) in = static_cast<uint32_t>(data.back()) & 0x0ff;
        uint256 txid(Span<const uint8_t>(data.data(), 32));

        return txid.GetHex() + 'i' + std::to_string(in);
    }
    catch (const std::exception& e) {
        std::throw_with_nested(InscriptionError("inscription id binary: " + hex(data)));
    }
}

}
