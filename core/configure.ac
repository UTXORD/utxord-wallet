AC_PREREQ([2.69])

AC_INIT([UTXORD Core], [m4_esyscmd_s([./version.sh])], [l2xl@protonmail.com])
AC_CONFIG_SRCDIR([src/contract/contract_builder.hpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

LT_INIT([pic-only])

AM_INIT_AUTOMAKE([1.16 foreign subdir-objects])

dnl Compiler checks (here before libtool).
if test "x${CXXFLAGS}" != "x"; then
  CXXFLAGS_overridden=yes
else
  CXXFLAGS_overridden=
fi

if test "x${CFLAGS}" != "x"; then
  CFLAGS_overridden=yes
else
  CFLAGS_overridden=
fi

if test "x${CPPFLAGS}" != "x"; then
  CPPFLAGS_overridden=yes
else
  CPPFLAGS_overridden=
fi

if test "x${LDFLAGS}" != "x"; then
  LDFLAGS_overridden=yes
else
  LDFLAGS_overridden=
fi

compiler_name=`basename "$CC"`
if test "x$compiler_name" = "xemcc"; then
    with_emscripten=yes
else
    with_emscripten=no
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_MKDIR_P

AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])
dnl AX_BOOST_BASE([1.70])

dnl PKG_WITH_MODULES([NLOHMANN_JSON], [nlohmann_json>=3.10.5], [], [AC_MSG_ERROR([nlohmann_json >= 3.10.5 is required for inscription metadata])], [nlohmann_json >= 3.10.5 is required])
dnl PKG_CHECK_MODULES([NLOHMANN_JSON], [nlohmann_json>=3.10.5], [], [AC_MSG_ERROR([nlohmann_json >= 3.10.5 is required for inscription metadata])])

AC_ARG_WITH([nlohmann_json], [AS_HELP_STRING([--with-nlohmann-json=@<:@=ARG@:>@],
            [use nlohmann_json from standard location (default),
             use nlohmann_json prefix path (ARG=<path>)])], [
             AS_CASE(["x${withval}"],
                [x],[with_nlohmann_json=yes],
                [xyes],[with_nlohmann_json=yes],
                [xno],[with_nlohmann_json=no],
                [with_nlohmann_json=yes; NLOHMANN_JSON_CPPFLAGS="-I${withval}/include"]
    )])

if test "with_nlohmann_json" != "no"; then
    CPP_FLAGS_NLOHMANN_SAVE=$CPPFLAGS

    CPPFLAGS="$CPPFLAGS $NLOHMANN_JSON_CPPFLAGS"

    AC_LANG_PUSH([C++])
    AC_CHECK_HEADER(nlohmann/json.hpp, [json_hpp=yes], [json_hpp=no])
    AC_LANG_POP([C++])

    CPPFLAGS=$CPP_FLAGS_NLOHMANN_SAVE

    if test "$json_hpp" = "no"; then
        AC_MSG_ERROR([nlohmann_json is required for inscription metadata])
    fi

    AC_SUBST(NLOHMANN_JSON_CPPFLAGS)
else
    AC_MSG_ERROR([nlohmann_json is required for inscription metadata])
fi

default_build_tests=yes

AC_ARG_ENABLE([python-binding],
              [AS_HELP_STRING([--enable-python-binding],[Enable API binding module for python (disabled by default)])],
              [
                if test $enableval != no; then
                  default_build_tests=no
                fi
              ],
              [enable_python_binding=no])

AC_ARG_ENABLE([wasm-binding],
              [AS_HELP_STRING([--enable-wasm-binding], [Enable plugin API binding for WebAssembly (disabled by default)])],
              [
                if test $enableval != no; then
                  default_build_tests=no
                fi
              ],
              [enable_wasm_binding=no])

if test "$enable_python_binding" = "yes" && test "$enable_wasm_binding" = "yes"; then
    AC_MSG_ERROR([WebAssembly and Python bindings cannot be built simultaneously])
fi

AC_ARG_ENABLE([build-tests],
              [AS_HELP_STRING([--enable-build-tests], [Enable build tests (enabled by default)])],
              [],
              [enable_build_tests=$default_build_tests])

AM_CONDITIONAL([BUILD_TESTS], [test "$enable_build_tests" = "yes"])
AM_CONDITIONAL([BIND_PYTHON], [test "$enable_python_binding" = "yes"])
AM_CONDITIONAL([BIND_WASM], [test "$enable_wasm_binding" = "yes"])

if test "$enable_python_binding" = "yes"; then
    AM_PATH_PYTHON([3])
    AC_MSG_NOTICE([python is $PYTHON])

    AX_PKG_SWIG([], [], [ AC_MSG_ERROR([SWIG is required to build.]) ])
    AX_PYTHON_DEVEL
    AX_SWIG_PYTHON
fi

dnl Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use compiler flags and macros suited for debugging (disabled by default)])],
    [],
    [enable_debug=no])

dnl If debugging is enabled, and the user hasn't overridden CXXFLAGS, clear
dnl them, to prevent autoconfs "-g -O2" being added. Otherwise we'd end up
dnl with "-O0 -g3 -g -O2".
if test "$CXXFLAGS_overridden" != "yes"; then
      CXXFLAGS=""
fi
if test "$CFLAGS_overridden" != "yes"; then
      CFLAGS=""
fi
if test "$LDLAGS_overridden" != "yes"; then
      LDLAGS=""
fi

AX_CHECK_COMPILE_FLAG([-Werror], [CXXFLAG_WERROR="-Werror"], [CXXFLAG_WERROR=""])

if test "x$enable_debug" = "xyes"; then

  dnl Disable all optimizations
  AX_CHECK_COMPILE_FLAG([-O0], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -O0"], [], [$CXXFLAG_WERROR])

  dnl Prefer -g3, fall back to -g if that is unavailable.
  AX_CHECK_COMPILE_FLAG(
    [-g3],
    [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g3"],
    [AX_CHECK_COMPILE_FLAG([-g], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g"], [], [$CXXFLAG_WERROR])],
    [$CXXFLAG_WERROR])

  AX_CHECK_PREPROC_FLAG([-DDEBUG], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-ftrapv], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -ftrapv"], [], [$CXXFLAG_WERROR])

fi


if test "$enable_wasm_binding" = "yes"; then
  if test "x$with_emscripten" != "xyes"; then
      AC_MSG_ERROR([Wrong compiler for WASM build])
  fi

  CPPFLAGS="$DEBUG_CPPFLAGS $CPPFLAGS"
  CXXFLAGS="$DEBUG_CXXFLAGS $CXXFLAGS -fexceptions -Wno-unqualified-std-cast-call"
  LDFLAGS="$LDFLAGS -fexceptions -sEXPORT_EXCEPTION_HANDLING_HELPERS -sLLD_REPORT_UNDEFINED  -sEXPORTED_FUNCTIONS=['_malloc'] -sEXPORT_NAME='utxord' -sMODULARIZE=1 -sNO_FILESYSTEM=1"

  if test "$enable_debug" != "yes"; then
      CFLAGS=" -O3 $CFLAGS"
      CXXFLAGS=" -O3 $CXXFLAGS"
      LDFLAGS=" -O3 $LDFLAGS"
  fi

else
  AX_PTHREAD(,[AC_MSG_ERROR([libpthread is required, but it was not found in your system])])

  CC="$PTHREAD_CC"
  CXX="$PTHREAD_CXX"
  LIBS="$PTHREAD_LIBS $LIBS"
  CPPFLAGS="$DEBUG_CPPFLAGS $CPPFLAGS"
  CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
  CXXFLAGS="$DEBUG_CXXFLAGS $PTHREAD_CFLAGS $CXXFLAGS"

  AX_TBB
fi

if test "x$enable_build_tests" = "xyes"; then
  AX_BOOST_BASE([1.70])
  AX_BOOST_FILESYSTEM
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/contract/Makefile
				])

if test "$enable_build_tests" = "yes"; then
    AC_MSG_NOTICE([Adding test makefiles])
    AC_CONFIG_FILES([test/Makefile test/contract/Makefile])
fi

if test "$enable_python_binding" = "yes"; then
    AC_CONFIG_FILES([src/python_binding/Makefile])
fi

if test "$enable_wasm_binding" = "yes"; then
    AC_CONFIG_FILES([src/wasm_binding/Makefile])
fi

dnl boost's m4 checks do something really nasty: they export these vars. As a
dnl result, they leak into subdir's configure and crazy things may happen.
dnl Until this is fixed upstream and we've synced, we'll just un-export them.

CPPFLAGS_TEMP="$CPPFLAGS"
unset CPPFLAGS
CPPFLAGS="$CPPFLAGS_TEMP"

LDFLAGS_TEMP="$LDFLAGS"
unset LDFLAGS
LDFLAGS="$LDFLAGS_TEMP"

LIBS_TEMP="$LIBS"
unset LIBS
LIBS="$LIBS_TEMP"

PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

PKGCONFIG_LIBDIR_TEMP="$PKG_CONFIG_LIBDIR"
unset PKG_CONFIG_LIBDIR
PKG_CONFIG_LIBDIR="$PKGCONFIG_LIBDIR_TEMP"

dnl AC_CONFIG_SUBDIRS does not support custom arguments per subproject
dnl so that is the reason to use more flexible AX_SUBDIRS_CONFIGURE.
dnl Unfortunately AX_SUBDIRS_CONFIGURE does not allow 'autoreconf' be called recursively for all subprojects (just does it for 'configure' script only)
dnl so we call 'autoreconf' for the every subproject manually with 'autogen.sh', placed at the source tree root.


AS_IF([test $enable_build_tests = yes], [build_apis=--enable-build-apis],
  [build_apis=--disable-build-apis]
)

AS_IF([test $enable_build_tests = yes], [build_tools=--enable-build-tools],
  [build_tools=--disable-build-tools]
)

AS_IF([test $enable_wasm_binding = yes], [build_plugin_api=--enable-plugin-api],
  [test $enable_python_binding = yes], [build_plugin_api=--enable-plugin-api],
  [build_plugin_api=--disable-plugin-api]
)


AX_SUBDIRS_CONFIGURE([l15], [--with-blockchain-node=../bitcoin, --with-secp256k1=node, --disable-shared, --disable-build-frost-signer, $build_tools, --disable-build-services, $build_apis, $build_plugin_api], [], [], [])


AC_OUTPUT

echo
echo "  ==============================================================================================================="
echo "  UTXORD core config result:"
echo "  build version    = $VERSION"
echo
echo "  debug enabled    = $enable_debug"
echo
echo "  python module    = $enable_python_binding"
echo "  wasm module      = $enable_wasm_binding"
echo "  tests            = $enable_build_tests"
echo
echo "  target os        = $host_os"
echo "  build os         = $build_os"
echo
echo "  CPPFLAGS         = $CPPFLAGS $ZMQ_CPPFLAGS"
echo "  CXX              = $CXX"
echo "  CXXFLAGS         = $CXXFLAGS"
echo "  LDFLAGS          = $LDFLAGS $ZMQ_LDFLAGS"
echo "  LIBS             = $LIBS $ZMQ_LIBS"
echo "  ARFLAGS          = $ARFLAGS"
echo "  ==============================================================================================================="
echo
