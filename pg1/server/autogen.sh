#!/bin/sh

aclocal_extra="-I macros"

if [ -f .autogen.conf ] ; then
    echo "autogen.sh:sourcing .autogen.conf"
    . .autogen.conf
fi

verbose=1

error ()
{
    echo "autogen.sh:error: $*"
    exit 1
}

run ()
{
    if [ $verbose = 1 ] ; then
	echo "autogen.sh:running: $*"
    fi

    $* || error "while running $*"
}

# check SDL installation
dir=`aclocal --print-ac-dir`
if [ -f "${dir}/sdl.m4" ] ; then
  echo "autogen.sh: using default SDL m4 file"
else
  echo "autogen.sh: using private SDL m4 file"
  aclocal_extra="${aclocal_extra} -I macros/extra"
fi

run aclocal ${aclocal_extra}
run autoheader
run autoconf
run automake -a

if [ -x ./config.status ] ; then
    run ./config.status --recheck
    run ./config.status
else
    if [ $# -gt 0 ] ; then
        run ./configure $*
    fi
fi
