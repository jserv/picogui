# utilities for Nifty configuration
import os
nifty_dir = os.path.join(os.environ['HOME'], '.nifty')

def get_config_file(name):
    if not os.path.exists(nifty_dir):
        os.mkdir(nifty_dir)
    return os.path.join(nifty_dir, name)

def open_config_file(name, mode='r'):
    return file(get_config_file(name), mode)

def exec_config_file(name, globals, locals=None):
    fn = get_config_file(name)
    if os.path.exists(fn):
        if locals is None:
            locals = globals
        execfile(fn, globals, locals)
