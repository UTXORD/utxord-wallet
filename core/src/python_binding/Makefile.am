COMMON_INCLUDES = \
 -I$(top_srcdir)/l15/node/src \
 -I$(top_srcdir)/l15/node/src/secp256k1/include \
 -I$(top_srcdir)/l15/node/src/univalue/include \
 -I$(top_srcdir)/l15/node/src/consensus \
 -I$(top_srcdir)/l15/node/src/crypto \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/primitives \
 -I$(top_srcdir)/l15/node/src/script \
 -I$(top_srcdir)/l15/node/src/support \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/l15/contrib/cex \
 -I$(top_srcdir)/l15/contrib/nlohmann_json \
 -I$(top_srcdir)/src/contract

SWIG_SRC_INCLUDES = \
 $(top_srcdir)/l15/src/common/common_error.hpp \
 $(top_srcdir)/l15/src/common/bech32.hpp \
 $(top_srcdir)/l15/src/common/utils.hpp \
 $(top_srcdir)/l15/src/common/transaction.hpp \
 $(top_srcdir)/src/contract/bip322.hpp \
 $(top_srcdir)/src/contract/inscription.hpp \
 $(top_srcdir)/src/contract/contract_error.hpp \
 $(top_srcdir)/src/contract/contract_builder.hpp \
 $(top_srcdir)/src/contract/create_inscription.hpp \
 $(top_srcdir)/src/contract/swap_inscription.hpp \
 $(top_srcdir)/src/contract/trustless_swap_inscription.hpp \
 $(top_srcdir)/src/contract/simple_transaction.hpp \
 $(top_srcdir)/src/contract/runes.hpp \
 $(top_srcdir)/l15/src/core/schnorr.hpp \
 $(top_srcdir)/l15/src/core/ecdsa.hpp \
 $(top_srcdir)/l15/src/core/keypair.hpp \
 $(top_srcdir)/l15/src/core/keyregistry.hpp \
 $(top_srcdir)/l15/src/core/master_key.hpp \
 $(top_srcdir)/l15/src/core/psbt.hpp


AM_CPPFLAGS = $(COMMON_INCLUDES) $(PYTHON_CPPFLAGS) $(PYTHON_INCLUDES)
AM_CXXFLAGS = -ffunction-sections -fdata-sections

SWIG_FLAGS = -stdstl
SWIG_SRC_FILES = utxord-pybind.i
SWIG_PYTHON_MODULE = libutxord-pybind-wrap.cxx
SWIG_PYTHON_SCRIPT = libutxord_pybind.py

data_DATA = $(SWIG_PYTHON_SCRIPT)

CLEANFILES = $(SWIG_PYTHON_MODULE) $(SWIG_PYTHON_SCRIPT)

BUILT_SOURCES = $(SWIG_PYTHON_MODULE) $(SWIG_PYTHON_SCRIPT)
EXTRA_DIST = $(SWIG_SRC_FILES)

$(SWIG_PYTHON_MODULE): $(SWIG_SRC_FILES) $(SWIG_SRC_INCLUDES)
	$(SWIG) $(COMMON_INCLUDES) -c++ -python -o $@ $<

$(SWIG_PYTHON_SCRIPT): $(SWIG_PYTHON_MODULE)

lib_LTLIBRARIES = _libutxord_pybind.la

_libutxord_pybind_la_SOURCES = $(SWIG_PYTHON_MODULE) tx_utils.cpp
_libutxord_pybind_la_LDFLAGS = -shared -module -avoid-version $(PYTHON_LDFLAGS) -Wl,--no-undefined,--gc-sections

_libutxord_pybind_la_LIBADD = $(PYTHON_LIBS) \
    $(top_builddir)/src/contract/libutxord-contract.la \
    $(top_builddir)/l15/libl15.la \
    $(top_builddir)/l15/node/src/secp256k1/libsecp256k1.la

