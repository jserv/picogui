# $Id$
#
# PicoGUI client module for Perl - Constant tables
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2002 Micah Dowty <micahjd@users.sourceforge.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# Contributors:
#
#
#
#

package PicoGUI::Constants;

$MAGIC    = 0x31415926;
$PROTOVER = 18;
$PORT     = 30450;

# Each entry in this table is a reference to a list containing
# the request's ID, and functions to respectively pack and unpack
# the parameters and return value.
#
%requests = (
    'ping'		=> [ 0 ],
    'update'		=> [ 1 ],
    'mkwidget'		=> [ 2, sub {pack('nnN',@_)} ],
    'mkbitmap'	 	=> [ 3 ],
    'mkfont'		=> [ 4, sub {pack('a40Nnn',@_,0)} ],
    'mkstring'		=> [ 5 ],
    'free'		=> [ 6, sub {pack('N',@_)} ],
    'set'		=> [ 7, sub {pack('NNnn',@_,0)} ],
    'get'		=> [ 8, sub {pack('Nnn',@_,0)} ],
    'mktheme'		=> [ 9 ],
    'mkcursor'		=> [ 10 ],
    'mkinfilter' 	=> [ 11, sub {pack('NNN',@_)} ],
    'getresourse' 	=> [ 12, sub {pack('N',@_)} ],
    'wait'		=> [ 13 ],
    'mkfillstyle' 	=> [ 14 ],
    'register'		=> [ 15, sub {pack('Nnn',@_)} ],
    'mkpopup'		=> [ 16, sub {pack('nnnn',@_)} ],
    'sizetext'		=> [ 17, sub {pack('N',@_)}, sub {($_[0]>>16, $_[0]&0xFFFF)} ],
    'batch'		=> [ 18 ],
    'regowner'		=> [ 19, sub {pack('nn',@_,0)} ],
    'unregowner' 	=> [ 20, sub {pack('nn',@_,0)} ],
    'setmode'		=> [ 21, sub {pack('nnnnN',@_)} ],
    'getmode'		=> [ 22, undef, sub {unpack('Nnnnnn',$_[0])} ],
    'mkcontext' 	=> [ 23 ],
    'rmcontext' 	=> [ 24, sub {pack('N*',@_)} ],
    'focus'		=> [ 25, sub {pack('N',@_)} ],
    'getstring' 	=> [ 26, sub {pack('N',@_)} ],
    'dup'		=> [ 27, sub {pack('N',@_)} ],
    'setpayload' 	=> [ 28, sub {pack('N',@_)} ],
    'getpayload' 	=> [ 29, sub {pack('N',@_)} ],
    'chcontext' 	=> [ 30, sub {pack('Nnn',@_,0)} ],
    'writeto'		=> [ 31, sub {pack('N',$_[0]).$_[1]} ],
    'updatepart' 	=> [ 32, sub {pack('N',@_)} ],
    'mkarray'		=> [ 33, sub {pack('N*',@_)} ],
    'render'		=> [ 34, sub {pack('NNN*',@_)} ],
    'newbitmap' 	=> [ 35, sub {pacl('nn',@_)} ],
    'thlookup'		=> [ 36, sub {pack('nn',@_)} ],
    'getinactive' 	=> [ 37 ],
    'setinactive' 	=> [ 38, sub {pack('N',@_)} ],
    'drivermsg' 	=> [ 39, sub {pack('NN',@_)} ],
    'loaddriver' 	=> [ 40 ],
    'getfstyle' 	=> [ 41, sub {pack('nn',@_,0)}, sub {unpack('a40nnN',$_[0])} ],
    'findwidget' 	=> [ 42 ],
    'checkevent' 	=> [ 43 ],
    'sizebitmap' 	=> [ 44, sub {pack('N',@_)}, sub {($_[0]>>16, $_[0]&0xFFFF)} ],
    'appmsg'		=> [ 45, sub {pack('N',$_[0]).$_[1]} ],
    'createwidget' 	=> [ 46, sub {pack('nn',@_,0)} ],
    'attachwidget' 	=> [ 47, sub {pack("NNnn",@_,0)} ],
    'findthobj' 	=> [ 48 ],
    'traversewgt' 	=> [ 49, sub {pack('Nnn',@_)} ],
    'mktemplate' 	=> [ 50 ],
    'setcontext' 	=> [ 51, sub {pack('N',@_)} ],
    'getcontext' 	=> [ 52 ],
    'infiltersend' 	=> [ 53 ],
);
    

1;
