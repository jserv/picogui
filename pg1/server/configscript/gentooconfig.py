# script that generates profile.user from USE variables
# edit gentooconfig_deps.py to define how that happens

import re, portage, os
usedefaults = [item.split()[0] for item in portage.usedefaults]

__builtins__.opts = {}

def use(name):
    if name in portage.usesplit:
        return True
    elif '-' + name in portage.usesplit:
        return False
    elif name in usedefaults:
        return True
    else:
        return False

__builtins__.use = use

class line(object):
    'a line in the profile'
    def __init__(self, text, match):
        self.orig_text = text.strip()
        self._parse(match)

    def _parse(self, match):
        pass

    def __str__(self):
        return self.orig_text

class emptyline(line):
    pattern = re.compile('^$')

class comment(line):
    pattern = re.compile('^#')

class valid_line(line):
    def __init__(self, text, match):
        line.__init__(self, text, match)
        opts[self.name] = self

    def __str__(self):
        if self.value is None:
            return '# %s is not set' % self.name
        else:
            return '%s=%s' % (self.name, self.value)

class unset(valid_line):
    pattern = re.compile('^# (.*) is not set$')

    def _parse(self, match):
        self.name = match.group(1)
        self.value = None

class vset(valid_line):
    pattern = re.compile('(.*)=(.*)$')

    def _parse(self, match):
        self.name, self.value = match.groups()

line_types = (unset, comment, emptyline, vset,)

config = []
for l in file('profile.user', 'r').readlines():
    for t in line_types:
        m = t.pattern.match(l)
        if m:
            config.append(t(l, m))
            break

import gentooconfig_deps
gentooconfig_deps.configure(opts)

os.rename('profile.user', 'profile.user.old')
f = file('profile.user', 'w')
for l in config:
    print >>f, l
