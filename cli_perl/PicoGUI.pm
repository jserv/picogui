# $Id: PicoGUI.pm,v 1.34 2000/10/10 00:25:32 micahjd Exp $
#
# PicoGUI client module for Perl
#
# Note: This library is very slow and incomplete. If you intend to 
# use it for anything, bug me (micah) to fix it.
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
package PicoGUI;
use Carp;
use Socket;
@ISA       = qw(Exporter);
@EXPORT    = qw(NewWidget Update NewString RestoreTheme
		NewFont NewBitmap delete SetBackground RestoreBackground
		SendPoint SendKey ThemeSet RegisterApp EventLoop NewPopup
		GetTextSize GrabKeyboard GrabPointingDevice GiveKeyboard
		GivePointingDevice ExitEventLoop EnterContext LeaveContext
		%PGKEY);

################################ Constants

%STYLES = (
	   'fixed' => (1<<0),
	   'default' => (1<<1),
	   'symbol' => (1<<2),
	   'subset' => (1<<3),
	   'extended' => (1<<4),
	   'ibmextend' => (1<<5),
	   'nobw' => (1<<6),
	   'doublespace' => (1<<7),
	   'bold' => (1<<8),
	   'italic' => (1<<9),
	   'underline' => (1<<10),
	   'strikeout' => (1<<11),
	   'grayline' => (1<<12),
	   'invert' => (1<<13),
	   'flush' => (1<<14),
	   'doublewidth' => (1<<15),
	   'italic2' => (1<<16)
	   );

%ALIGN = (
	  'center' => 0,
	  'top' => 1,
	  'left' => 2,
	  'bottom' => 3,
	  'right' => 4,
	  'nw' => 5,
	  'sw' => 6,
	  'ne' => 7,
	  'se' => 8,
	  'all' => 9
	  );
%rALIGN = reverse %ALIGN;

%SIDE = (
	 'top' => (1<<3),
	 'bottom' => (1<<4),
	 'left' => (1<<5),
	 'right' => (1<<6),
	 'all' => (1<<11)
	 );
%rSIDE = reverse %SIDE;

%RSHIPS = (
	   '-before' => 0,
	   '-after' => 1,
	   '-inside' => 2
	   );

%WTYPES = (
	   'toolbar' => 0,
	   'label' => 1,
	   'scroll' => 2,
	   'indicator' => 3,
	   'bitmap' => 4,
	   'button' => 5,
	   'box' => 8,
	   'field' => 9
	   );

%WPROP = (
	  '-size' => 1,
	  '-side' => 2,
	  '-align' => 3,
	  '-bgcolor' => 4,
	  '-color' => 5,
	  '-sizemode' => 6,
	  '-text' => 7,
	  '-font' => 8,
	  '-transparent' => 9,
	  '-bordercolor' => 10,
	  '-bordersize' => 11,
	  '-bitmap' => 12,
	  '-lgop' => 13,
	  '-value' => 14,
	  '-bitmask' => 15,
	  '-bind' => 16,
	  '-scroll' => 17,
	  '-virtualh' => 18,
	  '-hotkey' => 19
	  );

%ELEMENT = (
	    'button.border' => 0,
	    'button.fill' => 1,
	    'button.overlay' => 2,
	    'toolbar.border' => 3,
	    'toolbar.fill' => 4,
	    'scrollbar.border' => 5,
	    'scrollbar.fill' => 6,
	    'scrollind.border' => 7,
	    'scrollind.fill' => 8,
	    'scrollind.overlay' => 9,
	    'indicator.border' => 10,
	    'indicator.fill' => 11,
	    'indicator.overlay' => 12,
	    'panel.border' => 13,
	    'panel.fill' => 14,
	    'panelbar.border' => 15,
	    'panelbar.fill' => 16,
	    'popup.border' => 17,
	    'popup.fill' => 18
	    );

%STATE = (
	  'all' => 255,
	  'normal' => 0,
	  'hilight' => 1,
	  'hilighted' => 1,
	  'activate' => 2,
	  'activated' => 2,
	  'active' => 2
	  );
	   
%PARAM = (
	  'width' => 1,
	  'w' => 1,
	  'type' => 2,
	  'color' => 3,
	  'c' => 3,
	  'c1' => 3,
	  'c2' => 4,
	  'a' => 5,
	  'angle' => 5,
	  'translucent' => 6,
	  'trans' => 6,
	  't' => 6
	  );

%VALUES = (
	   'solid' => 0,
	   'dark' => -1,
	   'darken' => -1,
	   'bright' => 1,
	   'brighten' => 1,
	   'light' => 1,
	   'none' => 0,
	   'null' => 0,
	   'flat' => 1,
	   'gradient' => 2
	   );

%APPTYPE = (
	    'normal' => 1,
	    'default' => 1,
	    'toolbar' => 2
	    );

%EVENT = (
	  '-onactivate' => 1,
	  '-ondeactivate' => 2,
	  '-onclick' => 1,
	  '-onchange' => 1,
	  '-onfocus' => 1,
	  '-onunfocus' => 2,
	  '-onblur' => 2,
	  '-onchar' => 10,
	  '-onkeyup' => 11,
	  '-onkeydown' => 12,
	  '-onpointermove' => 13,
	  '-onpointerup' => 14,
	  '-onpointerdown' => 15
	  );

$MAGIC     = 0x31415926;
$PROTOVER  = 2;

@ERRT = qw( NONE MEMORY IO NETOWRK BADPARAM HANDLE INTERNAL BUSY );

%PGKEY = qw(
	    BACKSPACE   		8
	    TAB				9
	    CLEAR			12
	    RETURN			13
	    PAUSE			19
	    ESCAPE			27
	    SPACE			32
	    EXCLAIM			33
	    QUOTEDBL			34
	    HASH			35
	    DOLLAR			36
	    AMPERSAND			38
	    QUOTE			39
	    LEFTPAREN			40
	    RIGHTPAREN			41
	    ASTERISK			42
	    PLUS			43
	    COMMA			44
	    MINUS			45
	    PERIOD			46
	    SLASH			47
	    0				48
	    1				49
	    2				50
	    3				51
	    4				52
	    5				53
	    6				54
	    7				55
	    8				56
	    9				57
	    COLON			58
	    SEMICOLON			59
	    LESS			60
	    EQUALS			61
	    GREATER			62
	    QUESTION			63
	    AT				64
	    LEFTBRACKET			91
	    BACKSLASH			92
	    RIGHTBRACKET		93
	    CARET			94
	    UNDERSCORE			95
	    BACKQUOTE			96
	    a				97
	    b				98
	    c			     	99
	    d				100
	    e				101
	    f				102
	    g				103
	    h				104
	    i				105
	    j				106
	    k				107
	    l				108
	    m				109
	    n				110
	    o				111
	    p				112
	    q				113
	    r				114
	    s				115
	    t				116
	    u				117
	    v				118
	    w				119
	    x				120
	    y				121
	    z				122
	    DELETE			127
	    KP0				256
	    KP1				257
	    KP2				258
	    KP3				259
	    KP4				260
	    KP5				261
	    KP6				262
	    KP7				263
	    KP8				264
	    KP9				265
	    KP_PERIOD			266
	    KP_DIVIDE			267
	    KP_MULTIPLY			268
	    KP_MINUS			269
	    KP_PLUS			270
	    KP_ENTER			271
	    KP_EQUALS			272
	    UP				273
	    DOWN			274
	    RIGHT			275
	    LEFT			276
	    INSERT			277
	    HOME			278
	    END				279
	    PAGEUP			280
	    PAGEDOWN			281
	    F1				282
	    F2				283
	    F3				284
	    F4				285
	    F5				286
	    F6				287
	    F7				288
	    F8				289
	    F9				290
	    F10				291
	    F11				292
	    F12				293
	    F13				294
	    F14				295
	    F15				296
	    NUMLOCK			300
	    CAPSLOCK			301
	    SCROLLOCK			302
	    RSHIFT			303
	    LSHIFT			304
	    RCTRL			305
	    LCTRL			306
	    RALT			307
	    LALT			308
	    RMETA			309
	    LMETA			310
	    LSUPER			311
	    RSUPER			312
	    MODE			313
	    HELP			315
	    PRINT			316
	    SYSREQ			317
	    BREAK			318
	    MENU			319
	    POWER			320
	    EURO			321
	    LSHIFT			0x0001
	    RSHIFT			0x0002
	    SHIFT			0x0003
	    LCTRL			0x0040
	    RCTRL			0x0080
	    CTRL			0x00C0
	    LALT			0x0100
	    RALT			0x0200
	    ALT				0x0300
	    LMETA			0x0400
	    RMETA			0x0800
	    META			0x0C00
	    NUM				0x1000
	    CAPS			0x2000
	    MODE			0x4000
	    );

%LGOP = (
	 'null' => 0,
	 'none' => 1,
	 'or' => 2,
	 'and' => 3,
	 'xor' => 4,
	 'invert' => 5,
	 'invert_or' => 6,
	 'invert_and' => 7,
	 'invert_xor' => 8
	 );

################################ Code

# Open a connection to the server, parsing PicoGUI commandline options
# if they are present
sub _init {

    $default_rship = $default_parent = 0;

    # Default options
    %options = (
	'server' => 'localhost',
	'port' => '30450'
	);

    @g_args = @ARGV;
    @ARGV = ();
    foreach (@g_args) {
	if (/^--g-([a-z]*)=(\S*)/) {
	    $options{$1} = $2;
	}
	else {
	    push @ARGV,$_;
	}
    }
    
    # Establish the connection
    if ($options{'server'} =~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) {
	# IP address
	@addrs = pack('C4', split(/\./,$options{'server'}));
    } 
    else
    {
	# Do a DNS lookup
	($dummy,$dummy,$dummy,$dummy, @addrs) = 
	    gethostbyname($options{'server'});
    }
    $remote = pack("S n a4 x8", 2, $options{'port'}, $addrs[0]);
    socket(S, PF_INET, SOCK_STREAM, join("", getprotobyname('tcp')))
	or croak "PicoGUI - socket(): $!\n";
    connect(S, $remote) 
	or croak "PicoGUI - connect(): $!\n";
    select((select(S),$|=1)[0]);
    
    # Now we have a socket, read the hello packet
    read S,$pkt,8;
    ($magic,$protover) = unpack("Nn",$pkt);
    $magic==$MAGIC or 
	croak "PicoGUI - incorrect magic number ($MAGIC -> $magic)\n";

    $protover>=$PROTOVER or warn "PicoGUI - client is newer than the server. \n".
	"You might experiance compatibility problems."; 
}

# Close the connection on exit
END {
    close(S);
}

######### Internal subs

# Format a packet, store it in the buffer
sub _request {
    my ($type,$data) = @_;
    $buf .= pack("nnN",$type,++$id,length $data).$data;
    ++$numpackets;
}

# Flushes the buffer of packets - if there's only one, it gets sent as is
# More than one packet is wrapped in a batch packet
sub _flushpackets {
    return if (!$numpackets);
    if ($numpackets==1) {
	print S $buf;
    }
    else {
	print S pack("nnN",18,++$id,length $buf).$buf;
    }
    $buf = '';
    $numpackets = 0;

    # Read the response type
    read S,$rsp,2 or croak "PicoGUI - Error reading response code";
    ($rspt) = unpack("n",$rsp);
    if ($rspt==1) {
	# Error
	read S,$rsp,6 or croak "PicoGUI - Error reading error code";
	($r_id,$errt,$mlen) = unpack("nnn",$rsp);
	read S,$msg,$mlen or carp "PicoGUI - Error reading error string";
	$r_id == $id or carp "PicoGUI - incorrect packet ID ($id -> $r_id)\n";
	croak "PicoGUI - Server error of type $ERRT[$errt]: $msg\n";	
    }

    if ($rspt==2) {
	# Return code
	read S,$rsp,6 or croak "PicoGUI - Error reading return value";
	($r_id,$ret) = unpack("nN",$rsp);
	$r_id == $id or carp "PicoGUI - incorrect packet ID ($id -> $r_id)\n";
	return $ret;
    }
    
    if ($rspt==3) {
	# Event
	read S,$rsp,10 or croak "PicoGUI - Error reading event";
	return unpack("nNN",$rsp);
    }

    if ($rspt==4) {
	# Data
	read S,$rsp,6 or croak "PicoGUI - Error reading return data header";
	($r_id,$size) = unpack("nN",$rsp);
	$r_id == $id or carp "PicoGUI - incorrect packet ID ($id -> $r_id)\n";
	read S,$data,$size or croak "PicoGUI - Error reading return data";
	return $data;
    }

    croak "PicoGUI - Unexpected response type ($rspt)\n";
}

# Subs for each type of request
sub Update {
    _request(1);
    _flushpackets();
}
sub _mkwidget {
    _request(2,pack('nnN',@_));
    _flushpackets();
}
sub _mkbitmap {
    my ($w,$h,$fg,$bg,$bits) = @_;
    _request(3,pack('nnNN',$w,$h,$fg,$bg).$bits);
    _flushpackets();
}
sub _mkfont {
    _request(4,pack('a40Nnnn',@_));
    _flushpackets();
}
sub _set {
    _request(7,pack('NNnn',@_));
}
sub _get {
    _request(8,pack('Nnn',@_));
    _flushpackets();
}
sub _free {
    _request(6,pack('N',@_));
}
sub _mkstring {
    _request(5,join('',@_));
    _flushpackets();
}
sub _setbg {
    _request(9,pack('N',@_));
}
sub _in_key {
    _request(10,pack('NN',@_));
}
sub _in_point {
    _request(11,pack('Nnnnn',@_));
}
sub _in_direct {
    _request(12);
}
sub _wait {
    _request(13);
    _flushpackets();
}
sub _themeset {
    _request(14,pack('Nnnnn',@_));
}
sub _register {
    # This is broken: FIXME
    _request(15,pack('Nnn',@_));
    _flushpackets();
}
sub _mkpopup {
    _request(16,pack('nnnn',@_));
    _flushpackets();
}
sub _sizetext {
    _request(17,pack('NN',@_));
    _flushpackets();
}
sub GrabKeyboard {
    my %args = @_;

    _request(19);

    # Register event handlers here
    foreach (keys %args) {
	if (defined $EVENT{$_}) {
	    $bindings{'0:'.$EVENT{$_}} = $args{$_};
	}
	else {
	    croak "Unknown event: $_";
	}
    }
}
sub GrabPointingDevice {
    my %args = @_;
    _request(20);

    # Register event handlers here
    foreach (keys %args) {
	if (defined $EVENT{$_}) {
	    $bindings{'0:'.$EVENT{$_}} = $args{$_};
	}
	else {
	    croak "Unknown event: $_";
	}
    }
}
sub GiveKeyboard {
    _request(21);
}
sub GivePointingDevice {
    _request(22);
}
sub EnterContext {
    _request(23);
}
sub LeaveContext {
    _request(24);
}
sub _focus {
    _request(25,pack('N',@_));
}
sub _getstring {
    _request(26,pack('N',@_));
    _flushpackets();
}
sub RestoreTheme {
    _request(27);
}

######### Public functions

# Mandatory string followed by optional font object. Returns a list
# with (w,h) in it.
sub GetTextSize {
    my $x;
    $x = _sizetext($_[0]->{'h'},$_[1]->{'h'});
    return ($x>>16,$x&0xFFFF);
}

sub NewPopup {
    my ($x,$y,$w,$h) = @_;
    my $self = {-root => 1};

    # If there were only two args, it was width and height. 
    # Just center it instead.
    if (scalar(@_)<4) {
	$w = $x;
	$h = $y;
	$x = $y = -1;
    }

    bless $self;
    $self->{'h'} = _mkpopup($x,$y,$w,$h);

    # Default is inside this widget
    $default_rship = $RSHIPS{-inside};
    $default_parent = $self->{'h'};

    return $self;
}

sub RegisterApp {
    my %args = @_;
    my ($name,$type,$side,$sidemask,$w,$h,$minw,$maxw,$minh,$maxh);
    my $self = {-root => 1};

    # FIXME: This is very broken

    $name = $args{-name};
    $type = $args{-type} ? $APPTYPE{$args{-type}} : $APPTYPE{'normal'};
#    $side = $args{-side} ? $SIDE{$args{-side}} : $SIDE{'top'};
#    $sidemask = defined($args{-sidemask}) ? $args{-sidemask} : 0xFFFF;
#    $w = defined($args{-width}) ? $args{-width} : 0;
#    $h = defined($args{-height}) ? $args{-height} : 0;
#    $minw = defined($args{-minw}) ? $args{-minw} : 0;
#    $maxw = defined($args{-maxw}) ? $args{-maxw} : 0;
#    $minh = defined($args{-minh}) ? $args{-minh} : 0;
#    $maxh = defined($args{-maxh}) ? $args{-maxh} : 0;

    # Bless thy self and get on with it...
    bless $self;
    $self->{'h'} = _register($name,$type);

    # Default is inside this widget
    $default_rship = $RSHIPS{-inside};
    $default_parent = $self->{'h'};

    return $self;
}

sub GetHandle {
    my $self = shift;
    return $self->{'h'};
}

sub delete {
    my $self = shift;
    _free($self->{'h'});
}

sub focus {
    my $self = shift;
    _focus($self->{'h'});
}

sub GetString {
    my $self = shift;
    $_ = _getstring($self->{'h'});
    chop;
    return $_; 
}

sub SetBackground {
    my $self = shift;
    _setbg($self->{'h'});
}

sub RestoreBackground {
    _setbg(0);
}

sub SetWidget {
    my $self = shift;
    my %args = @_;
    my $prop;
    foreach (keys %args) {
	$arg = $args{$_};

	if ($EVENT{$_}) {
	    # We are just setting an event binding to be used in the main loop
	    $bindings{$self->{'h'}.':'.$EVENT{$_}} = $arg;
	}
	else {
	    # Setting a widget property that is passed on to the server
	    $prop = $WPROP{$_};
	    $arg = $ALIGN{$arg} if (/align/);
	    $arg = $SIDE{$arg} if (/side/);
	    $arg = $LGOP{$arg} if (/lgop/);
	    $arg = $arg->GetHandle() if (/text/ or /bitmap/ or
					 /font/ or /bitmask/ or /bind/);
	    croak "Undefined property" if (!defined $prop);
	    _set($self->GetHandle(),$arg,$prop);
	}
    }
}

sub GetWidget {
    my ($self,$prop) = @_;
    my ($arg);

    # Grab the widget property from the server
    $prop = $WPROP{$_ = $prop};
    croak "Undefined property" if (!defined $prop);
    $arg = _get($self->GetHandle(),$prop);

    # Any special formatting needed?
    
    if (/text/ or /bitmap/ or /font/ or /bitmask/ or /bind/) {
	# Handle- return as perl object
	my $obj = {};
	bless $obj;
	$obj->{'h'} = $arg;
	return $obj;
    }

    if (/side/) {
	# Reverse-lookup in %SIDE
	return $rSIDE{$arg};
    }

    if (/align/) {
	# Reverse-lookup in %ALIGN
	return $rALIGN{$arg};
    }

    return $arg;
}

sub NewFont {
    my $self = {};
    my ($name,$size,@styles) = @_;
    my $flags = 0;
    my $s;

    foreach (@styles) {
	$s = $STYLES{$_};
	croak "Undefined font style\n" if (!defined $s);
	$flags |= $s;
    }

    # Bless thy self and get on with it...
    bless $self;
    $self->{'h'} = _mkfont($name,$flags,$size);
    return $self;
}

sub NewString {
    my $self = {};
    my ($str) = @_;

    # Bless thy self and get on with it...
    bless $self;
    $self->{'h'} = _mkstring($str);
    return $self;
}

sub NewWidget {
    my $self = {};
    my %args = @_;
    my %set_arg;
    my ($type,$typenam,$rship,$parent,$h);
    $type = undef;

    # Default placement is after the previous widget
    # (Unless is was a special widget, like a root widget)
    $rship = $default_rship;
    $parent = $default_parent;

    foreach (keys %args) {
	if ($_ eq '-type') {
	    $typenam = $args{$_}; 
	    $type = $WTYPES{$args{$_}};
	    croak "Undefined widget type\n" if (!defined $type);
	}
	elsif (defined $RSHIPS{$_}) {
	    $rship = $RSHIPS{$_};
	    $parent = ($args{$_})->GetHandle();
	}
	else {
	    $set_arg{$_} = $args{$_};
	}
    }
    croak "Widget type was not specified\n" if (!defined $type);
    $h = _mkwidget($rship,$type,$parent);

    # Important stuff done, now make perl happy
    bless $self;
    $self->{'h'} = $h;
    $self->{'type'} = $typenam;

    $self->SetWidget(%set_arg);

    # Default is after this widget
    $default_rship = $RSHIPS{-after};
    $default_parent = $h;

    return $self;
}

# If w and h are specified, it is assumed that
# the data is in XBM format.  PNM doesn't need
# any params except for data or file.
sub NewBitmap {
    my $self = {};
    my %args = @_;
    my ($w,$h,$fg,$bg,$data);
    $w = $h = $fg = $bg = 0;
    $data = '';

    foreach (keys %args) {
	if ($_ eq '-width') {
	    $w = $args{$_};
	}
	elsif ($_ eq '-height') {
	    $h = $args{$_};
	}
	elsif ($_ eq '-fgcolor') {
	    $fg = $args{$_};
	}
	elsif ($_ eq '-bgcolor') {
	    $bg = $args{$_};
	}
	elsif ($_ eq '-data') {
	    $data = $args{$_};
	}
	elsif ($_ eq '-file') {
	    open BFILE,$args{$_} or croak
		"Error opening bitmap file: $args{$_}\n";
	    $data = join('',<BFILE>);
	    close BFILE;
	}
	else {
	    croak "Unknown parameter to NewBitmap\n";
	}
    }
    $h = _mkbitmap($w,$h,$fg,$bg,$data);

    # Ditto...
    bless $self;
    $self->{'h'} = $h;
    return $self;
}

# This is called after the app finishes it's initialization.
# It waits for events, and dispatches them to the functions they're
# bound to.
sub EventLoop {
    my $fromobj = {};

    $eventloop_on = 1;

    # Good place for an update...  (probably the first update)
    Update();

    while ($eventloop_on) {
	($event, $from, $param) = _wait();
	
	# Package the 'from' handle in an object
	bless $fromobj;
	$fromobj->{'h'} = $from;

	# Call the code reference
	$r = $bindings{$from.':'.$event};
	&$r($fromobj,$param) if (defined $r);
    }
}

sub ExitEventLoop {
    $eventloop_on = 0;
}

sub SendKey {
    my ($type,$key) = @_;
    _in_key($type,$key);
}

sub SendPoint {
    my ($type,$x,$y,$btn) = @_;
    _in_point($type,$x,$y,$btn);
}

sub ThemeSet {
    my %arg = @_;
    my $wgt,$el,$st,$par,$x,$v;
    my @prefices;

    foreach $x (keys %arg) {
	$v = $arg{$x};
	$x =~ s/^-//;
	
	if ($x eq 'file') {
	    open THEMEF,$v or croak "Error opening theme file";
	    while (<THEMEF>) {

		s/#.*//;
		next if (!/\S/);
		if (/\}/) {
		    pop @prefices;
		}
		elsif (/(\w+)\W*\{/) {
		    push @prefices, $1;
		}
		elsif (/\s*(\S+)\s*=\s*(\S+)/) {
		    $th = (join('.',@prefices).(@prefices?'.':'').$1);
		    $v = eval($2);
		    ThemeSet($th => $v);
		}
	    }
	    close THEMEF;
	}
	elsif ($x eq 'background') {
	    if ($v eq 'default' or $v eq 'none' or $v eq 'restore') {
		RestoreBackground();
	    }
	    else {
		NewBitmap(-file => $v)->SetBackground();
	    }
	}
	else {
	    ($wgt,$el,$st,$par) = split /\./,$x;
	    if (!defined $par) {
		# If the state is ommitted, assume 'all'
		$par = $st;
		$st = 'all';
	    }

	    $v = $VALUES{$v} if (defined $VALUES{$v});
	    _themeset($v,$ELEMENT{$wgt.".".$el},$STATE{$st},$PARAM{$par});
	}
    }
}

# A little shortcut that sets the text of a widget, freeing the memory
# used by the old text
sub ReplaceText {
    my $self = shift;
    my $oldtxt;
    
    $oldtxt = $self->GetWidget(-text);
    $self->SetWidget(-text => NewString($_[0]));
    $oldtxt->delete if ($oldtxt && $oldtxt->{'h'});
}

_init();
1;
### The End ###
