#pragma once

#include <string>
#include <concepts>

#include "common.hpp"
#include "common_error.hpp"

namespace utxord {

class InscriptionError : public l15::Error {
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

std::list<std::pair<l15::bytevector, l15::bytevector>> ParseEnvelopeScript(const CScript& script, CScript::const_iterator& it);


class Inscription
{
    std::string m_inscription_id;
    std::string m_content_type;
    l15::bytevector m_content;
    std::string m_collection_id;
    l15::bytevector m_metadata;

public:
    Inscription() = default;

    explicit Inscription(std::string inscription_id, std::list<std::pair<l15::bytevector, l15::bytevector>>&& tagged_data);

    Inscription(const Inscription& ) = default;
    Inscription(Inscription&& ) noexcept = default;

    Inscription& operator=(const Inscription&) = default;
    Inscription& operator=(Inscription&&) noexcept = default;

    const std::string& GetIscriptionId() const
    { return m_inscription_id; }

    const std::string& GetContentType() const
    { return m_content_type; }
    std::string GetContent() const
    { return l15::hex(m_content); }

    bool HasParent() const
    { return !m_collection_id.empty(); }

    const std::string& GetCollectionId() const
    { return m_collection_id; }

    std::string GetMetadata() const
    { return l15::hex(m_metadata); }
};

std::list<Inscription> ParseInscriptions(const std::string& hex_tx);

} // utxord

