
#include "univalue.h"

#include "core_io.h"

#include "simple_transaction.hpp"

namespace l15::utxord {

const uint32_t SimpleTransaction::m_protocol_version = 1;

CMutableTransaction SimpleTransaction::MakeTx() const
{
    CMutableTransaction tx;

    for(const auto& input: m_inputs) {
        tx.vin.emplace_back(uint256S(input->TxID()), input->NOut());
        tx.vin.back().scriptWitness.stack = input->Destination().Witness();
    }

    for(const auto& out: m_outputs) {
        tx.vout.emplace_back(out->Amount(), out->DestinationPubKeyScript());
    }

    return tx;
}

void SimpleTransaction::Sign(const core::MasterKey &master_key)
{
    CMutableTransaction tx = MakeTx();

    uint32_t nin = 0;
    for(auto& input: m_inputs) {
        try {
            core::ChannelKeys keypair = input->Destination().LookupKeyPair(master_key, TAPROOT_OUTPUT);
            input->Destination().Witness().Set(nin, keypair.SignTaprootTx(tx, nin, {{input->Destination().Amount(), input->Destination().DestinationPubKeyScript()}}, {}));
        }
        catch(const KeyError& e) {
            core::ChannelKeys keypair = input->Destination().LookupKeyPair(master_key, TAPROOT_DEFAULT);
            input->Destination().Witness().Set(nin, keypair.SignTaprootTx(tx, nin, {{input->Destination().Amount(), input->Destination().DestinationPubKeyScript()}}, {}));
        }
        ++nin;
    }
}

std::vector<std::string> SimpleTransaction::RawTransactions() const
{
    return { EncodeHexTx(CTransaction(MakeTx())) };
}

UniValue SimpleTransaction::MakeJson() const
{
    UniValue contract(UniValue::VOBJ);
    contract.pushKV(name_version, (int)m_protocol_version);
    contract.pushKV(name_mining_fee_rate, *m_mining_fee_rate);

    UniValue utxo_arr(UniValue::VARR);
    for (const auto& utxo: m_inputs) {
        UniValue spend = utxo->MakeJson();
        utxo_arr.push_back(move(spend));
    }
    contract.pushKV(name_utxo, utxo_arr);

    UniValue out_arr(UniValue::VARR);
    for (const auto& out: m_outputs) {
        UniValue dest = out->MakeJson();
        out_arr.push_back(dest);
    }

}

void SimpleTransaction::ReadJson(const UniValue &json)
{

}

} // l15::utxord

