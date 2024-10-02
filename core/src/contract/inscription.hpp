#pragma once

#include <contract_builder.hpp>
#include <string>

#include "common.hpp"
#include "inscription_common.hpp"

namespace utxord {


std::list<std::pair<l15::bytevector, l15::bytevector>> ParseEnvelopeScript(const CScript& script, CScript::const_iterator& it);


class Inscription
{
    std::string m_inscription_id;
    std::string m_content_type;
    l15::bytevector m_content;
    CAmount m_ord_shift = 0;
    std::string m_collection_id;
    l15::bytevector m_metadata;
    std::string m_content_encoding;
    std::string m_delegate_id;
    std::optional<l15::bytevector> m_rune_commitment;

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

    const l15::bytevector& GetContent() const
    { return m_content; }

    CAmount GetOrdShift() const
    { return m_ord_shift; }

    bool HasParent() const
    { return !m_collection_id.empty(); }

    const std::string& GetCollectionId() const
    { return m_collection_id; }

    const l15::bytevector& GetMetadata() const
    { return m_metadata; }

    const std::string& GetContentEncoding() const
    { return m_content_encoding; }

    const std::string& GetDelegateId() const
    { return m_delegate_id; }

    bool HasRuneCommitment() const
    { return m_rune_commitment.has_value(); }

    const l15::bytevector& GetRuneCommitment() const
    {
        // This is because excessive rune optimization and even empty commitment means rune 'A'
        if (!m_rune_commitment) throw InscriptionError("no rune");
        return *m_rune_commitment;
    }
};

std::list<Inscription> ParseInscriptions(const std::string& hex_tx);

} // utxord

