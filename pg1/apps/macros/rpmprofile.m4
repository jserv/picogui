dnl usage: AM_BEE_RPMPROFILE
dnl
dnl provides options and variables to generate rpm spec file.
dnl
dnl $Id$

AC_DEFUN(AM_BEE_RPMPROFILE, [

eval RPM_ROOT="${prefix}"

RPM_ROOT=`echo ${RPM_ROOT} | sed -e "s%NONE%${prefix}%"`
RPM_ROOT=`echo ${RPM_ROOT} | sed -e "s%NONE%/usr/local%"`

RPM_PREFIX="${RPM_ROOT}"

AC_ARG_WITH(rpm-root,
[  --with-rpm-root=<prefix>   ],
RPM_ROOT="${withval}"
)

if test ${RPM_ROOT} = "NONE" ; then

  RPM_ROOT=""
  RPM_PLATFORM=""
  MINUS_RPM_PLATFORM=""
  RPM_REQUIRE_MDL=""

else

  RPM_PLATFORM="${host}"
  MINUS_RPM_PLATFORM="-${host}"
  RPM_REQUIRE_MDL="${PACKAGE}-mdl = ${VERSION}"
fi

AC_ARG_WITH(rpm-platform, 
[  --with-rpm-platform=<prefix>   ],
if test toto${withval} = toto"" ; then
  RPM_PLATFORM=""
  MINUS_RPM_PLATFORM=""
  RPM_REQUIRE_MDL=""
else
  RPM_PLATFORM="${withval}"
  MINUS_RPM_PLATFORM="-${withval}"
  RPM_REQUIRE_MDL="${PACKAGE}-mdl = ${VERSION}"
fi
)

AC_SUBST(RPM_PREFIX)
AC_SUBST(RPM_PLATFORM)
AC_SUBST(MINUS_RPM_PLATFORM)
AC_SUBST(RPM_REQUIRE_MDL)

RPM_CONFIGURE="./configure ${ac_configure_args}"

AC_SUBST(RPM_ROOT)
AC_SUBST(RPM_CONFIGURE)

cat<<EOF > rpmmake.frag
# how to create the rpm

rpm: dist
	sudo rpm -tb ${PACKAGE}-${VERSION}.tar.gz
EOF

RPM_MAKE_FRAG="rpmmake.frag"

AC_SUBST_FILE(RPM_MAKE_FRAG)

])
