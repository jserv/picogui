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
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <asm/MC68VZ328.h>  /* for bus and port definition of DragonBall VZ */

/*****************************************************************************/

#define DEVICE_FILE_NAME  "/dev/ttyS1"
#define BAUDRATE          B9600

#define UMISC2_ADDR     0xfffff918
#define UMISC2          WORD_REF(UMISC2_ADDR)

/*****************************************************************************/

#define LOCAL_INFO  0
#define LOCAL_DEBUG 0
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

#if LOCAL_INFO
# define INFO(x...)    printf(__FILE__": " x)
#else
# define INFO(x...)
#endif

#if LOCAL_DEBUG
# define DPRINTF(x...) printf(__FILE__": " x)
# define WARNF(x...)   printf(__FILE__": " x)
#else
# define DPRINTF(x...)
# define WARNF(x...)   printf(__FILE__": " x)
# undef LOCAL_TRACE
# define LOCAL_TRACE 0
#endif

#if LOCAL_TRACE
# define TRACEF(x...)  printf(__FILE__": " x)
#else
# define TRACEF(x...)
#endif

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

/*****************************************************************************/

/*
 * Codes for special keys
 * ----------------------
 */

enum {
  SPu_CAPSLOCK,         SPd_CAPSLOCK, 
  SPu_CMD,              SPd_CMD, 
  SPu_CTRL,             SPd_CTRL, 
  SPu_DONE,             SPd_DONE, 
  SPu_FN,               SPd_FN, 
  SPu_LSHIFT,           SPd_LSHIFT, 
  SPu_RSHIFT,           SPd_RSHIFT, 
  SPu_RSPACE,           SPd_RSPACE, 
};


/*
 * Key definition
 * --------------
 */

typedef struct {
  u16 hw_code;
  u16 code;          
  u16 code_shift;        
  u16 code_fn;         
} HwKeyDef;


/* special code values */
#define NONE          0x0000
#define QUALMASK      0xF000
#define CODEMASK      0x03FF
#define KEYUPMASK     0x0800

#define SPECIAL       0x8000
#define SAME          0x9000
#define MOD_SHIFT     0xa000
#define CHARKEY       0xb000
#define ALPHKEY       0xc000
#define COMPOSE       0xd000

#define DEADKEY       NONE,0,0

#define SPECIALKEY(x) ( (x) | SPECIAL ),0,0
#define CHAR(x)       ( (x) | CHARKEY )
#define COMP(x)       ( (x) | COMPOSE )
#define ALPH(x)       ( (x) | ALPHKEY )
#define UP(x)         ( (x) | KEYUPMASK )
#define IGN(x)        NONE


/*
 * Key map, sorted on hw_code member to allow dichotomic search
 * ------------------------------------------------------------
 */

/* this is the table for the keyboard with corrupt baud rate (10'000 baud
 * instead of 9'600) including the corrupt values.
 */
static const HwKeyDef key_def_table[] = {
  /*------------+-------+------------------------+----------+----------------*/
  /*            |  hw   | code                   | shift    | fn             */
  /*------------+-------+------------------------+----------+----------------*/
  /*dLSHIFT  */ { 0x00ed, SPECIALKEY(SPd_LSHIFT)                             },
  /*dFN      */ { 0x02fd, SPECIALKEY(SPd_FN)                                 },
  /*dDONE    */ { 0x03fc, NONE                   , NONE     , PGKEY_ESCAPE   },
  /*dTAB     */ { 0x0af2, PGKEY_TAB              , MOD_SHIFT, SAME           },
  /*dCTRL    */ { 0x14eb, SPECIALKEY(SPd_CTRL)                               },
  /*dq       */ { 0x15ea, ALPH('q')              , ALPH('Q'), NONE           },
  /*dNUM1    */ { 0x16e9, CHAR('1')              , CHAR('!'), PGKEY_F1       },
  /*dz       */ { 0x1ae5, ALPH('z')              , ALPH('Z'), NONE           },
  /*ds       */ { 0x1be4, ALPH('s')              , ALPH('S'), NONE           },
  /*da       */ { 0x1ce3, ALPH('a')              , ALPH('A'), NONE           },
  /*dw       */ { 0x1de2, ALPH('w')              , ALPH('W'), NONE           },
  /*dNUM2    */ { 0x1ee1, CHAR('2')              , CHAR('@'), PGKEY_F2       },
  /*dDEL     */ { 0x1fe0, PGKEY_DELETE           , MOD_SHIFT, CHAR(8)        },
  /*dc       */ { 0x21de, ALPH('c')              , ALPH('C'), NONE           },
  /*dx       */ { 0x22dd, ALPH('x')              , ALPH('X'), NONE           },
  /*dd       */ { 0x23dc, ALPH('d')              , ALPH('D'), NONE           },
  /*de       */ { 0x24db, ALPH('e')              , ALPH('E'), NONE /*Euro*/  },
  /*dNUM4    */ { 0x25da, CHAR('4')              , CHAR('$'), PGKEY_F4       },
  /*dNUM3    */ { 0x26d9, CHAR('3')              , CHAR('#'), PGKEY_F3       },
  /*dUP      */ { 0x28d7, PGKEY_UP               , MOD_SHIFT, PGKEY_PAGEUP   },
  /*dLSPACE  */ { 0x29d6, CHAR(' ')              , MOD_SHIFT, IGN(Find)      },
  /*dv       */ { 0x2ad5, ALPH('v')              , ALPH('V'), NONE           },
  /*df       */ { 0x2bd4, ALPH('f')              , ALPH('F'), NONE           },
  /*dt       */ { 0x2cd3, ALPH('t')              , ALPH('T'), NONE           },
  /*dr       */ { 0x2dd2, ALPH('r')              , ALPH('R'), NONE           },
  /*dNUM5    */ { 0x2ed1, CHAR('5')              , CHAR('%'), PGKEY_F5       },
  /*dRIGHT   */ { 0x2fd0, PGKEY_RIGHT            , MOD_SHIFT, IGN(Details)   },
  /*dn       */ { 0x31ce, ALPH('n')              , ALPH('N'), NONE           },
  /*db       */ { 0x32cd, ALPH('b')              , ALPH('B'), NONE           },
  /*dh       */ { 0x33cc, ALPH('h')              , ALPH('H'), NONE           },
  /*dg       */ { 0x34cb, ALPH('g')              , ALPH('G'), NONE           },
  /*dy       */ { 0x35ca, ALPH('y')              , ALPH('Y'), NONE           },
  /*dNUM6    */ { 0x36c9, CHAR('6')              , CHAR('^'), PGKEY_F6       },
  /*dm       */ { 0x3ac5, ALPH('m')              , ALPH('M'), NONE           },
  /*dj       */ { 0x3bc4, ALPH('j')              , ALPH('J'), NONE           },
  /*du       */ { 0x3cc3, ALPH('u')              , ALPH('U'), NONE           },
  /*dNUM7    */ { 0x3dc2, CHAR('7')              , CHAR('&'), PGKEY_F7       },
  /*dNUM8    */ { 0x3ec1, CHAR('8')              , CHAR('*'), PGKEY_F8       },
  /*dCOMMA   */ { 0x41be, CHAR(',')              , CHAR('<'), COMP(',')      },
  /*dk       */ { 0x42bd, ALPH('k')              , ALPH('K'), NONE           },
  /*di       */ { 0x43bc, ALPH('i')              , ALPH('I'), NONE           },
  /*do       */ { 0x44bb, ALPH('o')              , ALPH('O'), NONE           },
  /*dNUM0    */ { 0x45ba, CHAR('0')              , CHAR(')'), PGKEY_F10      },
  /*dNUM9    */ { 0x46b9, CHAR('9')              , CHAR('('), PGKEY_F9       },
  /*dDASH    */ { 0x49b6, CHAR('.')              , CHAR('>'), COMP('^')      },
  /*dSLASH   */ { 0x4ab5, CHAR('/')              , CHAR('?'), NONE           },
  /*dl       */ { 0x4bb4, ALPH('l')              , ALPH('L'), NONE           },
  /*dSEMICOL */ { 0x4cb3, CHAR(';')              , CHAR(':'), COMP('\'')     },
  /*dp       */ { 0x4db2, ALPH('p')              , ALPH('P'), CHAR(0xa3)     },
  /*dMINUS   */ { 0x4eb1, CHAR('-')              , CHAR('_'), COMP('"')      },
  /*dQUOTE   */ { 0x52ad, CHAR('\'')             , CHAR('"'), COMP('`')      },
  /*dBRA     */ { 0x54ab, CHAR('[')              , CHAR('{'), NONE           },
  /*dEQUAL   */ { 0x55aa, CHAR('=')              , CHAR('+'), COMP('~')      },
  /*dCAPSLOCK*/ { 0x58a7, SPECIALKEY(SPd_CAPSLOCK)                           },
  /*dRSHIFT  */ { 0x59a6, SPECIALKEY(SPd_RSHIFT)                             },
  /*dENTER   */ { 0x5aa5, CHAR('\r')             , MOD_SHIFT, SAME           },
  /*dKET     */ { 0x5ba4, CHAR(']')              , CHAR('}'), IGN(Note)      },
  /*dRSPACE  */ { 0x5ca3, CHAR(' ')              , MOD_SHIFT, IGN(Find)      },
  /*dBKSLASH */ { 0x5da2, CHAR('\\')             , CHAR('|'), NONE           },
  /*dLEFT    */ { 0x5ea1, PGKEY_LEFT             , MOD_SHIFT, IGN(Show)      },
  /*dDOWN    */ { 0x609f, PGKEY_DOWN             , MOD_SHIFT, PGKEY_PAGEDOWN },
  /*dBKSPACE */ { 0x6699, CHAR(PGKEY_BACKSPACE)  , MOD_SHIFT, NONE           },
  /*dCMD     */ { 0x6798, SPECIALKEY(SPd_CMD)                                },
  /*uFN      */ { 0x827d, SPECIALKEY(SPu_FN)                                 },
  /*uDONE    */ { 0x837c, DEADKEY                                            },
  /*uTAB     */ { 0x8d72, DEADKEY                                            },
  /*uLSHIFT  */ { 0x926d, SPECIALKEY(SPu_LSHIFT)                             },
  /*uCTRL    */ { 0x946b, SPECIALKEY(SPu_CTRL)                               },
  /*uq       */ { 0x956a, DEADKEY                                            },
  /*uNUM1    */ { 0x9669, DEADKEY                                            },
  /*uz       */ { 0x9a65, DEADKEY                                            },
  /*us       */ { 0x9b64, DEADKEY                                            },
  /*ua       */ { 0x9c63, DEADKEY                                            },
  /*uw       */ { 0x9d62, DEADKEY                                            },
  /*uNUM2    */ { 0x9e61, DEADKEY                                            },
  /*uDEL     */ { 0x9f60, DEADKEY                                            },
  /*uc       */ { 0xa15e, DEADKEY                                            },
  /*ux       */ { 0xa25d, DEADKEY                                            },
  /*ud       */ { 0xa35c, DEADKEY                                            },
  /*ue       */ { 0xa45b, DEADKEY                                            },
  /*uNUM4    */ { 0xa55a, DEADKEY                                            },
  /*uNUM3    */ { 0xa659, DEADKEY                                            },
  /*uUP      */ { 0xa857, DEADKEY                                            },
  /*uLSPACE  */ { 0xa956, UP(PGKEY_SPACE)        , SAME     , NONE           },
  /*uv       */ { 0xaa55, DEADKEY                                            },
  /*uf       */ { 0xab54, DEADKEY                                            },
  /*ut       */ { 0xac53, DEADKEY                                            },
  /*ur       */ { 0xad52, DEADKEY                                            },
  /*uNUM5    */ { 0xae51, DEADKEY                                            },
  /*uRIGHT   */ { 0xaf50, DEADKEY                                            },
  /*un       */ { 0xb14e, DEADKEY                                            },
  /*ub       */ { 0xb24d, DEADKEY                                            },
  /*uh       */ { 0xb34c, DEADKEY                                            },
  /*ug       */ { 0xb44b, DEADKEY                                            },
  /*uy       */ { 0xb54a, DEADKEY                                            },
  /*uNUM6    */ { 0xb649, DEADKEY                                            },
  /*um       */ { 0xba45, DEADKEY                                            },
  /*uj       */ { 0xbb44, DEADKEY                                            },
  /*uu       */ { 0xbc43, DEADKEY                                            },
  /*uNUM7    */ { 0xbd42, DEADKEY                                            },
  /*uNUM8    */ { 0xbe41, DEADKEY                                            },
  /*uCOMMA   */ { 0xc13e, DEADKEY                                            },
  /*uk       */ { 0xc23d, DEADKEY                                            },
  /*ui       */ { 0xc33c, DEADKEY                                            },
  /*uo       */ { 0xc43b, DEADKEY                                            },
  /*uNUM0    */ { 0xc53a, DEADKEY                                            },
  /*uNUM9    */ { 0xc639, DEADKEY                                            },
  /*uDASH    */ { 0xc936, DEADKEY                                            },
  /*uSLASH   */ { 0xca35, DEADKEY                                            },
  /*ul       */ { 0xcb34, DEADKEY                                            },
  /*uSEMICOL */ { 0xcc33, DEADKEY                                            },
  /*up       */ { 0xcd32, DEADKEY                                            },
  /*uMINUS   */ { 0xce31, DEADKEY                                            },
  /*uQUOTE   */ { 0xd22d, DEADKEY                                            },
  /*uBRA     */ { 0xd42b, DEADKEY                                            },
  /*uEQUAL   */ { 0xd52a, DEADKEY                                            },
  /*uCAPSLOCK*/ { 0xd827, SPECIALKEY(SPu_CAPSLOCK)                           },
  /*uRSHIFT  */ { 0xd926, SPECIALKEY(SPu_RSHIFT)                             },
  /*uENTER   */ { 0xda25, DEADKEY                                            },
  /*uKET     */ { 0xdb24, DEADKEY                                            },
  /*uRSPACE  */ { 0xdc23, UP(PGKEY_SPACE)        , SAME     , NONE           },
  /*uBKSLASH */ { 0xdd22, DEADKEY                                            },
  /*uLEFT    */ { 0xde21, DEADKEY                                            },
  /*uDOWN    */ { 0xe01f, DEADKEY                                            },
  /*uBKSPACE */ { 0xe619, DEADKEY                                            },
  /*uCMD     */ { 0xe718, SPECIALKEY(SPu_CMD)                                },
  /*dLSHIFT  */ { 0xeda5, SPECIALKEY(SPd_LSHIFT)                             },
  /*------------+-------+------------------------+----------+----------------*/
};

/* ------------------------------------------------------------------------- */

/*
 * lookup function returning an index in the above table given a hawrdare key
 * code, or -1 if not found
 */

static int lookup(u16 hwcode)
{
  int upper_index = sizeof(key_def_table)/sizeof(HwKeyDef) -1;
  int lower_index = 0;

  INFO("remorakb: lookup(%04X)\r\n", hwcode);

  /* Dichotomic search for the hwcode in the table */
  while(1) {
    TRACEF("=> %3d[%04X] [%04X] %3d[%04X]\n",
            lower_index , key_def_table[lower_index].hw_code,
            hwcode,
            upper_index , key_def_table[upper_index].hw_code
            );

    if(key_def_table[lower_index].hw_code==hwcode)
      return lower_index;
    else if(key_def_table[upper_index].hw_code==hwcode)
      return upper_index;
    else if(upper_index-lower_index<=1)
      return -1;
    else {
      int mid_index = (upper_index+lower_index) /2;
      if(key_def_table[mid_index].hw_code>hwcode)
        upper_index = mid_index;
      else
        lower_index = mid_index;
    }
  }
}

/*****************************************************************************/

/*
 * Composition table
 */

typedef struct {
  unsigned char accent;
  unsigned char pairs[12*2];
} CompositionDef;


static const CompositionDef comp_def_table[] = {
  { ','   ,  { 'c',0xe7,  'C',0xc7,  0}},

  { '`'   ,  { 'a',0xe0,  'e',0xe8,  'i',0xec,  'o',0xf2,  'u',0xf9,
               'A',0xc0,  'E',0xc8,  'I',0xcc,  'O',0xd2,  'U',0xd9,  0}},

  { '\''  ,  { 'a',0xe1,  'e',0xe9,  'i',0xed,  'o',0xf3,  'u',0xfa,
               'A',0xc1,  'E',0xc9,  'I',0xcd,  'O',0xd3,  'U',0xda,  0}},

  { '^'   ,  { 'a',0xe2,  'e',0xea,  'i',0xee,  'o',0xf4,  'u',0xfb,
               'A',0xc2,  'E',0xca,  'I',0xce,  'o',0xd4,  'u',0xdb,  0}},

  { '~'   ,  { 'a',0xe3,                        'o',0xf5,
               'A',0xc3,                        'O',0xd5,             0}},

  { '"'   ,  { ' ',0xa8,
               'a',0xe4,  'e',0xeb,  'i',0xef,  'o',0xf6,  'u',0xfc,
               'A',0xc4,  'E',0xcb,  'I',0xcf,  'O',0xd6,  'U',0xdc,  0}},

  { 0, {0} }
};

/* ------------------------------------------------------------------------- */

/*
 * lookup function returning an accentuated char given an accent and a
 * char code, or the char code if not found
 */

static u16 compose_lookup(u16 code, u16 comp)
{
  const CompositionDef *def;
  TRACEF(">>> compose_lookup([%c], [%c])\n", code, comp);

  for(def=comp_def_table; def->accent; ++def) {
    if((def->accent&0xff) == comp) {
      const char *pair;
      for(pair=&def->pairs[0]; *pair; pair+=2) {
        if(code==*pair)
          return *(pair+1) & 0xff;
      }
    }
  }
  return code==' ' ? comp : code;
}

/*****************************************************************************/

/*
 * Static global variables
 */

/* modifiers */
static int mod_caps  = 0;
static int mod_ctr   = 0;
static int mod_fn    = 0;
static int mod_alt   = 0;
static int mod_rsh   = 0;
static int mod_lsh   = 0;
static u16 comp_char = 0;

/* misc */
static int kb_fd;
static struct termios saved_options;

/* ------------------------------------------------------------------------- */

static void kb_init_mods()
{
  TRACEF(">>> kb_init_mods()\n");
  mod_caps  = 0;
  mod_ctr   = 0;
  mod_fn    = 0;
  mod_alt   = 0;
  mod_rsh   = 0;
  mod_lsh   = 0;
  comp_char = 0;
}

/*****************************************************************************/

static int kb_getkey_index(int fd)
{
  ssize_t n;
  unsigned char c;
  int nbread = 0;
  int index;
  u16 hwcode = 0;

  TRACEF(">>> kb_getkey_index()\n");

  while(1) {
    /* try to read a valid key code */
    n = read(fd, &c, 1);
    nbread++;

    /* no key or error */
    if(n<=0)
      return -1;

    TRACEF("=>%d[%02X]\n", nbread, c);
    /* resynch sleep */
    if(c==0x74) {
      return -1;
    }

    /* key */
    hwcode <<= 8;
    hwcode |= c;

    /* test if we got two chars at least */
#if 0
    if(nbread<2)
      continue;
#endif

    /* lookup the code */
    index = lookup(hwcode);
    if(index!=-1)
      return index;

    if(nbread>=2) {
      /* mbd error */
      WARNF("*** kbd error, resetting\n");
      kb_init_mods();
      return -1;
    }
  }
}

/*****************************************************************************/

static int kb_fd_activate(int fd)
{
  int index;
  TRACEF(">>> kb_fd_activate()\n");

  /*
   * is the fd mine ?
   */
  if(fd!=kb_fd)
    return 0;

  /*
   * Listen to port and receive an index in key_def_table
   */
  index = kb_getkey_index(kb_fd);

  /*
   * Invalid index or nothing
   */
  if(index==-1)
    return 1;

  /*
   * Valid index
   */
  else {
    const HwKeyDef *def = &key_def_table[index];
    u32 pg_type = 0;
    u16 pg_code = 0;
    u16 pg_mods = 0;
    int upkey = 0;

    /* modifiers */
    if(mod_ctr)
      pg_mods |= PGMOD_CTRL;
    if(mod_alt)
      pg_mods |= PGMOD_ALT;

    /* is a special key ? */
    if((def->code&QUALMASK)==SPECIAL) {
      switch(def->code&CODEMASK) {
      case SPd_CAPSLOCK:
        mod_caps = !mod_caps;
        pg_type = PG_TRIGGER_KEYDOWN;
        pg_code = PGKEY_CAPSLOCK;
        if(mod_caps)
          pg_mods |= PGMOD_CAPS;
        break;
      case SPu_CAPSLOCK:
        pg_type = PG_TRIGGER_KEYUP;
        pg_code = PGKEY_CAPSLOCK;
        if(mod_caps)
          pg_mods |= PGMOD_CAPS;
        break;
      case SPd_CMD:           mod_alt  = 1;            break;
      case SPu_CMD:           mod_alt  = 0;            break;
      case SPd_CTRL:          mod_ctr  = 1;            break;
      case SPu_CTRL:          mod_ctr  = 0;            break;
      case SPd_FN:            mod_fn   = 1;            break;
      case SPu_FN:            mod_fn   = 0;            break;
      case SPd_LSHIFT:        mod_lsh  = 1;            break;
      case SPu_LSHIFT:        mod_lsh  = 0;            break;
      case SPd_RSHIFT:        mod_rsh  = 1;            break;
      case SPu_RSHIFT:        mod_rsh  = 0;            break;
      }
      
      /* we don't propagate special key press (maybe we should ?) */
      if(pg_code==0)
        return 1;
    }

    /* not a special key */
    else {
      u16 code, rawcode;
      u16 qual;

      /* determine map page */
      if(mod_fn)
        code = def->code_fn;
      else if(mod_lsh||mod_rsh)
        code = def->code_shift;
      else
        code = def->code;

      /* split qualifier and code */
      rawcode = code;
      qual = code & QUALMASK;
      code &= CODEMASK;

      /* according to qualifier, go to requested page */
      if(qual==SAME) {
        /* exactly like basic page */
        code = def->code;
	rawcode = code;
        qual = code & QUALMASK;
        code &= CODEMASK;
      }
      else if(qual==MOD_SHIFT) {
        /* exactly like basic page, plus a shift modifier */
        code = def->code;
	rawcode = code;
        qual = code & QUALMASK;
        code &= CODEMASK;
        pg_mods |= PGMOD_SHIFT;
      }

      if(mod_ctr && qual==ALPHKEY) {
        /* exactly like basic page, plus a ctrl modifier */
        code = def->code;
	rawcode = code;
        qual = code & QUALMASK;
        code &= CODEMASK;
      }
      else if(mod_alt && qual==ALPHKEY) {
        /* exactly like basic page, plus an alt modifier */
        code = def->code;
	rawcode = code;
        qual = code & QUALMASK;
        code &= CODEMASK;
      }
      else if(mod_caps && qual==ALPHKEY) {
        /* upper page, unless shifted (shift cancels caps) */
        code = mod_lsh||mod_rsh ? def->code : def->code_shift;
	rawcode = code;
        qual = code & QUALMASK;
        code &= CODEMASK;
      }

      upkey = (rawcode & KEYUPMASK) != 0;

      /* 
       * Interpret code
       */

      /* a dead key => exit */
      if(code==NONE) {
        return 1;
      }

      /* a composition first key => remember accent and exit */
      if(qual==COMPOSE) {
        comp_char = code;
        return 1;
      }

      /* ctrl/alt + alpha key => lowercase key + modifiers */
      if( (mod_ctr||mod_alt) && (qual==ALPHKEY) && (code>='a'&&code<='z') ) {
        code = PGKEY_a + code - 'a';
        comp_char = 0;
        qual = 0;
      }

      /* in composition mode => combine this key and last accent */
      if(comp_char) {
        if(qual==CHARKEY||qual==ALPHKEY)
          code = compose_lookup(code, comp_char);
      }

      TRACEF("\n[%s] [%s] [%s] [%s] [%s] [%s] [%s]",
              mod_caps  ? "CAPS" : "    ",
              mod_ctr   ? "CTR"  : "   ",
              mod_fn    ? "FN"   : "  ",
              mod_alt   ? "ALT"  : "   ",
              mod_rsh   ? "rSHFT": "     ",
              mod_lsh   ? "lSHFT": "     ",
              comp_char ? "comp" : "    "
              );
      
      comp_char = 0;
      pg_code = code;

      /* char or key ? */
      pg_type = (qual==CHARKEY||qual==ALPHKEY)
	? PG_TRIGGER_CHAR 
	: (upkey ? PG_TRIGGER_KEYUP : PG_TRIGGER_KEYDOWN);
    }
    

#if LOCAL_DEBUG
    if(pg_type==PG_TRIGGER_CHAR)
      DPRINTF("CHAR, char:['%c'=%02x]", pg_code, pg_code);
    else {
      DPRINTF("%s, key:[%02X=",
	       pg_type==PG_TRIGGER_KEYUP ? "KEYUP" : "KEYDOWN",
	       pg_code);
      print_pgkeyname(pg_code);
      printf("]");
    }
    printf("\r\t\t\t\t\t\tMODS:[");
    print_pgmods(pg_mods);
    printf("]\n");
#endif
    
    /*
     * Send it up !
     */
    infilter_send_key(pg_type, pg_code, pg_mods);
    if(pg_type==PG_TRIGGER_CHAR) {
      switch(pg_code) {
      case ' ':
	infilter_send_key(PG_TRIGGER_KEYDOWN, PGKEY_SPACE, pg_mods);
	infilter_send_key(PG_TRIGGER_KEYUP, PGKEY_SPACE, pg_mods);
	break;
      case '\r':
	infilter_send_key(PG_TRIGGER_KEYDOWN, PGKEY_RETURN, pg_mods);
	infilter_send_key(PG_TRIGGER_KEYUP, PGKEY_RETURN, pg_mods);
	break;
      }
    }
    else {
      switch(pg_code) {
      case PGKEY_ESCAPE:
	infilter_send_key(PG_TRIGGER_KEYUP, PGKEY_ESCAPE, pg_mods);
	break;
      }
    }
    drivermessage(PGDM_CURSORVISIBLE, pg_type!=PG_TRIGGER_CHAR, NULL);
    drivermessage(PGDM_CURSORBLKEN, 0, NULL);

    return 1;
  }
}

/*****************************************************************************/

static g_error kb_init(void)
{
  struct termios options;
  const char* device = get_param_str("remora-kb", "device", NULL);
  int baud_shift = atoi(get_param_str("remora-kb", "baud_shift", "0"));


  TRACEF(">>> kb_init()\n");

  if(device==NULL) {
    /* keyboard not connected, this is not an error */
    WARNF("remorakb: not using hard kb\n");
    kb_fd = -1;
    return success;
  }
  DPRINTF("remorakb: trying [%s] Nr %d\n",
	  device, device[strlen(device)-1] - '0');

  TRACEF("device=[%s]\n", device);
  kb_fd = open(device, O_RDONLY | O_NOCTTY | O_NDELAY);

  if(kb_fd < 0)
    return mkerror(PG_ERRT_IO,73);     /* Error opening kb device */

  tcgetattr(kb_fd, &saved_options);    /* Backup copy */
  tcgetattr(kb_fd, &options);          /* Work copy that will be modified */

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
  tcsetattr(kb_fd, TCSANOW, &options);

  /* other misc stuff */
  if(baud_shift) {
    if(device[strlen(device)-1]=='0') {
      DPRINTF("remorakb: changing UMISC\n");
      UMISC |= 0x0008;        /* set uart to invert receive polarity */
    }
    else {
      DPRINTF("remorakb: changing UMISC2\n");
      UMISC2 |= 0x0008;       /* set uart2 to invert receive polarity */
    }
  }
  else {
    DPRINTF("remorakb: no baud shift\n");
  }

  /* init states*/
  kb_init_mods();


  TRACEF("remorakb: done\n");
  return success;
}

/*****************************************************************************/

static void kb_fd_init(int *n, fd_set *readfds, struct timeval *timeout)
{
  if ((*n)<(kb_fd+1))
    *n = kb_fd+1;
  if (kb_fd>0)
    FD_SET(kb_fd, readfds);
}

/*****************************************************************************/

static void kb_close(void)
{
  TRACEF(">>> kb_close()\n");
  tcsetattr(kb_fd, TCSANOW, &saved_options);
  close(kb_fd);
}

/*****************************************************************************/

g_error remorakb_regfunc(struct inlib *i)
{
  TRACEF(">>> remorakb_regfunc()\n");
  i->init = &kb_init;
  i->fd_activate = &kb_fd_activate;
  i->fd_init = &kb_fd_init;
  i->close = &kb_close;
  return success;
}

/*****************************************************************************/
