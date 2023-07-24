#pragma once

#include "common_error.hpp"

namespace l15::utxord {

class ContractError : public Error {
public:
    explicit ContractError(std::string&& details) : Error(move(details)) {}
    explicit ContractError(const char* const details) : Error(details) {}
    ~ContractError() override = default;

    const char* what() const noexcept override
    { return "ContractError"; }
};

class ContractTermMissing : public ContractError {
public:
    explicit ContractTermMissing(std::string&& details) : ContractError(move(details)) {}
    explicit ContractTermMissing(const char* const details) : ContractError(details) {}
    ~ContractTermMissing() override = default;

    const char* what() const noexcept override
    { return "ContractTermsMissing"; }
};

class ContractTermWrongValue : public ContractError {
public:
    explicit ContractTermWrongValue(std::string&& details) : ContractError(move(details)) {}
    explicit ContractTermWrongValue(const char* const details) : ContractError(details) {}
    ~ContractTermWrongValue() override = default;

    const char* what() const noexcept override
    { return "ContractTermWrongValue"; }
};

class ContractValueMismatch : public ContractError {
public:
    explicit ContractValueMismatch(std::string&& details) : ContractError(move(details)) {}
    explicit ContractValueMismatch(const char* const details) : ContractError(details) {}
    ~ContractValueMismatch() override = default;

    const char* what() const noexcept override
    { return "ContractTermValueMismatch"; }
};

class ContractTermWrongFormat : public ContractError {
public:
    explicit ContractTermWrongFormat(std::string&& details) : ContractError(move(details)) {}
    explicit ContractTermWrongFormat(const char* const details) : ContractError(details) {}
    ~ContractTermWrongFormat() override = default;

    const char* what() const noexcept override
    { return "ContractTermWrongFormat"; }
};

class ContractStateError : public ContractError {
public:
    explicit ContractStateError(std::string&& details) : ContractError(move(details)) {}
    explicit ContractStateError(const char* const details) : ContractError(details) {}
    ~ContractStateError() override = default;

    const char* what() const noexcept override
    { return "ContractStateError"; }
};

class ContractProtocolError : public ContractError {
public:
    explicit ContractProtocolError(std::string&& details) : ContractError(move(details)) {}
    explicit ContractProtocolError(const char* const details) : ContractError(details) {}
    ~ContractProtocolError() override = default;

    const char* what() const noexcept override
    { return "ContractProtocolError"; }
};

}