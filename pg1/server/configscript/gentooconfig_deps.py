# defines how gentoo USE variables map to config settings

def unset_all(pattern):
    for k, o in opts.items():
        if k.find(pattern) >= 0:
            o.value = None

def configure(opts):
    if use('sdl'):
        opts['DRIVER_SDLFB'].value = 'y'
        opts['DRIVER_SDLINPUT'].value = 'y'
    else:
        unset_all('SDL')
    if use('X'):
        opts['DRIVER_X11'].value = 'y'
        opts['DRIVER_X11INPUT'].value = 'y'
    else:
        unset_all('X11')
    if use('svga'):
        opts['DRIVER_SVGAFB'].value = 'y'
    else:
        opts['DRIVER_SVGAFB'].value = None
    if use('ncurses'):
        opts['DRIVER_NCURSES'].value = 'y'
        opts['DRIVER_NCURSESINPUT'].value = 'y'
    else:
        opts['DRIVER_NCURSES'].value = None
        opts['DRIVER_NCURSESINPUT'].value = None
    if use('directfb'):
        opts['DRIVER_DIRECTFB'].value = 'y'
        opts['DRIVER_DIRECTFBINPUT'].value = 'y'
    else:
        opts['DRIVER_DIRECTFB'].value = None
        opts['DRIVER_DIRECTFBINPUT'].value = None
    if use('gpm'):
        opts['DRIVER_GPM'].value = 'y'
    else:
        opts['DRIVER_GPM'].value = None
    if use('truetype'):
        opts['CONFIG_FONTENGINE_FREETYPE'].value = 'y'
    else:
        opts['CONFIG_FONTENGINE_FREETYPE'].value = None
    if use('png'):
        opts['CONFIG_FORMAT_PNG'].value = 'y'
    else:
        opts['CONFIG_FORMAT_PNG'].value = None
    if use('jpeg'):
        opts['CONFIG_FORMAT_JPEG'].value = 'y'
    else:
        opts['CONFIG_FORMAT_JPEG'].value = None
