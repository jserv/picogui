/*
 * /dev/tty TTY Keyboard Driver
 *
 * Written by John Laur <johnl@blurbco.com>
 * 
 * Based on the Microwindows driver originally written by:
 *  
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Contributors:
 *   Alex McMains - 2001, RidgeRun, Inc. (aam@ridgerun.com)
 */

#include <termios.h>
#include <linux/vt.h>

#include <stdio.h>

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <picogui/pgkeys.h>
#include <picogui/constants.h>
#include <pgserver/configfile.h>

#define	KEYBOARD "/dev/tty"	/* keyboard associated with screen*/

static int ttykb_fd;		/* file descriptor for keyboard */
static struct termios old;	/* original terminal modes */

static int ascii_expect_special = 0;
static int keymap[128] = { /* ascii keymap */
        0,0,0,0,0,						/* 0   */
        0,0,0,0,PGKEY_TAB,					/* 5   */
        0,0,0,PGKEY_RETURN,0,					/* 10  */
        0,0,0,0,0,						/* 15  */
        0,0,0,0,0,						/* 20  */
        0,0,PGKEY_ESCAPE,0,0,					/* 25  */
        0,0,PGKEY_SPACE,PGKEY_EXCLAIM,PGKEY_QUOTEDBL,		/* 30  */
        PGKEY_HASH,PGKEY_DOLLAR,0,
        PGKEY_AMPERSAND,PGKEY_QUOTE,			        /* 35  */
        PGKEY_LEFTPAREN,PGKEY_RIGHTPAREN,PGKEY_ASTERISK,
        PGKEY_PLUS,PGKEY_COMMA,	              			/* 40  */
        PGKEY_RETURN,PGKEY_PERIOD,PGKEY_SLASH,PGKEY_0,PGKEY_1,	/* 45  */
        PGKEY_2,PGKEY_3,PGKEY_4,PGKEY_5,PGKEY_6,		/* 50  */
        PGKEY_7,PGKEY_8,PGKEY_9,PGKEY_COLON,PGKEY_SEMICOLON,	/* 55  */
        PGKEY_LESS,PGKEY_EQUALS,PGKEY_GREATER,
        PGKEY_QUESTION,PGKEY_AT,				/* 60  */
        65,66,67,68,69,				        	/* 65  */
        70,71,72,73,74,					        /* 70  */
        75,76,77,78,79,				        	/* 75  */
        80,81,82,83,84,			        		/* 80  */
        85,86,87,88,89,				        	/* 85  */
        90,PGKEY_LEFTBRACKET,PGKEY_BACKSLASH,
        PGKEY_RIGHTBRACKET,PGKEY_CARET, 			/* 90  */
        PGKEY_UNDERSCORE,PGKEY_BACKQUOTE,PGKEY_a,
        PGKEY_b,PGKEY_c,					/* 95  */
        PGKEY_d,PGKEY_e,PGKEY_f,PGKEY_g,PGKEY_h,		/* 100 */
        PGKEY_i,PGKEY_j,PGKEY_k,PGKEY_l,PGKEY_m,		/* 105 */
        PGKEY_n,PGKEY_o,PGKEY_p,PGKEY_q,PGKEY_r,		/* 110 */
        PGKEY_s,PGKEY_t,PGKEY_u,PGKEY_v,PGKEY_w,		/* 115 */
        PGKEY_x,PGKEY_y,PGKEY_z,PGKEY_LEFTBRACE,PGKEY_PIPE,	/* 120 */
        PGKEY_RIGHTBRACE,PGKEY_TILDE,PGKEY_BACKSPACE	        /* 125 */
};

/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
g_error ttykb_init(null)
{
        struct termios	new;
        char *vt;

        /**
         * When the --video-fbdev.vt switch is given on the command line, the user of pgserver
         * wants to run on a specific tty.  Make sure to open the correct one if this command line
         * option is given to pgserver.  --video-fbdev.vt won't work without this code!!!!!!
         */
        /*
         * Figure out what tty to open
         */
        vt = get_param_str("video-fbdev","vt","current");
        if (!strcmp(vt,"current") && !strcmp(vt,"auto")) {
                ttykb_fd = open(KEYBOARD, O_NONBLOCK);
        }
        else {
                char tmpbuf[32];
                
                sprintf(tmpbuf, "/dev/tty%d", atoi(vt));
                ttykb_fd = open(tmpbuf, O_NONBLOCK);
        }

	
        if (ttykb_fd < 0)
                return -1;

        if (tcgetattr(ttykb_fd, &old) < 0)
                goto err;

        new = old;
        /* If you uncomment ISIG and BRKINT below, then ^C will be ignored.*/
        new.c_lflag &= ~(ECHO | ICANON | IEXTEN /*| ISIG*/);
        new.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON /*| BRKINT*/);
        new.c_cflag &= ~(CSIZE | PARENB);
        new.c_cflag |= CS8;
        new.c_cc[VMIN] = 0;
        new.c_cc[VTIME] = 0;

        if(tcsetattr(ttykb_fd, TCSAFLUSH, &new) < 0)
                goto err;

        return 1;

 err:
        close(ttykb_fd);
        ttykb_fd = 0;
        return 0;
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */	
void ttykb_close(void)
{
	tcsetattr(ttykb_fd, TCSANOW, &old);
	close(ttykb_fd);
	ttykb_fd = 0;
}

void ttykb_fd_init(int *n,fd_set *readfds,struct timeval *timeout)
{
        if ((*n)<(ttykb_fd+1))
                *n = ttykb_fd+1;
        FD_SET(ttykb_fd,readfds);
}

static inline int ascii_map_fnlow(int key)
{
        ascii_expect_special = 0;

        switch ( key ) {
        case 0x41:
                return PGKEY_F1;
        case 0x42:
                return PGKEY_F2;
        case 0x43:
                return PGKEY_F3;
        case 0x44:
                return PGKEY_F4;
        case 0x45:
                return PGKEY_F5;
        default:
                return 0;
        }
}

static inline int ascii_map_fnmid(int key)
{
        ascii_expect_special = 0;

        switch ( key ) {
        case 0x37:
                return PGKEY_F6;
        case 0x38:
                return PGKEY_F7;
        case 0x39:
                return PGKEY_F8;
        default:
                return 0;
        }
}

static inline int ascii_map_fnhi(int key)
{
        ascii_expect_special = 0;

        switch ( key ) {
        case 0x30:
                return PGKEY_F9;
        case 0x31:
                return PGKEY_F10;
        case 0x33:
                return PGKEY_F11;
        case 0x34:
                return PGKEY_F12;
        default:
                return 0;
        }
}

static inline int ascii_map_arrow(int key)
{
        ascii_expect_special = 0;

        switch ( key ) {
        case 0x41:
                return PGKEY_UP;
        case 0x42:
                return PGKEY_DOWN;
        case 0x43:
                return PGKEY_RIGHT;
        case 0x44:
                return PGKEY_LEFT;
        default:
                return 0;
        }
}

static inline int ascii_map_pagekey(int key)
{
        ascii_expect_special = 0;

        switch ( key ) {
        case 0x31:
                return PGKEY_HOME;
        case 0x32:
                return PGKEY_INSERT;
        case 0x33:
                return PGKEY_DELETE;
        case 0x34:
                return PGKEY_END;
        case 0x35:
                return PGKEY_PAGEUP;
        case 0x36:
                return PGKEY_PAGEDOWN;
        default:
                return 0;
        }
}

/* ASCII special keys                                     */
/* 1b, 5b, 5b, 4x     keys: F1, F2, F3, F4, F5            */
/* 1b, 5b, 31, 3x, 7e keys: F6, F7, F8                    */
/* 1b, 5b, 32, 3x, 7e keys: F9, F10, F11, F12             */
/* 1b, 5b, 3x, 7e     keys: Home, Ins, Del, End, Up, Down */
/* 1b, 5b, 4x   Arrow keys: Up, Down, Right, Left         */
static int ascii_do_special(int curkey)
{
        static int last_key         = 0; /* used when next key is 0x7e */
        static int expect_fnlow     = 0; /* next key is F1-F5          */
        static int expect_fnmid     = 0; /* next key is F6-F8          */
        static int expect_fnhi      = 0; /* next key is F9-F12         */
        static int expect_7e_fnmid  = 0; /* key is 0x7e after F6-F8    */
        static int expect_7e_fnhi   = 0; /* key is 0x7e after F9-F12   */
        static int found_5b         = 0; /* key is 1st 0x5b after 0x1b */
        static int maybe_7e         = 0; /* key is page_key or fn key  */
        
        int key;

        if ( (curkey == 0x5b) && (!found_5b) ) {
                found_5b = 1;
                return 0;
        }
        else
                found_5b = 0;

        /* divide into groups               */
        /* 0x5b = fnlow                     */
        /* 0x4x = fnlow || arrow_key        */
        /* 0x3x = fnmid || fnhi || page_key */
        /* 0x7e = fnmid || fnhi || page_key */
        key = curkey;
        if ( (curkey != 0x5b) && (curkey != 0x7e) ) {
                if ( curkey & 0x40 )
                        key = 0x40;
                else 
                        key = 0x30;
        }

        switch ( key ) {
        case 0x5b: /* fnlow */
                expect_fnlow = 1;
                return 0;
        case 0x7e: /* fnmid || fnhi || page_key */
                if ( expect_7e_fnmid ) {
                        /* fnmid */
                        expect_7e_fnmid = 0;
                        return ascii_map_fnmid(last_key);
                }
                else if ( expect_7e_fnhi ) {
                        /* fnhi */
                        expect_7e_fnhi = 0;
                        return ascii_map_fnhi(last_key);
                }
                else if ( maybe_7e ) {
                        /* page_key */
                        maybe_7e     = 0;
                        expect_fnmid = 0;
                        expect_fnhi  = 0;
                        return ascii_map_pagekey(last_key);
                }
                else {
                        /* error */
                        ascii_expect_special = 0;
                        return 0;
                }
        case 0x40: /* fnlow || arrow_key */
                if ( expect_fnlow ) {
                        /* fn_low */
                        expect_fnlow = 0;
                        return ascii_map_fnlow(curkey);
                }
                else {
                        /* arrow_key */
                        return ascii_map_arrow(curkey);
                }
        case 0x30: /* fnmid || fnhi || page_key */                             
                last_key = curkey;

                if ( expect_fnmid ) {
                        /* fnmid */
                        expect_fnmid    = 0;
                        expect_7e_fnmid = 1;
                        return 0;
                }

                if ( expect_fnhi ) {
                        /* fnhi */
                        expect_fnhi    = 0;
                        expect_7e_fnhi = 1;
                        return 0;
                }       

                if ( curkey == 0x31 ) {
                        /* fnmid || page_key */
                        maybe_7e     = 1;
                        expect_fnmid = 1;
                }
                else if ( curkey == 0x32 ) {
                        /* fnhi || page_key */
                        maybe_7e    = 1;
                        expect_fnhi = 1;
                }
                else {
                        /* page_key */
                        maybe_7e = 1;
                }
                return 0;
        default: /* invalid key */
                ascii_expect_special = 0;
                return 0;

        } // end switch
}

static int ttykb_fd_activate(int fd)
{
        int curkey;
        unsigned char buf[1];
        int mappedKey = 0;
        int ret;	

        if ( fd != ttykb_fd ) 
                return 0;

        ret = read(fd, buf, 1);

        fprintf(stderr, __FUNCTION__ ": 0x%x\n", buf[0]);

        if ( ret > 0 ) {
                curkey = buf[0];

                if ( ascii_expect_special ) {
                        mappedKey = ascii_do_special(curkey);
                         if ( ! mappedKey )
                                 return 0;
                         else /* not printable */
                                 curkey = -1;
                }

                if ( curkey != -1 )
                {
                        if ( keymap[curkey] != 0 ) {   
                                switch ( curkey ) {
                                case 0x1b:
                                        ascii_expect_special = 1;
                                        return 0;
                                default:
                                        if ( ! mappedKey )
                                                mappedKey = keymap[curkey];
                                } /* end switch */
                        } /* end if */
                        else
                                return 0;
                }

                fprintf(stderr, __FUNCTION__ ": posting 0x%x\n", mappedKey);

                /* Is it printable */
                if ( (curkey >= 0x20) && (curkey <= 0x7e) ) 
                        infilter_send_key(PG_TRIGGER_CHAR, mappedKey, 0);

                /* FIXME: PG_TRIGGER_KEYUP is not implemented yet, but we
                 * at least need this so that hotkeys work correctly. 
                 * This needs to respond to a few keys PG_TRIGGER_CHAR 
                 * does not, like the arrow keys */
                infilter_send_key(PG_TRIGGER_KEYDOWN, mappedKey, 0);
                infilter_send_key(PG_TRIGGER_KEYUP, mappedKey, 0);

                return 1;
        } /* end if */

        if ( (ret < 0) && (errno != EINTR) && (errno != EAGAIN) ) {
                return -1;
        }

        return 0;
}

g_error ttykb_regfunc(struct inlib *i) {
        i->init = &ttykb_init;
        i->close = &ttykb_close;
        i->fd_init = &ttykb_fd_init;
        i->fd_activate = &ttykb_fd_activate;
        return success;
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
