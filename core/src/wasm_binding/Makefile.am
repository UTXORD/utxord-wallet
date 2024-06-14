
AM_CPPFLAGS = -DWASM -DWASM_MODULE \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/src/contract \
 -I$(top_srcdir)/l15/contrib/cli11 \
 -I$(top_srcdir)/l15/contrib/cex \
 -I$(top_srcdir)/l15/node/src \
 -I$(top_srcdir)/l15/node/src/secp256k1/include \
 -I$(top_srcdir)/l15/node/src/univalue/include \
 -I$(top_srcdir)/l15/node/src/consensus \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/script \
 -I$(top_srcdir)/l15/node/src/support


AM_LDFLAGS=-L$(top_builddir)/l15/node/src -L$(top_builddir)/l15/node/src/crypto --post-js contract.js
AM_LDFLAGS=-L$(top_builddir)/l15/node/src -L$(top_builddir)/l15/node/src/crypto --post-js contract.js


L15_LIBS = \
$(top_builddir)/l15/src/core/libl15-core.la  \
$(top_builddir)/l15/src/common/libl15-common.la \
$(top_builddir)/l15/node/src/libbitcoinconsensus.la \
$(top_builddir)/l15/node/src/secp256k1/libsecp256k1.la \
$(top_builddir)/l15/node/src/libunivalue.la \
$(top_builddir)/src/contract/libutxord-contract.la

BUILT_SOURCES = contract.cpp contract.js
EXTRA_DIST = contract.idl

CLEANFILES = $(BUILT_SOURCES) utxord.wasm

contract.js: contract.idl

contract.cpp: contract.idl
	webidl_binder $< contract

utxord.cpp: contract.cpp

bin_PROGRAMS = utxord.js

utxord_js_SOURCES = utxord.cpp
utxord_js_LDADD = $(L15_LIBS)
