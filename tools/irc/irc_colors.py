"""
Some constants and functions useful when creating colored IRC text.

All of this information taken from
   http://www.geocities.com/SiliconValley/Heights/1246/mcolor.html
which is perhaps not the most authoritative source, but...

"""

NORMAL = "\x0F"
BOLD = "\x02"
REVERSE = "\x16"
UNDERLINE = "\x1F"

# Apparently, this prefix can also be used with nothing after it to
# revert to normal color, but this is unreliable -- eg, if the next
# bit of text is some numbers, it won't work right.
COLOR_PREFIX = "\x03"
COLOUR_PREFIX = COLOR_PREFIX
COLORS = { "black" : "01",
           "dark blue" : "02",
           "dark green" : "03",
	   "green" : "03",
           "red" : "04",
           "brown" : "05",
           "purple" : "06",
           "orange" : "07",
           "yellow" : "08",
           "light green" : "09",
           "aqua" : "10",
           "light blue" : "11",
           "blue" : "12",
           "violet" : "13",
           "grey" : "14",
           "gray" : "14",
           "light grey" : "15",
           "light gray" : "15",
           "white" : "16",
           }
COLOURS = COLORS

def colorify(text, foreground, background=""):
    """Colorify a string for IRC.
    
    Returns a string that when displayed by a mIRC-like client, will
    display 'text' with the given foreground and background colors.

    """
    prefix  = COLOR_PREFIX
    prefix += COLORS[foreground]
    if background != "":
        prefix += "," + COLORS[background]
    return prefix + text + NORMAL

def boldify(text):
    """Boldify a string for IRC.

    Returns a string that when displayed by a mIRC-like client, will
    display 'text' in bold.

    """
    return BOLD + text + NORMAL

def reverse_colors(text):
    """Reverse foreground and background colors for a string on IRC.

    Returns a string that when displayed by a mIRC-like client, will
    display 'text' in reverse colors.

    """
    return REVERSE + text + NORMAL

def underline(text):
    """Underline a string for IRC.

    Returns a string that when displayed by a mIRC-like client, will
    display 'text' in underline.

    """
    return UNDERLINE + text + NORMAL
