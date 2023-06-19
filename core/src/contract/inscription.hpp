#pragma once

#include <string>

#include "common.hpp"
#include "common_error.hpp"

namespace l15::utxord {

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

class Inscription
{
    std::string m_inscription_id;
    std::string m_content_type;
    bytevector m_content;
    std::string m_collection_id;


public:
    explicit Inscription(const std::string& hex_tx);
    Inscription(const Inscription& ) = default;
    Inscription(Inscription&& ) noexcept = default;

    Inscription& operator=(const Inscription&) = default;
    Inscription& operator=(Inscription&&) noexcept = default;

    const std::string& GetIscriptionId() const
    { return m_inscription_id; }

    const std::string& GetContentType() const
    { return m_content_type; }
    std::string GetContent() const
    { return hex(m_content); }

    bool HasParent() const
    { return !m_collection_id.empty(); }

    const std::string& GetCollectionId() const
    { return m_collection_id; }

};

} // utxord

