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

run aclocal ${aclocal_extra}
run autoheader
run autoconf
run automake -a

if [ -x ./config.status ] ; then
    run ./config.status --recheck
    run ./config.status
else
    run ./configure $*
fi
