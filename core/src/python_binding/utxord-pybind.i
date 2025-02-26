%module libutxord_pybind

%{
#define SWIG_FILE_WITH_INIT
static PyObject* pContractFundsNotEnough;
static PyObject* pContractProtocolError;
static PyObject* pContractError;
static PyObject* pKeyError;
static PyObject* pError;
%}

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_list.i"
%include "exception.i"

%apply int64_t { CAmount }

%template(StringVector) std::vector<std::string>;

%apply std::vector<std::string> { l15::stringvector };
%apply const std::vector<std::string>& { const l15::stringvector& };

%{

#include "common.hpp"
#include "transaction.hpp"
#include "tx_utils.hpp"
#include "schnorr.hpp"
#include "ecdsa.hpp"
#include "keypair.hpp"
#include "keyregistry.hpp"
#include "mnemonic.hpp"
#include "bip322.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"
#include "trustless_swap_inscription.hpp"
#include "common_error.hpp"
#include "inscription.hpp"
#include "simple_transaction.hpp"

using namespace utxord;
using namespace l15;
using namespace l15::core;

%}

%exception {
    try {
        $action
    } catch (l15::Error& e) {
        PyErr_SetString(pError, (std::string(e.what()) + ": " + e.details()).c_str());
        SWIG_fail;
    } catch (std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        SWIG_fail;
    }
}

%catches(l15::KeyError) l15::core::KeyPair::PubKey();

%catches(l15::KeyError) l15::core::MasterKey::Derive(const string &path, bool for_script) const;
%catches(l15::KeyError) l15::core::MasterKey::DerivePubKey(const secp256k1_context* ctx, const ext_pubkey& extpk, uint32_t branch);

%catches(l15::KeyError) l15::core::MasterKey::GetPubKey(const secp256k1_context *ctx, const ext_pubkey& extpk);

%catches(l15::KeyError) l15::core::KeyRegistry::Derive(const std::string& path, bool for_script) const;
%catches(l15::KeyError) l15::core::KeyRegistry::Lookup(const xonly_pubkey& pk, const std::string& hint_json) const;

%catches(l15::KeyError) l15::core::KeyRegistry::Lookup(const std::string& addr, const std::string& hint_json) const;

%catches(l15::KeyError) l15::core::ExtPubKey::DeriveAddress(const std::string& path) const;

%catches(utxord::ContractError) utxord::IContractBuilder::MarketFee(CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::IContractBuilder::AddCustomFee(CAmount amount, std::string addr);

%catches(utxord::ContractError) utxord::CreateInscriptionBuilder::OrdOutput(CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::CreateInscriptionBuilder::AuthorFee(CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::CreateInscriptionBuilder::AddUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::CreateInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::CreateInscriptionBuilder::SignCommit(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::CreateInscriptionBuilder::SignInscription(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::CreateInscriptionBuilder::MarketSignInscription(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::CreateInscriptionBuilder::SignCollection(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::CreateInscriptionBuilder::RawTransactions() const;
%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::CreateInscriptionBuilder::TransactionsPSBT() const;
%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::CreateInscriptionBuilder::ApplyPSBTSignature(const l15::stringvector&);

%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::InscribePhase>::Serialize(uint32_t version, utxord::InscribePhase phase) const;
%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::InscribePhase>::Deserialize(const std::string& data, utxord::InscribePhase phase);

%catches(utxord::ContractError) utxord::SwapInscriptionBuilder::OrdUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::SwapInscriptionBuilder::AddFundsUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::SwapInscriptionBuilder::OrdPayoffAddress(std::string addr);
%catches(utxord::ContractError) utxord::SwapInscriptionBuilder::FundsPayoffAddress(std::string addr);
%catches(utxord::ContractError) utxord::SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::SignFundsPayBack(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::MarketSignOrdPayoffTx(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SwapInscriptionBuilder::MarketSignSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::SwapInscriptionBuilder::FundsCommitRawTransaction() const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::SwapInscriptionBuilder::FundsPayBackRawTransaction() const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::SwapInscriptionBuilder::OrdSwapRawTransaction() const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::SwapInscriptionBuilder::OrdPayoffRawTransaction() const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::CommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::FundCommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr, std::string change_addr);

%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::SwapPhase>::Serialize(uint32_t version, utxord::InscribePhase phase) const;
%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::SwapPhase>::Deserialize(const std::string& data, utxord::InscribePhase phase);

%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::CommitFunds(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::Brick1SwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::Brick2SwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::AddMainSwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::OrdPayoffAddress(std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::FundsPayoffAddress(std::string addr);
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
        l15::KeyError) utxord::TrustlessSwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::TrustlessSwapInscriptionBuilder::SignMarketSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::TrustlessSwapInscriptionBuilder::SignOrdCommitment(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::TrustlessSwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::TrustlessSwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::OrdCommitRawTransaction() const;
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::FundsCommitRawTransaction() const;
%catches(utxord::ContractError) utxord::TrustlessSwapInscriptionBuilder::OrdSwapRawTransaction() const;

%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::TrustlessSwapPhase>::Serialize(uint32_t version, utxord::InscribePhase phase) const;
%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::TrustlessSwapPhase>::Deserialize(const std::string& data, utxord::InscribePhase phase);

%catches(utxord::ContractError) utxord::SimpleTransaction::AddChangeOutput(std::string addr);
%catches(utxord::ContractError) utxord::SimpleTransaction::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractFundsNotEnough, utxord::ContractError,
         l15::KeyError) utxord::SimpleTransaction::Sign(const KeyRegistry &master_key, const std::string& key_filter_tag);

%catches(utxord::ContractFundsNotEnough, utxord::ContractError) utxord::SimpleTransaction::RawTransactions() const;

%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::TxPhase>::Serialize(uint32_t version, utxord::InscribePhase phase) const;
%catches(utxord::ContractProtocolError, utxord::ContractError) utxord::ContractBuilder<utxord::TxPhase>::Deserialize(const std::string& data, utxord::InscribePhase phase);

%typemap(throws) l15::KeyError %{
    PyErr_SetString(pKeyError, (std::string($1.what()) + ": " + $1.details()).c_str());
    SWIG_fail;
%}
%typemap(throws) utxord::ContractFundsNotEnough %{
    PyErr_SetString(pContractFundsNotEnough, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractProtocolError %{
    PyErr_SetString(pContractProtocolError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractError %{
    PyErr_SetString(pContractError, (std::string($1.what()) + ": " + $1.details()).c_str());
    SWIG_fail;
%}
%typemap(throws) l15::Error %{
    PyErr_SetString(pError, (std::string($1.what()) + ": " + $1.details()).c_str());
    SWIG_fail;
%}

%typemap(in) l15::seckey (seckey param, const char *begin) {
    if (!PyBytes_Check($input)) {
        SWIG_exception_fail(SWIG_TypeError, "argument is not bytes");
    }
    if (PyBytes_Size($input) != 32) {
        SWIG_exception_fail(SWIG_TypeError, (std::string("argument has wrong length: ") + std::to_string(PyBytes_Size($input))).c_str());
    }
    begin = PyBytes_AsString($input);
    try {
        param.assign(begin, begin+PyBytes_Size($input));
    } catch (...) {
         SWIG_exception_fail(SWIG_TypeError, "cannot convert bytes argument");
    }
    $1 = param;
}

%typemap(in) const bytevector& (bytevector param, const char *begin) {
    if (!PyBytes_Check($input)) {
        SWIG_exception_fail(SWIG_TypeError, "argument is not bytes");
    }
    begin = PyBytes_AsString($input);
    try {
        param.assign(begin, begin+PyBytes_Size($input));
    } catch (...) {
        SWIG_exception_fail(SWIG_TypeError, "cannot convert bytes argument");
    }
    $1 = &param;
}

%typemap(in) const l15::bytevector& (bytevector param, const char *begin) {
    if (!PyBytes_Check($input)) {
        SWIG_exception_fail(SWIG_TypeError, "argument is not bytes");
    }
    begin = PyBytes_AsString($input);
    try {
        param.assign(begin, begin+PyBytes_Size($input));
    } catch (...) {
        SWIG_exception_fail(SWIG_TypeError, "cannot convert bytes argument");
    }
    $1 = &param;
}

%typemap(in) bytevector (bytevector param, const char *begin) {
    if (!PyBytes_Check($input)) {
        SWIG_exception_fail(SWIG_TypeError, "argument is not bytes");
    }
    begin = PyBytes_AsString($input);
    try {
        param.assign(begin, begin+PyBytes_Size($input));
    } catch (...) {
         SWIG_exception_fail(SWIG_TypeError, "cannot convert bytes argument");
    }
    $1 = param;
}

%typemap(out) const l15::bytevector& { $result = PyBytes_FromStringAndSize((const char*)($1->data()), $1->size()); }
%typemap(out) const bytevector& { $result = PyBytes_FromStringAndSize((const char*)($1->data()), $1->size()); }
%typemap(out) l15::bytevector { $result = PyBytes_FromStringAndSize((const char*)($1.data()), $1.size()); }
%typemap(out) bytevector { $result = PyBytes_FromStringAndSize((const char*)($1.data()), $1.size()); }

%apply l15::seckey { seckey };
%apply l15::bytevector { l15::xonly_pubkey };
%apply l15::bytevector { l15::signature };
%apply l15::bytevector { l15::compressed_pubkey };
%apply bytevector { xonly_pubkey };
%apply bytevector { signature };
%apply bytevector { compressed_pubkey };

%ignore l15::core::KeyPair::KeyPair(const KeyPair&);
%ignore l15::core::KeyPair::KeyPair(KeyPair&&);
%ignore l15::core::KeyPair::KeyPair(secp256k1_context const *);
%ignore l15::core::KeyPair::KeyPair(secp256k1_context const *, seckey);
%ignore l15::core::KeyPair::KeyPair(SchnorrKeyPair &&);
%ignore l15::core::KeyPair::KeyPair(EcdsaKeyPair &&);


%ignore TransactionSerParams;
%ignore CScript;
%ignore ParseAmount;
%ignore FormatAmount;
%ignore Dust;
%ignore CalculateOutputAmount;
%ignore ScriptHash;
%ignore CreatePreimage;
%ignore GetCsvInBlocks;
%ignore Error;
%ignore KeyError;
%ignore WrongKey;
%ignore SignatureError;
%ignore TransactionError;
%ignore IllegalArgument;
%ignore ContractError;
%ignore ContractTermMissing;
%ignore ContractTermWrongValue;
%ignore ContractTermMismatch;
%ignore ContractTermWrongFormat;
%ignore ContractFormatError;
%ignore ContractStateError;
%ignore ContractProtocolError;
%ignore ContractFundsNotEnough;
%ignore WrongDerivationPath;
%ignore WrongKeyLookupFilter;
%ignore KeyNotFoundError;
%ignore MnemonicCheckSumError;
%ignore MnemonicLengthError;
%ignore MnemonicDictionaryError;

%ignore utxord::P2Address::type;
%ignore utxord::P2Witness::type;
%ignore utxord::OpReturnDestination::type;
%ignore utxord::OpReturnDestination::name_data;
%ignore utxord::UTXO::type;

%ignore utxord::SimpleTransaction::ReadJson;
%ignore utxord::SimpleTransaction::MakeJson;

%ignore utxord::CreateInscriptionBuilder::ReadJson;
%ignore utxord::CreateInscriptionBuilder::MakeJson;

%ignore utxord::SwapInscriptionBuilder::ReadJson;
%ignore utxord::SwapInscriptionBuilder::MakeJson;

%ignore utxord::TrustlessSwapInscriptionBuilder::ReadJson;
%ignore utxord::TrustlessSwapInscriptionBuilder::MakeJson;

%typemap(out) CMutableTransaction (PyObject* obj)
%{
    obj = PyDict_New();

    {
        PyObject *txid = PyUnicode_FromString($1.GetHash().GetHex().c_str());
        PyDict_SetItemString(obj, "txid", txid);
        Py_XDECREF(txid);
    }

    {
        PyObject *inputs = PyList_New($1.vin.size());
        for (size_t i = 0;i < $1.vin.size();++i) {
            PyObject *in = PyDict_New();

            {
                PyObject *hash = PyString_FromString($1.vin[i].prevout.hash.GetHex().c_str());
                PyDict_SetItemString(in, "txid", hash);
                Py_XDECREF(hash);
            }

            {
                PyObject *n = PyInt_FromLong($1.vin[i].prevout.n);
                PyDict_SetItemString(in, "vout", n);
                Py_XDECREF(n);
            }

            PyList_SET_ITEM(inputs, i, in);
        }
        PyDict_SetItemString(obj, "vin", inputs);
        Py_XDECREF(inputs);
    }

    {
        PyObject* outputs = PyList_New($1.vout.size());
        for (size_t i = 0; i < $1.vout.size(); ++i) {
            PyObject* out = PyDict_New();

            {
                PyObject* val = PyLong_FromLongLong($1.vout[i].nValue);
                PyDict_SetItemString(out, "value", val);
                Py_XDECREF(val);
            }

            {
                PyObject* n = PyInt_FromLong(i);
                PyDict_SetItemString(out, "n", n);
                Py_XDECREF(n);
            }

            PyObject *scriptpubkey = PyBytes_FromStringAndSize((const char*)($1.vout[i].scriptPubKey.data()), $1.vout[i].scriptPubKey.size());
            PyDict_SetItemString(out, "scriptPubKey", scriptpubkey);
            Py_XDECREF(scriptpubkey);

            int witversion;
            l15::bytevector witnessprogram;
            if ($1.vout[i].scriptPubKey.IsWitnessProgram(witversion, witnessprogram)) {

                if (witversion == 1) {
                    PyObject *scriptpubkey = PyString_FromString(utxord::GetTaprootPubKey($1.vout[i]).c_str());
                    PyDict_SetItemString(out, "pubKey", scriptpubkey);
                    Py_XDECREF(scriptpubkey);
                }
            }

            PyList_SET_ITEM(outputs, i, out);
        }
        PyDict_SetItemString(obj, "vout", outputs);
        Py_XDECREF(outputs);
    }

    $result = SWIG_Python_AppendOutput($result, obj);
%}

%include "utils.hpp"
%include "tx_utils.hpp"
%include "bech32.hpp"
%include "common_error.hpp"
%include "contract_error.hpp"
%include "schnorr.hpp"
%include "ecdsa.hpp"
%include "keypair.hpp"
%include "master_key.hpp"
%include "keyregistry.hpp"
%include "mnemonic.hpp"
%include "bip322.hpp"

%template(MnemonicParser) l15::core::MnemonicParser<std::vector<std::string>>;

%include "contract_builder.hpp"

%template (CreateInscriptionBase) utxord::ContractBuilder<utxord::InscribePhase>;
%template (SwapInscriptionBase) utxord::ContractBuilder<utxord::SwapPhase>;
%template (TrustlessSwapInscriptionBase) utxord::ContractBuilder<utxord::TrustlessSwapPhase>;
%template (SimpleTransactionBase) utxord::ContractBuilder<utxord::TxPhase>;

%include "create_inscription.hpp"
%include "swap_inscription.hpp"
%include "trustless_swap_inscription.hpp"
%include "simple_transaction.hpp"
%include "transaction.hpp"
%include "inscription.hpp"

%template(InscriptionList) std::list<utxord::Inscription>;


%init %{
    pError = PyErr_NewException("_libutxord_pybind.Error", NULL, NULL);
    Py_INCREF(pError);
    PyModule_AddObject(m, "Error", pError);

    pKeyError = PyErr_NewException("_libutxord_pybind.KeyError", pError, NULL);
    Py_INCREF(pKeyError);
    PyModule_AddObject(m, "KeyError", pKeyError);

    pContractError = PyErr_NewException("_libutxord_pybind.ContractError", pError, NULL);
    Py_INCREF(pContractError);
    PyModule_AddObject(m, "ContractError", pContractError);

    pContractProtocolError = PyErr_NewException("_libutxord_pybind.ContractProtocolError", pContractError, NULL);
    Py_INCREF(pContractProtocolError);
    PyModule_AddObject(m, "ContractProtocolError", pContractProtocolError);

    pContractFundsNotEnough = PyErr_NewException("_libutxord_pybind.ContractFundsNotEnough", pContractError, NULL);
    Py_INCREF(pContractFundsNotEnough);
    PyModule_AddObject(m, "ContractFundsNotEnough", pContractFundsNotEnough);
%}

%pythoncode %{
    Error = _libutxord_pybind.Error
    KeyError = _libutxord_pybind.KeyError
    ContractError = _libutxord_pybind.ContractError
    ContractProtocolError = _libutxord_pybind.ContractProtocolError
    ContractFundsNotEnough = _libutxord_pybind.ContractFundsNotEnough
%}
