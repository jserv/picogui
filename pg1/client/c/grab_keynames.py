#!/usr/bin/python

import sys, os

if len (sys.argv) > 1:
    pgkeys = file (sys.argv[1])
else:
    pgkeys = file('../../server/include/picogui/pgkeys.h')

keynames = {}
max_key = 0

for line in pgkeys.readlines():
    if not line.startswith ('#define PGKEY'):
        continue

    parts = line.strip().split()[1:]

    if parts[0] == 'PGKEY_MAX':
        max_key = int (parts[1])
        break

    keynames[int (parts[1])] = parts[0][6:].lower()
pgkeys.close()

if not max_key:
    max_key = max (keynames.keys())


out = file ('keynames.c', 'w')
print >> out, '''/*
* AUTO-GENERATED FILE, do not edit
*/

char *_pgKeyNames[] = {'''

for i in range(max_key + 1):
    if keynames.has_key (i):
        s = '"%s",' % keynames[i]
    else:
        s = '"",'
    print >> out, s, ' ' * (50 - len(s)), '/* %d */' % i

print >> out, '};'
out.close()
