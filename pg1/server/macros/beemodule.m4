dnl usage: BEE_MODULE(<module name>)
dnl 
dnl Add a --with-<module>=... configuration option and create
dnl <module>_CFLAGS and <module>_LDFLAGS accordingly.
dnl
dnl $Id$


AC_DEFUN(BEE_MODULE, [

AC_MSG_CHECKING([for module $1])

define(upper_$1, translit($1, [a-z], [A-Z]))

$1_CFLAGS=""
$1_LDFLAGS=""
$1_PREFIX=""

AC_ARG_WITH($1,
[  --with-$1=<prefix>    installation prefix of the $1 module],

[

AC_MSG_RESULT([in ${withval}])
indir([upper_$1])_CFLAGS="-I${withval}/include"
indir([upper_$1])_LDFLAGS="-L${withval}/lib"
indir([upper_$1])_PREFIX="${withval}"
], 

AC_MSG_RESULT([unspecified])
)

AC_SUBST(indir([upper_$1])_CFLAGS)
AC_SUBST(indir([upper_$1])_LDFLAGS)
AC_SUBST(indir([upper_$1])_PREFIX)
])

