
AM_CPPFLAGS =

if BIND_WASM
AM_CPPFLAGS += -DWASM
endif

AM_CPPFLAGS += \
 -I$(top_srcdir)/l15/contrib/nlohmann_json \
 -I$(top_srcdir)/l15/node/src \
 -I$(top_srcdir)/l15/node/src/univalue/include \
 -I$(top_srcdir)/l15/node/src/consensus \
 -I$(top_srcdir)/l15/node/src/primitives \
 -I$(top_srcdir)/l15/node/src/script \
 -I$(top_srcdir)/l15/node/src/support \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/secp256k1/include \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/l15/contrib/cex


noinst_LTLIBRARIES = libutxord-contract.la
libutxord_contract_la_SOURCES = \
	contract_builder.cpp \
	create_inscription.cpp \
	swap_inscription.cpp \
	trustless_swap_inscription.cpp \
	simple_transaction.cpp \
	runes.cpp

if !BIND_WASM
libutxord_contract_la_SOURCES += inscription.cpp
endif

libutxord_contract_la_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
libutxord_contract_la_CXXFLAGS = $(AM_CXXFLAGS) -ffunction-sections -fdata-sections
