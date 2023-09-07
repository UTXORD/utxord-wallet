#pragma once

#include <string>

#include <boost/container/flat_map.hpp>

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

std::list<std::pair<bytevector, bytevector>> ParseEnvelopeScript(const CScript& script);

class Inscription
{
    std::string m_inscription_id;
    std::string m_content_type;
    bytevector m_content;
    std::string m_collection_id;
    boost::container::flat_map<std::string, std::string> m_metadata;

public:
    template<class T>
    explicit Inscription(const T& tx, uint32_t nin = 0);
    explicit Inscription(const std::string& hex_tx, uint32_t nin = 0);
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

    const boost::container::flat_map<std::string, std::string>& GetMetadata() const
    { return m_metadata; }
};

Inscription ParseInscription(const std::string& hex_tx);

} // utxord

