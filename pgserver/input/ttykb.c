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
 */

#include <termios.h>

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <picogui/pgkeys.h>
#include <picogui/constants.h>

#define	KEYBOARD	"/dev/tty1"	/* keyboard associated with screen*/

static	int    ttykb_fd;		/* file descriptor for keyboard */
static	struct termios	old;		/* original terminal modes */

static int keymap[128] = {		/* The keymap */
  0,0,0,0,0,						/* 0   */
  0,0,0,0,PGKEY_TAB,					/* 5   */
  0,0,0,PGKEY_RETURN,0,					/* 10  */
  0,0,0,0,0,						/* 15  */
  0,0,0,0,0,						/* 20  */
  0,0,PGKEY_ESCAPE,0,0,					/* 25  */
  0,0,PGKEY_SPACE,PGKEY_EXCLAIM,PGKEY_QUOTEDBL,		/* 30  */
  PGKEY_HASH,PGKEY_DOLLAR,0,
    PGKEY_AMPERSAND,PGKEY_QUOTE,			/* 35  */
  PGKEY_LEFTPAREN,PGKEY_RIGHTPAREN,PGKEY_ASTERISK,
    PGKEY_PLUS,PGKEY_COMMA,				/* 40  */
  PGKEY_MINUS,PGKEY_PERIOD,PGKEY_SLASH,PGKEY_0,PGKEY_1,	/* 45  */
  PGKEY_2,PGKEY_3,PGKEY_4,PGKEY_5,PGKEY_6,		/* 50  */
  PGKEY_7,PGKEY_8,PGKEY_9,PGKEY_COLON,PGKEY_SEMICOLON,	/* 55  */
  PGKEY_LESS,PGKEY_EQUALS,PGKEY_GREATER,
    PGKEY_QUESTION,PGKEY_AT,				/* 60  */
  65,66,67,68,69,					/* 65  */
  70,71,72,73,74,					/* 70  */
  75,76,77,78,79,					/* 75  */
  80,81,82,83,84,					/* 80  */
  85,86,87,88,89,					/* 85  */
  90,PGKEY_LEFTBRACKET,PGKEY_BACKSLASH,
    PGKEY_RIGHTBRACKET,PGKEY_CARET, 			/* 90  */
  PGKEY_UNDERSCORE,PGKEY_BACKQUOTE,PGKEY_a,
    PGKEY_b,PGKEY_c,					/* 95  */
  PGKEY_d,PGKEY_e,PGKEY_f,PGKEY_g,PGKEY_h,		/* 100 */
  PGKEY_i,PGKEY_j,PGKEY_k,PGKEY_l,PGKEY_m,		/* 105 */
  PGKEY_n,PGKEY_o,PGKEY_p,PGKEY_q,PGKEY_r,		/* 110 */
  PGKEY_s,PGKEY_t,PGKEY_u,PGKEY_v,PGKEY_w,		/* 115 */
  PGKEY_x,PGKEY_y,PGKEY_z,PGKEY_LEFTBRACE,PGKEY_PIPE,	/* 120 */
  PGKEY_RIGHTBRACE,PGKEY_TILDE,PGKEY_BACKSPACE		/* 125 */
};	

/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
g_error ttykb_init(null)
{
  char *env;

	int		i;
	int		ledstate = 0;
	struct termios	new;

	ttykb_fd = open(KEYBOARD, O_NONBLOCK);
	
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

int ttykb_fd_activate(int fd)
{
	
	int	cc;			/* characters read */
	int	curkey;
	unsigned char buf[1];

	cc = read(fd, buf, 1);
	if (cc > 0) {
		curkey = buf[0];

		if (keymap[curkey] != 0)
			dispatch_key(TRIGGER_CHAR,keymap[curkey],0);
		return 1;		/* keypress*/
	}
	if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN)) {
		return -1;
	}
	return 0;
}

g_error ttykb_regfunc(struct inlib *i) {
        i->init = &ttykb_init;
        i->close = &ttykb_close;
        i->fd_init = &ttykb_fd_init;
        i->fd_activate = &ttykb_fd_activate;
        return sucess;
}
