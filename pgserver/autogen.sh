#!/bin/sh

# This is a small hack to install default configuration if none exists yet.
# This lets developers easily make local changes to profile.user without
# having to worry about them going into CVS. There might be a better way
# to do this.
# -- Micah
if [ ! -f profile.user ] ; then
    echo "Installing default configuration"
	 cp profile.defaults profile.user
fi

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
    run ./configure $*
fi
