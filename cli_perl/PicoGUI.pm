#############################################################################
#
# PicoGUI client module for Perl
# $Revision: 1.1 $
#
# Micah Dowty <micah@homesoftware.com>
#
# This file is released under the GPL. Please see the file COPYING that
# came with this distribution.
#
#############################################################################
package PicoGUI;
require Exporter;
@ISA       = qw(Exporter);
@EXPORT    = qw(NewWidget %ServerInfo Update NewString
		NewFont NewBitmap delete);

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
	 'right' => (1<<6)
	 );

%RSHIPS = (
	   '-before' => 0,
	   '-after' => 1,
	   '-inside' => 2
	   );

%WTYPES = (
	   'panel' => 0,
	   'label' => 1,
	   'scroll' => 2,
	   'indicator' => 3,
	   'bitmap' => 4
	   );

%WPROP = (
	  'panel-size' => 0,
	  'panel-side' => 1,
	  'panel-color' => 2,
	  'panel-bordercolor' => 3,
	  'panel-bordersize' => 4,
	  'panel-sizemode' => 5,
	  'label-side' => 0,
	  'label-color' => 1,
	  'label-font' => 2,
	  'label-text' => 3,
	  'label-align' => 4,
	  'label-transparent' => 5,
	  'bitmap-side' => 0,
	  'bitmap-color' => 1,
	  'bitmap-bitmap' => 2,
	  'bitmap-lgop' => 3,
	  'bitmap-align' => 4,
	  'bitmap-transparent' => 5,
	  'scroll-value' => 0,
	  'indicator-value' => 0,
	  'indicator-side' => 1,
	  'indicator-width' => 2
	  );

$MAGIC     = 0x31415926;
$PROTOVER  = 1;

@ERRT = qw( NONE MEMORY IO NETOWRK BADPARAM HANDLE INTERNAL );

################################ Code

# Open a connection to the server, parsing PicoGUI commandline options
# if they are present
sub _init {
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
	or die "PicoGUI - socket(): $!\n";
    connect(S, $remote) 
	or die "PicoGUI - connect(): $!\n";
    select((select(S),$|=1)[0]);
    
    # Now we have a socket, read the hello packet
    read S,$pkt,64;
    ($magic,$protover,$width,$height,$bpp,$title) = unpack("Nn4a50",$pkt);
    $magic==$MAGIC or 
	die "PicoGUI - incorrect magic number ($MAGIC -> $magic)\n";

    # TODO: fix this
    $protover==$PROTOVER or die "PicoGUI - protocol version not supported\n"; 

    %ServerInfo = (
	'Width' => $width,
	'Height' => $height,
	'BPP' => $bpp,
	'Title' => $title
	);

}

# Close the connection on exit
END {
    close(S) || die "close: $!";
}

######### Internal subs

sub _request {
    my ($type,$data) = @_;
    my ($rsp,$r_id,$ret,$errt,$msg);
    print S pack("nnN",$type,++$id,length $data);
    print S $data;
    read S,$rsp,88;
    ($r_id,$errt,$ret,$msg) = unpack("nnNa56",$rsp);
    $r_id == $id or die "PicoGUI - incorrect packet ID ($id -> $r_id)\n";
    $errt and die "PicoGUI - ERROR($ERRT[$errt]) $msg\n";
    return $ret;
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

######### Public functions

sub GetHandle {
    my $self = shift;
    return $self->{'h'};
}

sub delete {
    my $self = shift;
    _free($self->{'h'});
}

sub SetWidget {
    my $self = shift;
    my %args = @_;
    my $prop;
    foreach (keys %args) {
	$prop = $WPROP{$self->{'type'}.$_};
	$arg = $args{$_};
	$arg = $ALIGN{$arg} if (/align/);
	$arg = $SIDE{$arg} if (/side/);
	$arg = $arg->GetHandle() if (/text/ or /bitmap/ or /font/);
	die "Undefined property" if (!defined $prop);
	_set($self->GetHandle(),$arg,$prop);
    }
}

sub NewFont {
    my $self = {};
    my ($name,$size,@styles) = @_;
    my $flags = 0;
    my $s;

    foreach (@styles) {
	$s = $STYLES{$_};
	die "Undefined font style\n" if (!defined $s);
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
    $rship = $parent = 0;
    foreach (keys %args) {
	if ($_ eq '-type') {
	    $typenam = $args{$_}; 
	    $type = $WTYPES{$args{$_}};
	    die "Undefined widget type\n" if (!defined $type);
	}
	elsif (defined $RSHIPS{$_}) {
	    $rship = $RSHIPS{$_};
	    $parent = ($args{$_})->GetHandle();
	}
	else {
	    $set_arg{$_} = $args{$_};
	}
    }
    die "Widget type was not specified\n" if (!defined $type);
    $h = _mkwidget($rship,$type,$parent);

    # Important stuff done, now make perl happy
    bless $self;
    $self->{'h'} = $h;
    $self->{'type'} = $typenam;

    $self->SetWidget(%set_arg);

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
	    open BFILE,$args{$_} or die
		"Error opening bitmap file: $args{$_}\n";
	    $data = join('',<BFILE>);
	    close BFILE;
	}
	else {
	    die "Unknown parameter to NewBitmap\n";
	}
    }
    $h = _mkbitmap($w,$h,$fg,$bg,$data);

    # Ditto...
    bless $self;
    $self->{'h'} = $h;
    return $self;
}

_init();
1;
### The End ###











