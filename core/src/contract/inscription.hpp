#pragma once

#include <string>
#include <concepts>

#include "common.hpp"
#include "common_error.hpp"

namespace utxord {

using namespace l15;

class InscriptionError : public Error {
public:
    explicit InscriptionError(std::string&& details) : Error(move(details)) {}
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

std::list<std::pair<bytevector, bytevector>> ParseEnvelopeScript(const CScript& script, CScript::const_iterator& it);


class Inscription
{
    std::string m_inscription_id;
    std::string m_content_type;
    bytevector m_content;
    std::string m_collection_id;
    bytevector m_metadata;

public:
    Inscription() = default;

    explicit Inscription(std::string inscription_id, std::list<std::pair<bytevector, bytevector>>&& tagged_data);

    Inscription(const Inscription& ) = default;
    Inscription(Inscription&& ) noexcept = default;

    Inscription& operator=(const Inscription&) = default;
    Inscription& operator=(Inscription&&) noexcept = default;

    const std::string& GetIscriptionId() const
    { return m_inscription_id; }

    const std::string& GetContentType() const
    { return m_content_type; }
    const bytevector& GetContent() const
    { return m_content; }

    bool HasParent() const
    { return !m_collection_id.empty(); }

    const std::string& GetCollectionId() const
    { return m_collection_id; }

    const bytevector& GetMetadata() const
    { return m_metadata; }
};

std::list<Inscription> ParseInscriptions(const std::string& hex_tx);

} // utxord

