/* $Id$
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
#include <pgserver/widget.h>    /* For sending events */

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

#define LOCAL_INFO  0
#define LOCAL_DEBUG 1
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

#define _printf(x...) { usleep(1000000/100); printf(x); usleep(1000000/100); }

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

#if LOCAL_DEBUG

void print_pgmods(u16 mod)
{
  if(mod&PGMOD_LSHIFT) printf("LSHIFT ");
  if(mod&PGMOD_RSHIFT) printf("RSHIFT ");
  if(mod&PGMOD_SHIFT) printf("SHIFT ");
  if(mod&PGMOD_LCTRL) printf("LCTRL ");
  if(mod&PGMOD_RCTRL) printf("RCTRL ");
  if(mod&PGMOD_CTRL) printf("CTRL ");
  if(mod&PGMOD_LALT) printf("LALT ");
  if(mod&PGMOD_RALT) printf("RALT ");
  if(mod&PGMOD_ALT) printf("ALT ");
  if(mod&PGMOD_LMETA) printf("LMETA ");
  if(mod&PGMOD_RMETA) printf("RMETA ");
  if(mod&PGMOD_META) printf("META ");
  if(mod&PGMOD_NUM) printf("NUM ");
  if(mod&PGMOD_CAPS) printf("CAPS ");
  if(mod&PGMOD_MODE) printf("MODE ");
}

void print_pgkeyname(u16 code)
{
  switch(code) {
  case PGKEY_BACKSPACE:        printf("PGKEY_BACKSPACE");     break;
  case PGKEY_TAB:              printf("PGKEY_TAB");           break;
  case PGKEY_CLEAR:            printf("PGKEY_CLEAR");         break;
  case PGKEY_RETURN:           printf("PGKEY_RETURN");        break;
  case PGKEY_PAUSE:            printf("PGKEY_PAUSE");         break;
  case PGKEY_ESCAPE:           printf("PGKEY_ESCAPE");        break;
  case PGKEY_SPACE:            printf("PGKEY_SPACE");         break;
  case PGKEY_EXCLAIM:          printf("PGKEY_EXCLAIM");       break;
  case PGKEY_QUOTEDBL:         printf("PGKEY_QUOTEDBL");      break;
  case PGKEY_HASH:             printf("PGKEY_HASH");          break;
  case PGKEY_DOLLAR:           printf("PGKEY_DOLLAR");        break;
  case PGKEY_PERCENT:          printf("PGKEY_PERCENT");       break;
  case PGKEY_AMPERSAND:        printf("PGKEY_AMPERSAND");     break;
  case PGKEY_QUOTE:            printf("PGKEY_QUOTE");         break;
  case PGKEY_LEFTPAREN:        printf("PGKEY_LEFTPAREN");     break;
  case PGKEY_RIGHTPAREN:       printf("PGKEY_RIGHTPAREN");    break;
  case PGKEY_ASTERISK:         printf("PGKEY_ASTERISK");      break;
  case PGKEY_PLUS:             printf("PGKEY_PLUS");          break;
  case PGKEY_COMMA:            printf("PGKEY_COMMA");         break;
  case PGKEY_MINUS:            printf("PGKEY_MINUS");         break;
  case PGKEY_PERIOD:           printf("PGKEY_PERIOD");        break;
  case PGKEY_SLASH:            printf("PGKEY_SLASH");         break;
  case PGKEY_0:                printf("PGKEY_0");             break;
  case PGKEY_1:                printf("PGKEY_1");             break;
  case PGKEY_2:                printf("PGKEY_2");             break;
  case PGKEY_3:                printf("PGKEY_3");             break;
  case PGKEY_4:                printf("PGKEY_4");             break;
  case PGKEY_5:                printf("PGKEY_5");             break;
  case PGKEY_6:                printf("PGKEY_6");             break;
  case PGKEY_7:                printf("PGKEY_7");             break;
  case PGKEY_8:                printf("PGKEY_8");             break;
  case PGKEY_9:                printf("PGKEY_9");             break;
  case PGKEY_COLON:            printf("PGKEY_COLON");         break;
  case PGKEY_SEMICOLON:        printf("PGKEY_SEMICOLON");     break;
  case PGKEY_LESS:             printf("PGKEY_LESS");          break;
  case PGKEY_EQUALS:           printf("PGKEY_EQUALS");        break;
  case PGKEY_GREATER:          printf("PGKEY_GREATER");       break;
  case PGKEY_QUESTION:         printf("PGKEY_QUESTION");      break;
  case PGKEY_AT:               printf("PGKEY_AT");            break;
  case PGKEY_LEFTBRACKET:      printf("PGKEY_LEFTBRACKET");   break;
  case PGKEY_BACKSLASH:        printf("PGKEY_BACKSLASH");     break;
  case PGKEY_RIGHTBRACKET:     printf("PGKEY_RIGHTBRACKET");  break;
  case PGKEY_CARET:            printf("PGKEY_CARET");         break;
  case PGKEY_UNDERSCORE:       printf("PGKEY_UNDERSCORE");    break;
  case PGKEY_BACKQUOTE:        printf("PGKEY_BACKQUOTE");     break;
  case PGKEY_a:                printf("PGKEY_a");             break;
  case PGKEY_b:                printf("PGKEY_b");             break;
  case PGKEY_c:                printf("PGKEY_c");             break;
  case PGKEY_d:                printf("PGKEY_d");             break;
  case PGKEY_e:                printf("PGKEY_e");             break;
  case PGKEY_f:                printf("PGKEY_f");             break;
  case PGKEY_g:                printf("PGKEY_g");             break;
  case PGKEY_h:                printf("PGKEY_h");             break;
  case PGKEY_i:                printf("PGKEY_i");             break;
  case PGKEY_j:                printf("PGKEY_j");             break;
  case PGKEY_k:                printf("PGKEY_k");             break;
  case PGKEY_l:                printf("PGKEY_l");             break;
  case PGKEY_m:                printf("PGKEY_m");             break;
  case PGKEY_n:                printf("PGKEY_n");             break;
  case PGKEY_o:                printf("PGKEY_o");             break;
  case PGKEY_p:                printf("PGKEY_p");             break;
  case PGKEY_q:                printf("PGKEY_q");             break;
  case PGKEY_r:                printf("PGKEY_r");             break;
  case PGKEY_s:                printf("PGKEY_s");             break;
  case PGKEY_t:                printf("PGKEY_t");             break;
  case PGKEY_u:                printf("PGKEY_u");             break;
  case PGKEY_v:                printf("PGKEY_v");             break;
  case PGKEY_w:                printf("PGKEY_w");             break;
  case PGKEY_x:                printf("PGKEY_x");             break;
  case PGKEY_y:                printf("PGKEY_y");             break;
  case PGKEY_z:                printf("PGKEY_z");             break;
  case PGKEY_LEFTBRACE:        printf("PGKEY_LEFTBRACE");     break;
  case PGKEY_PIPE:             printf("PGKEY_PIPE");          break;
  case PGKEY_RIGHTBRACE:       printf("PGKEY_RIGHTBRACE");    break;
  case PGKEY_TILDE:            printf("PGKEY_TILDE");         break;
  case PGKEY_DELETE:           printf("PGKEY_DELETE");        break;
  case PGKEY_WORLD_0:          printf("PGKEY_WORLD_0");       break;
  case PGKEY_WORLD_1:          printf("PGKEY_WORLD_1");       break;
  case PGKEY_WORLD_2:          printf("PGKEY_WORLD_2");       break;
  case PGKEY_WORLD_3:          printf("PGKEY_WORLD_3");       break;
  case PGKEY_WORLD_4:          printf("PGKEY_WORLD_4");       break;
  case PGKEY_WORLD_5:          printf("PGKEY_WORLD_5");       break;
  case PGKEY_WORLD_6:          printf("PGKEY_WORLD_6");       break;
  case PGKEY_WORLD_7:          printf("PGKEY_WORLD_7");       break;
  case PGKEY_WORLD_8:          printf("PGKEY_WORLD_8");       break;
  case PGKEY_WORLD_9:          printf("PGKEY_WORLD_9");       break;
  case PGKEY_WORLD_10:         printf("PGKEY_WORLD_10");      break;
  case PGKEY_WORLD_11:         printf("PGKEY_WORLD_11");      break;
  case PGKEY_WORLD_12:         printf("PGKEY_WORLD_12");      break;
  case PGKEY_WORLD_13:         printf("PGKEY_WORLD_13");      break;
  case PGKEY_WORLD_14:         printf("PGKEY_WORLD_14");      break;
  case PGKEY_WORLD_15:         printf("PGKEY_WORLD_15");      break;
  case PGKEY_WORLD_16:         printf("PGKEY_WORLD_16");      break;
  case PGKEY_WORLD_17:         printf("PGKEY_WORLD_17");      break;
  case PGKEY_WORLD_18:         printf("PGKEY_WORLD_18");      break;
  case PGKEY_WORLD_19:         printf("PGKEY_WORLD_19");      break;
  case PGKEY_WORLD_20:         printf("PGKEY_WORLD_20");      break;
  case PGKEY_WORLD_21:         printf("PGKEY_WORLD_21");      break;
  case PGKEY_WORLD_22:         printf("PGKEY_WORLD_22");      break;
  case PGKEY_WORLD_23:         printf("PGKEY_WORLD_23");      break;
  case PGKEY_WORLD_24:         printf("PGKEY_WORLD_24");      break;
  case PGKEY_WORLD_25:         printf("PGKEY_WORLD_25");      break;
  case PGKEY_WORLD_26:         printf("PGKEY_WORLD_26");      break;
  case PGKEY_WORLD_27:         printf("PGKEY_WORLD_27");      break;
  case PGKEY_WORLD_28:         printf("PGKEY_WORLD_28");      break;
  case PGKEY_WORLD_29:         printf("PGKEY_WORLD_29");      break;
  case PGKEY_WORLD_30:         printf("PGKEY_WORLD_30");      break;
  case PGKEY_WORLD_31:         printf("PGKEY_WORLD_31");      break;
  case PGKEY_WORLD_32:         printf("PGKEY_WORLD_32");      break;
  case PGKEY_WORLD_33:         printf("PGKEY_WORLD_33");      break;
  case PGKEY_WORLD_34:         printf("PGKEY_WORLD_34");      break;
  case PGKEY_WORLD_35:         printf("PGKEY_WORLD_35");      break;
  case PGKEY_WORLD_36:         printf("PGKEY_WORLD_36");      break;
  case PGKEY_WORLD_37:         printf("PGKEY_WORLD_37");      break;
  case PGKEY_WORLD_38:         printf("PGKEY_WORLD_38");      break;
  case PGKEY_WORLD_39:         printf("PGKEY_WORLD_39");      break;
  case PGKEY_WORLD_40:         printf("PGKEY_WORLD_40");      break;
  case PGKEY_WORLD_41:         printf("PGKEY_WORLD_41");      break;
  case PGKEY_WORLD_42:         printf("PGKEY_WORLD_42");      break;
  case PGKEY_WORLD_43:         printf("PGKEY_WORLD_43");      break;
  case PGKEY_WORLD_44:         printf("PGKEY_WORLD_44");      break;
  case PGKEY_WORLD_45:         printf("PGKEY_WORLD_45");      break;
  case PGKEY_WORLD_46:         printf("PGKEY_WORLD_46");      break;
  case PGKEY_WORLD_47:         printf("PGKEY_WORLD_47");      break;
  case PGKEY_WORLD_48:         printf("PGKEY_WORLD_48");      break;
  case PGKEY_WORLD_49:         printf("PGKEY_WORLD_49");      break;
  case PGKEY_WORLD_50:         printf("PGKEY_WORLD_50");      break;
  case PGKEY_WORLD_51:         printf("PGKEY_WORLD_51");      break;
  case PGKEY_WORLD_52:         printf("PGKEY_WORLD_52");      break;
  case PGKEY_WORLD_53:         printf("PGKEY_WORLD_53");      break;
  case PGKEY_WORLD_54:         printf("PGKEY_WORLD_54");      break;
  case PGKEY_WORLD_55:         printf("PGKEY_WORLD_55");      break;
  case PGKEY_WORLD_56:         printf("PGKEY_WORLD_56");      break;
  case PGKEY_WORLD_57:         printf("PGKEY_WORLD_57");      break;
  case PGKEY_WORLD_58:         printf("PGKEY_WORLD_58");      break;
  case PGKEY_WORLD_59:         printf("PGKEY_WORLD_59");      break;
  case PGKEY_WORLD_60:         printf("PGKEY_WORLD_60");      break;
  case PGKEY_WORLD_61:         printf("PGKEY_WORLD_61");      break;
  case PGKEY_WORLD_62:         printf("PGKEY_WORLD_62");      break;
  case PGKEY_WORLD_63:         printf("PGKEY_WORLD_63");      break;
  case PGKEY_WORLD_64:         printf("PGKEY_WORLD_64");      break;
  case PGKEY_WORLD_65:         printf("PGKEY_WORLD_65");      break;
  case PGKEY_WORLD_66:         printf("PGKEY_WORLD_66");      break;
  case PGKEY_WORLD_67:         printf("PGKEY_WORLD_67");      break;
  case PGKEY_WORLD_68:         printf("PGKEY_WORLD_68");      break;
  case PGKEY_WORLD_69:         printf("PGKEY_WORLD_69");      break;
  case PGKEY_WORLD_70:         printf("PGKEY_WORLD_70");      break;
  case PGKEY_WORLD_71:         printf("PGKEY_WORLD_71");      break;
  case PGKEY_WORLD_72:         printf("PGKEY_WORLD_72");      break;
  case PGKEY_WORLD_73:         printf("PGKEY_WORLD_73");      break;
  case PGKEY_WORLD_74:         printf("PGKEY_WORLD_74");      break;
  case PGKEY_WORLD_75:         printf("PGKEY_WORLD_75");      break;
  case PGKEY_WORLD_76:         printf("PGKEY_WORLD_76");      break;
  case PGKEY_WORLD_77:         printf("PGKEY_WORLD_77");      break;
  case PGKEY_WORLD_78:         printf("PGKEY_WORLD_78");      break;
  case PGKEY_WORLD_79:         printf("PGKEY_WORLD_79");      break;
  case PGKEY_WORLD_80:         printf("PGKEY_WORLD_80");      break;
  case PGKEY_WORLD_81:         printf("PGKEY_WORLD_81");      break;
  case PGKEY_WORLD_82:         printf("PGKEY_WORLD_82");      break;
  case PGKEY_WORLD_83:         printf("PGKEY_WORLD_83");      break;
  case PGKEY_WORLD_84:         printf("PGKEY_WORLD_84");      break;
  case PGKEY_WORLD_85:         printf("PGKEY_WORLD_85");      break;
  case PGKEY_WORLD_86:         printf("PGKEY_WORLD_86");      break;
  case PGKEY_WORLD_87:         printf("PGKEY_WORLD_87");      break;
  case PGKEY_WORLD_88:         printf("PGKEY_WORLD_88");      break;
  case PGKEY_WORLD_89:         printf("PGKEY_WORLD_89");      break;
  case PGKEY_WORLD_90:         printf("PGKEY_WORLD_90");      break;
  case PGKEY_WORLD_91:         printf("PGKEY_WORLD_91");      break;
  case PGKEY_WORLD_92:         printf("PGKEY_WORLD_92");      break;
  case PGKEY_WORLD_93:         printf("PGKEY_WORLD_93");      break;
  case PGKEY_WORLD_94:         printf("PGKEY_WORLD_94");      break;
  case PGKEY_WORLD_95:         printf("PGKEY_WORLD_95");      break;
  case PGKEY_KP0:              printf("PGKEY_KP0");           break;
  case PGKEY_KP1:              printf("PGKEY_KP1");           break;
  case PGKEY_KP2:              printf("PGKEY_KP2");           break;
  case PGKEY_KP3:              printf("PGKEY_KP3");           break;
  case PGKEY_KP4:              printf("PGKEY_KP4");           break;
  case PGKEY_KP5:              printf("PGKEY_KP5");           break;
  case PGKEY_KP6:              printf("PGKEY_KP6");           break;
  case PGKEY_KP7:              printf("PGKEY_KP7");           break;
  case PGKEY_KP8:              printf("PGKEY_KP8");           break;
  case PGKEY_KP9:              printf("PGKEY_KP9");           break;
  case PGKEY_KP_PERIOD:        printf("PGKEY_KP_PERIOD");     break;
  case PGKEY_KP_DIVIDE:        printf("PGKEY_KP_DIVIDE");     break;
  case PGKEY_KP_MULTIPLY:      printf("PGKEY_KP_MULTIPLY");   break;
  case PGKEY_KP_MINUS:         printf("PGKEY_KP_MINUS");      break;
  case PGKEY_KP_PLUS:          printf("PGKEY_KP_PLUS");       break;
  case PGKEY_KP_ENTER:         printf("PGKEY_KP_ENTER");      break;
  case PGKEY_KP_EQUALS:        printf("PGKEY_KP_EQUALS");     break;
  case PGKEY_UP:               printf("PGKEY_UP");            break;
  case PGKEY_DOWN:             printf("PGKEY_DOWN");          break;
  case PGKEY_RIGHT:            printf("PGKEY_RIGHT");         break;
  case PGKEY_LEFT:             printf("PGKEY_LEFT");          break;
  case PGKEY_INSERT:           printf("PGKEY_INSERT");        break;
  case PGKEY_HOME:             printf("PGKEY_HOME");          break;
  case PGKEY_END:              printf("PGKEY_END");           break;
  case PGKEY_PAGEUP:           printf("PGKEY_PAGEUP");        break;
  case PGKEY_PAGEDOWN:         printf("PGKEY_PAGEDOWN");      break;
  case PGKEY_F1:               printf("PGKEY_F1");            break;
  case PGKEY_F2:               printf("PGKEY_F2");            break;
  case PGKEY_F3:               printf("PGKEY_F3");            break;
  case PGKEY_F4:               printf("PGKEY_F4");            break;
  case PGKEY_F5:               printf("PGKEY_F5");            break;
  case PGKEY_F6:               printf("PGKEY_F6");            break;
  case PGKEY_F7:               printf("PGKEY_F7");            break;
  case PGKEY_F8:               printf("PGKEY_F8");            break;
  case PGKEY_F9:               printf("PGKEY_F9");            break;
  case PGKEY_F10:              printf("PGKEY_F10");           break;
  case PGKEY_F11:              printf("PGKEY_F11");           break;
  case PGKEY_F12:              printf("PGKEY_F12");           break;
  case PGKEY_F13:              printf("PGKEY_F13");           break;
  case PGKEY_F14:              printf("PGKEY_F14");           break;
  case PGKEY_F15:              printf("PGKEY_F15");           break;
  case PGKEY_NUMLOCK:          printf("PGKEY_NUMLOCK");       break;
  case PGKEY_CAPSLOCK:         printf("PGKEY_CAPSLOCK");      break;
  case PGKEY_SCROLLOCK:        printf("PGKEY_SCROLLOCK");     break;
  case PGKEY_RSHIFT:           printf("PGKEY_RSHIFT");        break;
  case PGKEY_LSHIFT:           printf("PGKEY_LSHIFT");        break;
  case PGKEY_RCTRL:            printf("PGKEY_RCTRL");         break;
  case PGKEY_LCTRL:            printf("PGKEY_LCTRL");         break;
  case PGKEY_RALT:             printf("PGKEY_RALT");          break;
  case PGKEY_LALT:             printf("PGKEY_LALT");          break;
  case PGKEY_RMETA:            printf("PGKEY_RMETA");         break;
  case PGKEY_LMETA:            printf("PGKEY_LMETA");         break;
  case PGKEY_LSUPER:           printf("PGKEY_LSUPER");        break;
  case PGKEY_RSUPER:           printf("PGKEY_RSUPER");        break;
  case PGKEY_MODE:             printf("PGKEY_MODE");          break;
  case PGKEY_HELP:             printf("PGKEY_HELP");          break;
  case PGKEY_PRINT:            printf("PGKEY_PRINT");         break;
  case PGKEY_SYSREQ:           printf("PGKEY_SYSREQ");        break;
  case PGKEY_BREAK:            printf("PGKEY_BREAK");         break;
  case PGKEY_MENU:             printf("PGKEY_MENU");          break;
  case PGKEY_POWER:            printf("PGKEY_POWER");         break;
  case PGKEY_EURO:             printf("PGKEY_EURO");          break;
  case PGKEY_ALPHA:            printf("PGKEY_ALPHA");         break;
  }
}
#endif

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
  u16 pg_key;
  u8  pg_uchar;
  u8  pg_lchar;
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
  { "00"                , PGKEY_AMPERSAND                     , '&'  ,  '&'  },
  { "000"               , PGKEY_AT                            , '@'  ,  '@'  },
  { "0000"              , PGKEY_SLASH                         , '/'  ,  '/'  },
  { "000000"            , PGKEY_PERCENT                       , '%'  ,  '%'  },
  { "0000000"           , PGKEY_DOLLAR                        , '$'  ,  '$'  },
  { "00000000"          , 0                 /* pound */       , 0xa3 ,  0    },
  { "0000000000000"     , PGKEY_0                             , '0'  ,  '0'  },
  { "0000000000000000"  , 0                 /* psi */         , 0    ,  0    },
//{ "00000000000000000" , 0                 /* omega */       , 0    ,  0    },
  { "00000000000000000" , PGKEY_UNDERSCORE  /* omega */       , '_'  ,  '_'  },
  { "1"                 , PGKEY_SPACE                         , ' '  ,  ' '  },
  { "1,20"              , PGKEY_1                             , '1'  ,  '1'  },
  { "11"                , PGKEY_MINUS                         , '-'  ,  '-'  },
  { "111"               , PGKEY_QUESTION                      , '?'  ,  '?'  },
  { "1111"              , PGKEY_EXCLAIM                       , '!'  ,  '!'  },
  { "11111"             , PGKEY_COMMA                         , ','  ,  ','  },
  { "111111"            , PGKEY_PERIOD                        , '.'  ,  '.'  },
  { "1111111"           , PGKEY_COLON                         , ':'  ,  ':'  },
  { "111111111"         , PGKEY_QUOTEDBL                      , '"'  ,  '"'  },
  { "1111111111"        , PGKEY_QUOTE                         , '\'' ,  '\'' },
  { "11111111111111"    , PGKEY_LEFTBRACKET                   , '('  ,  '('  },
  { "111111111111111"   , PGKEY_RIGHTBRACKET                  , ')'  ,  ')'  },
  { "1111111111111111"  , PGKEY_1                             , '1'  ,  '1'  },
  { "2"                 , PGKEY_a                             , 'A'  ,  'a'  },
  { "2,20"              , PGKEY_2                             , '2'  ,  '2'  },
  { "22"                , PGKEY_b                             , 'B'  ,  'b'  },
  { "222"               , PGKEY_c                             , 'C'  ,  'c'  },
  { "2222"              , 0                 /* angstroem */   , 0xc5 ,  0xe5 },
  { "22222"             , 0                 /* a-trema */     , 0xc4 ,  0xe4 },
  { "222222"            , 0                 /* ae */          , 0xc6 ,  0xe6 },
  { "2222222"           , 0                 /* a-grave */     , 0xc0 ,  0xe0 },
  { "22222222"          , 0                 /* c-cedille */   , 0xc7 ,  0xe7 },
  { "222222222"         , PGKEY_2                             , '2'  ,  '2'  },
  { "3"                 , PGKEY_d                             , 'D'  ,  'd'  },
  { "3,20"              , PGKEY_3                             , '3'  ,  '3'  },
  { "33"                , PGKEY_e                             , 'E'  ,  'e'  },
  { "333"               , PGKEY_f                             , 'F'  ,  'f'  },
  { "3333"              , 0                 /* e-grave */     , 0xc8 ,  0xe8 },
  { "33333"             , 0                 /* e-aigu */      , 0xc9 ,  0xe9 },
  { "333333"            , PGKEY_3                             , '3'  ,  '3'  },
  { "4"                 , PGKEY_g                             , 'G'  ,  'g'  },
  { "4,20"              , PGKEY_4                             , '4'  ,  '4'  },
  { "44"                , PGKEY_h                             , 'H'  ,  'h'  },
  { "444"               , PGKEY_i                             , 'I'  ,  'i'  },
  { "4444"              , 0                 /* i-grave */     , 0xcc ,  0xec },
  { "44444"             , PGKEY_4                             , '4'  ,  '4'  },
  { "5"                 , PGKEY_j                             , 'J'  ,  'j'  },
  { "5,20"              , PGKEY_5                             , '5'  ,  '5'  },
  { "55"                , PGKEY_k                             , 'K'  ,  'k'  },
  { "555"               , PGKEY_l                             , 'L'  ,  'l'  },
  { "5555"              , PGKEY_5                             , '5'  ,  '5'  },
  { "6"                 , PGKEY_m                             , 'M'  ,  'm'  },
  { "6,20"              , PGKEY_6                             , '6'  ,  '6'  },
  { "66"                , PGKEY_n                             , 'N'  ,  'n'  },
  { "666"               , PGKEY_o                             , 'O'  ,  'o'  },
  { "6666"              , 0                 /* n-tilde */     , 0xd1 ,  0xf1 },
  { "66666"             , 0                 /* o-trema */     , 0xd6 ,  0xf6 },
  { "666666"            , 0                 /* o-barre */     , 0xd8 ,  0xf8 },
  { "6666666"           , 0                 /* o-grave */     , 0xd2 ,  0xf2 },
  { "66666666"          , PGKEY_6                             , '6'  ,  '6'  },
  { "7"                 , PGKEY_p                             , 'P'  ,  'p'  },
  { "7,20"              , PGKEY_7                             , '7'  ,  '7'  },
  { "77"                , PGKEY_q                             , 'Q'  ,  'q'  },
  { "777"               , PGKEY_r                             , 'R'  ,  'r'  },
  { "7777"              , PGKEY_s                             , 'S'  ,  's'  },
  { "77777"             , 0                 /* beta */        , 0xdf ,  0xdf },
  { "777777"            , PGKEY_7                             , '7'  ,  '7'  },
  { "7777777"           , 0                 /* pi */          , 0xb6 ,  0xb6 },
//{ "77777777"          , 0                 /* sigma */       , 0    ,  0    },
  { "77777777"          , PGKEY_LCTRL       /* sigma */       , 0    ,  0    },
  { "8"                 , PGKEY_t                             , 'T'  ,  't'  },
  { "8,20"              , PGKEY_8                             , '8'  ,  '8'  },
  { "8666*1111111*"     , PGKEY_F3          /* [E-mail] */    , 0    ,  0    },
  { "88"                , PGKEY_u                             , 'U'  ,  'u'  },
  { "888"               , PGKEY_v                             , 'V'  ,  'v'  },
  { "8888"              , 0                 /* u-trema */     , 0xdc ,  0xfc },
  { "88888"             , 0                 /* u-grave */     , 0xd9 ,  0xf9 },
  { "888888"            , PGKEY_8                             , '8'  ,  '8'  },
  { "9"                 , PGKEY_w                             , 'W'  ,  'w'  },
  { "9,20"              , PGKEY_9                             , '9'  ,  '9'  },
  { "99"                , PGKEY_x                             , 'X'  ,  'x'  },
  { "999"               , PGKEY_y                             , 'Y'  ,  'y'  },
  { "9991111111"        , PGKEY_F1          /* [WWW] */       , 0    ,  0    },
  { "9999"              , PGKEY_z                             , 'Z'  ,  'z'  },
  { "99999"             , PGKEY_9                             , '9'  ,  '9'  },
  { "<"                 , PGKEY_LEFT                          , 0    ,  0    },
  { ">"                 , PGKEY_RIGHT                         , 0    ,  0    },
  { "c"                 , PGKEY_BACKSPACE   /* <-- */         , 8    ,  8    },
  { "e"                 , PGKEY_ESCAPE      /* [No] */        , 0    ,  0    },
  { "eseee<see<s>>s"    , PGKEY_F4          /* [PhoneBook] */ , 0    ,  0    },
  { "s"                 , PGKEY_RETURN      /* [Yes] */       , 13   ,  13   },
  /*
   * Notes:
   * - The table above must be sorted using 'LC_ALL=C sort -k 2'
   * - SHIFT-SPACE toggles CAPSLOCK
   * - OMEGA is remapped to UNDERSCORE
   * - SIGMA toggles CTRL
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
  TRACEF("> str = [%s]"NL, str);
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
  TRACEF("> str = [%s]"NL, str);

  /* lookup resulting string */
  {
    int upper_index = sizeof(key_str_table)/sizeof(KeyStr) -1;
    int lower_index = 0;
    /* Dichotomic search for the hwcode in the table */
    while(1) {
      TRACEF("['%s' '%s']"NL,
	     key_str_table[lower_index].sequence,
	     key_str_table[upper_index].sequence);
      if(!strcmp(key_str_table[lower_index].sequence, str))
	return lower_index;
      else if(!strcmp(key_str_table[upper_index].sequence, str))
	return upper_index;
      else if(upper_index-lower_index<=1)
	return -1;
      else {
	int mid_index = (upper_index+lower_index) /2;
	int mid_cmp = strcmp(key_str_table[mid_index].sequence, str);
	if(mid_cmp==0)     return mid_index;
	else if(mid_cmp>0) upper_index = mid_index;
	else               lower_index = mid_index;
      }
    }
  }

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
static int  ctrl_down;
static int  chars_in_buffer;
static char buffer[100];

/* ------------------------------------------------------------------------- */

static void cb_init_mods()
{
  TRACEF(">>> cb_init_mods()"NL);
  mod_caps  = 0;
  chars_in_buffer = 0;
  skip_line = 0;
  ctrl_down = 0;
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

static int editting(void)
{
  if(kbdfocus==0) return 0;

  switch(kbdfocus->type) {
  case PG_WIDGET_FIELD:
  case PG_WIDGET_TERMINAL:
  case PG_WIDGET_TEXTBOX:
    return 1;

  case PG_WIDGET_TOOLBAR:
  case PG_WIDGET_LABEL:
  case PG_WIDGET_SCROLL:
  case PG_WIDGET_INDICATOR:
  case PG_WIDGET_BUTTON:
  case PG_WIDGET_PANEL:
  case PG_WIDGET_POPUP:
  case PG_WIDGET_BOX:
  case PG_WIDGET_BACKGROUND:
  case PG_WIDGET_MENUITEM:
  case PG_WIDGET_CANVAS:
  case PG_WIDGET_CHECKBOX:
  case PG_WIDGET_FLATBUTTON:
  case PG_WIDGET_LISTITEM:
  case PG_WIDGET_SUBMENUITEM:
  case PG_WIDGET_RADIOBUTTON:
  case PG_WIDGET_PANELBAR:
  default:
    return 0;
  }
}

/*****************************************************************************/

#if LOCAL_DEBUG
static void __infilter_send_key(u32 type,s16 key,s16 mods)
{
  if(type==PG_TRIGGER_CHAR)
    printf("CHAR, char:['%c'=%02x]", key, key);
  else {
    printf("%s, key:[%02X=",
	    type==PG_TRIGGER_KEYUP
	    ? "KEYUP"
	    : type==PG_TRIGGER_KEYDOWN ? "KEYDOWN" : "unknown-trigger",
	    key);
    print_pgkeyname(key);
    printf("]");
  }
  printf("\r\t\t\t\t\t\tMODS:[");
  print_pgmods(mods);
  printf("]"NL);

  infilter_send_key(type, key, mods);
}

# define infilter_send_key __infilter_send_key
#endif

/*****************************************************************************/

static inline u16 calc_mods(u16 extra)
{
  return
    ( ctrl_down ? PGMOD_CTRL : 0) |
    ( mod_caps  ? PGMOD_CAPS : 0) | 
    extra;
}

static void treat_key(int index)
{
  const KeyStr *key = &key_str_table[index];
  u16 pg_code = key->pg_key;
  u16 pg_mods_extra = 0;
  u8 pg_char = 0;

  if(pg_code==0) return;

  switch(pg_code) {
  case PGKEY_CAPSLOCK:
    infilter_send_key(PG_TRIGGER_KEYDOWN, PGKEY_CAPSLOCK, calc_mods(0));
    mod_caps = !mod_caps;
    infilter_send_key(PG_TRIGGER_KEYUP, PGKEY_CAPSLOCK, calc_mods(0));
    return;

  case PGKEY_LEFT:
    if(editting()) break; /* in editable widgets, remain a left key */
    pg_code = PGKEY_TAB;  /* otherwise become a shift-tab key */
    pg_mods_extra = PGMOD_SHIFT;
    break;

  case PGKEY_RIGHT:
    if(editting()) break; /* in editable widgets, remain a right key */
    pg_code = PGKEY_TAB;  /* otherwise become a tab key */
    break;

  case PGKEY_LCTRL:
    ctrl_down = !ctrl_down;
    return;

  default:
    pg_char = key->pg_lchar;
    if(mod_caps) {
      u8 uchr = key->pg_uchar;
      if(uchr) pg_char = uchr;
    }
  }

  if(ctrl_down)
  infilter_send_key(PG_TRIGGER_KEYDOWN, pg_code, calc_mods(pg_mods_extra));
  if(pg_char) infilter_send_key(PG_TRIGGER_CHAR, pg_char, calc_mods(pg_mods_extra));
  infilter_send_key(PG_TRIGGER_KEYUP, pg_code, calc_mods(pg_mods_extra));
  ctrl_down = 0;
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
    DPRINTF("=> no key or error (read() returned <0)"NL);
    skip_line = 1;
    return -1;
  }
  
  /* echo */
  outc(fd, c);
  
  if(c=='\n') {
    static const char* ok_str = "OK\r\n";
    outs(fd, ok_str);
    TRACEF("=>OK"NL);
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
   * Listen to port and try to translate
   */
  index = cb_getchar(fd);
  if(index!=-1) {
    DPRINTF("==>[%s]"NL, key_str_table[index].sequence);
    treat_key(index);
  }

  return 1;
}

/*****************************************************************************/

static g_error cb_init(void)
{
  struct termios options;
  const char* device = get_param_str("input-ericsson-chatboard",
				     "device",
				     NULL);
  int status;

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

  /* the enable signal of the ericsson keyboard is driven by the RTS signal
   * of the device (/dev/ttyS0 or /dev/ttyS1)
   * RTS high -> keyboard enable
   */
  ioctl(cb_fd, TIOCMGET, &status);  /* get the MODEM status bits */
  status |= TIOCM_RTS;              /* set RTS to high */
  ioctl(cb_fd, TIOCMSET, status);   /* set the MODEM status bits */

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
