%module libutxord_pybind

%include "std_shared_ptr.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_list.i"
%include "exception.i"

%apply unsigned int { uint32_t }
%apply unsigned long long { uint64_t }

%template(StringVector) std::vector<std::string>;
%template(SharedL15Error) std::shared_ptr<l15::Error>;

%{

#include "common.hpp"
#include "transaction.hpp"
#include "address.hpp"
#include "keypair.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"
#include "common_error.hpp"
#include "inscription.hpp"
#include "simple_transaction.hpp"

using namespace utxord;
using namespace l15;

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

%typemap(in) const bytevector& (bytevector param, const char *begin) {
    if (!PyBytes_Check($input)) {
        SWIG_exception_fail(SWIG_TypeError, "in method '" "GetAddress" "', argument " "1"" of type '" "bytes""'");
    }
    begin = PyBytes_AsString($input);
    param.assign(begin, begin+PyBytes_Size($input));
    $1 = &param;
}

%typemap(out) const l15::bytevector& { $result = PyBytes_FromStringAndSize((const char*)($1->data()), $1->size()); }
%typemap(out) l15::bytevector { $result = PyBytes_FromStringAndSize((const char*)($1.data()), $1.size()); }

%apply l15::bytevector { l15::xonly_pubkey };
%apply l15::bytevector { l15::signature };

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


%include "common_error.hpp"
%include "contract_error.hpp"
%include "address.hpp"
%include "keypair.hpp"
%include "contract_builder.hpp"

%template (CreateInscriptionBase) utxord::ContractBuilder<utxord::InscribePhase>;
%template (SwapInscriptionBase) utxord::ContractBuilder<utxord::SwapPhase>;
%template (SimpleTransactionBase) utxord::ContractBuilder<utxord::TxPhase>;

%include "create_inscription.hpp"
%include "swap_inscription.hpp"
%include "simple_transaction.hpp"
%include "transaction.hpp"
%include "transaction.h"
%include "inscription.hpp"

