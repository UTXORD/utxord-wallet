AM_CPPFLAGS = \
 -I$(top_srcdir)/src/contract \
 -I$(top_srcdir)/l15/node/src \
 -I$(top_srcdir)/l15/node/src/univalue/include \
 -I$(top_srcdir)/l15/node/src/consensus \
 -I$(top_srcdir)/l15/node/src/primitives \
 -I$(top_srcdir)/l15/node/src/script \
 -I$(top_srcdir)/l15/node/src/support \
 -I$(top_srcdir)/l15/node/src/policy \
 -I$(top_srcdir)/l15/node/src/crypto \
 -I$(top_srcdir)/l15/node/src/secp256k1/include \
 -I$(top_srcdir)/l15/src/common \
 -I$(top_srcdir)/l15/src/core \
 -I$(top_srcdir)/l15/src/api \
 -I$(top_srcdir)/l15/src/tools \
 -I$(top_srcdir)/l15/contrib/nlohmann_json \
 -I$(top_srcdir)/l15/contrib/cli11 \
 -I$(top_srcdir)/l15/contrib/cex


noinst_LTLIBRARIES = libutxord-test.la

contract: libutxord-test.la

libutxord_test_la_SOURCES = \
	test_case_wrapper.cpp

libutxord_test_la_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
libutxord_test_la_CXXFLAGS = $(AM_CXXFLAGS) -ffunction-sections -fdata-sections
