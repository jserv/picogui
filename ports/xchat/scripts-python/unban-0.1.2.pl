#
# unban.pl - unban by number ;-)
#
# Usage:
#      /banlist [channel]             - show banlist
#      /unban nr[,nr[...]] [channel]  - unban the ban(s) with number nr 
#                                       (from /banlist) on channel 
#      /exlist [channel]              - show exempt ban list
#      /unex nr[,nr[...]] [channel]   - remove exempt ban with number nr 
#                                        (from /exlist) on channel 
#      /invlist [channel]             - show always invite list
#      /uninv nr[,nr[...]] [channel]  - remove always invite entry with 
#                                       number nr (from /invlist) on channel 
# ...if chan is not given, uses current window's channel (if there is one)  
# 
# Example:
# $ /banlist
# +--------[ ban list #a_channel ]----------------------------
# |  1: *!*@p3E9BF205.dip.t-dialin.net
# |  2: *!*metaller@p3EE073A1.dip.t-dialin.net
# |  3: *!*magic*@*
# |  4: *!*@*magic-internet.de
# $ /unban 1,2
# --- Vetinari sets modes [#a_channel -bb *!*@p3E9BF205.dip.t-dialin.net *!*metaller@p3EE073A1.dip.t-dialin.net]
#
# Changes:
#   0.1.1: support mutiple bans to be removed at once.
#          HINT: do not use more numbers than your ircserver supports, 
#          i.e. 3 on IRCnet
#   0.1.2: if <NR> contains an @, the command /mode #channel -b $nr 
#          is executed
#
# Author: Vetinari <iranitev@gmx.net>
#
package IRC::XChat::UnBan;

my $script_name    = "Vetinari's unban.pl";
my $script_version = '0.1.2';

IRC::register($script_name, $script_version, "", "");
IRC::print("\cC0\cB$script_name\cB version\cC3 $script_version\cO "
           . "by \cB\cC4V\cC7etinari\cO loading...\n");

use vars qw(%invlist %exlist %banlist);
my $pkg = __PACKAGE__;

IRC::add_command_handler("BANLIST", "${pkg}::handle_banlist");
IRC::add_command_handler("UNBAN",   "${pkg}::handle_unban");
IRC::add_message_handler("367",     "${pkg}::handle_367");
IRC::add_message_handler("368",     "${pkg}::handle_368");

IRC::add_command_handler("EXLIST",  "${pkg}::handle_exlist");
IRC::add_command_handler("UNEX",    "${pkg}::handle_unex");
IRC::add_message_handler("348",     "${pkg}::handle_348");
IRC::add_message_handler("349",     "${pkg}::handle_349");

IRC::add_command_handler("INVLIST", "${pkg}::handle_invlist");
IRC::add_command_handler("UNINV",   "${pkg}::handle_uninv");
IRC::add_message_handler("346",     "${pkg}::handle_346");
IRC::add_message_handler("347",     "${pkg}::handle_347");

sub trim {
	local($_) = shift;
	s/^\s*//;
	s/\s*$//;
	return $_;
}

sub handle_uninv {
	my $nr   = trim(shift);
	my $chan = "";
	if ($nr =~ /\s/) {
		($nr,$chan) = split /\s+/, $nr;
	} 
	if ($nr =~ /\@/) {
		IRC::command("/mode ".IRC::get_info(2)." -I $nr");
		return 1;
	} elsif ($nr =~ /,/) {
		@nrs = split /,/, $nr;
	} else {
		$nrs[0] = $nr;
	}
	if (!defined $chan || $chan =~ /^$/) {
		$chan = IRC::get_info(2);
		if ($chan =~ /^$/) {
			IRC::print("UNINV: usage: /UNINV <NR>[,<NR>[...]] [<CHAN>]\n");
			return 1;
		}
	}

	my @remove = ();
	foreach my $num (@nrs) {
		my $mask = $invlist{lc $chan}[$num-1];
		if (!defined $mask || $mask =~ /^$/) {
			IRC::print("UNINV: number '$num' is not in invite list\n");
		} else {
			push @remove, $mask;
		}
	}
	if (@remove) {
		my $I    = 'I' x (scalar @remove);
		my $list = join " ", @remove;
		IRC::send_raw("MODE $chan -$I $list\r\n");
	}
	@{$invlist{lc $chan}} = ();
	return 1;
}

sub handle_invlist {
	my $chan = trim(shift);
	if (!defined $chan || $chan =~ /^\s*$/) {
		$chan = IRC::get_info(2);
	}
	IRC::send_raw("MODE $chan +I\r\n");
	@{$invlist{lc $chan}} = ();
	return 1;
}

sub handle_346 {
	my $line = trim(shift);
	$line =~ /:\S+\s+346\s+\S+\s+(\S+)\s+(\S+)/;
	my ($chan,$mask) = ($1,$2);
	push @{$invlist{lc $chan}}, $mask;
	return 1;
}
	
sub handle_347 {
	my $line = trim(shift);
	$line =~ /:\S+\s+347\s+\S+\s+(\S+)\s+:.*/;
	my $chan = $1;
	IRC::print("\cB\cC0\015\cC9-----\cB-\cB-\cB-[ "
               . "\cC4invite list \cC0\cB$chan\cB \cC9]--\cC3\cB"
               . "---\cB-\cB-\cB----\cC14\cB----\cB-----"
               . "\cB----\cB----\n"
            );
	my $count = 1;
	foreach my $mask (@{$invlist{lc $chan}}) {
        my $nr = sprintf "% 2d", $count;
		IRC::print("\cC9|\cC0 $nr: $mask\cO\n");
        ++$count;
	}
	return 1;
}

sub handle_unex {
	my $nr   = trim(shift);
	my $chan = "";
	my @nrs  = ();
	if ($nr =~ /\s/) {
		($nr,$chan) = split /\s+/, $nr;
	} 
	if ($nr =~ /\@/) {
		IRC::command("/mode ".IRC::get_info(2)." -e $nr");
		return 1;
	} elsif ($nr =~ /,/) {
		@nrs = split /,/, $nr;
	} else {
		$nrs[0] = $nr;
	}
	if (!defined $chan || $chan =~ /^$/) {
		$chan = IRC::get_info(2);
		if ($chan =~ /^$/) {
			IRC::print("UNEX: usage: /UNEX <NR>[,<NR>[...]] [<CHAN>]\n");
			return 1;
		}
	}

	my @remove = ();
	foreach my $num (@nrs) {
		my $mask = $exlist{lc $chan}[$num-1];
		if (!defined $mask || $mask =~ /^$/) {
			IRC::print("UNEX: number '$num' is not in exempt ban list\n");
		} else {
			push @remove, $mask;
		}
	}
	if (@remove) {
		my $e    = 'e' x (scalar @remove);
		my $list = join " ", @remove;
		IRC::send_raw("MODE $chan -$e $list\r\n");

	}
	@{$exlist{lc $chan}} = ();
	return 1;
}

sub handle_exlist {
	my $chan = trim(shift);
	if (!defined $chan || $chan =~ /^\s*$/) {
		$chan = IRC::get_info(2);
	}
	IRC::send_raw("MODE $chan +e\r\n");
	@{$exlist{lc $chan}} = ();
	return 1;
}

sub handle_348 {
	my $line = trim(shift);
	$line =~ /:\S+\s+348\s+\S+\s+(\S+)\s+(\S+)/;
	my ($chan,$mask) = ($1,$2);
	push @{$exlist{lc $chan}}, $mask;
	return 1;
}
	
sub handle_349 {
	my $line = trim(shift);
	$line =~ /:\S+\s+349\s+\S+\s+(\S+)\s+:.*/;
	my $chan = $1;
	IRC::print("\cB\cC0\015\cC9-----\cB-\cB-\cB-[ "
               . "\cC4exemptban list \cC0\cB$chan\cB \cC9]--\cC3\cB"
               . "---\cB-\cB-\cB----\cC14\cB----\cB-----"
               . "\cB----\cB----\n"
            );

    my $count = 1;
	foreach my $mask (@{$exlist{lc $chan}}) {
		my $nr = sprintf "% 2d", $count;
		IRC::print("\cC9|\cC0 $nr: $mask\cO\n");
		++$count;
	}
	return 1;
}

sub handle_banlist {
	my $chan = trim(shift);
	if (!defined $chan || $chan =~ /^\s*$/) {
		$chan = IRC::get_info(2);
	}
	@{$banlist{lc $chan}} = ();
	IRC::send_raw("MODE $chan +b\r\n");
	return 1;
}

sub handle_unban {
	my $nr   = trim(shift);
	my $chan = "";
	my @nrs  = ();
	if ($nr =~ /\s/) {
		($nr,$chan) = split /\s+/, $nr;
	} 
	if ($nr =~ /\@/) {
		IRC::command("/mode ".IRC::get_info(2)." -b $nr");
		return 1;
	} elsif ($nr =~ /,/) {
		@nrs = split /,/, $nr;
	} else {
		$nrs[0] = $nr;
	}
	if (!defined $chan || $chan =~ /^$/) {
		$chan = IRC::get_info(2);
		if ($chan =~ /^$/) {
			IRC::print("UNBAN: usage: /UNBAN <NR>[,<NR>[...]] [<CHAN>]\n");
			return 1;
		}
	}

	my @remove = ();
	foreach my $num (@nrs) {
		my $mask = $banlist{lc $chan}[$num-1];
		if (!defined $mask || $mask =~ /^$/) {
			IRC::print("UNBAN: number '$num' not in ban list\n");
		} else {
			push @remove, $mask;
		}
	}
	if (@remove) {
		my $b    = "b" x (scalar @remove);
		my $list = join " ", @remove;
		IRC::send_raw("MODE $chan -$b $list\r\n");
	}
	@{$banlist{lc $chan}} = ();
	return 1;
}

sub handle_367 {
	my $line = trim(shift);
	$line =~ /:\S+\s+367\s+\S+\s+(\S+)\s+(\S+)/;
	my ($chan,$mask) = ($1,$2);
	push @{$banlist{lc $chan}}, $mask;
	return 1;
}

sub handle_368 {
	my $line = trim(shift);
	$line =~ /:\S+\s+368\s+\S+\s+(\S+)\s+:.*/;
	my $chan = $1;
	IRC::print("\cB\cC0\015\cC9-----\cB-\cB-\cB-[ "
               . "\cC4ban list \cC0\cB$chan\cB \cC9]--\cC3\cB"
               . "---\cB-\cB-\cB----\cC14\cB----\cB-----"
               . "\cB----\cB----\n"
            );
	my $count = 1;
	foreach my $mask (@{$banlist{lc $chan}}) {
		my $nr = sprintf "% 2d", $count;
		IRC::print("\cC9|\cC0 $nr: $mask\cO\n");
		++$count;
	}
	return 1;
}

1;
# end 
