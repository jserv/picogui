#!/usr/bin/perl
#
# PicoGUI tracer proxy
#
# This requires Perl, Netcat, and xxd
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


# identifier tables (FIXME: load these from constants.h)
@requests = qw( ping update mkwidget mkbitmap mkfont mkstring free set get
		mktheme mkcursor mkinfilter getresource wait mkfillstyle register
		mkpopup sizetext batch regowner unregowner setmode getmode
		mkcontext rmcontext focus getstring dup setpayload getpayload
		chconect writeto updatepart mkarray render newbitmap thlookup
		getinactive setinactive drivermsg loaddriver getfstyle
		findwidget checkevent sizebitmap appmsg createwidget attachwidget
		undef
		);

@rshiplist =  qw(deprecated_before after inside before);
@widgetlist = qw(toolbar label scroll indicator bitmap
		 button panel popup box field background
		 menuitem terminal canvas checkbox
		 flatbutton listitem submenuitem radiobutton
		 textbox);


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
	      0x140A,'nwe_kbd_char',
	      0x140B,'nwe_kbd_keyup',
	      0x140C,'nwe_kbd_keyup',
	      0x1209,'nwe_pntr_move',
	      0x1205,'nwe_pntr_up',
	      0x1204,'nwe_pntr_down',
	      0x120D,'nwe_bgclick',
	      0x1101,'nwe_pntr_raw',
	      0x1301,'nwe_calib_penpos',
	      0x1001,'nwe_theme_inserted',
	      0x1002,'nwe_theme_removed');


if (!$ARGV[0]) {
    print <<EOF;

usage: traceproxy.pl server[:display] [local_display]

       server: the host running pgserver
      display: optional display the pgserver is running on
local_display: display to run the proxy on

EOF
    exit 1;
}

# Chop up arguments
$server = $ARGV[0];
$server =~ s/:(.*)//;
$srvport = 30450 + $1;
$localport = 30450 + $ARGV[1];

# make fifos for request and response packets
`rm -f requests-fifo; mkfifo requests-fifo`;
`rm -f responses-fifo; mkfifo responses-fifo`;
`rm -f responses2-fifo; mkfifo responses2-fifo`;

# Shuffle data between client and server, capturing a copy
if (fork()) {
    system "cat responses2-fifo | nc -l -p $localport | tee requests-fifo | nc $server $srvport | tee responses-fifo > responses2-fifo";
    exit();
}
open REQ,"requests-fifo";
open RSP,"responses-fifo";

# greeting packet
read(RSP,$pghello,8) or die $!;
($magic, $protover) = unpack "Nn", $pghello;
die "Invalid greeting packet\n" if ($magic != 0x31415926); 
printf "Connected with protocol version 0x%04X\n", $protover;

while (1) {
    print "\n";

    # Get a request packet
    read(REQ,$rqh,12) or die $!;
    ($reqid,$reqsize,$reqtype) = unpack("NNn",$rqh);
    read(REQ,$reqdata,$reqsize);
    dumprequest($reqtype,$reqdata);

    # Get a response packet
    read(RSP,$rsptypeword,2) or die $!;
    $rsptype = unpack("n",$rsptypeword);
    if ($rsptype==1) {
	# Error response packet
	read(RSP,$rsp_err,10) or die $!;
	($rsperrt,$rspmsglen,$dummy,$rspid) = unpack("nnnN",$rsp_err);

	# if the IDs don't match, print request packets until they do
	while ($rspid != $reqid) {
	    print "}\n\n";
	    read(REQ,$rqh,12) or die $!;
	    ($reqid,$reqsize,$reqtype) = unpack("NNn",$rqh);
	    read(REQ,$reqdata,$reqsize);
	    dumprequest($reqtype,$reqdata);
	}

	read(RSP,$rsperrmsg,$rspmsglen) or die $!;
	print "} = ";
	printf "ERROR 0x%04X \"%s\"\n",$rsperrt,$rsperrmsg; 
    }
    elsif ($rsptype==2) {
	# Return value
	read(RSP,$rsp_ret,10) or die $!;
	($dummy,$rspid,$rspdata) = unpack("nNN",$rsp_ret);

	# if the IDs don't match, print request packets until they do
	while ($rspid != $reqid) {
	    print "}\n\n";
	    read(REQ,$rqh,12) or die $!;
	    ($reqid,$reqsize,$reqtype) = unpack("NNn",$rqh);
	    read(REQ,$reqdata,$reqsize);
	    dumprequest($reqtype,$reqdata);
	}

	print "} = ";
	printf "%d (0x%X)\n",$rspdata,$rspdata;
    }
    elsif ($rsptype==3) {
	# Event
	read(RSP,$rsp_evt,10) or die $!;
	($evt,$from,$param) = unpack("nNN",$rsp_evt);
	print "} = ";
	print "event {\n";
	# Decode type
	print " type = ".$evtcoding{$evt}."\n";
	printf " from = %d (0x%X)\n",$from,$from;
	printf " param = %d (0x%X)\n",$param,$param;
	# If this is PG_EVENTCODING_DATA, get the data */
	$evtdata = "";
	if (($evt >> 8) == 3) {
	    read(RSP,$evtdata,$param) or die $!;
	    print " data = {\n";
	    hexdump($evtdata);
	    print " }\n";
	}
	print "}\n";
    }
    elsif ($rsptype==4) {
	# data response packet
	read(RSP,$rsp_data,10) or die $!;
	($dummy,$id,$size) = unpack("nNN",$rsp_data);
	read(RSP,$data,$size);
	print "} = ";
	print " data {\n";
	hexdump($data);
	print " }\n";
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
	my ($rship,$type,$parent) = unpack("nnN",$data);
	print " relationship = ".$rshiplist[$rship]."\n";
	print " type = ".$widgetlist[$type]."\n";
	printf " parent = %d (0x%X)\n", $parent, $parent;  
    }
    # Pick apart a batch packet
    elsif ($reqtype==18) {
	while ($data) {
	    my ($reqid,$reqsize,$type) = unpack("NNn",substr($data,0,12));
	    my $reqdata = substr($data,12,$reqsize);
	    my $pktsize = 12 + $reqsize;
	    # Pad to 32-bit boundary
	    $pktsize += 4 - ($pktsize & 3) if ($pktsize & 3);
	    substr($data,0,$pktsize) = "";
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
