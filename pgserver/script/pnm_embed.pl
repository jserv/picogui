#!/usr/bin/perl
#
# Munge a binary PNM file into a header file to be directly compiled
# into a program
#
# Micah Dowty <micah@homesoftware.com>
#

print "unsigned char ${ARGV[0]}_bits[] = {\n";
while (read STDIN,$c,1) {
    printf("0x%02X, ",ord($c));
    print "\n" if (!((++$i) % 10));
}
print "\n};\n#define ${ARGV[0]}_len $i\n\n";

