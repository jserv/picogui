# $Id: PicoGUI.pm,v 1.16 2000/06/03 18:37:47 micahjd Exp $
#
# PicoGUI client module for Perl
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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
@ISA       = qw(Exporter);
@EXPORT    = qw(NewWidget %ServerInfo Update NewString
		NewFont NewBitmap delete SetBackground RestoreBackground
		SendPoint SendKey ThemeSet RegisterApp EventLoop NewPopup);

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

%SIDE = (
	 'top' => (1<<3),
	 'bottom' => (1<<4),
	 'left' => (1<<5),
	 'right' => (1<<6),
	 'all' => (1<<11)
	 );

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
	   'button' => 5
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
	  '-bitmask' => 15
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
	  '-onblur' => 2
	  );

$MAGIC     = 0x31415926;
$PROTOVER  = 1;

@ERRT = qw( NONE MEMORY IO NETOWRK BADPARAM HANDLE INTERNAL );

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
    socket(S, 2, 1, join("", getprotobyname('tcp')))
	or croak "PicoGUI - socket(): $!\n";
    connect(S, $remote) 
	or croak "PicoGUI - connect(): $!\n";
    select((select(S),$|=1)[0]);
    
    # Now we have a socket, read the hello packet
    read S,$pkt,64;
    ($magic,$protover,$width,$height,$bpp,$title) = unpack("Nn4a50",$pkt);
    $magic==$MAGIC or 
	croak "PicoGUI - incorrect magic number ($MAGIC -> $magic)\n";

    # TODO: fix this
    $protover==$PROTOVER or croak "PicoGUI - protocol version not supported\n"; 

    %ServerInfo = (
	'Width' => $width,
	'Height' => $height,
	'BPP' => $bpp,
	'Title' => $title
	);

}

# Close the connection on exit
END {
    close(S);
}

######### Internal subs

sub _request {
    my ($type,$data) = @_;
    my ($rsp,$r_id,$ret,$errt,$msg,$rspt,$mlen);
    my ($etype,$efrom,$eparam);
    print S pack("nnN",$type,++$id,length $data);
    print S $data;

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

    croak "PicoGUI - Unexpected response type ($rspt)\n";
}

# Subs for each type of request
sub Update {
    _request(1);
}
sub _mkwidget {
    _request(2,pack('nnN',@_));
}
sub _mkbitmap {
    my ($w,$h,$fg,$bg,$bits) = @_;
    _request(3,pack('nnNN',$w,$h,$fg,$bg).$bits);
}
sub _mkfont {
    _request(4,pack('a40Nnnn',@_));
}
sub _set {
    _request(7,pack('NNnn',@_));
}
sub _get {
    _request(8,pack('Nnn',@_));
}
sub _free {
    _request(6,pack('N',@_));
}
sub _mkstring {
    _request(5,join('',@_));
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
}
sub _themeset {
    _request(14,pack('Nnnnn',@_));
}
sub _register {
    _request(15,pack('Nnnnnnnnnnn',@_));
}
sub _mkpopup {
    _request(16,pack('nnnn',@_));
}

######### Public functions

sub NewPopup {
    my ($x,$y,$w,$h) = @_;
    my $self = {-root => 1};

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

    $name = $args{-name};
    $type = $args{-type} ? $APPTYPE{$args{-type}} : $APPTYPE{'normal'};
    $side = $args{-side} ? $SIDE{$args{-side}} : $SIDE{'top'};
    $sidemask = defined($args{-sidemask}) ? $args{-sidemask} : 0xFFFF;
    $w = defined($args{-w}) ? $args{-w} : -1;
    $h = defined($args{-h}) ? $args{-h} : -1;
    $minw = defined($args{-minw}) ? $args{-minw} : -1;
    $maxw = defined($args{-maxw}) ? $args{-maxw} : -1;
    $minh = defined($args{-minh}) ? $args{-minh} : -1;
    $maxh = defined($args{-maxh}) ? $args{-maxh} : -1;

    # Bless thy self and get on with it...
    bless $self;
    $self->{'h'} = _register($name,$type,$side,$sidemask,$w,$h,$minw,
			     $maxw,$minh,$maxh);

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
	    $arg = $arg->GetHandle() if (/text/ or /bitmap/ or
					 /font/ or /bitmask/);
	    croak "Undefined property" if (!defined $prop);
	    _set($self->GetHandle(),$arg,$prop);
	}
    }
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
    my ($event,$from,$param,$r);

    # Good place for an update...  (probably the first update)
    Update();

    while (1) {     # This never returns
	($event, $from, $param) = _wait();
	
	# Call the code reference
	$r = $bindings{$from.':'.$event};
	&$r($param) if (defined $r);
    }
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

    foreach $x (keys %arg) {
	($wgt,$el,$st,$par) = split /\./,$x;
	$v = $arg{$x};
	$v = $VALUES{$v} if (defined $VALUES{$v});
	_themeset($v,$ELEMENT{$wgt.".".$el},$STATE{$st},$PARAM{$par});
    }
}

_init();
1;
### The End ###











