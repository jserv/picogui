default = {
    'C-x': {
      'C-c': 'raise SystemExit',
      'C-s': 'frame.save()',
      'k': 'frame.close()',
    },
    'A-space': 'print workspace.cursor_position',
    'C-space': 'workspace.set_mark()',
    'A-w': 'workspace.copy()',
    'C-w': 'workspace.cut()',
    'C-y': 'workspace.paste()',
    'A-y': 'workspace.rotate_paste()',
    'A-x': 'frame.minibuffer.focus()',
    'tab': 'buffer.indent(workspace)',
    'return': 'buffer.indent_if_auto(workspace)',
    'backspace': 'buffer.unindent_if_at_start(workspace)',
}

def _resolve(sequence, where=default):
    for key in sequence:
        where = where.get(key, None)
        if where is None:
            return
    return where

def resolve(sequence, bindings=(default,)):
    for where in bindings:
        r = _resolve(sequence, where)
        if r:
            return r

def bind(sequence, command, where=default):
    parent = _resolve(sequence[:-1], where)
    if parent is None:
        # may be recursive
        parent = {}
        bind(sequence[:-1], parent, where)
    parent[sequence[-1]] = command
