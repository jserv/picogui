# $Id$
#
# PicoGUI client module for Perl - TCP/IP PicoGUI server module
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

package PicoGUI::Server;
use Carp;
use IO::Socket;
use Socket;

sub new {
    my $self = {};
    my $that = shift;
    my $class = ref($that) || $that;
    bless $self, $class;
    $self->_connect(@_);
    return $self;
}

# Connect to the PicoGUI server, and accept the Hello packet.
# Accepts the options -server and -display
#
sub _connect {
    my $self = shift;
    my %options = @_;
    my ($pkt, $magic, $protover);

    $options{'-server'} = 'localhost' if (!$options{'-server'});
    $options{'-display'} = 0 if (!$options{'-display'});

    $self->{SOCKET} = new IO::Socket::INET (PeerAddr => $options{'-server'},
					    PeerPort => $options{'-display'} + 
					                $PicoGUI::Constants::PORT,
					    Proto    => 'tcp');
    $self->{SOCKET} or
	croak "PicoGUI - Can't connect to server\n";

    # Important! When using TCP to talk to the PicoGUI server, we need to 
    # turn on TCP_NODELAY, otherwise the round-trip time for a picogui request
    # will be way too high.
    # NOTE: I know this is ugly.. but I couldn't get it to work using the Socket
    #       module's constants or the setopt function.
    setsockopt($self->{SOCKET},6,1,1);

    # Now we have a socket, read the hello packet
    read $self->{SOCKET}, $pkt, 8;
    ($magic,$protover) = unpack('Nn',$pkt);

    $magic == $PicoGUI::Constants::MAGIC or 
	croak "PicoGUI - incorrect magic number ($MAGIC -> $magic)\n";
    
    $protover>= $PicoGUI::Constants::PROTOVER or warn "PicoGUI - client is newer than the server. \n".
	"You might experiance compatibility problems."; 
}

sub _send_request {
    my $self = shift;
    my ($type, $data, $id) = @_;
    $id = 0 if (!defined $id);
    $data = "" if (!defined $data);
    $self->{SOCKET}->send(pack('NNnn', $id, length $data, $type, 0));
    $self->{SOCKET}->send($data);
}

sub _recv_response {
    my $self = shift;
    my ($type,$pkt,$errt,$len,$dummy,$id,$data,$event,$from);

    # By coincidence, all the return packets so far are 12 bytes
    read $self->{SOCKET}, $pkt, 12;
    $type = unpack('n', $pkt);

    # Error packet
    if ($type == 1) {
	($type, $errt, $len, $dummy, $id) = unpack('nnnnN',$pkt);
	read $self->{SOCKET}, $data, $len;
	croak "PicoGUI error: $data\n";
    }

    # Return packet
    if ($type == 2) {
	($type, $dummy, $id, $data) = unpack('nnNN',$pkt);
	return $data;
    }

    # Event packet
    if ($type == 3) {
	($type, $event, $from, $data) = unpack('nnNN',$pkt);
	
	# If this event has data, get that too
	if (($event & 0xF00) == 0x300) {
	    $len = $data;
	    read $self->{SOCKET}, $data, $len;
	}

	return ($event,$from,$data);
    }

    # Data packet
    if ($type == 4) {
	($type, $dummy, $id, $len) = unpack('nnNN',$pkt);
	read $self->{SOCKET}, $data, $len;
	return $data;
    }

    croak "PicoGUI - Unknown response type\n";
}

sub _send_and_wait {
    # This is the proper place to add thread support
    my $self = shift;
    $self->_send_request(@_);
    return $self->_recv_response(@_);
}
	 
sub DESTROY {
    # Nothing to clean up, but we don't want AUTOLOAD catching this
}

# Use autoload so we can call requests as if they're functions in 
# the server object. The requests table has optional handlers for
# packing arguments and unpacking results.
#
sub AUTOLOAD {
    my $self = shift;
    my $type = ref($self) || croak "$self is not an object";
    my $name = $AUTOLOAD;
    my ($data) = @_;
    my $requestref;

    $name =~ s/.*://;      # Make the name not fully qualified
    $name = lc($name);     # Requests are all lowercase, allow mixed case for readability
    $requestref = $PicoGUI::Constants::requests{$name};
    croak "Accessing undefined request '$name' in PicoGUI::Server\n" if (!$requestref);    
    my ($req_type, $req_pack, $req_unpack) = @$requestref;

    # Optionally pack up arguments using the supplied function
    if ($req_pack) {
	$data = &$req_pack(@_);
    }

    # Actually run the request
    $data = $self->_send_and_wait($req_type,$data);

    # Optionally unpack args
    if ($req_unpack) {
	return &$req_unpack($data);
    }
    return $data;
}

1;
### The End ###
