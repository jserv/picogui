import constants, requests

ns = constants.cmd_ns('grop')

class Gropseq(object):
    '''Sequence of gropnodes to send as a batch'''
    def __init__(self, server=None):
        self._grops = []
        self.server = server

    def all(self):
        all = []
        for grop in self._grops:
            all.extend(grop)
        return all

        #widget.server.send_and_wait(requests.writecmd,
        #                            all)

    def append(self, grop):
        self._grops.append(grop)

class Grop(object):
    def __init__(self, name, seq):
        self.name = name
        self.seq = seq

    def __call__(self, *args):
        code, _ns = constants.resolve(self.name, ns, self.seq.server)
        args_resolved = [len(args) + 2, code]
        for a in args:
            r, _ns = constants.resolve(a, _ns)
            args_resolved.append(r)
        self.seq.append(args_resolved)

class GropMethod(object):
    def __init__(self, name):
        self.name = name

    def __get__(self, seq, seqclass):
        return Grop(self.name, seq)

for name in ns.keys():
    setattr(Gropseq, name, GropMethod(name))

__all__ = ('Gropseq',)
