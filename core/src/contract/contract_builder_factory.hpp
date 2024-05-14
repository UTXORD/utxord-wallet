#pragma once

#include "contract_builder.hpp"
#include "runes.hpp"

namespace utxord {
template <std::derived_from<IContractDestination> DEST, typename ... ARGS>
struct ContractDestinationFactory
{
    typedef DEST destination_type;

    static std::shared_ptr<IContractDestination> ReadJson(ChainMode chain, const UniValue& json)
    {
        if (destination_type::type == json[IJsonSerializable::name_type].getValStr()) {
            return destination_type::Construct(chain, json);
        }
        else /*if (sizeof...(ARGS))*/ {
            return ContractDestinationFactory<ARGS...>::ReadJson(chain, json);
        }
    }
};

template <std::derived_from<IContractDestination> DEST>
struct ContractDestinationFactory<DEST>
{
    typedef DEST destination_type;

    static std::shared_ptr<IContractDestination> ReadJson(ChainMode chain, const UniValue& json, bool allow_zero_destination = false)
    {
        return destination_type::Construct(chain, json);
    }
};

// ZeroDestination must be last in the ContractDestinationFactory type list if any
template <>
struct ContractDestinationFactory<ZeroDestination>
{
    static std::shared_ptr<IContractDestination> ReadJson(ChainMode chain, const UniValue& json)
    { return std::make_shared<ZeroDestination>(json); }
};


    typedef ContractDestinationFactory<P2Witness, RuneStoneDestination, ZeroDestination> DestinationFactory;
    typedef ContractDestinationFactory<P2Witness, RuneStoneDestination> NoZeroDestinationFactory;
};