%module libutxord_pybind

%{
#define SWIG_FILE_WITH_INIT
static PyObject* pContractFundsNotEnough;
static PyObject* pContractProtocolError;
static PyObject* pContractStateError;
static PyObject* pContractFormatError;
static PyObject* pContractTermWrongFormat;
static PyObject* pContractTermMismatch;
static PyObject* pContractTermWrongValue;
static PyObject* pContractTermMissing;
static PyObject* pContractError;
static PyObject* pWrongKey;
static PyObject* pKeyError;
static PyObject* pSignatureError;
static PyObject* pTransactionError;
static PyObject* pIllegalArgument;
static PyObject* pWrongKeyLookupFilter;
static PyObject* pKeyNotFound;
static PyObject* pWrongDerivationPath;
static PyObject* pMnemonicCheckSumError;
static PyObject* pMnemonicLengthError;
static PyObject* pMnemonicDictionaryError;
static PyObject* pInscriptionError;
static PyObject* pInscriptionFormatError;
static PyObject* pError;
%}

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_list.i"
%include "exception.i"

%apply int64_t { CAmount }

%template(StringVector) std::vector<std::string>;

%{

#include "common.hpp"
#include "transaction.hpp"
#include "schnorr.hpp"
#include "ecdsa.hpp"
#include "keypair.hpp"
#include "keyregistry.hpp"
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

%template(InscriptionList) std::list<utxord::Inscription>;

%exception {
    try {
        $action
        } catch (l15::Error& e) {
            PyErr_SetString(PyExc_Exception, (std::string(e.what()) + ": " + e.details()).c_str());
            SWIG_fail;
        } catch (std::exception& e) {
            PyErr_SetString(PyExc_Exception, e.what());
            SWIG_fail;
        }
}

%catches(l15::WrongKey) l15::core::KeyPair::PubKey();

%catches(l15::core::WrongDerivationPath,
         l15::WrongKey,
         l15::KeyError) l15::core::MasterKey::Derive(const string &path, bool for_script) const;

%catches(l15::core::WrongDerivationPath,
         l15::WrongKey,
         l15::KeyError) l15::core::MasterKey::DerivePubKey(const secp256k1_context* ctx, const ext_pubkey& extpk, uint32_t branch);

%catches(l15::WrongKey) l15::core::MasterKey::GetPubKey(const secp256k1_context *ctx, const ext_pubkey& extpk);
%catches(l15::core::WrongKeyLookupFilter) l15::core::KeyRegistry::AddKeyType(std::string name, const std::string& filter_json);
%catches(l15::IllegalArgument) l15::core::KeyRegistry::AddKeyToCache(const std::string& key);

%catches(l15::core::WrongDerivationPath,
         l15::WrongKey,
         l15::KeyError) l15::core::KeyRegistry::Derive(const std::string& path, bool for_script) const;

%catches(l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError) l15::core::KeyRegistry::Lookup(const xonly_pubkey& pk, const std::string& hint_json) const;

%catches(l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError) l15::core::KeyRegistry::Lookup(const std::string& addr, const std::string& hint_json) const;

%catches(l15::core::WrongDerivationPath, l15::KeyError) l15::core::ExtPubKey::DeriveAddress(const std::string& path) const;

%catches(utxord::ContractStateError, l15::IllegalArgument) utxord::CreateInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::CreateInscriptionBuilder::SignCommit(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::CreateInscriptionBuilder::SignInscription(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::CreateInscriptionBuilder::MarketSignInscription(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::CreateInscriptionBuilder::SignCollection(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractTermMissing,
         utxord::ContractFundsNotEnough) utxord::CreateInscriptionBuilder::RawTransactions() const;

%catches(utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractProtocolError) CreateInscriptionBase::Serialize(uint32_t version, utxord::InscribePhase phase) const;

%catches(utxord::ContractTermMissing,
         utxord::ContractTermWrongFormat,
         utxord::ContractTermMismatch,
         utxord::ContractTermWrongValue,
         utxord::ContractProtocolError) CreateInscriptionBase::Deserialize(const std::string& data, utxord::InscribePhase phase);

%catches(utxord::ContractTermWrongValue) utxord::SwapInscriptionBuilder::OrdUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractTermWrongValue) utxord::SwapInscriptionBuilder::AddFundsUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(l15::IllegalArgument) utxord::SwapInscriptionBuilder::OrdPayoffAddress(std::string addr);
%catches(l15::IllegalArgument) utxord::SwapInscriptionBuilder::FundsPayoffAddress(std::string addr);
%catches(utxord::ContractStateError, l15::IllegalArgument) utxord::SwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::SignFundsPayBack(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::MarketSignOrdPayoffTx(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SwapInscriptionBuilder::MarketSignSwap(const KeyRegistry &master_key, const std::string& key_filter);

%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::SwapInscriptionBuilder::FundsCommitRawTransaction() const;
%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::SwapInscriptionBuilder::FundsPayBackRawTransaction() const;
%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::SwapInscriptionBuilder::OrdSwapRawTransaction() const;
%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::SwapInscriptionBuilder::OrdPayoffRawTransaction() const;

%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::CommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::FundCommitOrdinal(std::string txid, uint32_t nout, CAmount amount, std::string addr, std::string change_addr);
%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::CommitFunds(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::Brick1SwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::Brick2SwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);
%catches(utxord::ContractStateError, utxord::ContractTermWrongValue) utxord::TrustlessSwapInscriptionBuilder::AddMainSwapUTXO(std::string txid, uint32_t nout, CAmount amount, std::string addr);

%catches(l15::IllegalArgument) utxord::TrustlessSwapInscriptionBuilder::OrdPayoffAddress(std::string addr);
%catches(l15::IllegalArgument) utxord::TrustlessSwapInscriptionBuilder::FundsPayoffAddress(std::string addr);

%catches(utxord::ContractStateError, l15::IllegalArgument) utxord::TrustlessSwapInscriptionBuilder::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractStateError,
         utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::TrustlessSwapInscriptionBuilder::SignOrdSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::TrustlessSwapInscriptionBuilder::SignMarketSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::TrustlessSwapInscriptionBuilder::SignOrdCommitment(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::TrustlessSwapInscriptionBuilder::SignFundsCommitment(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractStateError,
         utxord::ContractTermMissing,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::TrustlessSwapInscriptionBuilder::SignFundsSwap(const KeyRegistry &masterKey, const std::string& key_filter);

%catches(utxord::ContractStateError) utxord::TrustlessSwapInscriptionBuilder::OrdCommitRawTransaction() const;
%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::TrustlessSwapInscriptionBuilder::FundsCommitRawTransaction() const;
%catches(utxord::ContractStateError, utxord::ContractFundsNotEnough) utxord::TrustlessSwapInscriptionBuilder::OrdSwapRawTransaction() const;

%catches(utxord::ContractStateError, utxord::ContractTermWrongValue, l15::IllegalArgument) utxord::SimpleTransaction::AddChangeOutput(std::string addr);
%catches(utxord::ContractStateError, l15::IllegalArgument) utxord::SimpleTransaction::GetMinFundingAmount(const std::string& params) const;

%catches(utxord::ContractStateError,
         utxord::ContractTermWrongValue,
         utxord::ContractFundsNotEnough,
         l15::IllegalArgument,
         l15::core::WrongKeyLookupFilter,
         l15::WrongKey,
         l15::KeyError,
         l15::core::KeyNotFoundError,
         l15::SignatureError) utxord::SimpleTransaction::Sign(const KeyRegistry &master_key, const std::string& key_filter_tag);

%catches(utxord::ContractTermMissing,
         utxord::ContractFundsNotEnough) utxord::SimpleTransaction::RawTransactions() const;

%catches(l15::TransactionError) l15::core::Deserialize(const std::string& hex);
%catches(l15::IllegalArgument) l15::core::GetTaprootAddress(const std::string& chain_mode, const std::string& pubkey);
%catches(l15::IllegalArgument) l15::core::GetAddress(const std::string& chain_mode, const bytevector& pubkeyscript);

%catches(l15::TransactionError, utxord::InscriptionFormatError, utxord::InscriptionError) utxord::ParseInscriptions(const std::string& hex_tx);

%typemap(throws) l15::core::WrongKeyLookupFilter %{
    PyErr_SetString(pWrongKeyLookupFilter, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::core::KeyNotFound %{
    PyErr_SetString(pKeyNotFound, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::core::WrongDerivationPath %{
    PyErr_SetString(pWrongDerivationPath, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::core::MnemonicCheckSumError %{
    PyErr_SetString(pMnemonicCheckSumError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::core::MnemonicLengthError %{
    PyErr_SetString(pMnemonicLengthError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::core::MnemonicDictionaryError %{
    PyErr_SetString(pMnemonicDictionaryError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::WrongKey %{
    PyErr_SetString(pWrongKey, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::KeyError %{
    PyErr_SetString(pKeyError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::SignatureError %{
    PyErr_SetString(pSignatureError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::TransactionError %{
    PyErr_SetString(pTransactionError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::IllegalArgument %{
    PyErr_SetString(pIllegalArgument, const_cast<char*>($1.details()));
    SWIG_fail;
%}

%typemap(throws) utxord::InscriptionFormatError %{
    PyErr_SetString(pInscriptionFormatError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::InscriptionError %{
    PyErr_SetString(pInscriptionError, const_cast<char*>($1.details()));
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
%typemap(throws) utxord::ContractStateError %{
    PyErr_SetString(pContractStateError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractFormatError %{
    PyErr_SetString(pContractFormatError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractTermWrongFormat %{
    PyErr_SetString(pContractTermWrongFormat, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractTermMismatch %{
    PyErr_SetString(pContractTermMismatch, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractTermWrongValue %{
    PyErr_SetString(pContractTermWrongValue, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractTermMissing %{
    PyErr_SetString(pContractTermMissing, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) utxord::ContractError %{
    PyErr_SetString(pContractError, const_cast<char*>($1.details()));
    SWIG_fail;
%}
%typemap(throws) l15::Error %{
    PyErr_SetString(pError, const_cast<char*>($1.details()));
    SWIG_fail;
%}


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

%apply l15::bytevector { l15::xonly_pubkey };
%apply l15::bytevector { l15::signature };
%apply bytevector { xonly_pubkey };
%apply bytevector { signature };

%ignore TransactionSerParams;

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

            int witversion;
            l15::bytevector witnessprogram;
            if ($1.vout[i].scriptPubKey.IsWitnessProgram(witversion, witnessprogram)) {

                PyObject *scriptpubkey = PyBytes_FromStringAndSize((const char*)($1.vout[i].scriptPubKey.data()), $1.vout[i].scriptPubKey.size());
                PyDict_SetItemString(out, "scriptPubKey", scriptpubkey);
                Py_XDECREF(scriptpubkey);

                if (witversion == 1) {
                    PyObject *scriptpubkey = PyString_FromString(l15::core::GetTaprootPubKey($1.vout[i]).c_str());
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
%include "bech32.hpp"
%include "common_error.hpp"
%include "contract_error.hpp"
%include "keypair.hpp"
%include "master_key.hpp"
%include "keyregistry.hpp"
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

%init %{
    pError = PyErr_NewException("_libutxord_pybind.Error", NULL, NULL);
    Py_INCREF(pError);
    PyModule_AddObject(m, "Error", pError);

    pWrongKeyLookupFilter = PyErr_NewException("_libutxord_pybind.WrongKeyLookupFilter", pError, NULL);
    Py_INCREF(pWrongKeyLookupFilter);
    PyModule_AddObject(m, "WrongKeyLookupFilter", pWrongKeyLookupFilter);

    pMnemonicDictionaryError = PyErr_NewException("_libutxord_pybind.MnemonicDictionaryError", pError, NULL);
    Py_INCREF(pMnemonicDictionaryError);
    PyModule_AddObject(m, "MnemonicDictionaryError", pMnemonicDictionaryError);

    pMnemonicLengthError = PyErr_NewException("_libutxord_pybind.MnemonicLengthError", pError, NULL);
    Py_INCREF(pMnemonicLengthError);
    PyModule_AddObject(m, "MnemonicLengthError", pMnemonicLengthError);

    pMnemonicCheckSumError = PyErr_NewException("_libutxord_pybind.MnemonicCheckSumError", pError, NULL);
    Py_INCREF(pMnemonicCheckSumError);
    PyModule_AddObject(m, "MnemonicCheckSumError", pMnemonicCheckSumError);

    pWrongDerivationPath = PyErr_NewException("_libutxord_pybind.WrongDerivationPath", pError, NULL);
    Py_INCREF(pWrongDerivationPath);
    PyModule_AddObject(m, "WrongDerivationPath", pWrongDerivationPath);

    pKeyNotFound = PyErr_NewException("_libutxord_pybind.KeyNotFound", pError, NULL);
    Py_INCREF(pKeyNotFound);
    PyModule_AddObject(m, "KeyNotFound", pKeyNotFound);

    pWrongKeyLookupFilter = PyErr_NewException("_libutxord_pybind.WrongKeyLookupFilter", pError, NULL);
    Py_INCREF(pWrongKeyLookupFilter);
    PyModule_AddObject(m, "WrongKeyLookupFilter", pWrongKeyLookupFilter);

    pIllegalArgument = PyErr_NewException("_libutxord_pybind.IllegalArgument", pError, NULL);
    Py_INCREF(pIllegalArgument);
    PyModule_AddObject(m, "IllegalArgument", pIllegalArgument);

    pTransactionError = PyErr_NewException("_libutxord_pybind.TransactionError", pError, NULL);
    Py_INCREF(pTransactionError);
    PyModule_AddObject(m, "TransactionError", pTransactionError);

    pSignatureError = PyErr_NewException("_libutxord_pybind.SignatureError", pError, NULL);
    Py_INCREF(pSignatureError);
    PyModule_AddObject(m, "SignatureError", pSignatureError);

    pKeyError = PyErr_NewException("_libutxord_pybind.KeyError", pError, NULL);
    Py_INCREF(pKeyError);
    PyModule_AddObject(m, "KeyError", pKeyError);

    pKeyError = PyErr_NewException("_libutxord_pybind.KeyError", pError, NULL);
    Py_INCREF(pKeyError);
    PyModule_AddObject(m, "KeyError", pKeyError);

    pWrongKey = PyErr_NewException("_libutxord_pybind.WrongKey", pKeyError, NULL);
    Py_INCREF(pWrongKey);
    PyModule_AddObject(m, "WrongKey", pWrongKey);

    pInscriptionError = PyErr_NewException("_libutxord_pybind.InscriptionError", pError, NULL);
    Py_INCREF(pInscriptionError);
    PyModule_AddObject(m, "InscriptionError", pInscriptionError);

    pInscriptionFormatError = PyErr_NewException("_libutxord_pybind.InscriptionFormatError", pInscriptionError, NULL);
    Py_INCREF(pInscriptionFormatError);
    PyModule_AddObject(m, "InscriptionFormatError", pInscriptionFormatError);

    pContractError = PyErr_NewException("_libutxord_pybind.ContractError", pError, NULL);
    Py_INCREF(pContractError);
    PyModule_AddObject(m, "ContractError", pContractError);

    pContractTermMissing = PyErr_NewException("_libutxord_pybind.ContractTermMissing", pContractError, NULL);
    Py_INCREF(pContractTermMissing);
    PyModule_AddObject(m, "ContractTermMissing", pContractTermMissing);

    pContractTermWrongValue = PyErr_NewException("_libutxord_pybind.ContractTermWrongValue", pContractError, NULL);
    Py_INCREF(pContractTermWrongValue);
    PyModule_AddObject(m, "ContractTermWrongValue", pContractTermWrongValue);

    pContractTermMismatch = PyErr_NewException("_libutxord_pybind.ContractTermMismatch", pContractError, NULL);
    Py_INCREF(pContractTermMismatch);
    PyModule_AddObject(m, "ContractTermMismatch", pContractTermMismatch);

    pContractTermWrongFormat = PyErr_NewException("_libutxord_pybind.ContractTermWrongFormat", pContractError, NULL);
    Py_INCREF(pContractTermWrongFormat);
    PyModule_AddObject(m, "ContractTermWrongFormat", pContractTermWrongFormat);

    pContractFormatError = PyErr_NewException("_libutxord_pybind.ContractFormatError", pContractError, NULL);
    Py_INCREF(pContractFormatError);
    PyModule_AddObject(m, "ContractFormatError", pContractFormatError);

    pContractStateError = PyErr_NewException("_libutxord_pybind.ContractStateError", pContractError, NULL);
    Py_INCREF(pContractStateError);
    PyModule_AddObject(m, "ContractStateError", pContractStateError);

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
    WrongKey = _libutxord_pybind.WrongKey
    SignatureError = _libutxord_pybind.SignatureError
    TransactionError = _libutxord_pybind.TransactionError
    IllegalArgument = _libutxord_pybind.IllegalArgument
    WrongKeyLookupFilter = _libutxord_pybind.WrongKeyLookupFilter
    KeyNotFound = _libutxord_pybind.KeyNotFound
    WrongDerivationPath = _libutxord_pybind.WrongDerivationPath
    MnemonicCheckSumError = _libutxord_pybind.MnemonicCheckSumError
    MnemonicLengthError = _libutxord_pybind.MnemonicLengthError
    MnemonicDictionaryError = _libutxord_pybind.MnemonicDictionaryError
    InscriptionError = _libutxord_pybind.InscriptionError
    InscriptionFormatError = _libutxord_pybind.InscriptionFormatError
    ContractError = _libutxord_pybind.ContractError
    ContractFundsNotEnough = _libutxord_pybind.ContractFundsNotEnough
    ContractProtocolError = _libutxord_pybind.ContractProtocolError
    ContractStateError = _libutxord_pybind.ContractStateError
    ContractFormatError = _libutxord_pybind.ContractFormatError
    ContractTermWrongFormat = _libutxord_pybind.ContractTermWrongFormat
    ContractTermMismatch = _libutxord_pybind.ContractTermMismatch
    ContractTermWrongValue = _libutxord_pybind.ContractTermWrongValue
    ContractTermMissing = _libutxord_pybind.ContractTermMissing

%}
