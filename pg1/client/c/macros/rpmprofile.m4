dnl usage: AM_BEE_RPMPROFILE
dnl
dnl provides options and variables to generate rpm spec file.
dnl
dnl $Id$

AC_DEFUN(AM_BEE_RPMPROFILE, [

eval RPM_PREFIX="${prefix}"

RPM_PREFIX=`echo ${RPM_PREFIX} | sed -e "s%NONE%${prefix}%"`
RPM_PREFIX=`echo ${RPM_PREFIX} | sed -e "s%NONE%/usr/local%"`

AC_ARG_WITH(rpm-prefix, 
[  --with-rpm-prefix=<prefix>   ],
RPM_PREFIX="${withval}"
)

if test ${host} = "NONE" ; then
  RPM_PLATFORM=""
else
  RPM_PLATFORM="${host}"
fi

AC_ARG_WITH(rpm-platform, 
[  --with-rpm-platform=<prefix>   ],
RPM_PLATFORM="${withval}"
)

AC_SUBST(RPM_PREFIX)
AC_SUBST(RPM_PLATFORM)

rpm_args=

rpm_prefix=:
rpm_r_prefix=:

for ac_arg in ${ac_configure_args}
do
  case "$ac_arg" in
  --prefix=*) rpm_prefix="${ac_arg}" ;;
  --with-rpm-prefix=*) rpm_r_prefix="${ac_arg}" ;;
  *" "*|*"	"*|*[\[\]\~\#\$\^\&\*\(\)\{\}\\\|\;\<\>\?]*)
  rpm_args="$rpm_args '$ac_arg'" ;;
  *) rpm_args="$rpm_args $ac_arg" ;;
  esac
done

if test ${rpm_r_prefix} != ":" ; then
  rpm_args="`echo ${rpm_r_prefix} | sed -e 's/with-rpm-//'` ${rpm_args}"
else
  if test ${rpm_prefix} != ":" ; then
    rpm_args="${rpm_prefix} ${rpm_args}"
  fi
fi

RPM_CONFIGURE="./configure ${rpm_args}"

AC_SUBST(RPM_CONFIGURE)

cat<<EOF > rpmmake.frag
# how to create the rpm

rpm: dist
	sudo rpm -tb ${PACKAGE}-${VERSION}.tar.gz
EOF

RPM_MAKE_FRAG="rpmmake.frag"

AC_SUBST_FILE(RPM_MAKE_FRAG)

])
