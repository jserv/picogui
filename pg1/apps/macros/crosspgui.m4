dnl usage: AM_PGUI_CROSSCOMPILE
dnl 
dnl Tests the compilation environment, add the --with-cross option and
dnl define CC_FOR_BUILD variable to the build-platform compiler
dnl
dnl $Id$

AC_DEFUN(AM_PGUI_CROSSCOMPILE, [

AC_CANONICAL_HOST

AC_CACHE_CHECK([for previous host],
		ac_cv_canonical_host, [ ac_cv_canonical_host="$host" ])

if test "$host" != "$ac_cv_canonical_host" ; then
  rm -f "$cache_file"

  AC_MSG_WARN([host platform has changed, removing config.cache])
  AC_MSG_WARN([you can now run the script again])

  exit 0
fi

AC_ARG_WITH(prefix,
[  --with-prefix=<tool prefix>   Use given prefix for gcc, ar,...], [

  if test "$withval" = "yes" ; then
    with_prefix="$host"
  fi
], [
  with_prefix=""
])

if test "$host" != "NONE" ; then
  CC_FOR_BUILD=${CC_FOR_BUILD:-gcc}

  if test "$with_prefix" != "" ; then
    CC="${with_prefix}-gcc"
    LD="${with_prefix}-ld"
    AR="${with_prefix}-ar"
    NM="${with_prefix}-nm"
    RANLIB="${with_prefix}-ranlib"
  fi
fi


dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB

AC_CHECK_PROG(AR, ${AR:-ar}, ${AR:-ar})
AC_CHECK_PROG(NM, ${NM:-nm}, ${NM:-nm})

dnl if we don't cross-compile, the build compiler is the host compiler

if test "$cross_compiling" = "no" ; then
  CC_FOR_BUILD="$CC"
fi

AC_SUBST(CC_FOR_BUILD)


])

