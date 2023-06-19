# initilly forked from:
# https://github.com/m-a-d-n-e-s-s/madness/blob/master/config/acx_with_tbb.m4
#
# under the GNU General Public License v2.0 terms:
# https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#

AC_DEFUN([AX_TBB], [
  ax_with_tbb_include="no"
  ax_with_tbb_lib="no"
  ax_with_tbb="no"

  # Configure to use Intel TBB and specify the include path.
  AC_ARG_WITH([tbb-include],
    [AS_HELP_STRING([--with-tbb-include@<:@=DIR@:>@],
      [Enables use of Intel TBB as the task scheduler.])],
    [
      case $withval in
      yes)
        AC_MSG_ERROR([You must specify a directory for --with-tbb-include.])
      ;;
      no)
      ;;
      *)
        CPPFLAGS="$CPPFLAGS -I$withval"
        ax_with_tbb_include="yes"
        ax_with_tbb="yes"
      esac
    ]
  )


  # Configure to use Intel TBB and specify the library path.
  AC_ARG_WITH([tbb-lib],
    [AS_HELP_STRING([--with-tbb-lib@<:@=DIR@:>@],
      [Enables use of Intel TBB as the task scheduler.])],
    [
      case $withval in
      yes)
        AC_MSG_ERROR([You must specify a directory for --with-tbb-lib.])
      ;;
      no)
      ;;
      *)
        LIBS="$LIBS -L$withval"
        ax_with_tbb_lib="yes"
        ax_with_tbb="yes"
      esac
    ]
  )

  # Configure to use Intel TBB
  AC_ARG_WITH([tbb],
    [AS_HELP_STRING([--with-tbb@<:@=Install DIR@:>@],
      [Enables use of Intel TBB as the task scheduler.])],
    [
      case $withval in
      yes)
        ax_with_tbb="yes"
      ;;
      no)
      ;;
      *)
        if test "$ax_with_tbb_include" == no; then
          CPPFLAGS="$CPPFLAGS -I$withval/include"
        fi
        if test "$ax_with_tbb_lib" == no; then
          LIBS="$LIBS -L$withval/lib"
        fi
        ax_with_tbb="yes"
      esac
    ],
    [ax_with_tbb="yes"]
  )

  # Check that we can compile with Intel TBB
  if test $ax_with_tbb != "no"; then
    AC_LANG_SAVE
    AC_LANG([C++])

    # Check for Intel TBB header.
    AC_CHECK_HEADER([tbb/tbb.h], [],
                    [ax_with_tbb=no
                     AC_MSG_NOTICE([Unable to compile with Intel TBB.])])
    AC_LANG_RESTORE
  fi

  if test $ax_with_tbb != "no"; then
    AC_LANG_SAVE
    AC_LANG([C++])
    # Check for Intel TBB library.
    AC_CHECK_LIB([tbb], [TBB_runtime_interface_version], [LIBS="-ltbb $LIBS"],
                   [ax_with_tbb=no
                    AC_MSG_NOTICE([Unable to link with Intel TBB])])
    AC_LANG_RESTORE
  fi
])


