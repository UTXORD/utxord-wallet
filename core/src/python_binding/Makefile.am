COMMON_INCLUDES = \
 -I$(top_srcdir)/bitcoin/src \
 -I$(top_srcdir)/bitcoin/src/secp256k1/include \
 -I$(top_srcdir)/bitcoin/src/univalue/include \
 -I$(top_srcdir)/bitcoin/src/consensus \
 -I$(top_srcdir)/bitcoin/src/policy \
 -I$(top_srcdir)/bitcoin/src/primitives \
 -I$(top_srcdir)/bitcoin/src/script \
 -I$(top_srcdir)/bitcoin/src/support \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/l15/contrib/cex \
 -I$(top_srcdir)/src/contract

SWIG_SRC_INCLUDES = \
 $(top_srcdir)/l15/src/common/common_error.hpp \
 $(top_srcdir)/l15/src/core/transaction.hpp \
 $(top_srcdir)/src/contract/contract_error.hpp \
 $(top_srcdir)/src/contract/contract_builder.hpp \
 $(top_srcdir)/src/contract/create_inscription.hpp \
 $(top_srcdir)/src/contract/swap_inscription.hpp \
 $(top_srcdir)/src/contract/simple_transaction.hpp \
 $(top_srcdir)/l15/src/core/schnorr.hpp \
 $(top_srcdir)/l15/src/core/ecdsa.hpp \
 $(top_srcdir)/l15/src/core/keypair.hpp \
 $(top_srcdir)/l15/src/core/keyregistry.hpp



AM_CPPFLAGS = $(COMMON_INCLUDES) $(PYTHON_CPPFLAGS) $(PYTHON_INCLUDES)

SWIG_FLAGS = -stdstl
SWIG_SRC_FILES = utxord-pybind.i
SWIG_PYTHON_MODULE = libutxord-pybind-wrap.cxx
CLEANFILES = $(SWIG_PYTHON_MODULE) libutxord_pybind.py

BUILT_SOURCES = $(SWIG_PYTHON_MODULE)
EXTRA_DIST = $(SWIG_SRC_FILES)

$(SWIG_PYTHON_MODULE): $(SWIG_SRC_FILES) $(SWIG_SRC_INCLUDES)
	$(SWIG) $(COMMON_INCLUDES) -c++ -python -o $@ $<


lib_LTLIBRARIES = _libutxord_pybind.la

_libutxord_pybind_la_SOURCES = $(SWIG_PYTHON_MODULE)
_libutxord_pybind_la_LDFLAGS = -shared -module -avoid-version $(PYTHON_LDFLAGS) -Wl,--no-undefined

_libutxord_pybind_la_LIBADD = \
    $(top_builddir)/l15/src/core/libl15-core.la \
    $(top_builddir)/l15/src/common/libl15-common.la \
    $(top_builddir)/bitcoin/src/secp256k1/libsecp256k1.la \
    $(top_builddir)/bitcoin/src/libbitcoinconsensus.la \
    $(top_builddir)/bitcoin/src/libunivalue.la \
    $(top_builddir)/src/contract/libutxord-contract.la \
    $(PYTHON_LIBS)

install-completed-library:
	cp $(srcdir)/__init__.py $(top_builddir)/src/python_binding/
	cp $(top_builddir)/src/python_binding/.libs/_libutxord_pybind.so $(top_builddir)/src/python_binding/_libutxord_pybind.so

