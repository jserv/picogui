#!/usr/bin/perl -w
# VIM: set ts=4, set sw=4
#
# alias.pl - make alias commands
#
# usage: /alias NEWCMD COMMANDS
#        /unalias NEWCMD
#
# NEWCMD:   will be the name of the new command. 
# COMMANDS: list of commands seperated by ; (a semicolon). the usual xchat
#           substitutions &N or %M (where N and M are numbers) will work.
#           The other substitutions (%c, %n, %t, %v but NOT %m) are also 
#           valid.
#           ...if you need the server name, uncomment the line 
#               #  $string =~ s/\%s/IRC::get_info(3)/ge; # server
#           in substitute().
# 
# Any new command which was not removed via "/unalias CMD" on script 
# shutdown (client exit or script unload) will be saved in $alias_file
# (see below) and reloaded on the next startup of the script.
# 
# Author: Vetinari <iranitev@gmx.net>
#
# Changes: 
#     from 0.1.0: 
#        - added %c, %n, %t, %v substitutions
#        - moved the subtitutions to a separate sub
#        - better (hehe) command parsing
# 
package IRC::XChat::Alias;

### config ### 
my $alias_file = "$ENV{'HOME'}/.xchat/alias.conf";
### end config ###

my $script_name    = "alias.pl";
my $script_version = '0.1.1';
my $pkg = __PACKAGE__;

IRC::register($script_name, $script_version, "${pkg}::save_aliases", "");
IRC::print("\cC0\cB$script_name\cB version\cC3 $script_version\cO "
           . "by \cB\cC4V\cC7etinari\cO loading...\n");

use vars qw(%aliases);

IRC::add_command_handler("ALIAS",   "${pkg}::cmd_alias");
IRC::add_command_handler("UNALIAS", "${pkg}::cmd_unalias");

load_aliases();

sub trim {
    local $_ = shift;
    s/^\s*//;
    s/\s*$//;
    return $_;
}

sub substitute {
    my $string = shift;
    my @args   = @_;
    my $date   = scalar localtime();
    $date   =~ s/\s+\d+$//;
    $string =~ s/\%v/IRC::get_info(0)/ge;
    $string =~ s/\%n/IRC::get_info(1)/ge;
    $string =~ s/\%c/IRC::get_info(2)/ge;
  #  $string =~ s/\%s/IRC::get_info(3)/ge; # server
    $string =~ s/\%t/$date/g;
    $string =~ s/(\%)(\d+)/$args[$2-1]/g;
  #  IRC::print("subtitute(): $string\n");
    $string =~ s/(\&)(\d+)/join(" ",@args[$2-1 .. $#args])/ge;
  #  IRC::print("subtitute(): $string\n");
    return $string;    
}

sub cmd_alias {
    my $line = trim(shift);
    $line    =~ s/^(\S+)//;
    my $name = lc $1;
    if (!defined $name || $name =~ /^$/) {
        $name = join ", ", keys %aliases;
        IRC::print("ALIAS: defined aliases: $name\n");
        return 1;
    }

    $line =~ s/^\s*//;
    if ($line =~ /^$/) {
        if (exists $aliases{$name}) {
            IRC::print("ALIAS for '$name': $aliases{$name}\n");
        } else {
            IRC::print("ALIAS: no alias for '$name'\n");
        }
        return 1;
    }

    if (exists $aliases{$name}) {
        IRC::print("ALIAS: overwriting existing alias '$name'\n");
        undef &{"${pkg}::alias_$name"};
    }
    my $code = "# alias code:
sub alias_$name {
  my \$arg = trim(shift);
  my \@args = split /\\s+/, \$arg;
  unshift \@args, '$name'; # %1 in xchat's user commands is the command name
  my \@cmds = split /\\s*;\\s*/, \$aliases{'$name'};
  foreach my \$cmd (\@cmds) {
    # IRC::print(\"CMD(0): \$cmd\");
    \$cmd = substitute(\$cmd, \@args);
    # IRC::print(\"CMD(1): \$cmd\");
    IRC::command(\"\$cmd\");
  }
  return 1;
}\n";
    # IRC::print("$code");
    eval $code;
    if ($@) {
        IRC::print("ALIAS: eval error: $@\n");
    } else {
        IRC::add_command_handler("$name", "${pkg}::alias_$name");
        $aliases{$name} = $line;
        IRC::print("ALIAS: added '$name' to aliases\n");
    }
	return 1;
}

sub cmd_unalias {
    my $name = lc trim(shift);
    unless (exists $aliases{$name}) {
        IRC::print("UNALIAS: no alias for '$name'\n");
        return 1;
    }
    delete $aliases{$name};
    undef &{"${pkg}::alias_$name"};
    eval "sub alias_$name { return 0 }";
    return 1;
}

sub save_aliases {
    open F, ">$alias_file"
        or do {
            IRC::print("ALIAS: Couldn't open alias file: $!\n");
            return 1;
        };
    print F "# $alias_file - config for alias.pl\n";
    foreach my $line (keys %aliases) {
        IRC::print("SAVE: $line, $aliases{$line}\n");
        print F $line," ",$aliases{$line},"\n";
    }
    close F;
    return 1;
}

sub load_aliases {
    IRC::print("ALIAS: ...loading aliases:\n");
    open F, $alias_file
        or do {
            IRC::print("load_aliases(): not loading aliases: $!\n");
            return 1;
        };
    my $line;
    while (defined ($line = <F>)) {
        next if $line =~ /^\s*#/;
        chomp $line;
        IRC::command("/alias $line");
    }
    close F;
}

1;
#end
