/*
 *  Copyright (C) SSV Embedded Systems GmbH, Arnd Bergmann <abe@ist1.de>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive for
 *  more details.
 *
 *  Definitions and structures for accessing sed133x based LCD controllers
 *
 *  This file is shared between PicoGUI and the linux kernel driver. If you make 
 *  changes here, remember to update _both_ files!
 */

#ifndef _SED133X_H_
#define _SED133X_H_

/* write a command to the sed133x chip, first the command code, then
   further data bytes */
#if 0
#define sed_command_l(cmd,dat,len) do { \
	outb((cmd),(u32)sed133x_command_port); \
	outsb(sed133x_data_port,(dat),(len)); \
} while (0)
#else
#define sed_command_l(cmd,dat,len) do { \
	unsigned int i; \
	/* volatile int j; for (j= 0; j < 100; j++) {} */\
	outb((cmd),(u32)sed133x_command_port); \
	for (i=0; i < len;i++) { \
		/* for (j=0; j < 100; j++) {} */\
		outb((dat)[i],(u32)sed133x_data_port); \
	} \
} while (0)
#endif

/* fill the memory area at the cursor position */ 
#define sed_memset(dat,len) do { \
	unsigned int i; \
	outb(MWRITE,sed133x_command_port); \
	for (i=(len);i;i--) \
		outb((dat),sed133x_data_port); \
} while (0)

/* similar to sed_command_l, left for debugging purposes */
#define sed_read_l(cmd,dat,len) do { \
	unsigned int i; \
	outb((cmd),sed133x_command_port); \
	for (i=0; i < (len); i++) {\
		(dat)[i] = inb(sed133x_command_port); \
	} \
} while (0)

/* to fix flickering, you can only write after the rising edge of 
   bit 6 on the data port */
#define sed_wait_ready() do {  \
	while (!(inb(sed133x_data_port) & 0x40)); \
	while ((inb(sed133x_data_port) & 0x40)); \
} while (0)

/* start of the video memory area / end of character memory */
#define SED133X_GFX_OFFSET 8192

/* convenience macros for fixed length data structures */
#define sed_command(cmd,dat) sed_command_l(cmd,(u8 *)&(dat),sizeof(dat))
#define sed_read(cmd,dat) sed_read_l(cmd,(u8 *)&(dat),sizeof(dat))

/* XXX the macro did not work when using FLICKER_FIX. why? */
//static inline void command_l(u8 cmd, u8*dat, unsigned int len)
//	{ sed_command_l(cmd,dat,len); }

/* complete list of command codes, hopefully correct */
enum sed133x_commands {
  /* *COMMAND* 	*CODE*      *DESCRIPTION*            *LENGTH* */

  /* system control */
    SYSTEM_SET	= 0x40, /* initialize device/display 	 8 */
    SLEEP_IN	= 0x53, /* enter standby mode 		 0 */
  /* display control */
    DISP_OFF	= 0x58, /* disable display/flashing 	 1 */
    DISP_ON	= 0x59, /* enable display/flashing	 1 */
    SCROLL	= 0x44, /* display start address 	10 */
    CSRFORM	= 0x5d, /* cursor type 			 2 */
    CGRAM_ADR	= 0x5c, /* character data start address  2 */ 
    CSRDIR	= 0x4c, /* direction of cursor movement  0 */
           /* ... 0x4f */
    HDOT_SCR	= 0x5a, /* horizontal scroll position 	 1 */
    OVLAY	= 0x5b, /* display overlay format 	 1 */
  /* drawing control */
    CSRW	= 0x46, /* cursor address (write) 	 2 */
    CSRR	= 0x47, /* cursor address (read)  	 2 */
  /* memory control */
    MWRITE	= 0x42, /* write to display memory 	any */
    MREAD	= 0x43  /* read display memory 		any */
};

struct sed133x_system_set {
	u8 m0 : 1; /* external character rom */
	u8 m1 : 1; /* ram area for user defined characters */
	u8 m2 : 1; /* 8 / 16 pixel height for character rom */
	u8 ws : 1; /* single / dual panel */
	u8 on : 1; /* reserved (1) */
	u8 iv : 1; /* screen offset for inverse mode */
	u8 tl : 1; /* LCD / TV mode */
	u8 dr : 1; /* additional shift-clock cycles on two-panel LCD*/
	
	u8 fx : 3; /* character width */
	u8    : 4;
	u8 wf : 1; /* 16-line / two-frame AC drive */

	u8 fy : 4; /* character height */
	u8    : 4;

	u8 cr;     /* address range of one line */
	u8 tcr;    /* line length (in bytes) */
	u8 lf;     /* lines per frame */
	u16 ap;    /* horizontal address range of display */
} __attribute__((packed));

struct sed133x_displ_onoff {
	enum { ATTR_OFF, ATTR_ON, ATTR_FLASH, ATTR_FFLASH }
	fc  : 2, /* cursor attributes   */
        fp1 : 2, /* first screen block  */
        fp2 : 2, /* second screen block */
        fp3 : 2; /* third screen block  */
} __attribute__((packed));

struct sed133x_csrform {
	u8 x : 4; /* horizontal size   */
	u8   : 4;
	u8 y : 4; /* vertical size     */
	u8   : 3;
	u8 cm :1; /* underline / block */
} __attribute__((packed));

struct sed133x_ovlay {
	enum { MX_OR, MX_XOR, MX_AND, MX_POR }
	mx   : 2; /* composition method */
	enum { DM_TEXT, DM_GRAPHICS }
	dm1 : 1, /* display mode block 1 */
	dm3 : 1; /* display mode block 2 */
	u8 ov : 1; /* two/three layer overlay */
	u8 : 3;
} __attribute__((packed));

struct sed133x_scroll {
	u16 sad1;
	u8 sl1;
} __attribute__((packed));

struct sed133x_scroll1 {
	u16 sad1;
	u8 sl1;
	u16 sad2;
	u8 sl2;
	u16 sad3;
//	u16 sad4;
} __attribute__((packed));

enum sed133x_cursordir { CSR_RIGHT=0, CSR_LEFT, CSR_UP, CSR_DOWN };

#endif
