/* $Id: errortext.c,v 1.13 2000/11/18 07:47:07 micahjd Exp $
 *
 * errortext.c - optional error message strings
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors:
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#include <pgserver/g_error.h>

/************************ Numeric Errors */

/* If TINY_MESSAGES is defined, no error
 * strings are stored- the user gets a nice
 * hexadecimal error code that is in most
 * cases not too much worse than the string :)
 */

#ifdef TINY_MESSAGES

/* Eek! Cryptic, but saves lots of space */
const char *errortext(g_error e) {
  static char err[11];
  sprintf(err,"Err 0x%04X",e);
  return err;
}

#else /* !TINY_MESSAGES */

/************************ String errors */

static const char *errors[];

const char *errortext(g_error e) {
	return errors[(e & 0xFF)-1];
}

/* This is Brandon's table of Haiku error messages :)
 * Define HAIKU_MESSAGES to activate
 */

#ifdef HAIKU_MESSAGES
static const char *errors[] = {
	/* 01 */ "A bit of a mess\nmapped to failure in SIDE\nParamater crash",
	/* 02 */ "Alignment is hard\nScrewing up can be a fact\nYour bitmap at fault",
	/* 03 */ "NO L-G-O-P\nOh please not \"X\"s for eyes\nI will have to guess",
	/* 04 */ "Your PG_WP_BITMAP\nA lost droplet in the mist\nHas escaped its handle",
	/* 05 */ "I can still see you\nAlthough you try hard to hide\nYour bitmask is bad",
	/* 06 */ "Oh no, woe is me\nYour widget has a mistake\nProperties are bad",
	/* 07 */ "I can't imagine\nExisting in negative space\nNeither your bitmap",
	/* 08 */ "No Phenolthalene\nYou are not classy enough\nIndicator lost",
	/* 09 */ "You are quite insane\nYour purple phenolthalene\nWill never exist",
	/* 10 */ "A Labeled poorly\nSiding with aluminum\nIs now invalid",
	/* 11 */ "Fuzzy math abounds\ntwo plus two is never six\nRealign your bits",
	/* 12 */ "Searching for a search\nA null font is all I find\nI think I will crash",
	/* 13 */ "I think I give up\nError too hard to handle\nFor it can't be found",
	/* 14 */ "Property not found\nTo crash inevitable\nno way around it",
	/* 15 */ "Toolbar has no side\nClient confusion abounds\nThe Server can't live",
	/* 16 */ "I have no spare time\nTo start a new argument\nAnd neither do you",
	/* 17 */ "Widgets are heavy\nAnd accelerate quickly\nGet a new handle",
	/* 18 */ "No code is perfect\nYours is not an exception\nYou are bound to bind",
	/* 19 */ "The scrollbar just broke\nI can't fix it without source\nopen and unbreak",
	/* 20 */ "Darn Error Message\nYou know widget does not work\nYou figure it out",
	/* 21 */ "A pointer mess up\nNo good pointer in our out\nI have to go pee",
	/* 22 */ "Don't look for content\nIt will not be found in here\nI would look elsewhere",
	/* 23 */ "The broken widget\nI saw fall into the heap\nWill never be found",
	/* 24 */ "The bad error flashed\nPlease don't point that thing at me\nControl Alt backslash",
	/* 25 */ "Something is leaking\nI smell stinky memory fumes\nPatch the leaky pipe",
	/* 26 */ "Handles in the stream\nBut none have bitten for you\nYour bait must be bad",
	/* 27 */ "You are the big cause\nGeneral Protection Fault\nSorry, no handle",
	/* 28 */ "Bad news overwhelms\nI hope you can handle it\nAll has just been lost",
	/* 29 */ "Perhaps I am dumb\nHowever it aint likley\nYou need to fix it",
	/* 30 */ "Application bad\nMicrosoft worse FUD FUD FUD\nDon't give in to it",
	/* 31 */ "Cute as a button\nWait, it doesn't exits yet\nProbably won't ever",
	/* 32 */ "Crooked alignment\nI can't fix it without help\nAre you my mommy?",
	/* 33 */ "Buttons lost, unfound\nBe happy, you know whats wrong\nI don't have a clue",
	/* 34 */ "What is a bitmask?\nIf I had one, I would say\nI need one.  Get it?",
	/* 35 */ "Buttons need fonts too\nGo to your local font bank\nThen, hug your button",
	/* 36 */ "Can not handle stream\nPlease fix the button string\nThen recompile",
	/* 37 */ "Buttons have feelings\nDon't start an arg it can't end\nYou will bewilder",
	/* 38 */ "Panic! bad panel\nMaybe a fatal mistake\nFix and try again",
	/* 39 */ "Panel property\nDon't touch for fear of lawyers.\nDefinistration",
	/* 40 */ "Popup, hop on Pop\nWatch Pop pop, Laugh and don't stop\nDon't argue with Pop",
	/* 41 */ "Box does not own deed\nIt isn't his property\nBad deeds cause box harm",
	/* 42 */ "Don't fight for number\nBoxing is bad for you health\nSo, be smart, don't start",
	/* 43 */ "Don't take the sides\nWithout them I have no shape\nI can't field a guess",
	/* 44 */ "The text will not fit\nFor your field font is not good\nSide property strikes",
	/* 45 */ "You should not have that\nIt is Airfield property\nYou invalid thief",
	/* 46 */ "While viewing busted\nYou got a Nifty error\nCheers, DJ Doomsday",
	/* 47 */ "Video error\nIt needs some resolution\nThis isn't my mode",
	/* 48 */ "PNM Bitmap\nWas once a best friend now dead\nI don't want errors",
	/* 49 */ "Set me up Scotty\nError in confinment beam\nOh NO, not again",
	/* 50 */ "Caution, wet socket\nDry, clean and reinsert it\nShould work better then",
	/* 51 */ "Caution, socket dry\nMoisten thouroughly, try it\nShould work better then",
	/* 52 */ "Servers don't split tips\nOnly have one, it will serve\nDon't forget the tip",
	/* 53 */ "All lines are busy\nCall again a bit later\nWe will listen then",
	/* 54 */ "Signals bother me\nCan't handle them anymore\nBummer error huh?",
	/* 55 */ "I am no Genie\nYour command is not my wish\nPlease type it again",
	/* 56 */ "Memory can leak\nIt is caused by poor coding\nIn your case it did",
	/* 57 */ "Structured time is good\nYou are very generous\nBut we must have more",
	/* 58 */ "Creation is hard\nMistakes are bound to happen\nPlease fix your mistake",
	/* 59 */ "NULL parents are bad\nThey can cause child abuse\nBut the server died",
	/* 60 */ "Only use your root\nStay within your roots or else\nDon't take what's not yours",
	/* 61 */ "XMB data\nToo small in natural size\nDon't test my patience",
	/* 62 */ "Did you want something?\nYou know the right way to ask\nFrom now on, use it",
	/* 63 */ "Request for header\nMissing some ingredience\nWhip up a new batch",
	/* 64 */ "I need little bits\nYou have whole batches of bits\nGive me a full one",
	/* 65 */ "Can not gain access\nForce other apps to play nice\nPlease remove keyboard",
	/* 66 */ "Mouse eaten by cat\nPlease wait for the hungry app\nIt will be yours soon",
	/* 67 */ "To give is devine\nBut only give what you own\nThe keyboard, not yours",
	/* 68 */ "You won't see a point\nThe device does exist here\nBut it isn't yours",
	/* 69 */ "Exercise is good\nMakes big man a bit buffer\nWe have no big man",
	/* 70 */ "\"Context Underflow\"\nWhat exactly does that mean?\nGo look it up",
	/* 71 */ "Caution, no input\nPossibly hanging by a\nNonexistant thread",
	/* 72 */ "Video modes set\nTo change this to different res\nEdit the dot C file",
	/* 73 */ "A Keyboard is good\nKnowledge of the keyboard, lost\nI will exit now",
	/* 74 */ "Check your computer\nIt has no mice to be seen\nCheck the cat's stomach",
	/* 75 */ "Drive the server nuts\nGive it a video driver\nSober ones work best",
	/* 76 */ "Drivers for one car\nOnly use one at a time\nTwo drivers can crash",
	/* 77 */ "Can't see video\nDriver error not stable\nLoad the right driver",
	/* 78 */ "Video Drivers\nNone work for hire for you\nWrite your own driver",
	/* 79 */ "Constants save the world\nGravity, is unknown to me\nSo is your constant",
	/* 80 */ "The theme is broken\nSend it to the government\nIts Presidential",
	/* 81 */ "Theme not long enough\nWait for it to regrow tail\nYank it off again",
	/* 82 */ "Checksum has gone bad\nRecompile your gui theme\nGive it one more chance",
	/* 83 */ "Theme now a victim\nFrench Revolution left it\nWithout attached head",
	/* 84 */ "Theme suprise - not there\nNot FORD tough, broke before end\nEarly EOF",
	/* 85 */ "Compile again\nI'll complain until it's fixed\nGet new compiler",
	/* 86 */ "Offset cost of run\nCommon error in your themes\nGrab new compiler",
	/* 87 */ "Theme broken, fix it\nThat way no more broken themes\nYou know how themes are",
	/* 88 */ "Stack not filling right\nA possible underflow\nFillstyle problems",
	/* 89 */ "Overflowed stack heap\nFlush, clean and try flush again\nOverflows can spill",
	/* 90 */ "Out of my known range\nVariable locality\nRun home to daddy",
	/* 91 */ "I got the first part\nThen a bad bad thing happened\nYou stopped instructing",
	/* 92 */ "Handle gone mushy\nCan't lift own group anymore\nWe'll figure it out",
	/* 93 */ "Take Charge for your stuff\nString handle not my problem\nIs NULL in getstring",
	/* 94 */ "Packet is too big\nCan't open mouth wide enough\nMouthoc() has failed",
};

#else 

/* Micah's original error table */

static const char *errors[] = {
  /* 1  */  "The parameter of a bitmap, PG_WP_SIDE, is not right",
  /* 2  */  "The attempt at alignment (in your bitmap) yields a munged PG_WP_ALIGN",
  /* 3  */  "What LGOP is that in your bitmap? I suppose I will have to guess.",
  /* 4  */  "Your PG_WP_BITMAP, a droplet in the mist, has escaped its handle",
  /* 5  */  "PG_WP_BITMASK is invalid. No transparency for you!",
  /* 6  */  "Invalid property for bitmap widget. I shall complain about it!",
  /* 7  */  "Indicator with negative size not render well. Invert space-time and try again",
  /* 8  */  "Indicator was lost; invalid side",
  /* 9  */  "What strange property you specify for indicator; nowhere to be found",
  /* 10 */  "Invalid PG_WP_SIDE for a label",
  /* 11 */  "Nice try... Bad ALIGN value for label",
  /* 12 */  "Your label font is nowhere to be seen...",
  /* 13 */  "Strings, labels, everywhere. Your handle not ever found.",
  /* 14 */  "Your client lib newer that I: label property not found",
  /* 15 */  "Invalid PG_WP_SIDE of toolbar.  Where shall I put it?",
  /* 16 */  "Toolbars do not have many properties.  Do not invent new ones without telling me.",
  /* 17 */  "In binding the scrollbar, you have misplaced the handle: cannot lift widget",
  /* 18 */  "The widget you wish to bind, has no PG_WP_SCROLL",
  /* 19 */  "The property for scrollbar is invalid. Abort, retry, fail?",
  /* 20 */  "The widget you seek to create cannot exist",
  /* 21 */  "A mess of pointers; something must go wrong. widget has no in and out pointers",
  /* 22 */  "Derive constant not understood. Do not drink and derive.",
  /* 23 */  "This widget is stubborn and antisocial",
  /* 24 */  "The pointer to memory has nowhere to point",
  /* 25 */  "Insufficient... um... what's that word?",
  /* 26 */  "You step in the stream, but the water has moved on; This handle is not here",
  /* 27 */  "Access Denied! No handle for you!",
  /* 28 */  "The handle you seek has been found, but it is not what you are looking for",
  /* 29 */  "The handle you try to bequeath is invalid. Your heirs are disappointed",
  /* 30 */  "The application type you need does not exist.",
  /* 31 */  "Bad PG_WP_SIDE for button.",
  /* 32 */  "Button can't be aligned with that",
  /* 33 */  "The bitmap you specify for a button can not be found",
  /* 34 */  "Button bitmask is missing!",
  /* 35 */  "Button font can't be found",
  /* 36 */  "Doh! Bad string handle for button!",
  /* 37 */  "A nonexistant button property does not exist",
  /* 38 */  "Yet another error; Bad PG_WP_SIDE for a panel",
  /* 39 */  "Invalid property for the panel",
  /* 40 */  "Ha! Popups have no properties!",
  /* 41 */  "Bad side parameter for a box",
  /* 42 */  "Invalid property for a box",
  /* 43 */  "Invalid side property for the field",
  /* 44 */  "Bad font for field",
  /* 45 */  "Invalid field property",
  /* 46 */  "Error initializing video",
  /* 47 */  "Error setting video mode",
  /* 48 */  "Invalid PNM bitmap",
  /* 49 */  "Error in WSAStartup???",
  /* 50 */  "Error in socket???",
  /* 51 */  "Something's wrong with setsockopt()",
  /* 52 */  "Error in bind() - Is there already a PicoGUI server running here?",
  /* 53 */  "Error in listen() Hmm...",
  /* 54 */  "Argh! Can't set up a signal handler",
  /* 55 */  "Unable to execute the session manager. Check the command line",
  /* 56 */  "Memory leak detected on exit.  Fire up gdb and call a plumber",
  /* 57 */  "Request data structure is too small. It's all the client lib's fault!",
  /* 58 */  "mkwidget can't create this widget... Try something else",
  /* 59 */  "<gasp>... You tried to derive a widget from a NULL parent!",
  /* 60 */  "Stay inside your root widget",
  /* 61 */  "XBM data is too small",
  /* 62 */  "An undefined request (just recieved); Nothing to do-- but give an error",
  /* 63 */  "Incomplete request header found in batch",
  /* 64 */  "Incomplete request data in batch",
  /* 65 */  "Can't get exclusive keyboard access. Another app is already being selfish",
  /* 66 */  "Can't get exclusive pointing device access, another app is using it",
  /* 67 */  "How can you expect to give up the keyboard when you are not the owner?",
  /* 68 */  "You don't own that pointing device!",
  /* 69 */  "Can't find a connection buffer for the client! Network code must be haunted",
  /* 70 */  "Context underflow",
  /* 71 */  "Can't create the input thread!",
  /* 72 */  "This driver doesn't support changing video modes",
  /* 73 */  "Error initializing keyboard",
  /* 74 */  "Error initializing mouse",
  /* 75 */  "Nonexistant input driver",
  /* 76 */  "This input driver is already loaded",
  /* 77 */  "Nonexistant video driver",
  /* 78 */  "All installed video drivers failed!",
  /* 79 */  "Unknown PG_APPSPEC constant!",
  /* 80 */  "This is not a PicoGUI theme file (bad magic number)",
  /* 81 */  "Length mismatch in PicoGUI theme file (possible file truncation)",
  /* 82 */  "Bad checksum in PicoGUI theme file (probable file corruption)",
  /* 83 */  "Theme file does not have a header!",
  /* 84 */  "Unexpected EOF in theme file (bug in theme compiler?)",
  /* 85 */  "Theme heap overflow (bug in theme compiler?)",
  /* 86 */  "Out-of-range offset in theme file (bug in theme compiler?)",
  /* 87 */  "Unknown loader in theme (theme is newer than server?)",
  /* 88 */  "Stack underflow in fillstyle interpreter",
  /* 89 */  "Stack overflow in fillstyle interpreter",
  /* 90 */  "Local variable out of range in fillstyle",
  /* 91 */  "Fillstyle opcode parameter truncated",
  /* 92 */  "Invalid handle in handle_group()",
  /* 93 */  "Dereferenced string handle is null in getstring",
  /* 94 */  "Request packet too big; memory allocation failed",
};

#endif /* HAIKU_MESSAGES */
#endif /* TINY_MESSAGES */

/* The End */




