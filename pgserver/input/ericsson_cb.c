/* $Id: ericsson_cb.c,v 1.3 2002/03/05 21:14:26 bauermeister Exp $
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Experimental keyboard driver for the Remora project
 * Copyright (C) 2001 Smartdata <www.smartdata.ch>
 *
 * Contributors:
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>
#include <sys/ioctl.h>

/*****************************************************************************/

#define DEVICE_FILE_NAME  "/dev/ttyS1"
#define BAUDRATE          B9600

/*****************************************************************************/

#define LOCAL_INFO  1
#define LOCAL_DEBUG 1
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

#define _printf(x...) { usleep(1000000/10); printf(x); usleep(1000000/10); }

#if LOCAL_INFO
# define INFO(x...)    _printf(__FILE__": " x)
#else
# define INFO(x...)
#endif

#if LOCAL_DEBUG
# define DPRINTF(x...) _printf(__FILE__": " x)
# define WARNF(x...)   _printf(__FILE__": " x)
#else
# define DPRINTF(x...)
# define WARNF(x...)   _printf(__FILE__": " x)
# undef LOCAL_TRACE
# define LOCAL_TRACE 0
#endif

#if LOCAL_TRACE
# define TRACEF(x...)  _printf(__FILE__": " x)
#else
# define TRACEF(x...)
#endif

#define NL "\r\n"

/* ------------------------------------------------------------------------- */

/*****************************************************************************
 *
 * AT commands sequences sent by the chatboard
 * -------------------------------------------
 *
 *   ---------------------- Power-on init
 *   AT
 *   AT+CGMM
 *   AT+CGMR
 *
 *   ---------------------- WWW key
 *   AT*EDME?
 *   AT*ESVM?
 *   AT+CKPD="eseee<see<<s>>ss1"
 *   AT+CKPD="c",25
 *   AT+CKPD="91c91c911111110c"
 *
 *   ---------------------- ATTACHMENT key
 *   AT+CKPD="#11111110c"
 *
 *   ---------------------- EMAIL key
 *   AT*EDME?
 *   AT*ESVM?
 *   AT+CKPD="eseee<see<<s>>ss1"
 *   AT+CKPD="c",25
 *   AT+CKPD="8666*1111111*0c"
 *
 *   ---------------------- SMS key
 *   AT*EDME?
 *   AT*ESVM?
 *   AT+CKPD="eseee<see<<s>>ss1"
 *   AT+CKPD="c",25
 *
 *   ---------------------- PHONEBOOK key
 *   AT*EDME?
 *   AT*ESVM?
 *   AT+CKPD="eseee<see<s>>s"
 *
 *   ---------------------- YES key
 *   AT+CKPD="s"
 *
 *   ---------------------- NO key
 *   AT+CKPD="e"
 *
 *   ---------------------- Inside SMS:
 *   ------------------ 1 key
 *   AT+CKPD="1",20
 *   ------------------ 2 key
 *   AT+CKPD="2",20
 *   ------------------ A key
 *   AT+CKPD="2"
 *   ------------------ B key
 *   AT+CKPD="22"
 *
 ****************************************************************************/

typedef struct {
  const char* sequence;
  u16 key_code;
  u8  chr_code_lower;
  u8  chr_code_upper;
} KeyStr;

static const KeyStr key_str_table[] = {
  /*--------------------+-------------------+-----------------+------+-------*/
  /* AT+CKPD sequence   | Key code          | Key label       | Uchr | Lchr  */
  /*--------------------+-------------------+-----------------+------+-------*/
  { "#"                 , PGKEY_HASH                          , '#'  ,  '#'  },
  { "##"                , PGKEY_ASTERISK                      , '*'  ,  '*'  },
  { "#1111111"          , PGKEY_F2          /* [Attachmnt] */ , 0    ,  0    },
  { "*"                 , PGKEY_CAPSLOCK    /* shift-space */ , 0    ,  0    },
  { "0"                 , PGKEY_PLUS                          , '+'  ,  '+'  },
  { "0,20"              , PGKEY_0                             , '0'  ,  '0'  },
  { "0,20?huh?"         , PGKEY_RIGHTBRACKET                  , ')'  ,  ')'  },
  { "00"                , PGKEY_AMPERSAND                     , '&'  ,  '&'  },
  { "000"               , PGKEY_AT                            , '@'  ,  '@'  },
  { "0000"              , PGKEY_SLASH                         , '/'  ,  '/'  },
  { "000000"            , PGKEY_PERCENT                       , '%'  ,  '%'  },
  { "0000000"           , PGKEY_DOLLAR                        , '$'  ,  '$'  },
  { "00000000"          , 0                 /* pound */       , 0xa3 ,  0    },
  { "0000000000000000"  , 0                 /* psi */         , 0    ,  0    },
  { "00000000000000000" , 0                 /* omega */       , 0    ,  0    },
  { "1"                 , PGKEY_SPACE                         , ' '  ,  ' '  },
  { "1,20"              , PGKEY_1                             , '1'  ,  '1'  },
  { "11"                , PGKEY_MINUS                         , '-'  ,  '-'  },
  { "111"               , PGKEY_QUESTION                      , '?'  ,  '?'  },
  { "11111"             , PGKEY_COMMA                         , ','  ,  ','  },
  { "111111"            , PGKEY_PERIOD                        , '.'  ,  '.'  },
  { "1111111"           , PGKEY_COLON                         , ':'  ,  ':'  },
  { "111111111"         , PGKEY_QUOTEDBL                      , '"'  ,  '"'  },
  { "1111111111"        , PGKEY_QUOTE                         , '\'' ,  '\'' },
  { "11111111111111"    , PGKEY_LEFTBRACKET                   , '('  ,  '('  },
  { "1111111111111111"  , PGKEY_EXCLAIM                       , '!'  ,  '!'  },
  { "2"                 , PGKEY_a                             , 'A'  ,  'a'  },
  { "2,20"              , PGKEY_2                             , '2'  ,  '2'  },
  { "22"                , PGKEY_b                             , 'B'  ,  'b'  },
  { "222"               , PGKEY_c                             , 'C'  ,  'c'  },
  { "2222"              , 0                 /* angstroem */   , 0xc5 ,  0xe5 },
  { "22222"             , 0                 /* a-trema */     , 0xc4 ,  0xe4 },
  { "222222"            , 0                 /* ae */          , 0xc6 ,  0xe6 },
  { "2222222"           , 0                 /* a-grave */     , 0xc0 ,  0xe0 },
  { "3"                 , PGKEY_d                             , 'D'  ,  'd'  },
  { "3,20"              , PGKEY_3                             , '3'  ,  '3'  },
  { "33"                , 0                 /* e-grave */     , 0xc8 ,  0xe8 },
  { "33"                , PGKEY_e                             , 'E'  ,  'e'  },
  { "333"               , PGKEY_f                             , 'F'  ,  'f'  },
  { "33333"             , 0                 /* e-aigu */      , 0xc9 ,  0xe9 },
  { "4"                 , PGKEY_g                             , 'G'  ,  'g'  },
  { "4,20"              , PGKEY_4                             , '4'  ,  '4'  },
  { "44"                , PGKEY_h                             , 'H'  ,  'h'  },
  { "444"               , PGKEY_i                             , 'I'  ,  'i'  },
  { "4444"              , 0                 /* i-grave */     , 0xcc ,  0xec },
  { "5"                 , PGKEY_j                             , 'J'  ,  'j'  },
  { "5,20"              , PGKEY_5                             , '5'  ,  '5'  },
  { "55"                , PGKEY_k                             , 'K'  ,  'k'  },
  { "555"               , PGKEY_l                             , 'L'  ,  'l'  },
  { "6"                 , PGKEY_m                             , 'M'  ,  'm'  },
  { "6,20"              , PGKEY_6                             , '6'  ,  '6'  },
  { "66"                , PGKEY_n                             , 'N'  ,  'n'  },
  { "666"               , PGKEY_o                             , 'O'  ,  'o'  },
  { "6666"              , 0                 /* n-tilde */     , 0xd1 ,  0xf1 },
  { "66666"             , 0                 /* o-trema */     , 0xd6 ,  0xf6 },
  { "666666"            , 0                 /* o-barre */     , 0xd8 ,  0xf8 },
  { "6666666"           , 0                 /* o-grave */     , 0xd2 ,  0xf2 },
  { "7"                 , PGKEY_p                             , 'P'  ,  'p'  },
  { "7,20"              , PGKEY_7                             , '7'  ,  '7'  },
  { "77"                , PGKEY_q                             , 'Q'  ,  'q'  },
  { "777"               , PGKEY_r                             , 'R'  ,  'r'  },
  { "7777"              , PGKEY_s                             , 'S'  ,  's'  },
  { "77777"             , 0                 /* beta */        , 0xdf ,  0xdf },
  { "7777777"           , 0                 /* pi */          , 0xb6 ,  0xb6 },
  { "77777777"          , 0                 /* sigma */       , 0    ,  0    },
  { "8"                 , PGKEY_t                             , 'T'  ,  't'  },
  { "8,20"              , PGKEY_8                             , '8'  ,  '8'  },
  { "8666*1111111*"     , PGKEY_F3          /* [E-mail] */    , 0    ,  0    },
  { "88"                , PGKEY_u                             , 'U'  ,  'u'  },
  { "888"               , PGKEY_v                             , 'V'  ,  'v'  },
  { "8888"              , 0                 /* u-trema */     , 0xdc ,  0xfc },
  { "88888"             , 0                 /* u-grave */     , 0xd9 ,  0xf9 },
  { "9"                 , PGKEY_w                             , 'W'  ,  'w'  },
  { "9,20"              , PGKEY_9                             , '9'  ,  '9'  },
  { "99"                , PGKEY_x                             , 'X'  ,  'x'  },
  { "999"               , PGKEY_y                             , 'Y'  ,  'y'  },
  { "9991111111"        , PGKEY_F1          /* [WWW] */       , 0    ,  0    },
  { "9999"              , PGKEY_z                             , 'Z'  ,  'z'  },
  { "9?huh?"            , 0                 /* c-cedille */   , 0xc7 ,  0xe7 },
  { "<"                 , PGKEY_LEFT                          , 0    ,  0    },
  { ">"                 , PGKEY_RIGHT                         , 0    ,  0    },
  { "c"                 , PGKEY_BACKSPACE   /* <-- */         , 0    ,  0    },
  { "e"                 , PGKEY_ESCAPE      /* [No] */        , 0    ,  0    },
  { "eseee<see<s>>s"    , PGKEY_F4          /* [PhoneBook] */ , 0    ,  0    },
  { "s"                 , PGKEY_RETURN      /* [Yes] */       , 0    ,  0    },
  /*
   * The table above must be sorted using 'LC_ALL=C sort -k 2'
   */
};


/* ------------------------------------------------------------------------- */

/*
 * lookup function returning an index in the above table given a string
 * or -1 if not found
 */

static int lookup(char* str, int buflen)
{
  int i, j;
  static const char* head = "AT+CKPD=\"";
  TRACEF(">>> lookup"NL);

  /* we look for AT+CKPD commands */
  DPRINTF("> str = [%s]"NL, str);
  if(strncmp(str, head, 9))
    return -1;

  /* skip command */
  str += 9;

  /* filter the string a bit :
   * - remobe dbl quotes
   * - char 'c' deletes the previous char
   */
  for(i=j=0; i<buflen; ++i) {
    register char c = str[i];
    if(c=='"') continue; /* remove dbl quotes */
    if(c=='c' && j) {
      --j;
      continue;
    }
    str[j++] = str[i];
    if(c==0) break;
  }
  DPRINTF("> str = [%s]"NL, str);

  /* lookup resulting string */
  // TODO: Do it 


  /* done */
  return -1;
}

/*****************************************************************************/

/*
 * Static global variables
 */

/* modifiers */
static int mod_caps  = 0;

/* misc */
static struct termios saved_options;

static int  cb_fd;
static int  skip_line;
static int  chars_in_buffer;
static char buffer[100];

/* ------------------------------------------------------------------------- */

static void cb_init_mods()
{
  TRACEF(">>> cb_init_mods()"NL);
  mod_caps  = 0;
  chars_in_buffer = 0;
  skip_line = 0;
}

static void outc(int fd, unsigned char c)
{
  // usleep(1000000/20); /* throttle */
  write(fd, &c, 1);
}

static void outs(int fd, const char* str)
{
  for(; *str; ++str) outc(fd, *str);
}

/*****************************************************************************/

static int cb_getchar(int fd)
{
  ssize_t n;
  unsigned char c;
  int nbread = 0;
  int index;
  u16 hwcode = 0;

  TRACEF(">>> cb_getchar()"NL);

  /* try to read a valid char */
  n = read(fd, &c, 1);

  /* no key or error */
  if(n<=0) {
    DPRINTF("==> n<=0"NL);
    skip_line = 1;
    return -1;
  }
  
  /* echo */
  outc(fd, c);
  
  //DPRINTF("<%02x>"NL, c);
  
  if(c=='\n') {
    static const char* ok_str = "OK\r\n";
    outs(fd, ok_str);
    DPRINTF("==>OK"NL);
    if(skip_line) {
      skip_line = 0;
      chars_in_buffer = 0;
      return -1;
    }
    else {
      buffer[chars_in_buffer] = 0;
      chars_in_buffer = 0;
      return lookup(buffer, sizeof(buffer));
    }
  }
  
  if(c<' ') return -1;
  
  if(chars_in_buffer>=sizeof(buffer)-1) {
    skip_line = 1;
  }
  else {
    buffer[chars_in_buffer++] = c;
  }

  return -1;
}

/*****************************************************************************/

static int cb_fd_activate(int fd)
{
  int index;
  TRACEF(">>> cb_fd_activate()"NL);

  /*
   * is the fd mine ?
   */
  if(fd!=cb_fd)
    return 0;


  /*
   * Listen to port and try to translete
   */
  index = cb_getchar(fd);

  /*
   * Invalid index or nothing
   */
  if(index==-1)
    return 1;


}

/*****************************************************************************/

static g_error cb_init(void)
{
  struct termios options;
  const char* device = get_param_str("input-ericsson-chatboard",
				     "device",
				     NULL);

  TRACEF(">>> cb_init()"NL);

  if(device==NULL) {
    /* keyboard not connected, this is not an error */
    WARNF("ericsson_cb: not using chatboard\n");
    cb_fd = -1;
    return success;
  }
  DPRINTF("ericsson_cb: trying [%s] Nr %d"NL,
	  device, device[strlen(device)-1] - '0');

  TRACEF("device=[%s]"NL, device);
  cb_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);

  if(cb_fd < 0) {
    fprintf(stderr, "ericsson_cb: could not open device %s: %s\n",
	    device, strerror(errno));
    return mkerror(PG_ERRT_IO,73);     /* Error opening cb device */
  }
  TRACEF("ericsson_cb: open ok"NL);

  tcgetattr(cb_fd, &saved_options);    /* Backup copy */
  tcgetattr(cb_fd, &options);          /* Work copy that will be modified */

  /* uart settings */
  cfsetispeed(&options, B9600);        /* 9600 baud rates */
  options.c_cflag |= (CLOCAL | CREAD); /* Enable rx and set the local mode */
  options.c_cflag &= ~PARENB;          /* None parity */
  options.c_cflag &= ~CSIZE;           /* Mask the character bit size */
  options.c_cflag |= CS8;              /* 8 data bits */
  options.c_cflag &= ~CSTOPB;          /* 1 stop bits */
  options.c_cflag &= ~CRTSCTS;         /* Disable hardware flow control */
  options.c_iflag &= ~(IXON | IXOFF | IXANY); /* Disable sw flow control */

  /* driver settings for raw input and output */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;

  /* set parameters */  
  tcsetattr(cb_fd, TCSANOW, &options);

  /* init states*/
  cb_init_mods();

  TRACEF("ericsson_cb: done"NL);
  return success;
}

/*****************************************************************************/

static void cb_fd_init(int *n, fd_set *readfds, struct timeval *timeout)
{
  if ((*n)<(cb_fd+1))
    *n = cb_fd+1;
  if (cb_fd>0)
    FD_SET(cb_fd, readfds);
}

/*****************************************************************************/

static void cb_close(void)
{
  TRACEF(">>> cb_close()"NL);
  tcsetattr(cb_fd, TCSANOW, &saved_options);
  close(cb_fd);
}

/*****************************************************************************/

g_error ericsson_cb_regfunc(struct inlib* i)
{
  TRACEF(">>> ericsson_cb_regfunc()"NL);
  i->init = &cb_init;
  i->fd_activate = &cb_fd_activate;
  i->fd_init = &cb_fd_init;
  i->close = &cb_close;
  return success;
}

/*****************************************************************************/
