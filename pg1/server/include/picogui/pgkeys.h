/* $Id$
 *
 * pgkeys.h - Constants for the keyboard keys and modifiers
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
 * Contributors:
 * 
 * 
 * 
 */

#ifndef __PGKEYS_H
#define __PGKEYS_H

/* Keysyms for PG_TRIGGER_KEYUP and PG_TRIGGER_KEYDOWN.
   These are copied from SDL's SDL_keysym.h
   (because I think it's a good character set! Thank you SDL!
   SDL_keysym.h is Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga
   go to www.libsdl.org!)
*/

#define PGKEY_BACKSPACE          8
#define PGKEY_TAB  	         9 
#define PGKEY_CLEAR  	         12
#define PGKEY_RETURN  	         13
#define PGKEY_PAUSE  	         19
#define PGKEY_ESCAPE           	 27
#define PGKEY_SPACE        	 32
#define PGKEY_EXCLAIM    	 33
#define PGKEY_QUOTEDBL           34
#define PGKEY_HASH      	 35
#define PGKEY_DOLLAR    	 36
#define PGKEY_PERCENT            37
#define PGKEY_AMPERSAND          38
#define PGKEY_QUOTE     	 39
#define PGKEY_LEFTPAREN  	 40
#define PGKEY_RIGHTPAREN  	 41
#define PGKEY_ASTERISK  	 42
#define PGKEY_PLUS      	 43
#define PGKEY_COMMA       	 44
#define PGKEY_MINUS        	 45
#define PGKEY_PERIOD      	 46
#define PGKEY_SLASH     	 47
#define PGKEY_0         	 48
#define PGKEY_1         	 49
#define PGKEY_2          	 50
#define PGKEY_3         	 51
#define PGKEY_4         	 52
#define PGKEY_5         	 53
#define PGKEY_6         	 54
#define PGKEY_7           	 55
#define PGKEY_8             	 56
#define PGKEY_9          	 57
#define PGKEY_COLON        	 58
#define PGKEY_SEMICOLON  	 59
#define PGKEY_LESS        	 60
#define PGKEY_EQUALS      	 61
#define PGKEY_GREATER   	 62
#define PGKEY_QUESTION  	 63
#define PGKEY_AT         	 64
#define PGKEY_LEFTBRACKET  	 91
#define PGKEY_BACKSLASH  	 92
#define PGKEY_RIGHTBRACKET  	 93
#define PGKEY_CARET     	 94
#define PGKEY_UNDERSCORE  	 95
#define PGKEY_BACKQUOTE  	 96
#define PGKEY_a         	 97
#define PGKEY_b         	 98
#define PGKEY_c          	 99
#define PGKEY_d            	 100
#define PGKEY_e         	 101
#define PGKEY_f         	 102
#define PGKEY_g         	 103
#define PGKEY_h         	 104
#define PGKEY_i          	 105
#define PGKEY_j         	 106
#define PGKEY_k         	 107
#define PGKEY_l          	 108
#define PGKEY_m          	 109
#define PGKEY_n          	 110
#define PGKEY_o          	 111
#define PGKEY_p            	 112
#define PGKEY_q                	 113
#define PGKEY_r           	 114
#define PGKEY_s           	 115
#define PGKEY_t           	 116
#define PGKEY_u           	 117
#define PGKEY_v           	 118
#define PGKEY_w           	 119
#define PGKEY_x           	 120
#define PGKEY_y           	 121
#define PGKEY_z           	 122
#define PGKEY_LEFTBRACE          123
#define PGKEY_PIPE               124
#define PGKEY_RIGHTBRACE         125
#define PGKEY_TILDE              126
#define PGKEY_DELETE           	 127
#define PGKEY_WORLD_0            160		/* 0xA0 */
#define PGKEY_WORLD_1            161
#define PGKEY_WORLD_2            162
#define PGKEY_WORLD_3            163
#define PGKEY_WORLD_4            164
#define PGKEY_WORLD_5  	         165
#define PGKEY_WORLD_6  	         166
#define PGKEY_WORLD_7      	 167
#define PGKEY_WORLD_8  	         168
#define PGKEY_WORLD_9  	         169
#define PGKEY_WORLD_10  	 170
#define PGKEY_WORLD_11  	 171
#define PGKEY_WORLD_12  	 172
#define PGKEY_WORLD_13  	 173
#define PGKEY_WORLD_14  	 174
#define PGKEY_WORLD_15  	 175
#define PGKEY_WORLD_16  	 176
#define PGKEY_WORLD_17  	 177
#define PGKEY_WORLD_18  	 178
#define PGKEY_WORLD_19  	 179
#define PGKEY_WORLD_20  	 180
#define PGKEY_WORLD_21  	 181
#define PGKEY_WORLD_22  	 182
#define PGKEY_WORLD_23  	 183
#define PGKEY_WORLD_24  	 184
#define PGKEY_WORLD_25  	 185
#define PGKEY_WORLD_26  	 186
#define PGKEY_WORLD_27  	 187
#define PGKEY_WORLD_28  	 188
#define PGKEY_WORLD_29  	 189
#define PGKEY_WORLD_30  	 190
#define PGKEY_WORLD_31  	 191
#define PGKEY_WORLD_32  	 192
#define PGKEY_WORLD_33  	 193
#define PGKEY_WORLD_34  	 194
#define PGKEY_WORLD_35  	 195
#define PGKEY_WORLD_36  	 196
#define PGKEY_WORLD_37  	 197
#define PGKEY_WORLD_38  	 198
#define PGKEY_WORLD_39  	 199
#define PGKEY_WORLD_40  	 200
#define PGKEY_WORLD_41  	 201
#define PGKEY_WORLD_42  	 202
#define PGKEY_WORLD_43  	 203
#define PGKEY_WORLD_44  	 204
#define PGKEY_WORLD_45  	 205
#define PGKEY_WORLD_46  	 206
#define PGKEY_WORLD_47  	 207
#define PGKEY_WORLD_48  	 208
#define PGKEY_WORLD_49  	 209
#define PGKEY_WORLD_50  	 210
#define PGKEY_WORLD_51  	 211
#define PGKEY_WORLD_52  	 212
#define PGKEY_WORLD_53  	 213
#define PGKEY_WORLD_54  	 214
#define PGKEY_WORLD_55  	 215
#define PGKEY_WORLD_56  	 216
#define PGKEY_WORLD_57  	 217
#define PGKEY_WORLD_58  	 218
#define PGKEY_WORLD_59  	 219
#define PGKEY_WORLD_60  	 220
#define PGKEY_WORLD_61  	 221
#define PGKEY_WORLD_62  	 222
#define PGKEY_WORLD_63  	 223
#define PGKEY_WORLD_64  	 224
#define PGKEY_WORLD_65  	 225
#define PGKEY_WORLD_66  	 226
#define PGKEY_WORLD_67  	 227
#define PGKEY_WORLD_68  	 228
#define PGKEY_WORLD_69  	 229
#define PGKEY_WORLD_70  	 230
#define PGKEY_WORLD_71  	 231
#define PGKEY_WORLD_72  	 232
#define PGKEY_WORLD_73  	 233
#define PGKEY_WORLD_74  	 234
#define PGKEY_WORLD_75  	 235
#define PGKEY_WORLD_76  	 236
#define PGKEY_WORLD_77  	 237
#define PGKEY_WORLD_78  	 238
#define PGKEY_WORLD_79  	 239
#define PGKEY_WORLD_80  	 240
#define PGKEY_WORLD_81  	 241
#define PGKEY_WORLD_82  	 242
#define PGKEY_WORLD_83  	 243
#define PGKEY_WORLD_84  	 244
#define PGKEY_WORLD_85  	 245
#define PGKEY_WORLD_86  	 246
#define PGKEY_WORLD_87  	 247
#define PGKEY_WORLD_88  	 248
#define PGKEY_WORLD_89  	 249
#define PGKEY_WORLD_90  	 250
#define PGKEY_WORLD_91  	 251
#define PGKEY_WORLD_92  	 252
#define PGKEY_WORLD_93  	 253
#define PGKEY_WORLD_94  	 254
#define PGKEY_WORLD_95  	 255		/* 0xFF */
#define PGKEY_KP0           	 256
#define PGKEY_KP1        	 257
#define PGKEY_KP2        	 258
#define PGKEY_KP3        	 259
#define PGKEY_KP4        	 260
#define PGKEY_KP5        	 261
#define PGKEY_KP6        	 262
#define PGKEY_KP7        	 263
#define PGKEY_KP8        	 264
#define PGKEY_KP9        	 265
#define PGKEY_KP_PERIOD  	 266
#define PGKEY_KP_DIVIDE  	 267
#define PGKEY_KP_MULTIPLY  	 268
#define PGKEY_KP_MINUS  	 269
#define PGKEY_KP_PLUS        	 270
#define PGKEY_KP_ENTER  	 271
#define PGKEY_KP_EQUALS  	 272
#define PGKEY_UP        	 273
#define PGKEY_DOWN        	 274
#define PGKEY_RIGHT        	 275
#define PGKEY_LEFT        	 276
#define PGKEY_INSERT        	 277
#define PGKEY_HOME        	 278
#define PGKEY_END        	 279
#define PGKEY_PAGEUP        	 280
#define PGKEY_PAGEDOWN  	 281
#define PGKEY_F1        	 282
#define PGKEY_F2        	 283
#define PGKEY_F3        	 284
#define PGKEY_F4        	 285
#define PGKEY_F5        	 286
#define PGKEY_F6        	 287
#define PGKEY_F7        	 288
#define PGKEY_F8        	 289
#define PGKEY_F9        	 290
#define PGKEY_F10        	 291
#define PGKEY_F11        	 292
#define PGKEY_F12        	 293
#define PGKEY_F13        	 294
#define PGKEY_F14        	 295
#define PGKEY_F15        	 296
#define PGKEY_NUMLOCK        	 300
#define PGKEY_CAPSLOCK  	 301
#define PGKEY_SCROLLOCK  	 302
#define PGKEY_RSHIFT        	 303
#define PGKEY_LSHIFT        	 304
#define PGKEY_RCTRL        	 305
#define PGKEY_LCTRL        	 306
#define PGKEY_RALT        	 307
#define PGKEY_LALT        	 308
#define PGKEY_RMETA        	 309
#define PGKEY_LMETA        	 310
#define PGKEY_LSUPER        	 311		/* Left "Windows" key */
#define PGKEY_RSUPER        	 312		/* Right "Windows" key */
#define PGKEY_MODE        	 313		/* "Alt Gr" key */
#define PGKEY_HELP  	         315
#define PGKEY_PRINT        	 316
#define PGKEY_SYSREQ        	 317
#define PGKEY_BREAK        	 318
#define PGKEY_MENU        	 319
#define PGKEY_POWER        	 320		/* Power Macintosh power key */
#define PGKEY_EURO        	 321		/* Some european keyboards */
#define PGKEY_ALPHA              322   /* Selects letters on a numeric keypad
					* (for celphones and similar devices) */

/**
 * The following are used only by the BTKEY input driver, to signal
 * various device-related events.
 */
#define PG_KBD_CONNECTED                  323
#define PG_KBD_NOT_CONNECTED              324
#define PG_PHONE_CONNECTED                325
#define PG_PHONE_NOT_CONNECTED            326
#define PG_PHONE_KEY_FORWARDING_ENABLED   327
#define PG_PHONE_KEY_FORWARDING_DISABLED  328
#define PG_PHONE_KEY_BT_STOP              329
#define PG_PHONE_KEY_BT_START             330

#define PGKEY_MAX                         330

/* Modifier keys (also from SDL) */
#define PGMOD_LSHIFT  0x0001
#define PGMOD_RSHIFT  0x0002
#define PGMOD_SHIFT   0x0003
#define PGMOD_LCTRL   0x0040
#define PGMOD_RCTRL   0x0080
#define PGMOD_CTRL    0x00C0
#define PGMOD_LALT    0x0100
#define PGMOD_RALT    0x0200
#define PGMOD_ALT     0x0300
#define PGMOD_LMETA   0x0400
#define PGMOD_RMETA   0x0800
#define PGMOD_META    0x0C00
#define PGMOD_NUM     0x1000
#define PGMOD_CAPS    0x2000
#define PGMOD_MODE    0x4000

#endif /* __PGKEYS_H */

/* The End */

