
AM_CPPFLAGS = -DWASM_MODULE \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/l15/src/inscribeit \
 -I$(top_srcdir)/l15/contrib/secp256k1/include \
 -I$(top_srcdir)/l15/contrib/cli11 \
 -I$(top_srcdir)/l15/contrib/cex \
 -I$(top_srcdir)/bitcoin/src \
 -I$(top_srcdir)/bitcoin/src/univalue/include \
 -I$(top_srcdir)/bitcoin/src/consensus \
 -I$(top_srcdir)/bitcoin/src/script \
 -I$(top_srcdir)/bitcoin/src/support


AM_LDFLAGS=-L$(top_builddir)/bitcoin/src -L$(top_builddir)/bitcoin/src/crypto --post-js utxord.js


L15_LIBS = \
$(top_builddir)/l15/src/core/libl15-core.la  \
$(top_builddir)/l15/src/common/libl15-common.la \
$(top_builddir)/bitcoin/src/libbitcoinconsensus.la \
$(top_builddir)/l15/contrib/secp256k1/libsecp256k1.la \
$(top_builddir)/bitcoin/src/libunivalue.la \
$(top_builddir)/src/contract/libutxord-contract.la

BUILT_SOURCES = utxord.cpp utxord.js
EXTRA_DIST = utxord.idl

CLEANFILES = $(BUILT_SOURCES) l15.wasm

utxord.js: utxord.idl

utxord.cpp: utxord.idl
	webidl_binder $< utxord

l15.cpp: utxord.cpp

bin_PROGRAMS = l15.js

l15_js_SOURCES = l15.cpp
l15_js_LDADD = $(L15_LIBS)
