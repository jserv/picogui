bindings = {
    'C-x': {
      'C-c': 'raise SystemExit',
      'C-s': 'frame.save()',
    },
}

def resolve(sequence):
    where = bindings
    for key in sequence:
        where = where.get(key, None)
        if where is None:
            return
    return where

def bind(sequence, command):
    parent = resole(sequence[:-1])
    if parent is None:
        # may be recursive
        parent = {}
        bind(sequence[:-1], parent)
    parent[sequence[-1]] = command
