%module libutxord_pybind

%include "std_shared_ptr.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "exception.i"

%apply unsigned int { uint32_t }
%apply unsigned long long { uint64_t }

%template(StringVector) std::vector<std::string>;
%template(SharedL15Error) std::shared_ptr<l15::Error>;

%{

#include "common.hpp"
#include "transaction.hpp"
#include "create_inscription.hpp"
#include "swap_inscription.hpp"
#include "common_error.hpp"
#include "inscription.hpp"

using namespace utxord;
using namespace l15;

%}

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

            if (l15::core::IsTaproot($1.vout[i])) {
                PyObject *scriptpubkey = PyString_FromString(l15::core::GetTaprootPubKey($1.vout[i]).c_str());
                PyDict_SetItemString(out, "pubKey", scriptpubkey);
                Py_XDECREF(scriptpubkey);
            }

            PyList_SET_ITEM(outputs, i, out);
        }
        PyDict_SetItemString(obj, "vout", outputs);
        Py_XDECREF(outputs);
    }

    $result = SWIG_Python_AppendOutput($result, obj);
%}

namespace utxord {

        %typemap(out) Inscription (PyObject* obj)
        %{
            obj = PyDict_New();

            {
                PyObject * id = PyUnicode_FromString($1.GetIscriptionId().c_str());
                PyDict_SetItemString(obj, "id", id);
                Py_XDECREF(id);
            }

            {
                PyObject * content_type = PyUnicode_FromString($1.GetContentType().c_str());
                PyDict_SetItemString(obj, "content_type", content_type);
                Py_XDECREF(content_type);
            }

            {
                PyObject * content = PyBytes_FromStringAndSize((const char*)($1.GetContent().data()), $1.GetContent().size());
                PyDict_SetItemString(obj, "content", content);
                Py_XDECREF(content);
            }

            if ($1.HasParent()) {
                PyObject * collection_id = PyUnicode_FromString($1.GetCollectionId().c_str());
                PyDict_SetItemString(obj, "collection_id", collection_id);
                Py_XDECREF(collection_id);
            }

            if (!$1.GetMetadata().empty()) {
                PyObject * metadata = PyBytes_FromStringAndSize((const char*)($1.GetMetadata().data()), $1.GetMetadata().size());
                PyDict_SetItemString(obj, "metadata", metadata);
                Py_XDECREF(metadata);
            }

            $result = SWIG_Python_AppendOutput($result, obj);
        %}
}

%include "common_error.hpp"
%include "contract_error.hpp"
%include "create_inscription.hpp"
%include "swap_inscription.hpp"
%include "transaction.hpp"
%include "transaction.h"
%include "inscription.hpp"

