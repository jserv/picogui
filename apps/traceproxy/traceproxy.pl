#!/usr/bin/perl
#
# PicoGUI tracer proxy
#
# For the hexdump to work correctly, the "xxd" program needs to be installed
#
# Proxy a network connection between 1 picogui client and a server, decoding
# and printing all the communications between them. Good for debugging,
# prodding at the innards of an unknown app, or just curiosity.
#
# This whole thing is quite hackish and excruciatingly messy.
# If it turns out to be useful it should be rewritten in cleaner perl or in C
#
# -- Micah Dowty <micahjd@users.sourceforge.net>
#

use IO::Socket;

if (!$ARGV[0]) {
    print <<EOF;

usage: traceproxy.pl server[:display] [local_display]

       server: the host running pgserver
      display: optional display the pgserver is running on
local_display: display to run the proxy on

EOF
    exit 1;
}

# identifier tables (FIXME: load these from constants.h)
@requests = qw( ping update mkwidget mkbitmap mkfont mkstring free set get
		mktheme in_key in_point in_direct wait mkfillstyle register
		mkpopup sizetext batch regowner unregowner setmode getmode
		mkcontext rmcontext focus getstring dup setpayload getpayload
		chconect writeto updatepart mkarray render newbitmap thlookup
		getinactive setinactive drivermsg loaddriver getfstyle
		findwidget checkevent sizebitmap appmsg undef
		);

%evtcoding = (0x001,'activate',
	      0x002,'deactivate',
	      0x003,'close',
	      0x204,'pntr_down',
	      0x205,'pntr_up',
	      0x206,'pntr_release',
	      0x306,'data',
	      0x107,'resize',
	      0x108,'build',
	      0x209,'pntr_move',
	      0x40A,'kbd_char',
	      0x40B,'kbd_keyup',
	      0x40C,'kbd_keydown',
	      0x301,'appmsg',
	      0x140B,'nwe_kbd_keyup',
	      0x140C,'nwe_kbd_keyup',
	      0x1209,'nwe_pntr_move',
	      0x1205,'nwe_pntr_up',
	      0x1204,'nwe_pntr_down',
	      0x120D,'nwe_bgclick',
	      0x1101,'nwe_pntr_raw');

# Chop up arguments
$server = $ARGV[0];
$server =~ s/:(.*)//;
$srvdisplay = $1;
$srvdisplay = 0 if (!$srvdisplay);
$localdisplay = $ARGV[1];
$localdisplay = 0 if (!$localdisplay);

print "Connecting to pgserver on $server:$srvdisplay...\n";

$server = IO::Socket::INET->new(Proto      => 'tcp',
				PeerAddr   => $server,
				PeerPort   => 30450 + $srvdisplay)
    or die "can't connect to server";

print "Waiting for greeting packet...\n";
read($server,$pghello,8) or die $!;
($magic, $protover) = unpack "Nn", $pghello;
die "Invalid greeting packet\n" if ($magic != 0x31415926); 
printf "Connected with protocol version 0x%04X\n", $protover;

$clientsocket = IO::Socket::INET->new( Proto     => 'tcp',
				       LocalPort => 30450 + $localdisplay,
				       Listen    => SOMAXCONN,
				       Reuse     => 1)
    or die "can't listen for clients";
print "Waiting for a client on display :$localdisplay...\n";
$client = $clientsocket->accept();
$client->autoflush(1);

print "Sending greeting packet...\n";
print $client $pghello;
print "Client connected.\n";

# Now the connections are set up. Shuffle packets between client
# and server, decoding their contents.

while (1) {
    # Blank line before each real packet recieved
    print "\n";

    # Get a request packet
    read($client,$rqh,8) or die $!;
    ($reqtype,$reqid,$reqsize) = unpack("nnN",$rqh);
    read($client,$reqdata,$reqsize);
    dumprequest($reqtype,$reqdata);

    # Send it to the server
    print $server $rqh.$reqdata;

    # Get a response packet
    print "} = ";
    read($server,$rsptypeword,2) or die $!;
    $rsptype = unpack("n",$rsptypeword);
    if ($rsptype==1) {
	# Error response packet
	read($server,$rsp_err,6) or die $!;
	($rspid,$rsperrt,$rspmsglen) = unpack("nnn",$rsp_err);
	read($server,$rsperrmsg,$rspmsglen) or die $!;
	printf "ERROR 0x%04X \"%s\"\n",$rsperrt,$rsperrmsg; 
	print $client $rsptypeword.$rsp_err.$rsperrmsg;
    }
    elsif ($rsptype==2) {
	# Return value
	read($server,$rsp_ret,6) or die $!;
	($rspid,$rspdata) = unpack("nn",$rsp_ret);
	printf "%d (0x%X)\n",$rspdata,$rspdata;
	print $client $rsptypeword.$rsp_ret;
    }
    elsif ($rsptype==3) {
	# Event
	read($server,$rsp_evt,10) or die $!;
	($evt,$from,$param) = unpack("nNN",$rsp_evt);
	print "event {\n";
	# Decode type
	print " type = ".$evtcoding{$evt}."\n";
	printf " from = %d (0x%X)\n",$from,$from;
	# If this is PG_EVENTCODING_DATA, get the data */
	$evtdata = "";
	if ($evt&0xF00 == 0x300) {
	    read($server,$evtdata,$param) or die $!;
	    print "\tevent data: ($param bytes)\n";
	    hexdump($evtdata);
	}
	print "}\n";
	print $client $rsptypeword.$rsp_evt.$evtdata;
    }
    elsif ($rsptype==4) {
	# data response packet
	read($server,$rsp_data,6) or die $!;
	($id,$size) = unpack("nN",$rsp_data);
	read($server,$data,$size) or die $!;
	print "\treturn data: ($size bytes)\n";
	hexdump($data);
	print $client $rsptypeword.$rsp_data,$data;
    }
    else {
	# Have to die here, we can't recover from an unknown response type
	die "Unknown response type $rsptype\n";
    }
}

sub dumprequest {
    my ($reqtype,$data) = @_;
    print "$requests[$reqtype] {\n";
 
    # Different decoding depending on the request type
    if ($reqtype==2) { 
	# MKWIDGET
	($rship,$type,$parent) = unpack("nnN",$data);
	print " relationship = ".qw(deprecated_before after inside
				    before)[$rship]."\n";
	print " type = ".qw(toolbar label scroll indicator bitmap
			    button panel popup box field background
			    menuitem terminal canvas checkbox
			    flatbutton listitem submenuitem radiobutton
			    textbox)[$type]."\n";
	printf " parent = %d (0x%X)\n", $parent;  
    }
    # Pick apart a batch packet
    elsif ($reqtype==18) {
	while ($data) {
	    my ($type,$reqid,$reqsize) = unpack("nnN",substr($data,0,8));
	    my $reqdata = substr($data,8,$reqsize);
	    substr($data,0,8+$reqsize) = "";
	    dumprequest($type,$reqdata);
	    print "}\n";
	}
    }
    # Hex dump anything else
    else {
	hexdump($data);
    }
}

sub hexdump {
    open HEX,"|xxd";
    print HEX @_;
    close HEX;
}
