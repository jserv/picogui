import sys, pax, os
try:
    from name_mappings import mappings
except:
    mappings = {}

f = file('../../html/book-outline.html')
outline = f.read()
f.close()
del f

includes = file('includes.txt', 'w')

tree = pax.html2pax(outline)

decorations = (
    '*',
    '=',
    '-',
    '.',
)

def find_dir(title):
    n = title.lower()
    if os.path.isdir(n):
        return n
    w = n.split()
    if os.path.isdir(w[0]):
        return w[0]
    if os.path.isdir(w[-1]):
        return w[-1]
    return None

def init_dir(dir, title):
    if dir is None:
        return
    name = '%s/%s.txt' % (dir, dir)
    print >>includes, '.. include::', name
    print >>includes
    return file(name, 'w')

def find_file(dir, title, index):
    if mappings.has_key((dir, title)):
        title = mappings[(dir, title)]
    else:
        title = title.lower()
        if ' ' in title:
            alt = raw_input('Title %r is too long.\n'
                            'Input an alternate filename, or ENTER to use it anyway: '
                            % title)
            if alt:
                mappings[(dir, title)] = alt
                title = alt
    name = '%s/%s.txt' % (dir, title)
    print >>index, '.. include:: %s.txt' % title
    print >>index
    return file(name, 'w')

def walk_element(self, level, dir, index, f):
    parent_f = f
    if hasattr(self, 'name'):
        if self.name == 'li':
            title = self.children[0].strip()
            if level == 0: # directory
                dir = find_dir(title)
                print 'entering dir', dir
                f = index = init_dir(dir, title)
            elif level == 1: # file
                f = find_file(dir, title, index)
            if f:
                if level > 1:
                    print >>f
                print >>f, title
                print >>f, decorations[level] * (len(title) + 1)
                if len(self.children) == 1:
                    print >>f
                    print >>f, '(unwritten)'
        if self.name == 'ul':
            level += 1
    for element in getattr(self, 'children', ()):
        walk_element(element, level, dir, index, f)
    if f is not parent_f:
        f.close()

try:
    walk_element(tree, -1, None, None, None)
except KeyboardInterrupt:
    pass

mf = file('name_mappings.py', 'w')
mf.write('mappings = %r\n' % mappings)
