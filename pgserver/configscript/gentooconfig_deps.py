# defines how gentoo USE variables map to config settings

def configure(opts):
    if use('sdl'):
        opts['DRIVER_SDLFB'].value = 'y'
    else:
        for k, o in opts.items():
            if k.find('SDL') >= 0:
                o.value = None
    if use('X'):
        opts['DRIVER_X11'].value = 'y'
    else:
        opts['DRIVER_X11'].value = None
    if use('svga'):
        opts['DRIVER_SVGAFB'].value = 'y'
    else:
        opts['DRIVER_SVGAFB'].value = None
    if use('ncurses'):
        opts['DRIVER_NCURSES'].value = 'y'
    else:
        opts['DRIVER_NCURSES'].value = None
