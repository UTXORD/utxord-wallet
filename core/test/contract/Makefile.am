
AM_CPPFLAGS =

if BIND_WASM
AM_CPPFLAGS += -DWASM
endif

AM_CPPFLAGS += \
 -I$(top_srcdir)/l15/contrib/nlohmann_json \
 -I$(top_srcdir)/l15/contrib \
 -I$(top_srcdir)/l15/contrib/cex \
 -I$(top_srcdir)/l15/contrib/cli11 \
 -I$(top_srcdir)/l15/node/src/secp256k1/include \
 -I$(top_srcdir)/l15/node/src \
 -I$(top_srcdir)/l15/node/src/univalue/include \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/consensus \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/script \
 -I$(top_srcdir)/l15/node/src/support \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/tools \
 -I$(top_srcdir)/l15/src/api \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/test \
 -I$(top_srcdir)/src/contract


L15_LIBS = \
$(top_builddir)/src/contract/libutxord-contract.la \
$(top_builddir)/l15/src/api/libl15-api.la \
$(top_builddir)/l15/src/core/libl15-core.la \
$(top_builddir)/l15/src/tools/libl15-tools.la \
$(top_builddir)/l15/src/common/libl15-common.la \
-lbitcoin_common -lbitcoin_util -lbitcoin_consensus \
$(top_builddir)/l15/node/src/crypto/libbitcoin_crypto_base.la \
$(top_builddir)/l15/node/src/crypto/libbitcoin_crypto_avx2.la \
$(top_builddir)/l15/node/src/crypto/libbitcoin_crypto_x86_shani.la \
$(top_builddir)/l15/node/src/crypto/libbitcoin_crypto_sse41.la \
$(top_builddir)/l15/node/src/secp256k1/libsecp256k1.la \
$(top_builddir)/l15/node/src/libunivalue.la \
$(BOOST_FILESYSTEM_LIB)


AM_LDFLAGS=-L$(top_builddir)/l15/node/src


bin_PROGRAMS = \
test_contract_builder \
test_create_inscription \
test_swap_inscription \
test_trustless_swap_inscription \
test_inscription \
test_simple_tx \
test_runes


test_contract_builder_SOURCES = test_contract_builder.cpp
test_contract_builder_LDADD = $(L15_LIBS)

test_create_inscription_SOURCES = test_create_inscription.cpp
test_create_inscription_LDADD = $(L15_LIBS)

test_swap_inscription_SOURCES = test_swap_inscription.cpp
test_swap_inscription_LDADD = $(L15_LIBS)

test_trustless_swap_inscription_SOURCES = test_trustless_swap_inscription.cpp
test_trustless_swap_inscription_LDADD = $(L15_LIBS)

test_inscription_SOURCES = test_inscription.cpp
test_inscription_LDADD = $(L15_LIBS)

test_simple_tx_SOURCES = test_simple_tx.cpp
test_simple_tx_LDADD = $(L15_LIBS)

test_runes_SOURCES = test_runes.cpp
test_runes_LDADD = $(L15_LIBS)
