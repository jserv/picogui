#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <picogui/constants.h>
#include <picogui/canvas.h>
#include <picogui/network.h>
#include <picogui/pgkeys.h>
#include <picogui/theme.h>
#include <picogui/client_c.h>

/***************************************** Header cruft made by h2xs *******/

static int
not_here(char *s)
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static double
constant__H_PG_C(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ANVAS")) {	/* _H_PG_C removed */
#ifdef _H_PG_CANVAS
	    return _H_PG_CANVAS;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 7, "LI_C")) {	/* _H_PG_C removed */
#ifdef _H_PG_CLI_C
	    return _H_PG_CLI_C;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 7, "ONSTANTS")) {	/* _H_PG_C removed */
#ifdef _H_PG_CONSTANTS
	    return _H_PG_CONSTANTS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant__(char *name, int len, int arg)
{
    if (1 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[1 + 5]) {
    case 'C':
	if (!strnEQ(name + 1,"H_PG_", 5))
	    break;
	return constant__H_PG_C(name, len, arg);
    case 'N':
	if (strEQ(name + 1, "H_PG_NETWORK")) {	/* _ removed */
#ifdef _H_PG_NETWORK
	    return _H_PG_NETWORK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_NWE_P(char *name, int len, int arg)
{
    if (8 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 4]) {
    case 'D':
	if (strEQ(name + 8, "NTR_DOWN")) {	/* PG_NWE_P removed */
#ifdef PG_NWE_PNTR_DOWN
	    return PG_NWE_PNTR_DOWN;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 8, "NTR_MOVE")) {	/* PG_NWE_P removed */
#ifdef PG_NWE_PNTR_MOVE
	    return PG_NWE_PNTR_MOVE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 8, "NTR_UP")) {	/* PG_NWE_P removed */
#ifdef PG_NWE_PNTR_UP
	    return PG_NWE_PNTR_UP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_NWE_KBD_K(char *name, int len, int arg)
{
    if (12 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 2]) {
    case 'D':
	if (strEQ(name + 12, "EYDOWN")) {	/* PG_NWE_KBD_K removed */
#ifdef PG_NWE_KBD_KEYDOWN
	    return PG_NWE_KBD_KEYDOWN;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 12, "EYUP")) {	/* PG_NWE_KBD_K removed */
#ifdef PG_NWE_KBD_KEYUP
	    return PG_NWE_KBD_KEYUP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_NWE_K(char *name, int len, int arg)
{
    if (8 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 3]) {
    case 'C':
	if (strEQ(name + 8, "BD_CHAR")) {	/* PG_NWE_K removed */
#ifdef PG_NWE_KBD_CHAR
	    return PG_NWE_KBD_CHAR;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 8,"BD_", 3))
	    break;
	return constant_PG_NWE_KBD_K(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_N(char *name, int len, int arg)
{
    if (4 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 3]) {
    case 'B':
	if (strEQ(name + 4, "WE_BGCLICK")) {	/* PG_N removed */
#ifdef PG_NWE_BGCLICK
	    return PG_NWE_BGCLICK;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 4,"WE_", 3))
	    break;
	return constant_PG_NWE_K(name, len, arg);
    case 'P':
	if (!strnEQ(name + 4,"WE_", 3))
	    break;
	return constant_PG_NWE_P(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_O(char *name, int len, int arg)
{
    if (4 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 3]) {
    case 'K':
	if (strEQ(name + 4, "WN_KEYBOARD")) {	/* PG_O removed */
#ifdef PG_OWN_KEYBOARD
	    return PG_OWN_KEYBOARD;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 4, "WN_POINTER")) {	/* PG_O removed */
#ifdef PG_OWN_POINTER
	    return PG_OWN_POINTER;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 4, "WN_SYSEVENTS")) {	/* PG_O removed */
#ifdef PG_OWN_SYSEVENTS
	    return PG_OWN_SYSEVENTS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_PO(char *name, int len, int arg)
{
    if (5 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 4]) {
    case 'A':
	if (strEQ(name + 5, "PUP_ATCURSOR")) {	/* PG_PO removed */
#ifdef PG_POPUP_ATCURSOR
	    return PG_POPUP_ATCURSOR;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 5, "PUP_CENTER")) {	/* PG_PO removed */
#ifdef PG_POPUP_CENTER
	    return PG_POPUP_CENTER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_P(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'O':
	return constant_PG_PO(name, len, arg);
    case 'R':
	if (strEQ(name + 4, "ROTOCOL_VER")) {	/* PG_P removed */
#ifdef PG_PROTOCOL_VER
	    return PG_PROTOCOL_VER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_A_N(char *name, int len, int arg)
{
    switch (name[6 + 0]) {
    case 'E':
	if (strEQ(name + 6, "E")) {	/* PG_A_N removed */
#ifdef PG_A_NE
	    return PG_A_NE;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 6, "W")) {	/* PG_A_N removed */
#ifdef PG_A_NW
	    return PG_A_NW;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_A_S(char *name, int len, int arg)
{
    switch (name[6 + 0]) {
    case 'E':
	if (strEQ(name + 6, "E")) {	/* PG_A_S removed */
#ifdef PG_A_SE
	    return PG_A_SE;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 6, "W")) {	/* PG_A_S removed */
#ifdef PG_A_SW
	    return PG_A_SW;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_A_(char *name, int len, int arg)
{
    switch (name[5 + 0]) {
    case 'A':
	if (strEQ(name + 5, "ALL")) {	/* PG_A_ removed */
#ifdef PG_A_ALL
	    return PG_A_ALL;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (strEQ(name + 5, "BOTTOM")) {	/* PG_A_ removed */
#ifdef PG_A_BOTTOM
	    return PG_A_BOTTOM;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 5, "CENTER")) {	/* PG_A_ removed */
#ifdef PG_A_CENTER
	    return PG_A_CENTER;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 5, "LEFT")) {	/* PG_A_ removed */
#ifdef PG_A_LEFT
	    return PG_A_LEFT;
#else
	    goto not_there;
#endif
	}
    case 'N':
	return constant_PG_A_N(name, len, arg);
    case 'R':
	if (strEQ(name + 5, "RIGHT")) {	/* PG_A_ removed */
#ifdef PG_A_RIGHT
	    return PG_A_RIGHT;
#else
	    goto not_there;
#endif
	}
    case 'S':
	return constant_PG_A_S(name, len, arg);
    case 'T':
	if (strEQ(name + 5, "TOP")) {	/* PG_A_ removed */
#ifdef PG_A_TOP
	    return PG_A_TOP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APP_(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'N':
	if (strEQ(name + 7, "NORMAL")) {	/* PG_APP_ removed */
#ifdef PG_APP_NORMAL
	    return PG_APP_NORMAL;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 7, "TOOLBAR")) {	/* PG_APP_ removed */
#ifdef PG_APP_TOOLBAR
	    return PG_APP_TOOLBAR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APPSPEC_S(char *name, int len, int arg)
{
    if (12 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 3]) {
    case '\0':
	if (strEQ(name + 12, "IDE")) {	/* PG_APPSPEC_S removed */
#ifdef PG_APPSPEC_SIDE
	    return PG_APPSPEC_SIDE;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 12, "IDEMASK")) {	/* PG_APPSPEC_S removed */
#ifdef PG_APPSPEC_SIDEMASK
	    return PG_APPSPEC_SIDEMASK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APPSPEC_MI(char *name, int len, int arg)
{
    if (13 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[13 + 1]) {
    case 'H':
	if (strEQ(name + 13, "NHEIGHT")) {	/* PG_APPSPEC_MI removed */
#ifdef PG_APPSPEC_MINHEIGHT
	    return PG_APPSPEC_MINHEIGHT;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 13, "NWIDTH")) {	/* PG_APPSPEC_MI removed */
#ifdef PG_APPSPEC_MINWIDTH
	    return PG_APPSPEC_MINWIDTH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APPSPEC_MA(char *name, int len, int arg)
{
    if (13 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[13 + 1]) {
    case 'H':
	if (strEQ(name + 13, "XHEIGHT")) {	/* PG_APPSPEC_MA removed */
#ifdef PG_APPSPEC_MAXHEIGHT
	    return PG_APPSPEC_MAXHEIGHT;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 13, "XWIDTH")) {	/* PG_APPSPEC_MA removed */
#ifdef PG_APPSPEC_MAXWIDTH
	    return PG_APPSPEC_MAXWIDTH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APPSPEC_M(char *name, int len, int arg)
{
    switch (name[12 + 0]) {
    case 'A':
	return constant_PG_APPSPEC_MA(name, len, arg);
    case 'I':
	return constant_PG_APPSPEC_MI(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_APPS(char *name, int len, int arg)
{
    if (7 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 4]) {
    case 'H':
	if (strEQ(name + 7, "PEC_HEIGHT")) {	/* PG_APPS removed */
#ifdef PG_APPSPEC_HEIGHT
	    return PG_APPSPEC_HEIGHT;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (!strnEQ(name + 7,"PEC_", 4))
	    break;
	return constant_PG_APPSPEC_M(name, len, arg);
    case 'S':
	if (!strnEQ(name + 7,"PEC_", 4))
	    break;
	return constant_PG_APPSPEC_S(name, len, arg);
    case 'W':
	if (strEQ(name + 7, "PEC_WIDTH")) {	/* PG_APPS removed */
#ifdef PG_APPSPEC_WIDTH
	    return PG_APPSPEC_WIDTH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_AP(char *name, int len, int arg)
{
    if (5 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 1]) {
    case 'M':
	if (strEQ(name + 5, "PMAX")) {	/* PG_AP removed */
#ifdef PG_APPMAX
	    return PG_APPMAX;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (!strnEQ(name + 5,"P", 1))
	    break;
	return constant_PG_APPS(name, len, arg);
    case '_':
	if (!strnEQ(name + 5,"P", 1))
	    break;
	return constant_PG_APP_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_A(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'M':
	if (strEQ(name + 4, "MAX")) {	/* PG_A removed */
#ifdef PG_AMAX
	    return PG_AMAX;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_PG_AP(name, len, arg);
    case '_':
	return constant_PG_A_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_REQ(char *name, int len, int arg)
{
    if (6 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[6 + 5]) {
    case 'M':
	if (strEQ(name + 6, "UEST_MAGIC")) {	/* PG_REQ removed */
#ifdef PG_REQUEST_MAGIC
	    return PG_REQUEST_MAGIC;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 6, "UEST_PORT")) {	/* PG_REQ removed */
#ifdef PG_REQUEST_PORT
	    return PG_REQUEST_PORT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_RESPONSE_E(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case 'R':
	if (strEQ(name + 13, "RR")) {	/* PG_RESPONSE_E removed */
#ifdef PG_RESPONSE_ERR
	    return PG_RESPONSE_ERR;
#else
	    goto not_there;
#endif
	}
    case 'V':
	if (strEQ(name + 13, "VENT")) {	/* PG_RESPONSE_E removed */
#ifdef PG_RESPONSE_EVENT
	    return PG_RESPONSE_EVENT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_RES(char *name, int len, int arg)
{
    if (6 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[6 + 6]) {
    case 'D':
	if (strEQ(name + 6, "PONSE_DATA")) {	/* PG_RES removed */
#ifdef PG_RESPONSE_DATA
	    return PG_RESPONSE_DATA;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (!strnEQ(name + 6,"PONSE_", 6))
	    break;
	return constant_PG_RESPONSE_E(name, len, arg);
    case 'R':
	if (strEQ(name + 6, "PONSE_RET")) {	/* PG_RES removed */
#ifdef PG_RESPONSE_RET
	    return PG_RESPONSE_RET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_R(char *name, int len, int arg)
{
    if (4 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 1]) {
    case 'Q':
	if (!strnEQ(name + 4,"E", 1))
	    break;
	return constant_PG_REQ(name, len, arg);
    case 'S':
	if (!strnEQ(name + 4,"E", 1))
	    break;
	return constant_PG_RES(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_S_(char *name, int len, int arg)
{
    switch (name[5 + 0]) {
    case 'A':
	if (strEQ(name + 5, "ALL")) {	/* PG_S_ removed */
#ifdef PG_S_ALL
	    return PG_S_ALL;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (strEQ(name + 5, "BOTTOM")) {	/* PG_S_ removed */
#ifdef PG_S_BOTTOM
	    return PG_S_BOTTOM;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 5, "LEFT")) {	/* PG_S_ removed */
#ifdef PG_S_LEFT
	    return PG_S_LEFT;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 5, "RIGHT")) {	/* PG_S_ removed */
#ifdef PG_S_RIGHT
	    return PG_S_RIGHT;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 5, "TOP")) {	/* PG_S_ removed */
#ifdef PG_S_TOP
	    return PG_S_TOP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_SZMODE_P(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'E':
	if (strEQ(name + 11, "ERCENT")) {	/* PG_SZMODE_P removed */
#ifdef PG_SZMODE_PERCENT
	    return PG_SZMODE_PERCENT;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 11, "IXEL")) {	/* PG_SZMODE_P removed */
#ifdef PG_SZMODE_PIXEL
	    return PG_SZMODE_PIXEL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_SZMODE_(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'C':
	if (strEQ(name + 10, "CNTFRACT")) {	/* PG_SZMODE_ removed */
#ifdef PG_SZMODE_CNTFRACT
	    return PG_SZMODE_CNTFRACT;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_PG_SZMODE_P(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_SZ(char *name, int len, int arg)
{
    if (5 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 4]) {
    case 'M':
	if (strEQ(name + 5, "MODEMASK")) {	/* PG_SZ removed */
#ifdef PG_SZMODEMASK
	    return PG_SZMODEMASK;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 5,"MODE", 4))
	    break;
	return constant_PG_SZMODE_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_S(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'Z':
	return constant_PG_SZ(name, len, arg);
    case '_':
	return constant_PG_S_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_TYPE_F(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'I':
	if (strEQ(name + 9, "ILLSTYLE")) {	/* PG_TYPE_F removed */
#ifdef PG_TYPE_FILLSTYLE
	    return PG_TYPE_FILLSTYLE;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "ONTDESC")) {	/* PG_TYPE_F removed */
#ifdef PG_TYPE_FONTDESC
	    return PG_TYPE_FONTDESC;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_TY(char *name, int len, int arg)
{
    if (5 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 3]) {
    case 'B':
	if (strEQ(name + 5, "PE_BITMAP")) {	/* PG_TY removed */
#ifdef PG_TYPE_BITMAP
	    return PG_TYPE_BITMAP;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (!strnEQ(name + 5,"PE_", 3))
	    break;
	return constant_PG_TYPE_F(name, len, arg);
    case 'S':
	if (strEQ(name + 5, "PE_STRING")) {	/* PG_TY removed */
#ifdef PG_TYPE_STRING
	    return PG_TYPE_STRING;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 5, "PE_THEME")) {	/* PG_TY removed */
#ifdef PG_TYPE_THEME
	    return PG_TYPE_THEME;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 5, "PE_WIDGET")) {	/* PG_TY removed */
#ifdef PG_TYPE_WIDGET
	    return PG_TYPE_WIDGET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_TRIGGER_K(char *name, int len, int arg)
{
    if (12 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 2]) {
    case 'D':
	if (strEQ(name + 12, "EYDOWN")) {	/* PG_TRIGGER_K removed */
#ifdef PG_TRIGGER_KEYDOWN
	    return PG_TRIGGER_KEYDOWN;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 12, "EYUP")) {	/* PG_TRIGGER_K removed */
#ifdef PG_TRIGGER_KEYUP
	    return PG_TRIGGER_KEYUP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_TR(char *name, int len, int arg)
{
    if (5 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 6]) {
    case 'C':
	if (strEQ(name + 5, "IGGER_CHAR")) {	/* PG_TR removed */
#ifdef PG_TRIGGER_CHAR
	    return PG_TRIGGER_CHAR;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (strEQ(name + 5, "IGGER_DOWN")) {	/* PG_TR removed */
#ifdef PG_TRIGGER_DOWN
	    return PG_TRIGGER_DOWN;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 5,"IGGER_", 6))
	    break;
	return constant_PG_TRIGGER_K(name, len, arg);
    case 'M':
	if (strEQ(name + 5, "IGGER_MOVE")) {	/* PG_TR removed */
#ifdef PG_TRIGGER_MOVE
	    return PG_TRIGGER_MOVE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 5, "IGGER_UP")) {	/* PG_TR removed */
#ifdef PG_TRIGGER_UP
	    return PG_TRIGGER_UP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_T(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'R':
	return constant_PG_TR(name, len, arg);
    case 'Y':
	return constant_PG_TY(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_DI(char *name, int len, int arg)
{
    if (5 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 2]) {
    case 'H':
	if (strEQ(name + 5, "R_HORIZONTAL")) {	/* PG_DI removed */
#ifdef PG_DIR_HORIZONTAL
	    return PG_DIR_HORIZONTAL;
#else
	    goto not_there;
#endif
	}
    case 'V':
	if (strEQ(name + 5, "R_VERTICAL")) {	/* PG_DI removed */
#ifdef PG_DIR_VERTICAL
	    return PG_DIR_VERTICAL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_DERIVE_B(char *name, int len, int arg)
{
    if (11 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[11 + 5]) {
    case '\0':
	if (strEQ(name + 11, "EFORE")) {	/* PG_DERIVE_B removed */
#ifdef PG_DERIVE_BEFORE
	    return PG_DERIVE_BEFORE;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (strEQ(name + 11, "EFORE_OLD")) {	/* PG_DERIVE_B removed */
#ifdef PG_DERIVE_BEFORE_OLD
	    return PG_DERIVE_BEFORE_OLD;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_DE(char *name, int len, int arg)
{
    if (5 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 5]) {
    case 'A':
	if (strEQ(name + 5, "RIVE_AFTER")) {	/* PG_DE removed */
#ifdef PG_DERIVE_AFTER
	    return PG_DERIVE_AFTER;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (!strnEQ(name + 5,"RIVE_", 5))
	    break;
	return constant_PG_DERIVE_B(name, len, arg);
    case 'I':
	if (strEQ(name + 5, "RIVE_INSIDE")) {	/* PG_DE removed */
#ifdef PG_DERIVE_INSIDE
	    return PG_DERIVE_INSIDE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_D(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'E':
	return constant_PG_DE(name, len, arg);
    case 'I':
	return constant_PG_DI(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_EVENTCODING_P(char *name, int len, int arg)
{
    switch (name[16 + 0]) {
    case 'A':
	if (strEQ(name + 16, "ARAM")) {	/* PG_EVENTCODING_P removed */
#ifdef PG_EVENTCODING_PARAM
	    return PG_EVENTCODING_PARAM;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 16, "NTR")) {	/* PG_EVENTCODING_P removed */
#ifdef PG_EVENTCODING_PNTR
	    return PG_EVENTCODING_PNTR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_EVENTCODING_(char *name, int len, int arg)
{
    switch (name[15 + 0]) {
    case 'D':
	if (strEQ(name + 15, "DATA")) {	/* PG_EVENTCODING_ removed */
#ifdef PG_EVENTCODING_DATA
	    return PG_EVENTCODING_DATA;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (strEQ(name + 15, "KBD")) {	/* PG_EVENTCODING_ removed */
#ifdef PG_EVENTCODING_KBD
	    return PG_EVENTCODING_KBD;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_PG_EVENTCODING_P(name, len, arg);
    case 'X':
	if (strEQ(name + 15, "XY")) {	/* PG_EVENTCODING_ removed */
#ifdef PG_EVENTCODING_XY
	    return PG_EVENTCODING_XY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_EV(char *name, int len, int arg)
{
    if (5 + 9 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 9]) {
    case 'M':
	if (strEQ(name + 5, "ENTCODINGMASK")) {	/* PG_EV removed */
#ifdef PG_EVENTCODINGMASK
	    return PG_EVENTCODINGMASK;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 5,"ENTCODING", 9))
	    break;
	return constant_PG_EVENTCODING_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_EXEV_P(char *name, int len, int arg)
{
    if (9 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 4]) {
    case 'D':
	if (strEQ(name + 9, "NTR_DOWN")) {	/* PG_EXEV_P removed */
#ifdef PG_EXEV_PNTR_DOWN
	    return PG_EXEV_PNTR_DOWN;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 9, "NTR_MOVE")) {	/* PG_EXEV_P removed */
#ifdef PG_EXEV_PNTR_MOVE
	    return PG_EXEV_PNTR_MOVE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 9, "NTR_UP")) {	/* PG_EXEV_P removed */
#ifdef PG_EXEV_PNTR_UP
	    return PG_EXEV_PNTR_UP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_EX(char *name, int len, int arg)
{
    if (5 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 3]) {
    case 'C':
	if (strEQ(name + 5, "EV_CHAR")) {	/* PG_EX removed */
#ifdef PG_EXEV_CHAR
	    return PG_EXEV_CHAR;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (strEQ(name + 5, "EV_KEY")) {	/* PG_EX removed */
#ifdef PG_EXEV_KEY
	    return PG_EXEV_KEY;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 5, "EV_NOCLICK")) {	/* PG_EX removed */
#ifdef PG_EXEV_NOCLICK
	    return PG_EXEV_NOCLICK;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (!strnEQ(name + 5,"EV_", 3))
	    break;
	return constant_PG_EXEV_P(name, len, arg);
    case 'T':
	if (strEQ(name + 5, "EV_TOGGLE")) {	/* PG_EX removed */
#ifdef PG_EXEV_TOGGLE
	    return PG_EXEV_TOGGLE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_ERRT_N(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'E':
	if (strEQ(name + 9, "ETWORK")) {	/* PG_ERRT_N removed */
#ifdef PG_ERRT_NETWORK
	    return PG_ERRT_NETWORK;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "ONE")) {	/* PG_ERRT_N removed */
#ifdef PG_ERRT_NONE
	    return PG_ERRT_NONE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_ERRT_I(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'N':
	if (strEQ(name + 9, "NTERNAL")) {	/* PG_ERRT_I removed */
#ifdef PG_ERRT_INTERNAL
	    return PG_ERRT_INTERNAL;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "O")) {	/* PG_ERRT_I removed */
#ifdef PG_ERRT_IO
	    return PG_ERRT_IO;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_ERRT_B(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'A':
	if (strEQ(name + 9, "ADPARAM")) {	/* PG_ERRT_B removed */
#ifdef PG_ERRT_BADPARAM
	    return PG_ERRT_BADPARAM;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 9, "USY")) {	/* PG_ERRT_B removed */
#ifdef PG_ERRT_BUSY
	    return PG_ERRT_BUSY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_ER(char *name, int len, int arg)
{
    if (5 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 3]) {
    case 'B':
	if (!strnEQ(name + 5,"RT_", 3))
	    break;
	return constant_PG_ERRT_B(name, len, arg);
    case 'C':
	if (strEQ(name + 5, "RT_CLIENT")) {	/* PG_ER removed */
#ifdef PG_ERRT_CLIENT
	    return PG_ERRT_CLIENT;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 5, "RT_FILEFMT")) {	/* PG_ER removed */
#ifdef PG_ERRT_FILEFMT
	    return PG_ERRT_FILEFMT;
#else
	    goto not_there;
#endif
	}
    case 'H':
	if (strEQ(name + 5, "RT_HANDLE")) {	/* PG_ER removed */
#ifdef PG_ERRT_HANDLE
	    return PG_ERRT_HANDLE;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (!strnEQ(name + 5,"RT_", 3))
	    break;
	return constant_PG_ERRT_I(name, len, arg);
    case 'M':
	if (strEQ(name + 5, "RT_MEMORY")) {	/* PG_ER removed */
#ifdef PG_ERRT_MEMORY
	    return PG_ERRT_MEMORY;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (!strnEQ(name + 5,"RT_", 3))
	    break;
	return constant_PG_ERRT_N(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_E(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'R':
	return constant_PG_ER(name, len, arg);
    case 'V':
	return constant_PG_EV(name, len, arg);
    case 'X':
	return constant_PG_EX(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_V(char *name, int len, int arg)
{
    if (4 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 3]) {
    case 'D':
	if (strEQ(name + 4, "ID_DOUBLEBUFFER")) {	/* PG_V removed */
#ifdef PG_VID_DOUBLEBUFFER
	    return PG_VID_DOUBLEBUFFER;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 4, "ID_FULLSCREEN")) {	/* PG_V removed */
#ifdef PG_VID_FULLSCREEN
	    return PG_VID_FULLSCREEN;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 4, "ID_ROTATE90")) {	/* PG_V removed */
#ifdef PG_VID_ROTATE90
	    return PG_VID_ROTATE90;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_F(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'I':
	if (strEQ(name + 11, "IXED")) {	/* PG_FSTYLE_F removed */
#ifdef PG_FSTYLE_FIXED
	    return PG_FSTYLE_FIXED;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 11, "LUSH")) {	/* PG_FSTYLE_F removed */
#ifdef PG_FSTYLE_FLUSH
	    return PG_FSTYLE_FLUSH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_IT(char *name, int len, int arg)
{
    if (12 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 4]) {
    case '\0':
	if (strEQ(name + 12, "ALIC")) {	/* PG_FSTYLE_IT removed */
#ifdef PG_FSTYLE_ITALIC
	    return PG_FSTYLE_ITALIC;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 12, "ALIC2")) {	/* PG_FSTYLE_IT removed */
#ifdef PG_FSTYLE_ITALIC2
	    return PG_FSTYLE_ITALIC2;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_I(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'B':
	if (strEQ(name + 11, "BMEXTEND")) {	/* PG_FSTYLE_I removed */
#ifdef PG_FSTYLE_IBMEXTEND
	    return PG_FSTYLE_IBMEXTEND;
#else
	    goto not_there;
#endif
	}
    case 'T':
	return constant_PG_FSTYLE_IT(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_S(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'T':
	if (strEQ(name + 11, "TRIKEOUT")) {	/* PG_FSTYLE_S removed */
#ifdef PG_FSTYLE_STRIKEOUT
	    return PG_FSTYLE_STRIKEOUT;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 11, "UBSET")) {	/* PG_FSTYLE_S removed */
#ifdef PG_FSTYLE_SUBSET
	    return PG_FSTYLE_SUBSET;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 11, "YMBOL")) {	/* PG_FSTYLE_S removed */
#ifdef PG_FSTYLE_SYMBOL
	    return PG_FSTYLE_SYMBOL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_DO(char *name, int len, int arg)
{
    if (12 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 4]) {
    case 'S':
	if (strEQ(name + 12, "UBLESPACE")) {	/* PG_FSTYLE_DO removed */
#ifdef PG_FSTYLE_DOUBLESPACE
	    return PG_FSTYLE_DOUBLESPACE;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 12, "UBLEWIDTH")) {	/* PG_FSTYLE_DO removed */
#ifdef PG_FSTYLE_DOUBLEWIDTH
	    return PG_FSTYLE_DOUBLEWIDTH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FSTYLE_D(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'E':
	if (strEQ(name + 11, "EFAULT")) {	/* PG_FSTYLE_D removed */
#ifdef PG_FSTYLE_DEFAULT
	    return PG_FSTYLE_DEFAULT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	return constant_PG_FSTYLE_DO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FS(char *name, int len, int arg)
{
    if (5 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 5]) {
    case 'B':
	if (strEQ(name + 5, "TYLE_BOLD")) {	/* PG_FS removed */
#ifdef PG_FSTYLE_BOLD
	    return PG_FSTYLE_BOLD;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (!strnEQ(name + 5,"TYLE_", 5))
	    break;
	return constant_PG_FSTYLE_D(name, len, arg);
    case 'E':
	if (strEQ(name + 5, "TYLE_EXTENDED")) {	/* PG_FS removed */
#ifdef PG_FSTYLE_EXTENDED
	    return PG_FSTYLE_EXTENDED;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (!strnEQ(name + 5,"TYLE_", 5))
	    break;
	return constant_PG_FSTYLE_F(name, len, arg);
    case 'G':
	if (strEQ(name + 5, "TYLE_GRAYLINE")) {	/* PG_FS removed */
#ifdef PG_FSTYLE_GRAYLINE
	    return PG_FSTYLE_GRAYLINE;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (!strnEQ(name + 5,"TYLE_", 5))
	    break;
	return constant_PG_FSTYLE_I(name, len, arg);
    case 'S':
	if (!strnEQ(name + 5,"TYLE_", 5))
	    break;
	return constant_PG_FSTYLE_S(name, len, arg);
    case 'U':
	if (strEQ(name + 5, "TYLE_UNDERLINE")) {	/* PG_FS removed */
#ifdef PG_FSTYLE_UNDERLINE
	    return PG_FSTYLE_UNDERLINE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FM_O(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'F':
	if (strEQ(name + 7, "FF")) {	/* PG_FM_O removed */
#ifdef PG_FM_OFF
	    return PG_FM_OFF;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 7, "N")) {	/* PG_FM_O removed */
#ifdef PG_FM_ON
	    return PG_FM_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_FM(char *name, int len, int arg)
{
    if (5 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 1]) {
    case 'O':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_FM_O(name, len, arg);
    case 'S':
	if (strEQ(name + 5, "_SET")) {	/* PG_FM removed */
#ifdef PG_FM_SET
	    return PG_FM_SET;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 5, "_TOGGLE")) {	/* PG_FM removed */
#ifdef PG_FM_TOGGLE
	    return PG_FM_TOGGLE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_F(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'M':
	return constant_PG_FM(name, len, arg);
    case 'S':
	return constant_PG_FS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_AB(char *name, int len, int arg)
{
    if (8 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 6]) {
    case 'X':
	if (strEQ(name + 8, "SOLUTEX")) {	/* PG_WP_AB removed */
#ifdef PG_WP_ABSOLUTEX
	    return PG_WP_ABSOLUTEX;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 8, "SOLUTEY")) {	/* PG_WP_AB removed */
#ifdef PG_WP_ABSOLUTEY
	    return PG_WP_ABSOLUTEY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_A(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'B':
	return constant_PG_WP_AB(name, len, arg);
    case 'L':
	if (strEQ(name + 7, "LIGN")) {	/* PG_WP_A removed */
#ifdef PG_WP_ALIGN
	    return PG_WP_ALIGN;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_BIT(char *name, int len, int arg)
{
    if (9 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 2]) {
    case 'P':
	if (strEQ(name + 9, "MAP")) {	/* PG_WP_BIT removed */
#ifdef PG_WP_BITMAP
	    return PG_WP_BITMAP;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 9, "MASK")) {	/* PG_WP_BIT removed */
#ifdef PG_WP_BITMASK
	    return PG_WP_BITMASK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_BI(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'N':
	if (strEQ(name + 8, "ND")) {	/* PG_WP_BI removed */
#ifdef PG_WP_BIND
	    return PG_WP_BIND;
#else
	    goto not_there;
#endif
	}
    case 'T':
	return constant_PG_WP_BIT(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_B(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'G':
	if (strEQ(name + 7, "GCOLOR")) {	/* PG_WP_B removed */
#ifdef PG_WP_BGCOLOR
	    return PG_WP_BGCOLOR;
#else
	    goto not_there;
#endif
	}
    case 'I':
	return constant_PG_WP_BI(name, len, arg);
    case 'O':
	if (strEQ(name + 7, "ORDERCOLOR")) {	/* PG_WP_B removed */
#ifdef PG_WP_BORDERCOLOR
	    return PG_WP_BORDERCOLOR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_SIZ(char *name, int len, int arg)
{
    if (9 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 1]) {
    case '\0':
	if (strEQ(name + 9, "E")) {	/* PG_WP_SIZ removed */
#ifdef PG_WP_SIZE
	    return PG_WP_SIZE;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 9, "EMODE")) {	/* PG_WP_SIZ removed */
#ifdef PG_WP_SIZEMODE
	    return PG_WP_SIZEMODE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_SI(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'D':
	if (strEQ(name + 8, "DE")) {	/* PG_WP_SI removed */
#ifdef PG_WP_SIDE
	    return PG_WP_SIDE;
#else
	    goto not_there;
#endif
	}
    case 'Z':
	return constant_PG_WP_SIZ(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_S(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'C':
	if (strEQ(name + 7, "CROLL")) {	/* PG_WP_S removed */
#ifdef PG_WP_SCROLL
	    return PG_WP_SCROLL;
#else
	    goto not_there;
#endif
	}
    case 'I':
	return constant_PG_WP_SI(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_T(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	if (strEQ(name + 7, "EXT")) {	/* PG_WP_T removed */
#ifdef PG_WP_TEXT
	    return PG_WP_TEXT;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 7, "RANSPARENT")) {	/* PG_WP_T removed */
#ifdef PG_WP_TRANSPARENT
	    return PG_WP_TRANSPARENT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP_V(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ALUE")) {	/* PG_WP_V removed */
#ifdef PG_WP_VALUE
	    return PG_WP_VALUE;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 7, "IRTUALH")) {	/* PG_WP_V removed */
#ifdef PG_WP_VIRTUALH
	    return PG_WP_VIRTUALH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WP(char *name, int len, int arg)
{
    if (5 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 1]) {
    case 'A':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WP_A(name, len, arg);
    case 'B':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WP_B(name, len, arg);
    case 'C':
	if (strEQ(name + 5, "_COLOR")) {	/* PG_WP removed */
#ifdef PG_WP_COLOR
	    return PG_WP_COLOR;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (strEQ(name + 5, "_DIRECTION")) {	/* PG_WP removed */
#ifdef PG_WP_DIRECTION
	    return PG_WP_DIRECTION;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 5, "_EXTDEVENTS")) {	/* PG_WP removed */
#ifdef PG_WP_EXTDEVENTS
	    return PG_WP_EXTDEVENTS;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 5, "_FONT")) {	/* PG_WP removed */
#ifdef PG_WP_FONT
	    return PG_WP_FONT;
#else
	    goto not_there;
#endif
	}
    case 'H':
	if (strEQ(name + 5, "_HOTKEY")) {	/* PG_WP removed */
#ifdef PG_WP_HOTKEY
	    return PG_WP_HOTKEY;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 5, "_LGOP")) {	/* PG_WP removed */
#ifdef PG_WP_LGOP
	    return PG_WP_LGOP;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 5, "_ON")) {	/* PG_WP removed */
#ifdef PG_WP_ON
	    return PG_WP_ON;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WP_S(name, len, arg);
    case 'T':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WP_T(name, len, arg);
    case 'V':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WP_V(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_F(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'I':
	if (strEQ(name + 11, "IELD")) {	/* PG_WIDGET_F removed */
#ifdef PG_WIDGET_FIELD
	    return PG_WIDGET_FIELD;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 11, "LATBUTTON")) {	/* PG_WIDGET_F removed */
#ifdef PG_WIDGET_FLATBUTTON
	    return PG_WIDGET_FLATBUTTON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_P(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'A':
	if (strEQ(name + 11, "ANEL")) {	/* PG_WIDGET_P removed */
#ifdef PG_WIDGET_PANEL
	    return PG_WIDGET_PANEL;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 11, "OPUP")) {	/* PG_WIDGET_P removed */
#ifdef PG_WIDGET_POPUP
	    return PG_WIDGET_POPUP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_B(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'A':
	if (strEQ(name + 11, "ACKGROUND")) {	/* PG_WIDGET_B removed */
#ifdef PG_WIDGET_BACKGROUND
	    return PG_WIDGET_BACKGROUND;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 11, "ITMAP")) {	/* PG_WIDGET_B removed */
#ifdef PG_WIDGET_BITMAP
	    return PG_WIDGET_BITMAP;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 11, "OX")) {	/* PG_WIDGET_B removed */
#ifdef PG_WIDGET_BOX
	    return PG_WIDGET_BOX;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 11, "UTTON")) {	/* PG_WIDGET_B removed */
#ifdef PG_WIDGET_BUTTON
	    return PG_WIDGET_BUTTON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_C(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'A':
	if (strEQ(name + 11, "ANVAS")) {	/* PG_WIDGET_C removed */
#ifdef PG_WIDGET_CANVAS
	    return PG_WIDGET_CANVAS;
#else
	    goto not_there;
#endif
	}
    case 'H':
	if (strEQ(name + 11, "HECKBOX")) {	/* PG_WIDGET_C removed */
#ifdef PG_WIDGET_CHECKBOX
	    return PG_WIDGET_CHECKBOX;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_T(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'E':
	if (strEQ(name + 11, "ERMINAL")) {	/* PG_WIDGET_T removed */
#ifdef PG_WIDGET_TERMINAL
	    return PG_WIDGET_TERMINAL;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 11, "OOLBAR")) {	/* PG_WIDGET_T removed */
#ifdef PG_WIDGET_TOOLBAR
	    return PG_WIDGET_TOOLBAR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WIDGET_(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'B':
	return constant_PG_WIDGET_B(name, len, arg);
    case 'C':
	return constant_PG_WIDGET_C(name, len, arg);
    case 'F':
	return constant_PG_WIDGET_F(name, len, arg);
    case 'I':
	if (strEQ(name + 10, "INDICATOR")) {	/* PG_WIDGET_ removed */
#ifdef PG_WIDGET_INDICATOR
	    return PG_WIDGET_INDICATOR;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 10, "LABEL")) {	/* PG_WIDGET_ removed */
#ifdef PG_WIDGET_LABEL
	    return PG_WIDGET_LABEL;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 10, "MENUITEM")) {	/* PG_WIDGET_ removed */
#ifdef PG_WIDGET_MENUITEM
	    return PG_WIDGET_MENUITEM;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_PG_WIDGET_P(name, len, arg);
    case 'S':
	if (strEQ(name + 10, "SCROLL")) {	/* PG_WIDGET_ removed */
#ifdef PG_WIDGET_SCROLL
	    return PG_WIDGET_SCROLL;
#else
	    goto not_there;
#endif
	}
    case 'T':
	return constant_PG_WIDGET_T(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WI(char *name, int len, int arg)
{
    if (5 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 4]) {
    case 'M':
	if (strEQ(name + 5, "DGETMAX")) {	/* PG_WI removed */
#ifdef PG_WIDGETMAX
	    return PG_WIDGETMAX;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 5,"DGET", 4))
	    break;
	return constant_PG_WIDGET_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WE_P(char *name, int len, int arg)
{
    if (7 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 4]) {
    case 'D':
	if (strEQ(name + 7, "NTR_DOWN")) {	/* PG_WE_P removed */
#ifdef PG_WE_PNTR_DOWN
	    return PG_WE_PNTR_DOWN;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 7, "NTR_MOVE")) {	/* PG_WE_P removed */
#ifdef PG_WE_PNTR_MOVE
	    return PG_WE_PNTR_MOVE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 7, "NTR_UP")) {	/* PG_WE_P removed */
#ifdef PG_WE_PNTR_UP
	    return PG_WE_PNTR_UP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WE_D(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ATA")) {	/* PG_WE_D removed */
#ifdef PG_WE_DATA
	    return PG_WE_DATA;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 7, "EACTIVATE")) {	/* PG_WE_D removed */
#ifdef PG_WE_DEACTIVATE
	    return PG_WE_DEACTIVATE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WE_KBD_K(char *name, int len, int arg)
{
    if (11 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[11 + 2]) {
    case 'D':
	if (strEQ(name + 11, "EYDOWN")) {	/* PG_WE_KBD_K removed */
#ifdef PG_WE_KBD_KEYDOWN
	    return PG_WE_KBD_KEYDOWN;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 11, "EYUP")) {	/* PG_WE_KBD_K removed */
#ifdef PG_WE_KBD_KEYUP
	    return PG_WE_KBD_KEYUP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WE_K(char *name, int len, int arg)
{
    if (7 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 3]) {
    case 'C':
	if (strEQ(name + 7, "BD_CHAR")) {	/* PG_WE_K removed */
#ifdef PG_WE_KBD_CHAR
	    return PG_WE_KBD_CHAR;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 7,"BD_", 3))
	    break;
	return constant_PG_WE_KBD_K(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_WE(char *name, int len, int arg)
{
    if (5 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 1]) {
    case 'A':
	if (strEQ(name + 5, "_ACTIVATE")) {	/* PG_WE removed */
#ifdef PG_WE_ACTIVATE
	    return PG_WE_ACTIVATE;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (strEQ(name + 5, "_BUILD")) {	/* PG_WE removed */
#ifdef PG_WE_BUILD
	    return PG_WE_BUILD;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 5, "_CLOSE")) {	/* PG_WE removed */
#ifdef PG_WE_CLOSE
	    return PG_WE_CLOSE;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WE_D(name, len, arg);
    case 'K':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WE_K(name, len, arg);
    case 'P':
	if (!strnEQ(name + 5,"_", 1))
	    break;
	return constant_PG_WE_P(name, len, arg);
    case 'R':
	if (strEQ(name + 5, "_RESIZE")) {	/* PG_WE removed */
#ifdef PG_WE_RESIZE
	    return PG_WE_RESIZE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_W(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'E':
	return constant_PG_WE(name, len, arg);
    case 'I':
	return constant_PG_WI(name, len, arg);
    case 'P':
	return constant_PG_WP(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_GROPF(char *name, int len, int arg)
{
    if (8 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 1]) {
    case 'I':
	if (strEQ(name + 8, "_INCREMENTAL")) {	/* PG_GROPF removed */
#ifdef PG_GROPF_INCREMENTAL
	    return PG_GROPF_INCREMENTAL;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "_PSEUDOINCREMENTAL")) {	/* PG_GROPF removed */
#ifdef PG_GROPF_PSEUDOINCREMENTAL
	    return PG_GROPF_PSEUDOINCREMENTAL;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 8, "_TRANSLATE")) {	/* PG_GROPF removed */
#ifdef PG_GROPF_TRANSLATE
	    return PG_GROPF_TRANSLATE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_GROP_B(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'A':
	if (strEQ(name + 9, "AR")) {	/* PG_GROP_B removed */
#ifdef PG_GROP_BAR
	    return PG_GROP_BAR;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 9, "ITMAP")) {	/* PG_GROP_B removed */
#ifdef PG_GROP_BITMAP
	    return PG_GROP_BITMAP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_GROP_TE(char *name, int len, int arg)
{
    if (10 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[10 + 2]) {
    case '\0':
	if (strEQ(name + 10, "XT")) {	/* PG_GROP_TE removed */
#ifdef PG_GROP_TEXT
	    return PG_GROP_TEXT;
#else
	    goto not_there;
#endif
	}
    case 'G':
	if (strEQ(name + 10, "XTGRID")) {	/* PG_GROP_TE removed */
#ifdef PG_GROP_TEXTGRID
	    return PG_GROP_TEXTGRID;
#else
	    goto not_there;
#endif
	}
    case 'V':
	if (strEQ(name + 10, "XTV")) {	/* PG_GROP_TE removed */
#ifdef PG_GROP_TEXTV
	    return PG_GROP_TEXTV;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_GROP_T(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'E':
	return constant_PG_GROP_TE(name, len, arg);
    case 'I':
	if (strEQ(name + 9, "ILEBITMAP")) {	/* PG_GROP_T removed */
#ifdef PG_GROP_TILEBITMAP
	    return PG_GROP_TILEBITMAP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_GROP_(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'B':
	return constant_PG_GROP_B(name, len, arg);
    case 'D':
	if (strEQ(name + 8, "DIM")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_DIM
	    return PG_GROP_DIM;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 8, "FRAME")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_FRAME
	    return PG_GROP_FRAME;
#else
	    goto not_there;
#endif
	}
    case 'G':
	if (strEQ(name + 8, "GRADIENT")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_GRADIENT
	    return PG_GROP_GRADIENT;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 8, "LINE")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_LINE
	    return PG_GROP_LINE;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 8, "NULL")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_NULL
	    return PG_GROP_NULL;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "PIXEL")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_PIXEL
	    return PG_GROP_PIXEL;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 8, "RECT")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_RECT
	    return PG_GROP_RECT;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 8, "SLAB")) {	/* PG_GROP_ removed */
#ifdef PG_GROP_SLAB
	    return PG_GROP_SLAB;
#else
	    goto not_there;
#endif
	}
    case 'T':
	return constant_PG_GROP_T(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_G(char *name, int len, int arg)
{
    if (4 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 3]) {
    case 'F':
	if (!strnEQ(name + 4,"ROP", 3))
	    break;
	return constant_PG_GROPF(name, len, arg);
    case '_':
	if (!strnEQ(name + 4,"ROP", 3))
	    break;
	return constant_PG_GROP_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_LGOP_N(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'O':
	if (strEQ(name + 9, "ONE")) {	/* PG_LGOP_N removed */
#ifdef PG_LGOP_NONE
	    return PG_LGOP_NONE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 9, "ULL")) {	/* PG_LGOP_N removed */
#ifdef PG_LGOP_NULL
	    return PG_LGOP_NULL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_LGOP_INVERT_(char *name, int len, int arg)
{
    switch (name[15 + 0]) {
    case 'A':
	if (strEQ(name + 15, "AND")) {	/* PG_LGOP_INVERT_ removed */
#ifdef PG_LGOP_INVERT_AND
	    return PG_LGOP_INVERT_AND;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 15, "OR")) {	/* PG_LGOP_INVERT_ removed */
#ifdef PG_LGOP_INVERT_OR
	    return PG_LGOP_INVERT_OR;
#else
	    goto not_there;
#endif
	}
    case 'X':
	if (strEQ(name + 15, "XOR")) {	/* PG_LGOP_INVERT_ removed */
#ifdef PG_LGOP_INVERT_XOR
	    return PG_LGOP_INVERT_XOR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_LGOP_I(char *name, int len, int arg)
{
    if (9 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 5]) {
    case '\0':
	if (strEQ(name + 9, "NVERT")) {	/* PG_LGOP_I removed */
#ifdef PG_LGOP_INVERT
	    return PG_LGOP_INVERT;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"NVERT", 5))
	    break;
	return constant_PG_LGOP_INVERT_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_LGOP_(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'A':
	if (strEQ(name + 8, "AND")) {	/* PG_LGOP_ removed */
#ifdef PG_LGOP_AND
	    return PG_LGOP_AND;
#else
	    goto not_there;
#endif
	}
    case 'I':
	return constant_PG_LGOP_I(name, len, arg);
    case 'N':
	return constant_PG_LGOP_N(name, len, arg);
    case 'O':
	if (strEQ(name + 8, "OR")) {	/* PG_LGOP_ removed */
#ifdef PG_LGOP_OR
	    return PG_LGOP_OR;
#else
	    goto not_there;
#endif
	}
    case 'X':
	if (strEQ(name + 8, "XOR")) {	/* PG_LGOP_ removed */
#ifdef PG_LGOP_XOR
	    return PG_LGOP_XOR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_L(char *name, int len, int arg)
{
    if (4 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 3]) {
    case 'M':
	if (strEQ(name + 4, "GOPMAX")) {	/* PG_L removed */
#ifdef PG_LGOPMAX
	    return PG_LGOPMAX;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 4,"GOP", 3))
	    break;
	return constant_PG_LGOP_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_MS(char *name, int len, int arg)
{
    if (5 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[5 + 5]) {
    case 'C':
	if (strEQ(name + 5, "GBTN_CANCEL")) {	/* PG_MS removed */
#ifdef PG_MSGBTN_CANCEL
	    return PG_MSGBTN_CANCEL;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 5, "GBTN_NO")) {	/* PG_MS removed */
#ifdef PG_MSGBTN_NO
	    return PG_MSGBTN_NO;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 5, "GBTN_OK")) {	/* PG_MS removed */
#ifdef PG_MSGBTN_OK
	    return PG_MSGBTN_OK;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 5, "GBTN_YES")) {	/* PG_MS removed */
#ifdef PG_MSGBTN_YES
	    return PG_MSGBTN_YES;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_M(char *name, int len, int arg)
{
    switch (name[4 + 0]) {
    case 'A':
	if (strEQ(name + 4, "AX_RESPONSE_SZ")) {	/* PG_M removed */
#ifdef PG_MAX_RESPONSE_SZ
	    return PG_MAX_RESPONSE_SZ;
#else
	    goto not_there;
#endif
	}
    case 'S':
	return constant_PG_MS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PG_(char *name, int len, int arg)
{
    switch (name[3 + 0]) {
    case 'A':
	return constant_PG_A(name, len, arg);
    case 'D':
	return constant_PG_D(name, len, arg);
    case 'E':
	return constant_PG_E(name, len, arg);
    case 'F':
	return constant_PG_F(name, len, arg);
    case 'G':
	return constant_PG_G(name, len, arg);
    case 'L':
	return constant_PG_L(name, len, arg);
    case 'M':
	return constant_PG_M(name, len, arg);
    case 'N':
	return constant_PG_N(name, len, arg);
    case 'O':
	return constant_PG_O(name, len, arg);
    case 'P':
	return constant_PG_P(name, len, arg);
    case 'R':
	return constant_PG_R(name, len, arg);
    case 'S':
	return constant_PG_S(name, len, arg);
    case 'T':
	return constant_PG_T(name, len, arg);
    case 'V':
	return constant_PG_V(name, len, arg);
    case 'W':
	return constant_PG_W(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_RE(char *name, int len, int arg)
{
    if (8 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 1]) {
    case 'I':
	if (strEQ(name + 8, "GISTER")) {	/* PGREQ_RE removed */
#ifdef PGREQ_REGISTER
	    return PGREQ_REGISTER;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 8, "GOWNER")) {	/* PGREQ_RE removed */
#ifdef PGREQ_REGOWNER
	    return PGREQ_REGOWNER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_R(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	return constant_PGREQ_RE(name, len, arg);
    case 'M':
	if (strEQ(name + 7, "MCONTEXT")) {	/* PGREQ_R removed */
#ifdef PGREQ_RMCONTEXT
	    return PGREQ_RMCONTEXT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_SE(char *name, int len, int arg)
{
    if (8 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 1]) {
    case '\0':
	if (strEQ(name + 8, "T")) {	/* PGREQ_SE removed */
#ifdef PGREQ_SET
	    return PGREQ_SET;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 8, "TMODE")) {	/* PGREQ_SE removed */
#ifdef PGREQ_SETMODE
	    return PGREQ_SETMODE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "TPAYLOAD")) {	/* PGREQ_SE removed */
#ifdef PGREQ_SETPAYLOAD
	    return PGREQ_SETPAYLOAD;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_S(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	return constant_PGREQ_SE(name, len, arg);
    case 'I':
	if (strEQ(name + 7, "IZETEXT")) {	/* PGREQ_S removed */
#ifdef PGREQ_SIZETEXT
	    return PGREQ_SIZETEXT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_UN(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'D':
	if (strEQ(name + 8, "DEF")) {	/* PGREQ_UN removed */
#ifdef PGREQ_UNDEF
	    return PGREQ_UNDEF;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 8, "REGOWNER")) {	/* PGREQ_UN removed */
#ifdef PGREQ_UNREGOWNER
	    return PGREQ_UNREGOWNER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_UP(char *name, int len, int arg)
{
    if (8 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 4]) {
    case '\0':
	if (strEQ(name + 8, "DATE")) {	/* PGREQ_UP removed */
#ifdef PGREQ_UPDATE
	    return PGREQ_UPDATE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "DATEPART")) {	/* PGREQ_UP removed */
#ifdef PGREQ_UPDATEPART
	    return PGREQ_UPDATEPART;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_U(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'N':
	return constant_PGREQ_UN(name, len, arg);
    case 'P':
	return constant_PGREQ_UP(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_F(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'O':
	if (strEQ(name + 7, "OCUS")) {	/* PGREQ_F removed */
#ifdef PGREQ_FOCUS
	    return PGREQ_FOCUS;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 7, "REE")) {	/* PGREQ_F removed */
#ifdef PGREQ_FREE
	    return PGREQ_FREE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_W(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "AIT")) {	/* PGREQ_W removed */
#ifdef PGREQ_WAIT
	    return PGREQ_WAIT;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 7, "RITETO")) {	/* PGREQ_W removed */
#ifdef PGREQ_WRITETO
	    return PGREQ_WRITETO;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_G(char *name, int len, int arg)
{
    if (7 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 2]) {
    case '\0':
	if (strEQ(name + 7, "ET")) {	/* PGREQ_G removed */
#ifdef PGREQ_GET
	    return PGREQ_GET;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 7, "ETMODE")) {	/* PGREQ_G removed */
#ifdef PGREQ_GETMODE
	    return PGREQ_GETMODE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 7, "ETPAYLOAD")) {	/* PGREQ_G removed */
#ifdef PGREQ_GETPAYLOAD
	    return PGREQ_GETPAYLOAD;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "ETSTRING")) {	/* PGREQ_G removed */
#ifdef PGREQ_GETSTRING
	    return PGREQ_GETSTRING;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_I(char *name, int len, int arg)
{
    if (7 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 2]) {
    case 'D':
	if (strEQ(name + 7, "N_DIRECT")) {	/* PGREQ_I removed */
#ifdef PGREQ_IN_DIRECT
	    return PGREQ_IN_DIRECT;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (strEQ(name + 7, "N_KEY")) {	/* PGREQ_I removed */
#ifdef PGREQ_IN_KEY
	    return PGREQ_IN_KEY;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 7, "N_POINT")) {	/* PGREQ_I removed */
#ifdef PGREQ_IN_POINT
	    return PGREQ_IN_POINT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_MKF(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'I':
	if (strEQ(name + 9, "ILLSTYLE")) {	/* PGREQ_MKF removed */
#ifdef PGREQ_MKFILLSTYLE
	    return PGREQ_MKFILLSTYLE;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "ONT")) {	/* PGREQ_MKF removed */
#ifdef PGREQ_MKFONT
	    return PGREQ_MKFONT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_MKM(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'E':
	if (strEQ(name + 9, "ENU")) {	/* PGREQ_MKM removed */
#ifdef PGREQ_MKMENU
	    return PGREQ_MKMENU;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 9, "SGDLG")) {	/* PGREQ_MKM removed */
#ifdef PGREQ_MKMSGDLG
	    return PGREQ_MKMSGDLG;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGREQ_M(char *name, int len, int arg)
{
    if (7 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 1]) {
    case 'B':
	if (strEQ(name + 7, "KBITMAP")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKBITMAP
	    return PGREQ_MKBITMAP;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 7, "KCONTEXT")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKCONTEXT
	    return PGREQ_MKCONTEXT;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (!strnEQ(name + 7,"K", 1))
	    break;
	return constant_PGREQ_MKF(name, len, arg);
    case 'M':
	if (!strnEQ(name + 7,"K", 1))
	    break;
	return constant_PGREQ_MKM(name, len, arg);
    case 'P':
	if (strEQ(name + 7, "KPOPUP")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKPOPUP
	    return PGREQ_MKPOPUP;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "KSTRING")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKSTRING
	    return PGREQ_MKSTRING;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 7, "KTHEME")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKTHEME
	    return PGREQ_MKTHEME;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 7, "KWIDGET")) {	/* PGREQ_M removed */
#ifdef PGREQ_MKWIDGET
	    return PGREQ_MKWIDGET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGR(char *name, int len, int arg)
{
    if (3 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[3 + 3]) {
    case 'B':
	if (strEQ(name + 3, "EQ_BATCH")) {	/* PGR removed */
#ifdef PGREQ_BATCH
	    return PGREQ_BATCH;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_F(name, len, arg);
    case 'G':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_G(name, len, arg);
    case 'I':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_I(name, len, arg);
    case 'M':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_M(name, len, arg);
    case 'P':
	if (strEQ(name + 3, "EQ_PING")) {	/* PGR removed */
#ifdef PGREQ_PING
	    return PGREQ_PING;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_R(name, len, arg);
    case 'S':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_S(name, len, arg);
    case 'U':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_U(name, len, arg);
    case 'W':
	if (!strnEQ(name + 3,"EQ_", 3))
	    break;
	return constant_PGREQ_W(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGCANVAS_S(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'C':
	if (strEQ(name + 10, "CROLL")) {	/* PGCANVAS_S removed */
#ifdef PGCANVAS_SCROLL
	    return PGCANVAS_SCROLL;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 10, "ETGROP")) {	/* PGCANVAS_S removed */
#ifdef PGCANVAS_SETGROP
	    return PGCANVAS_SETGROP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGCANVAS_G(char *name, int len, int arg)
{
    if (10 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[10 + 3]) {
    case '\0':
	if (strEQ(name + 10, "ROP")) {	/* PGCANVAS_G removed */
#ifdef PGCANVAS_GROP
	    return PGCANVAS_GROP;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 10, "ROPFLAGS")) {	/* PGCANVAS_G removed */
#ifdef PGCANVAS_GROPFLAGS
	    return PGCANVAS_GROPFLAGS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGCANVAS_M(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'O':
	if (strEQ(name + 10, "OVEGROP")) {	/* PGCANVAS_M removed */
#ifdef PGCANVAS_MOVEGROP
	    return PGCANVAS_MOVEGROP;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 10, "UTATEGROP")) {	/* PGCANVAS_M removed */
#ifdef PGCANVAS_MUTATEGROP
	    return PGCANVAS_MUTATEGROP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGC(char *name, int len, int arg)
{
    if (3 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[3 + 6]) {
    case 'C':
	if (strEQ(name + 3, "ANVAS_COLORCONV")) {	/* PGC removed */
#ifdef PGCANVAS_COLORCONV
	    return PGCANVAS_COLORCONV;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 3, "ANVAS_EXECFILL")) {	/* PGC removed */
#ifdef PGCANVAS_EXECFILL
	    return PGCANVAS_EXECFILL;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 3, "ANVAS_FINDGROP")) {	/* PGC removed */
#ifdef PGCANVAS_FINDGROP
	    return PGCANVAS_FINDGROP;
#else
	    goto not_there;
#endif
	}
    case 'G':
	if (!strnEQ(name + 3,"ANVAS_", 6))
	    break;
	return constant_PGCANVAS_G(name, len, arg);
    case 'I':
	if (strEQ(name + 3, "ANVAS_INCREMENTAL")) {	/* PGC removed */
#ifdef PGCANVAS_INCREMENTAL
	    return PGCANVAS_INCREMENTAL;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (!strnEQ(name + 3,"ANVAS_", 6))
	    break;
	return constant_PGCANVAS_M(name, len, arg);
    case 'N':
	if (strEQ(name + 3, "ANVAS_NUKE")) {	/* PGC removed */
#ifdef PGCANVAS_NUKE
	    return PGCANVAS_NUKE;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 3, "ANVAS_REDRAW")) {	/* PGC removed */
#ifdef PGCANVAS_REDRAW
	    return PGCANVAS_REDRAW;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (!strnEQ(name + 3,"ANVAS_", 6))
	    break;
	return constant_PGCANVAS_S(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_POPUP_(char *name, int len, int arg)
{
    if (13 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[13 + 2]) {
    case 'N':
	if (strEQ(name + 13, "MENU")) {	/* PGTH_O_POPUP_ removed */
#ifdef PGTH_O_POPUP_MENU
	    return PGTH_O_POPUP_MENU;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 13, "MESSAGEDLG")) {	/* PGTH_O_POPUP_ removed */
#ifdef PGTH_O_POPUP_MESSAGEDLG
	    return PGTH_O_POPUP_MESSAGEDLG;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_PO(char *name, int len, int arg)
{
    if (9 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 3]) {
    case '\0':
	if (strEQ(name + 9, "PUP")) {	/* PGTH_O_PO removed */
#ifdef PGTH_O_POPUP
	    return PGTH_O_POPUP;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"PUP", 3))
	    break;
	return constant_PGTH_O_POPUP_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_PANELBAR_(char *name, int len, int arg)
{
    switch (name[16 + 0]) {
    case 'H':
	if (strEQ(name + 16, "HILIGHT")) {	/* PGTH_O_PANELBAR_ removed */
#ifdef PGTH_O_PANELBAR_HILIGHT
	    return PGTH_O_PANELBAR_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 16, "ON")) {	/* PGTH_O_PANELBAR_ removed */
#ifdef PGTH_O_PANELBAR_ON
	    return PGTH_O_PANELBAR_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_PANELB(char *name, int len, int arg)
{
    if (13 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[13 + 2]) {
    case '\0':
	if (strEQ(name + 13, "AR")) {	/* PGTH_O_PANELB removed */
#ifdef PGTH_O_PANELBAR
	    return PGTH_O_PANELBAR;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 13,"AR", 2))
	    break;
	return constant_PGTH_O_PANELBAR_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_PA(char *name, int len, int arg)
{
    if (9 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 3]) {
    case '\0':
	if (strEQ(name + 9, "NEL")) {	/* PGTH_O_PA removed */
#ifdef PGTH_O_PANEL
	    return PGTH_O_PANEL;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (!strnEQ(name + 9,"NEL", 3))
	    break;
	return constant_PGTH_O_PANELB(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_P(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'A':
	return constant_PGTH_O_PA(name, len, arg);
    case 'O':
	return constant_PGTH_O_PO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_ROTATEBTN_(char *name, int len, int arg)
{
    switch (name[17 + 0]) {
    case 'H':
	if (strEQ(name + 17, "HILIGHT")) {	/* PGTH_O_ROTATEBTN_ removed */
#ifdef PGTH_O_ROTATEBTN_HILIGHT
	    return PGTH_O_ROTATEBTN_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 17, "ON")) {	/* PGTH_O_ROTATEBTN_ removed */
#ifdef PGTH_O_ROTATEBTN_ON
	    return PGTH_O_ROTATEBTN_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_R(char *name, int len, int arg)
{
    if (8 + 8 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 8]) {
    case '\0':
	if (strEQ(name + 8, "OTATEBTN")) {	/* PGTH_O_R removed */
#ifdef PGTH_O_ROTATEBTN
	    return PGTH_O_ROTATEBTN;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 8,"OTATEBTN", 8))
	    break;
	return constant_PGTH_O_ROTATEBTN_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_BAS(char *name, int len, int arg)
{
    if (10 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[10 + 2]) {
    case 'C':
	if (strEQ(name + 10, "E_CONTAINER")) {	/* PGTH_O_BAS removed */
#ifdef PGTH_O_BASE_CONTAINER
	    return PGTH_O_BASE_CONTAINER;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (strEQ(name + 10, "E_DISPLAY")) {	/* PGTH_O_BAS removed */
#ifdef PGTH_O_BASE_DISPLAY
	    return PGTH_O_BASE_DISPLAY;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 10, "E_INTERACTIVE")) {	/* PGTH_O_BAS removed */
#ifdef PGTH_O_BASE_INTERACTIVE
	    return PGTH_O_BASE_INTERACTIVE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 10, "E_PANELBTN")) {	/* PGTH_O_BAS removed */
#ifdef PGTH_O_BASE_PANELBTN
	    return PGTH_O_BASE_PANELBTN;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 10, "E_TLCONTAINER")) {	/* PGTH_O_BAS removed */
#ifdef PGTH_O_BASE_TLCONTAINER
	    return PGTH_O_BASE_TLCONTAINER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_BA(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'C':
	if (strEQ(name + 9, "CKGROUND")) {	/* PGTH_O_BA removed */
#ifdef PGTH_O_BACKGROUND
	    return PGTH_O_BACKGROUND;
#else
	    goto not_there;
#endif
	}
    case 'S':
	return constant_PGTH_O_BAS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_BUTTON_(char *name, int len, int arg)
{
    switch (name[14 + 0]) {
    case 'H':
	if (strEQ(name + 14, "HILIGHT")) {	/* PGTH_O_BUTTON_ removed */
#ifdef PGTH_O_BUTTON_HILIGHT
	    return PGTH_O_BUTTON_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 14, "ON")) {	/* PGTH_O_BUTTON_ removed */
#ifdef PGTH_O_BUTTON_ON
	    return PGTH_O_BUTTON_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_BU(char *name, int len, int arg)
{
    if (9 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 4]) {
    case '\0':
	if (strEQ(name + 9, "TTON")) {	/* PGTH_O_BU removed */
#ifdef PGTH_O_BUTTON
	    return PGTH_O_BUTTON;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"TTON", 4))
	    break;
	return constant_PGTH_O_BUTTON_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_B(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'A':
	return constant_PGTH_O_BA(name, len, arg);
    case 'I':
	if (strEQ(name + 8, "ITMAP")) {	/* PGTH_O_B removed */
#ifdef PGTH_O_BITMAP
	    return PGTH_O_BITMAP;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 8, "OX")) {	/* PGTH_O_B removed */
#ifdef PGTH_O_BOX
	    return PGTH_O_BOX;
#else
	    goto not_there;
#endif
	}
    case 'U':
	return constant_PGTH_O_BU(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_SCROLL_(char *name, int len, int arg)
{
    switch (name[14 + 0]) {
    case 'H':
	if (strEQ(name + 14, "HILIGHT")) {	/* PGTH_O_SCROLL_ removed */
#ifdef PGTH_O_SCROLL_HILIGHT
	    return PGTH_O_SCROLL_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 14, "ON")) {	/* PGTH_O_SCROLL_ removed */
#ifdef PGTH_O_SCROLL_ON
	    return PGTH_O_SCROLL_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_S(char *name, int len, int arg)
{
    if (8 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 5]) {
    case '\0':
	if (strEQ(name + 8, "CROLL")) {	/* PGTH_O_S removed */
#ifdef PGTH_O_SCROLL
	    return PGTH_O_SCROLL;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 8,"CROLL", 5))
	    break;
	return constant_PGTH_O_SCROLL_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_CHECKBOX_(char *name, int len, int arg)
{
    switch (name[16 + 0]) {
    case 'H':
	if (strEQ(name + 16, "HILIGHT")) {	/* PGTH_O_CHECKBOX_ removed */
#ifdef PGTH_O_CHECKBOX_HILIGHT
	    return PGTH_O_CHECKBOX_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 16, "ON")) {	/* PGTH_O_CHECKBOX_ removed */
#ifdef PGTH_O_CHECKBOX_ON
	    return PGTH_O_CHECKBOX_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_CH(char *name, int len, int arg)
{
    if (9 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 6]) {
    case '\0':
	if (strEQ(name + 9, "ECKBOX")) {	/* PGTH_O_CH removed */
#ifdef PGTH_O_CHECKBOX
	    return PGTH_O_CHECKBOX;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"ECKBOX", 6))
	    break;
	return constant_PGTH_O_CHECKBOX_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_CLOSEBTN_(char *name, int len, int arg)
{
    switch (name[16 + 0]) {
    case 'H':
	if (strEQ(name + 16, "HILIGHT")) {	/* PGTH_O_CLOSEBTN_ removed */
#ifdef PGTH_O_CLOSEBTN_HILIGHT
	    return PGTH_O_CLOSEBTN_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 16, "ON")) {	/* PGTH_O_CLOSEBTN_ removed */
#ifdef PGTH_O_CLOSEBTN_ON
	    return PGTH_O_CLOSEBTN_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_CL(char *name, int len, int arg)
{
    if (9 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 6]) {
    case '\0':
	if (strEQ(name + 9, "OSEBTN")) {	/* PGTH_O_CL removed */
#ifdef PGTH_O_CLOSEBTN
	    return PGTH_O_CLOSEBTN;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"OSEBTN", 6))
	    break;
	return constant_PGTH_O_CLOSEBTN_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_C(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'H':
	return constant_PGTH_O_CH(name, len, arg);
    case 'L':
	return constant_PGTH_O_CL(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_T(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'H':
	if (strEQ(name + 8, "HEMEINFO")) {	/* PGTH_O_T removed */
#ifdef PGTH_O_THEMEINFO
	    return PGTH_O_THEMEINFO;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 8, "OOLBAR")) {	/* PGTH_O_T removed */
#ifdef PGTH_O_TOOLBAR
	    return PGTH_O_TOOLBAR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_FLATBUTTON_(char *name, int len, int arg)
{
    switch (name[18 + 0]) {
    case 'H':
	if (strEQ(name + 18, "HILIGHT")) {	/* PGTH_O_FLATBUTTON_ removed */
#ifdef PGTH_O_FLATBUTTON_HILIGHT
	    return PGTH_O_FLATBUTTON_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 18, "ON")) {	/* PGTH_O_FLATBUTTON_ removed */
#ifdef PGTH_O_FLATBUTTON_ON
	    return PGTH_O_FLATBUTTON_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_FL(char *name, int len, int arg)
{
    if (9 + 8 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 8]) {
    case '\0':
	if (strEQ(name + 9, "ATBUTTON")) {	/* PGTH_O_FL removed */
#ifdef PGTH_O_FLATBUTTON
	    return PGTH_O_FLATBUTTON;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 9,"ATBUTTON", 8))
	    break;
	return constant_PGTH_O_FLATBUTTON_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_F(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'I':
	if (strEQ(name + 8, "IELD")) {	/* PGTH_O_F removed */
#ifdef PGTH_O_FIELD
	    return PGTH_O_FIELD;
#else
	    goto not_there;
#endif
	}
    case 'L':
	return constant_PGTH_O_FL(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_ZOOMBTN_(char *name, int len, int arg)
{
    switch (name[15 + 0]) {
    case 'H':
	if (strEQ(name + 15, "HILIGHT")) {	/* PGTH_O_ZOOMBTN_ removed */
#ifdef PGTH_O_ZOOMBTN_HILIGHT
	    return PGTH_O_ZOOMBTN_HILIGHT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 15, "ON")) {	/* PGTH_O_ZOOMBTN_ removed */
#ifdef PGTH_O_ZOOMBTN_ON
	    return PGTH_O_ZOOMBTN_ON;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_Z(char *name, int len, int arg)
{
    if (8 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 6]) {
    case '\0':
	if (strEQ(name + 8, "OOMBTN")) {	/* PGTH_O_Z removed */
#ifdef PGTH_O_ZOOMBTN
	    return PGTH_O_ZOOMBTN;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 8,"OOMBTN", 6))
	    break;
	return constant_PGTH_O_ZOOMBTN_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_LABEL_D(char *name, int len, int arg)
{
    if (14 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[14 + 3]) {
    case 'E':
	if (strEQ(name + 14, "LGTEXT")) {	/* PGTH_O_LABEL_D removed */
#ifdef PGTH_O_LABEL_DLGTEXT
	    return PGTH_O_LABEL_DLGTEXT;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 14, "LGTITLE")) {	/* PGTH_O_LABEL_D removed */
#ifdef PGTH_O_LABEL_DLGTITLE
	    return PGTH_O_LABEL_DLGTITLE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_LABEL_(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case 'D':
	return constant_PGTH_O_LABEL_D(name, len, arg);
    case 'S':
	if (strEQ(name + 13, "SCROLL")) {	/* PGTH_O_LABEL_ removed */
#ifdef PGTH_O_LABEL_SCROLL
	    return PGTH_O_LABEL_SCROLL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_L(char *name, int len, int arg)
{
    if (8 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 4]) {
    case '\0':
	if (strEQ(name + 8, "ABEL")) {	/* PGTH_O_L removed */
#ifdef PGTH_O_LABEL
	    return PGTH_O_LABEL;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 8,"ABEL", 4))
	    break;
	return constant_PGTH_O_LABEL_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_M(char *name, int len, int arg)
{
    if (8 + 7 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 7]) {
    case '\0':
	if (strEQ(name + 8, "ENUITEM")) {	/* PGTH_O_M removed */
#ifdef PGTH_O_MENUITEM
	    return PGTH_O_MENUITEM;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (strEQ(name + 8, "ENUITEM_HILIGHT")) {	/* PGTH_O_M removed */
#ifdef PGTH_O_MENUITEM_HILIGHT
	    return PGTH_O_MENUITEM_HILIGHT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O_(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'B':
	return constant_PGTH_O_B(name, len, arg);
    case 'C':
	return constant_PGTH_O_C(name, len, arg);
    case 'D':
	if (strEQ(name + 7, "DEFAULT")) {	/* PGTH_O_ removed */
#ifdef PGTH_O_DEFAULT
	    return PGTH_O_DEFAULT;
#else
	    goto not_there;
#endif
	}
    case 'F':
	return constant_PGTH_O_F(name, len, arg);
    case 'I':
	if (strEQ(name + 7, "INDICATOR")) {	/* PGTH_O_ removed */
#ifdef PGTH_O_INDICATOR
	    return PGTH_O_INDICATOR;
#else
	    goto not_there;
#endif
	}
    case 'L':
	return constant_PGTH_O_L(name, len, arg);
    case 'M':
	return constant_PGTH_O_M(name, len, arg);
    case 'P':
	return constant_PGTH_O_P(name, len, arg);
    case 'R':
	return constant_PGTH_O_R(name, len, arg);
    case 'S':
	return constant_PGTH_O_S(name, len, arg);
    case 'T':
	return constant_PGTH_O_T(name, len, arg);
    case 'Z':
	return constant_PGTH_O_Z(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPSIMPLE_G(char *name, int len, int arg)
{
    switch (name[15 + 0]) {
    case 'E':
	if (strEQ(name + 15, "ET")) {	/* PGTH_OPSIMPLE_G removed */
#ifdef PGTH_OPSIMPLE_GET
	    return PGTH_OPSIMPLE_GET;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 15, "ROP")) {	/* PGTH_OPSIMPLE_G removed */
#ifdef PGTH_OPSIMPLE_GROP
	    return PGTH_OPSIMPLE_GROP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPS(char *name, int len, int arg)
{
    if (8 + 6 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 6]) {
    case 'C':
	if (strEQ(name + 8, "IMPLE_CMDCODE")) {	/* PGTH_OPS removed */
#ifdef PGTH_OPSIMPLE_CMDCODE
	    return PGTH_OPSIMPLE_CMDCODE;
#else
	    goto not_there;
#endif
	}
    case 'G':
	if (!strnEQ(name + 8,"IMPLE_", 6))
	    break;
	return constant_PGTH_OPSIMPLE_G(name, len, arg);
    case 'L':
	if (strEQ(name + 8, "IMPLE_LITERAL")) {	/* PGTH_OPS removed */
#ifdef PGTH_OPSIMPLE_LITERAL
	    return PGTH_OPSIMPLE_LITERAL;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 8, "IMPLE_SET")) {	/* PGTH_OPS removed */
#ifdef PGTH_OPSIMPLE_SET
	    return PGTH_OPSIMPLE_SET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_P(char *name, int len, int arg)
{
    switch (name[12 + 0]) {
    case 'L':
	if (strEQ(name + 12, "LUS")) {	/* PGTH_OPCMD_P removed */
#ifdef PGTH_OPCMD_PLUS
	    return PGTH_OPCMD_PLUS;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 12, "ROPERTY")) {	/* PGTH_OPCMD_P removed */
#ifdef PGTH_OPCMD_PROPERTY
	    return PGTH_OPCMD_PROPERTY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_S(char *name, int len, int arg)
{
    if (12 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 4]) {
    case 'L':
	if (strEQ(name + 12, "HIFTL")) {	/* PGTH_OPCMD_S removed */
#ifdef PGTH_OPCMD_SHIFTL
	    return PGTH_OPCMD_SHIFTL;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 12, "HIFTR")) {	/* PGTH_OPCMD_S removed */
#ifdef PGTH_OPCMD_SHIFTR
	    return PGTH_OPCMD_SHIFTR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_C(char *name, int len, int arg)
{
    if (12 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[12 + 4]) {
    case '\0':
	if (strEQ(name + 12, "OLOR")) {	/* PGTH_OPCMD_C removed */
#ifdef PGTH_OPCMD_COLOR
	    return PGTH_OPCMD_COLOR;
#else
	    goto not_there;
#endif
	}
    case 'A':
	if (strEQ(name + 12, "OLORADD")) {	/* PGTH_OPCMD_C removed */
#ifdef PGTH_OPCMD_COLORADD
	    return PGTH_OPCMD_COLORADD;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (strEQ(name + 12, "OLORDIV")) {	/* PGTH_OPCMD_C removed */
#ifdef PGTH_OPCMD_COLORDIV
	    return PGTH_OPCMD_COLORDIV;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 12, "OLORMULT")) {	/* PGTH_OPCMD_C removed */
#ifdef PGTH_OPCMD_COLORMULT
	    return PGTH_OPCMD_COLORMULT;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 12, "OLORSUB")) {	/* PGTH_OPCMD_C removed */
#ifdef PGTH_OPCMD_COLORSUB
	    return PGTH_OPCMD_COLORSUB;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_LONGG(char *name, int len, int arg)
{
    switch (name[16 + 0]) {
    case 'E':
	if (strEQ(name + 16, "ET")) {	/* PGTH_OPCMD_LONGG removed */
#ifdef PGTH_OPCMD_LONGGET
	    return PGTH_OPCMD_LONGGET;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 16, "ROP")) {	/* PGTH_OPCMD_LONGG removed */
#ifdef PGTH_OPCMD_LONGGROP
	    return PGTH_OPCMD_LONGGROP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_LON(char *name, int len, int arg)
{
    if (14 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[14 + 1]) {
    case 'G':
	if (!strnEQ(name + 14,"G", 1))
	    break;
	return constant_PGTH_OPCMD_LONGG(name, len, arg);
    case 'L':
	if (strEQ(name + 14, "GLITERAL")) {	/* PGTH_OPCMD_LON removed */
#ifdef PGTH_OPCMD_LONGLITERAL
	    return PGTH_OPCMD_LONGLITERAL;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 14, "GSET")) {	/* PGTH_OPCMD_LON removed */
#ifdef PGTH_OPCMD_LONGSET
	    return PGTH_OPCMD_LONGSET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_LOG(char *name, int len, int arg)
{
    if (14 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[14 + 5]) {
    case 'A':
	if (strEQ(name + 14, "ICAL_AND")) {	/* PGTH_OPCMD_LOG removed */
#ifdef PGTH_OPCMD_LOGICAL_AND
	    return PGTH_OPCMD_LOGICAL_AND;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 14, "ICAL_NOT")) {	/* PGTH_OPCMD_LOG removed */
#ifdef PGTH_OPCMD_LOGICAL_NOT
	    return PGTH_OPCMD_LOGICAL_NOT;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 14, "ICAL_OR")) {	/* PGTH_OPCMD_LOG removed */
#ifdef PGTH_OPCMD_LOGICAL_OR
	    return PGTH_OPCMD_LOGICAL_OR;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_LO(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case 'C':
	if (strEQ(name + 13, "CALPROP")) {	/* PGTH_OPCMD_LO removed */
#ifdef PGTH_OPCMD_LOCALPROP
	    return PGTH_OPCMD_LOCALPROP;
#else
	    goto not_there;
#endif
	}
    case 'G':
	return constant_PGTH_OPCMD_LOG(name, len, arg);
    case 'N':
	return constant_PGTH_OPCMD_LON(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_L(char *name, int len, int arg)
{
    switch (name[12 + 0]) {
    case 'O':
	return constant_PGTH_OPCMD_LO(name, len, arg);
    case 'T':
	if (strEQ(name + 12, "T")) {	/* PGTH_OPCMD_L removed */
#ifdef PGTH_OPCMD_LT
	    return PGTH_OPCMD_LT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPCMD_M(char *name, int len, int arg)
{
    switch (name[12 + 0]) {
    case 'I':
	if (strEQ(name + 12, "INUS")) {	/* PGTH_OPCMD_M removed */
#ifdef PGTH_OPCMD_MINUS
	    return PGTH_OPCMD_MINUS;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 12, "ULTIPLY")) {	/* PGTH_OPCMD_M removed */
#ifdef PGTH_OPCMD_MULTIPLY
	    return PGTH_OPCMD_MULTIPLY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OPC(char *name, int len, int arg)
{
    if (8 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 3]) {
    case 'A':
	if (strEQ(name + 8, "MD_AND")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_AND
	    return PGTH_OPCMD_AND;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (!strnEQ(name + 8,"MD_", 3))
	    break;
	return constant_PGTH_OPCMD_C(name, len, arg);
    case 'D':
	if (strEQ(name + 8, "MD_DIVIDE")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_DIVIDE
	    return PGTH_OPCMD_DIVIDE;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 8, "MD_EQ")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_EQ
	    return PGTH_OPCMD_EQ;
#else
	    goto not_there;
#endif
	}
    case 'G':
	if (strEQ(name + 8, "MD_GT")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_GT
	    return PGTH_OPCMD_GT;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (!strnEQ(name + 8,"MD_", 3))
	    break;
	return constant_PGTH_OPCMD_L(name, len, arg);
    case 'M':
	if (!strnEQ(name + 8,"MD_", 3))
	    break;
	return constant_PGTH_OPCMD_M(name, len, arg);
    case 'O':
	if (strEQ(name + 8, "MD_OR")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_OR
	    return PGTH_OPCMD_OR;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (!strnEQ(name + 8,"MD_", 3))
	    break;
	return constant_PGTH_OPCMD_P(name, len, arg);
    case 'Q':
	if (strEQ(name + 8, "MD_QUESTIONCOLON")) {	/* PGTH_OPC removed */
#ifdef PGTH_OPCMD_QUESTIONCOLON
	    return PGTH_OPCMD_QUESTIONCOLON;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (!strnEQ(name + 8,"MD_", 3))
	    break;
	return constant_PGTH_OPCMD_S(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_OP(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'C':
	return constant_PGTH_OPC(name, len, arg);
    case 'S':
	return constant_PGTH_OPS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_O(char *name, int len, int arg)
{
    switch (name[6 + 0]) {
    case 'N':
	if (strEQ(name + 6, "NUM")) {	/* PGTH_O removed */
#ifdef PGTH_ONUM
	    return PGTH_ONUM;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_PGTH_OP(name, len, arg);
    case '_':
	return constant_PGTH_O_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_O(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'F':
	if (strEQ(name + 8, "FFSET")) {	/* PGTH_P_O removed */
#ifdef PGTH_P_OFFSET
	    return PGTH_P_OFFSET;
#else
	    goto not_there;
#endif
	}
    case 'V':
	if (strEQ(name + 8, "VERLAY")) {	/* PGTH_P_O removed */
#ifdef PGTH_P_OVERLAY
	    return PGTH_P_OVERLAY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_BG(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'C':
	if (strEQ(name + 9, "COLOR")) {	/* PGTH_P_BG removed */
#ifdef PGTH_P_BGCOLOR
	    return PGTH_P_BGCOLOR;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 9, "FILL")) {	/* PGTH_P_BG removed */
#ifdef PGTH_P_BGFILL
	    return PGTH_P_BGFILL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_BI(char *name, int len, int arg)
{
    if (9 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 4]) {
    case '1':
	if (strEQ(name + 9, "TMAP1")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAP1
	    return PGTH_P_BITMAP1;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 9, "TMAP2")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAP2
	    return PGTH_P_BITMAP2;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 9, "TMAP3")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAP3
	    return PGTH_P_BITMAP3;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 9, "TMAP4")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAP4
	    return PGTH_P_BITMAP4;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 9, "TMAPMARGIN")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAPMARGIN
	    return PGTH_P_BITMAPMARGIN;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 9, "TMAPSIDE")) {	/* PGTH_P_BI removed */
#ifdef PGTH_P_BITMAPSIDE
	    return PGTH_P_BITMAPSIDE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_B(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'A':
	if (strEQ(name + 8, "ACKDROP")) {	/* PGTH_P_B removed */
#ifdef PGTH_P_BACKDROP
	    return PGTH_P_BACKDROP;
#else
	    goto not_there;
#endif
	}
    case 'G':
	return constant_PGTH_P_BG(name, len, arg);
    case 'I':
	return constant_PGTH_P_BI(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_ST(char *name, int len, int arg)
{
    if (9 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 5]) {
    case 'C':
	if (strEQ(name + 9, "RING_CANCEL")) {	/* PGTH_P_ST removed */
#ifdef PGTH_P_STRING_CANCEL
	    return PGTH_P_STRING_CANCEL;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 9, "RING_NO")) {	/* PGTH_P_ST removed */
#ifdef PGTH_P_STRING_NO
	    return PGTH_P_STRING_NO;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "RING_OK")) {	/* PGTH_P_ST removed */
#ifdef PGTH_P_STRING_OK
	    return PGTH_P_STRING_OK;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 9, "RING_YES")) {	/* PGTH_P_ST removed */
#ifdef PGTH_P_STRING_YES
	    return PGTH_P_STRING_YES;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_S(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'H':
	if (strEQ(name + 8, "HADOWCOLOR")) {	/* PGTH_P_S removed */
#ifdef PGTH_P_SHADOWCOLOR
	    return PGTH_P_SHADOWCOLOR;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 8, "IDE")) {	/* PGTH_P_S removed */
#ifdef PGTH_P_SIDE
	    return PGTH_P_SIDE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "PACING")) {	/* PGTH_P_S removed */
#ifdef PGTH_P_SPACING
	    return PGTH_P_SPACING;
#else
	    goto not_there;
#endif
	}
    case 'T':
	return constant_PGTH_P_ST(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_C(char *name, int len, int arg)
{
    if (8 + 10 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 10]) {
    case 'P':
	if (strEQ(name + 8, "URSORBITMAP")) {	/* PGTH_P_C removed */
#ifdef PGTH_P_CURSORBITMAP
	    return PGTH_P_CURSORBITMAP;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 8, "URSORBITMASK")) {	/* PGTH_P_C removed */
#ifdef PGTH_P_CURSORBITMASK
	    return PGTH_P_CURSORBITMASK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_F(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'G':
	if (strEQ(name + 8, "GCOLOR")) {	/* PGTH_P_F removed */
#ifdef PGTH_P_FGCOLOR
	    return PGTH_P_FGCOLOR;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 8, "ONT")) {	/* PGTH_P_F removed */
#ifdef PGTH_P_FONT
	    return PGTH_P_FONT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_WIDG(char *name, int len, int arg)
{
    if (11 + 7 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[11 + 7]) {
    case 'P':
	if (strEQ(name + 11, "ETBITMAP")) {	/* PGTH_P_WIDG removed */
#ifdef PGTH_P_WIDGETBITMAP
	    return PGTH_P_WIDGETBITMAP;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 11, "ETBITMASK")) {	/* PGTH_P_WIDG removed */
#ifdef PGTH_P_WIDGETBITMASK
	    return PGTH_P_WIDGETBITMASK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_W(char *name, int len, int arg)
{
    if (8 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 2]) {
    case 'G':
	if (!strnEQ(name + 8,"ID", 2))
	    break;
	return constant_PGTH_P_WIDG(name, len, arg);
    case 'T':
	if (strEQ(name + 8, "IDTH")) {	/* PGTH_P_W removed */
#ifdef PGTH_P_WIDTH
	    return PGTH_P_WIDTH;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_HO(char *name, int len, int arg)
{
    if (9 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 5]) {
    case 'C':
	if (strEQ(name + 9, "TKEY_CANCEL")) {	/* PGTH_P_HO removed */
#ifdef PGTH_P_HOTKEY_CANCEL
	    return PGTH_P_HOTKEY_CANCEL;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 9, "TKEY_NO")) {	/* PGTH_P_HO removed */
#ifdef PGTH_P_HOTKEY_NO
	    return PGTH_P_HOTKEY_NO;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 9, "TKEY_OK")) {	/* PGTH_P_HO removed */
#ifdef PGTH_P_HOTKEY_OK
	    return PGTH_P_HOTKEY_OK;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 9, "TKEY_YES")) {	/* PGTH_P_HO removed */
#ifdef PGTH_P_HOTKEY_YES
	    return PGTH_P_HOTKEY_YES;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_H(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'E':
	if (strEQ(name + 8, "EIGHT")) {	/* PGTH_P_H removed */
#ifdef PGTH_P_HEIGHT
	    return PGTH_P_HEIGHT;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 8, "ILIGHTCOLOR")) {	/* PGTH_P_H removed */
#ifdef PGTH_P_HILIGHTCOLOR
	    return PGTH_P_HILIGHTCOLOR;
#else
	    goto not_there;
#endif
	}
    case 'O':
	return constant_PGTH_P_HO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P_I(char *name, int len, int arg)
{
    if (8 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 4]) {
    case 'C':
	if (strEQ(name + 8, "CON_CANCEL")) {	/* PGTH_P_I removed */
#ifdef PGTH_P_ICON_CANCEL
	    return PGTH_P_ICON_CANCEL;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 8, "CON_OK")) {	/* PGTH_P_I removed */
#ifdef PGTH_P_ICON_OK
	    return PGTH_P_ICON_OK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_P(char *name, int len, int arg)
{
    if (6 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[6 + 1]) {
    case 'A':
	if (strEQ(name + 6, "_ALIGN")) {	/* PGTH_P removed */
#ifdef PGTH_P_ALIGN
	    return PGTH_P_ALIGN;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_B(name, len, arg);
    case 'C':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_C(name, len, arg);
    case 'F':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_F(name, len, arg);
    case 'H':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_H(name, len, arg);
    case 'I':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_I(name, len, arg);
    case 'M':
	if (strEQ(name + 6, "_MARGIN")) {	/* PGTH_P removed */
#ifdef PGTH_P_MARGIN
	    return PGTH_P_MARGIN;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 6, "_NAME")) {	/* PGTH_P removed */
#ifdef PGTH_P_NAME
	    return PGTH_P_NAME;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_O(name, len, arg);
    case 'S':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_S(name, len, arg);
    case 'T':
	if (strEQ(name + 6, "_TEXT")) {	/* PGTH_P removed */
#ifdef PGTH_P_TEXT
	    return PGTH_P_TEXT;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (!strnEQ(name + 6,"_", 1))
	    break;
	return constant_PGTH_P_W(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_TAG_A(char *name, int len, int arg)
{
    if (10 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[10 + 5]) {
    case '\0':
	if (strEQ(name + 10, "UTHOR")) {	/* PGTH_TAG_A removed */
#ifdef PGTH_TAG_AUTHOR
	    return PGTH_TAG_AUTHOR;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 10, "UTHOREMAIL")) {	/* PGTH_TAG_A removed */
#ifdef PGTH_TAG_AUTHOREMAIL
	    return PGTH_TAG_AUTHOREMAIL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_T(char *name, int len, int arg)
{
    if (6 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[6 + 3]) {
    case 'A':
	if (!strnEQ(name + 6,"AG_", 3))
	    break;
	return constant_PGTH_TAG_A(name, len, arg);
    case 'R':
	if (strEQ(name + 6, "AG_README")) {	/* PGTH_T removed */
#ifdef PGTH_TAG_README
	    return PGTH_TAG_README;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 6, "AG_URL")) {	/* PGTH_T removed */
#ifdef PGTH_TAG_URL
	    return PGTH_TAG_URL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGTH_L(char *name, int len, int arg)
{
    if (6 + 4 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[6 + 4]) {
    case 'C':
	if (strEQ(name + 6, "OAD_COPY")) {	/* PGTH_L removed */
#ifdef PGTH_LOAD_COPY
	    return PGTH_LOAD_COPY;
#else
	    goto not_there;
#endif
	}
    case 'N':
	if (strEQ(name + 6, "OAD_NONE")) {	/* PGTH_L removed */
#ifdef PGTH_LOAD_NONE
	    return PGTH_LOAD_NONE;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 6, "OAD_REQUEST")) {	/* PGTH_L removed */
#ifdef PGTH_LOAD_REQUEST
	    return PGTH_LOAD_REQUEST;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGT(char *name, int len, int arg)
{
    if (3 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[3 + 2]) {
    case 'F':
	if (strEQ(name + 3, "H_FORMATVERSION")) {	/* PGT removed */
#ifdef PGTH_FORMATVERSION
	    return PGTH_FORMATVERSION;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (!strnEQ(name + 3,"H_", 2))
	    break;
	return constant_PGTH_L(name, len, arg);
    case 'O':
	if (!strnEQ(name + 3,"H_", 2))
	    break;
	return constant_PGTH_O(name, len, arg);
    case 'P':
	if (!strnEQ(name + 3,"H_", 2))
	    break;
	return constant_PGTH_P(name, len, arg);
    case 'T':
	if (!strnEQ(name + 3,"H_", 2))
	    break;
	return constant_PGTH_T(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_A(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'M':
	if (strEQ(name + 7, "MPERSAND")) {	/* PGKEY_A removed */
#ifdef PGKEY_AMPERSAND
	    return PGKEY_AMPERSAND;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "STERISK")) {	/* PGKEY_A removed */
#ifdef PGKEY_ASTERISK
	    return PGKEY_ASTERISK;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 7, "T")) {	/* PGKEY_A removed */
#ifdef PGKEY_AT
	    return PGKEY_AT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_BACKS(char *name, int len, int arg)
{
    switch (name[11 + 0]) {
    case 'L':
	if (strEQ(name + 11, "LASH")) {	/* PGKEY_BACKS removed */
#ifdef PGKEY_BACKSLASH
	    return PGKEY_BACKSLASH;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 11, "PACE")) {	/* PGKEY_BACKS removed */
#ifdef PGKEY_BACKSPACE
	    return PGKEY_BACKSPACE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_BA(char *name, int len, int arg)
{
    if (8 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 2]) {
    case 'Q':
	if (strEQ(name + 8, "CKQUOTE")) {	/* PGKEY_BA removed */
#ifdef PGKEY_BACKQUOTE
	    return PGKEY_BACKQUOTE;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (!strnEQ(name + 8,"CK", 2))
	    break;
	return constant_PGKEY_BACKS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_B(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	return constant_PGKEY_BA(name, len, arg);
    case 'R':
	if (strEQ(name + 7, "REAK")) {	/* PGKEY_B removed */
#ifdef PGKEY_BREAK
	    return PGKEY_BREAK;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_CO(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'L':
	if (strEQ(name + 8, "LON")) {	/* PGKEY_CO removed */
#ifdef PGKEY_COLON
	    return PGKEY_COLON;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 8, "MMA")) {	/* PGKEY_CO removed */
#ifdef PGKEY_COMMA
	    return PGKEY_COMMA;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_CA(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'P':
	if (strEQ(name + 8, "PSLOCK")) {	/* PGKEY_CA removed */
#ifdef PGKEY_CAPSLOCK
	    return PGKEY_CAPSLOCK;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 8, "RET")) {	/* PGKEY_CA removed */
#ifdef PGKEY_CARET
	    return PGKEY_CARET;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_C(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	return constant_PGKEY_CA(name, len, arg);
    case 'L':
	if (strEQ(name + 7, "LEAR")) {	/* PGKEY_C removed */
#ifdef PGKEY_CLEAR
	    return PGKEY_CLEAR;
#else
	    goto not_there;
#endif
	}
    case 'O':
	return constant_PGKEY_CO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_DO(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'L':
	if (strEQ(name + 8, "LLAR")) {	/* PGKEY_DO removed */
#ifdef PGKEY_DOLLAR
	    return PGKEY_DOLLAR;
#else
	    goto not_there;
#endif
	}
    case 'W':
	if (strEQ(name + 8, "WN")) {	/* PGKEY_DO removed */
#ifdef PGKEY_DOWN
	    return PGKEY_DOWN;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_D(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	if (strEQ(name + 7, "ELETE")) {	/* PGKEY_D removed */
#ifdef PGKEY_DELETE
	    return PGKEY_DELETE;
#else
	    goto not_there;
#endif
	}
    case 'O':
	return constant_PGKEY_DO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_E(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'N':
	if (strEQ(name + 7, "ND")) {	/* PGKEY_E removed */
#ifdef PGKEY_END
	    return PGKEY_END;
#else
	    goto not_there;
#endif
	}
    case 'Q':
	if (strEQ(name + 7, "QUALS")) {	/* PGKEY_E removed */
#ifdef PGKEY_EQUALS
	    return PGKEY_EQUALS;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "SCAPE")) {	/* PGKEY_E removed */
#ifdef PGKEY_ESCAPE
	    return PGKEY_ESCAPE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 7, "URO")) {	/* PGKEY_E removed */
#ifdef PGKEY_EURO
	    return PGKEY_EURO;
#else
	    goto not_there;
#endif
	}
    case 'X':
	if (strEQ(name + 7, "XCLAIM")) {	/* PGKEY_E removed */
#ifdef PGKEY_EXCLAIM
	    return PGKEY_EXCLAIM;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_F1(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case '\0':
	if (strEQ(name + 8, "")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F1
	    return PGKEY_F1;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 8, "0")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F10
	    return PGKEY_F10;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 8, "1")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F11
	    return PGKEY_F11;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 8, "2")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F12
	    return PGKEY_F12;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 8, "3")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F13
	    return PGKEY_F13;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 8, "4")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F14
	    return PGKEY_F14;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 8, "5")) {	/* PGKEY_F1 removed */
#ifdef PGKEY_F15
	    return PGKEY_F15;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_F(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case '1':
	return constant_PGKEY_F1(name, len, arg);
    case '2':
	if (strEQ(name + 7, "2")) {	/* PGKEY_F removed */
#ifdef PGKEY_F2
	    return PGKEY_F2;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 7, "3")) {	/* PGKEY_F removed */
#ifdef PGKEY_F3
	    return PGKEY_F3;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 7, "4")) {	/* PGKEY_F removed */
#ifdef PGKEY_F4
	    return PGKEY_F4;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 7, "5")) {	/* PGKEY_F removed */
#ifdef PGKEY_F5
	    return PGKEY_F5;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 7, "6")) {	/* PGKEY_F removed */
#ifdef PGKEY_F6
	    return PGKEY_F6;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 7, "7")) {	/* PGKEY_F removed */
#ifdef PGKEY_F7
	    return PGKEY_F7;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 7, "8")) {	/* PGKEY_F removed */
#ifdef PGKEY_F8
	    return PGKEY_F8;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 7, "9")) {	/* PGKEY_F removed */
#ifdef PGKEY_F9
	    return PGKEY_F9;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_H(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ASH")) {	/* PGKEY_H removed */
#ifdef PGKEY_HASH
	    return PGKEY_HASH;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 7, "ELP")) {	/* PGKEY_H removed */
#ifdef PGKEY_HELP
	    return PGKEY_HELP;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 7, "OME")) {	/* PGKEY_H removed */
#ifdef PGKEY_HOME
	    return PGKEY_HOME;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_KP_P(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'E':
	if (strEQ(name + 10, "ERIOD")) {	/* PGKEY_KP_P removed */
#ifdef PGKEY_KP_PERIOD
	    return PGKEY_KP_PERIOD;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 10, "LUS")) {	/* PGKEY_KP_P removed */
#ifdef PGKEY_KP_PLUS
	    return PGKEY_KP_PLUS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_KP_E(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'N':
	if (strEQ(name + 10, "NTER")) {	/* PGKEY_KP_E removed */
#ifdef PGKEY_KP_ENTER
	    return PGKEY_KP_ENTER;
#else
	    goto not_there;
#endif
	}
    case 'Q':
	if (strEQ(name + 10, "QUALS")) {	/* PGKEY_KP_E removed */
#ifdef PGKEY_KP_EQUALS
	    return PGKEY_KP_EQUALS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_KP_M(char *name, int len, int arg)
{
    switch (name[10 + 0]) {
    case 'I':
	if (strEQ(name + 10, "INUS")) {	/* PGKEY_KP_M removed */
#ifdef PGKEY_KP_MINUS
	    return PGKEY_KP_MINUS;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 10, "ULTIPLY")) {	/* PGKEY_KP_M removed */
#ifdef PGKEY_KP_MULTIPLY
	    return PGKEY_KP_MULTIPLY;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_KP_(char *name, int len, int arg)
{
    switch (name[9 + 0]) {
    case 'D':
	if (strEQ(name + 9, "DIVIDE")) {	/* PGKEY_KP_ removed */
#ifdef PGKEY_KP_DIVIDE
	    return PGKEY_KP_DIVIDE;
#else
	    goto not_there;
#endif
	}
    case 'E':
	return constant_PGKEY_KP_E(name, len, arg);
    case 'M':
	return constant_PGKEY_KP_M(name, len, arg);
    case 'P':
	return constant_PGKEY_KP_P(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_K(char *name, int len, int arg)
{
    if (7 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 1]) {
    case '0':
	if (strEQ(name + 7, "P0")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP0
	    return PGKEY_KP0;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 7, "P1")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP1
	    return PGKEY_KP1;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 7, "P2")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP2
	    return PGKEY_KP2;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 7, "P3")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP3
	    return PGKEY_KP3;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 7, "P4")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP4
	    return PGKEY_KP4;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 7, "P5")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP5
	    return PGKEY_KP5;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 7, "P6")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP6
	    return PGKEY_KP6;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 7, "P7")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP7
	    return PGKEY_KP7;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 7, "P8")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP8
	    return PGKEY_KP8;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 7, "P9")) {	/* PGKEY_K removed */
#ifdef PGKEY_KP9
	    return PGKEY_KP9;
#else
	    goto not_there;
#endif
	}
    case '_':
	if (!strnEQ(name + 7,"P", 1))
	    break;
	return constant_PGKEY_KP_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_LS(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'H':
	if (strEQ(name + 8, "HIFT")) {	/* PGKEY_LS removed */
#ifdef PGKEY_LSHIFT
	    return PGKEY_LSHIFT;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 8, "UPER")) {	/* PGKEY_LS removed */
#ifdef PGKEY_LSUPER
	    return PGKEY_LSUPER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_LEF(char *name, int len, int arg)
{
    if (9 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 1]) {
    case '\0':
	if (strEQ(name + 9, "T")) {	/* PGKEY_LEF removed */
#ifdef PGKEY_LEFT
	    return PGKEY_LEFT;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (strEQ(name + 9, "TBRACKET")) {	/* PGKEY_LEF removed */
#ifdef PGKEY_LEFTBRACKET
	    return PGKEY_LEFTBRACKET;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 9, "TPAREN")) {	/* PGKEY_LEF removed */
#ifdef PGKEY_LEFTPAREN
	    return PGKEY_LEFTPAREN;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_LE(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'F':
	return constant_PGKEY_LEF(name, len, arg);
    case 'S':
	if (strEQ(name + 8, "SS")) {	/* PGKEY_LE removed */
#ifdef PGKEY_LESS
	    return PGKEY_LESS;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_L(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ALT")) {	/* PGKEY_L removed */
#ifdef PGKEY_LALT
	    return PGKEY_LALT;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 7, "CTRL")) {	/* PGKEY_L removed */
#ifdef PGKEY_LCTRL
	    return PGKEY_LCTRL;
#else
	    goto not_there;
#endif
	}
    case 'E':
	return constant_PGKEY_LE(name, len, arg);
    case 'M':
	if (strEQ(name + 7, "META")) {	/* PGKEY_L removed */
#ifdef PGKEY_LMETA
	    return PGKEY_LMETA;
#else
	    goto not_there;
#endif
	}
    case 'S':
	return constant_PGKEY_LS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_M(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	if (strEQ(name + 7, "ENU")) {	/* PGKEY_M removed */
#ifdef PGKEY_MENU
	    return PGKEY_MENU;
#else
	    goto not_there;
#endif
	}
    case 'I':
	if (strEQ(name + 7, "INUS")) {	/* PGKEY_M removed */
#ifdef PGKEY_MINUS
	    return PGKEY_MINUS;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 7, "ODE")) {	/* PGKEY_M removed */
#ifdef PGKEY_MODE
	    return PGKEY_MODE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_PAG(char *name, int len, int arg)
{
    if (9 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 1]) {
    case 'D':
	if (strEQ(name + 9, "EDOWN")) {	/* PGKEY_PAG removed */
#ifdef PGKEY_PAGEDOWN
	    return PGKEY_PAGEDOWN;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 9, "EUP")) {	/* PGKEY_PAG removed */
#ifdef PGKEY_PAGEUP
	    return PGKEY_PAGEUP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_PA(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'G':
	return constant_PGKEY_PAG(name, len, arg);
    case 'U':
	if (strEQ(name + 8, "USE")) {	/* PGKEY_PA removed */
#ifdef PGKEY_PAUSE
	    return PGKEY_PAUSE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_P(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	return constant_PGKEY_PA(name, len, arg);
    case 'E':
	if (strEQ(name + 7, "ERIOD")) {	/* PGKEY_P removed */
#ifdef PGKEY_PERIOD
	    return PGKEY_PERIOD;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 7, "LUS")) {	/* PGKEY_P removed */
#ifdef PGKEY_PLUS
	    return PGKEY_PLUS;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 7, "OWER")) {	/* PGKEY_P removed */
#ifdef PGKEY_POWER
	    return PGKEY_POWER;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (strEQ(name + 7, "RINT")) {	/* PGKEY_P removed */
#ifdef PGKEY_PRINT
	    return PGKEY_PRINT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_QUO(char *name, int len, int arg)
{
    if (9 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[9 + 2]) {
    case '\0':
	if (strEQ(name + 9, "TE")) {	/* PGKEY_QUO removed */
#ifdef PGKEY_QUOTE
	    return PGKEY_QUOTE;
#else
	    goto not_there;
#endif
	}
    case 'D':
	if (strEQ(name + 9, "TEDBL")) {	/* PGKEY_QUO removed */
#ifdef PGKEY_QUOTEDBL
	    return PGKEY_QUOTEDBL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_Q(char *name, int len, int arg)
{
    if (7 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 1]) {
    case 'E':
	if (strEQ(name + 7, "UESTION")) {	/* PGKEY_Q removed */
#ifdef PGKEY_QUESTION
	    return PGKEY_QUESTION;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (!strnEQ(name + 7,"U", 1))
	    break;
	return constant_PGKEY_QUO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_RS(char *name, int len, int arg)
{
    switch (name[8 + 0]) {
    case 'H':
	if (strEQ(name + 8, "HIFT")) {	/* PGKEY_RS removed */
#ifdef PGKEY_RSHIFT
	    return PGKEY_RSHIFT;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 8, "UPER")) {	/* PGKEY_RS removed */
#ifdef PGKEY_RSUPER
	    return PGKEY_RSUPER;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_RI(char *name, int len, int arg)
{
    if (8 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[8 + 3]) {
    case '\0':
	if (strEQ(name + 8, "GHT")) {	/* PGKEY_RI removed */
#ifdef PGKEY_RIGHT
	    return PGKEY_RIGHT;
#else
	    goto not_there;
#endif
	}
    case 'B':
	if (strEQ(name + 8, "GHTBRACKET")) {	/* PGKEY_RI removed */
#ifdef PGKEY_RIGHTBRACKET
	    return PGKEY_RIGHTBRACKET;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 8, "GHTPAREN")) {	/* PGKEY_RI removed */
#ifdef PGKEY_RIGHTPAREN
	    return PGKEY_RIGHTPAREN;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_R(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ALT")) {	/* PGKEY_R removed */
#ifdef PGKEY_RALT
	    return PGKEY_RALT;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 7, "CTRL")) {	/* PGKEY_R removed */
#ifdef PGKEY_RCTRL
	    return PGKEY_RCTRL;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 7, "ETURN")) {	/* PGKEY_R removed */
#ifdef PGKEY_RETURN
	    return PGKEY_RETURN;
#else
	    goto not_there;
#endif
	}
    case 'I':
	return constant_PGKEY_RI(name, len, arg);
    case 'M':
	if (strEQ(name + 7, "META")) {	/* PGKEY_R removed */
#ifdef PGKEY_RMETA
	    return PGKEY_RMETA;
#else
	    goto not_there;
#endif
	}
    case 'S':
	return constant_PGKEY_RS(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_S(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'C':
	if (strEQ(name + 7, "CROLLOCK")) {	/* PGKEY_S removed */
#ifdef PGKEY_SCROLLOCK
	    return PGKEY_SCROLLOCK;
#else
	    goto not_there;
#endif
	}
    case 'E':
	if (strEQ(name + 7, "EMICOLON")) {	/* PGKEY_S removed */
#ifdef PGKEY_SEMICOLON
	    return PGKEY_SEMICOLON;
#else
	    goto not_there;
#endif
	}
    case 'L':
	if (strEQ(name + 7, "LASH")) {	/* PGKEY_S removed */
#ifdef PGKEY_SLASH
	    return PGKEY_SLASH;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 7, "PACE")) {	/* PGKEY_S removed */
#ifdef PGKEY_SPACE
	    return PGKEY_SPACE;
#else
	    goto not_there;
#endif
	}
    case 'Y':
	if (strEQ(name + 7, "YSREQ")) {	/* PGKEY_S removed */
#ifdef PGKEY_SYSREQ
	    return PGKEY_SYSREQ;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_U(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'N':
	if (strEQ(name + 7, "NDERSCORE")) {	/* PGKEY_U removed */
#ifdef PGKEY_UNDERSCORE
	    return PGKEY_UNDERSCORE;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (strEQ(name + 7, "P")) {	/* PGKEY_U removed */
#ifdef PGKEY_UP
	    return PGKEY_UP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_1(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_1
	    return PGKEY_WORLD_1;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_10
	    return PGKEY_WORLD_10;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_11
	    return PGKEY_WORLD_11;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_12
	    return PGKEY_WORLD_12;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_13
	    return PGKEY_WORLD_13;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_14
	    return PGKEY_WORLD_14;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_15
	    return PGKEY_WORLD_15;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_16
	    return PGKEY_WORLD_16;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_17
	    return PGKEY_WORLD_17;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_18
	    return PGKEY_WORLD_18;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_1 removed */
#ifdef PGKEY_WORLD_19
	    return PGKEY_WORLD_19;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_2(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_2
	    return PGKEY_WORLD_2;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_20
	    return PGKEY_WORLD_20;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_21
	    return PGKEY_WORLD_21;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_22
	    return PGKEY_WORLD_22;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_23
	    return PGKEY_WORLD_23;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_24
	    return PGKEY_WORLD_24;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_25
	    return PGKEY_WORLD_25;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_26
	    return PGKEY_WORLD_26;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_27
	    return PGKEY_WORLD_27;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_28
	    return PGKEY_WORLD_28;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_2 removed */
#ifdef PGKEY_WORLD_29
	    return PGKEY_WORLD_29;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_3(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_3
	    return PGKEY_WORLD_3;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_30
	    return PGKEY_WORLD_30;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_31
	    return PGKEY_WORLD_31;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_32
	    return PGKEY_WORLD_32;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_33
	    return PGKEY_WORLD_33;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_34
	    return PGKEY_WORLD_34;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_35
	    return PGKEY_WORLD_35;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_36
	    return PGKEY_WORLD_36;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_37
	    return PGKEY_WORLD_37;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_38
	    return PGKEY_WORLD_38;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_3 removed */
#ifdef PGKEY_WORLD_39
	    return PGKEY_WORLD_39;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_4(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_4
	    return PGKEY_WORLD_4;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_40
	    return PGKEY_WORLD_40;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_41
	    return PGKEY_WORLD_41;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_42
	    return PGKEY_WORLD_42;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_43
	    return PGKEY_WORLD_43;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_44
	    return PGKEY_WORLD_44;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_45
	    return PGKEY_WORLD_45;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_46
	    return PGKEY_WORLD_46;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_47
	    return PGKEY_WORLD_47;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_48
	    return PGKEY_WORLD_48;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_4 removed */
#ifdef PGKEY_WORLD_49
	    return PGKEY_WORLD_49;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_5(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_5
	    return PGKEY_WORLD_5;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_50
	    return PGKEY_WORLD_50;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_51
	    return PGKEY_WORLD_51;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_52
	    return PGKEY_WORLD_52;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_53
	    return PGKEY_WORLD_53;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_54
	    return PGKEY_WORLD_54;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_55
	    return PGKEY_WORLD_55;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_56
	    return PGKEY_WORLD_56;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_57
	    return PGKEY_WORLD_57;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_58
	    return PGKEY_WORLD_58;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_5 removed */
#ifdef PGKEY_WORLD_59
	    return PGKEY_WORLD_59;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_6(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_6
	    return PGKEY_WORLD_6;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_60
	    return PGKEY_WORLD_60;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_61
	    return PGKEY_WORLD_61;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_62
	    return PGKEY_WORLD_62;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_63
	    return PGKEY_WORLD_63;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_64
	    return PGKEY_WORLD_64;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_65
	    return PGKEY_WORLD_65;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_66
	    return PGKEY_WORLD_66;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_67
	    return PGKEY_WORLD_67;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_68
	    return PGKEY_WORLD_68;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_6 removed */
#ifdef PGKEY_WORLD_69
	    return PGKEY_WORLD_69;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_7(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_7
	    return PGKEY_WORLD_7;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_70
	    return PGKEY_WORLD_70;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_71
	    return PGKEY_WORLD_71;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_72
	    return PGKEY_WORLD_72;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_73
	    return PGKEY_WORLD_73;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_74
	    return PGKEY_WORLD_74;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_75
	    return PGKEY_WORLD_75;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_76
	    return PGKEY_WORLD_76;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_77
	    return PGKEY_WORLD_77;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_78
	    return PGKEY_WORLD_78;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_7 removed */
#ifdef PGKEY_WORLD_79
	    return PGKEY_WORLD_79;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_8(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_8
	    return PGKEY_WORLD_8;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_80
	    return PGKEY_WORLD_80;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_81
	    return PGKEY_WORLD_81;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_82
	    return PGKEY_WORLD_82;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_83
	    return PGKEY_WORLD_83;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_84
	    return PGKEY_WORLD_84;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_85
	    return PGKEY_WORLD_85;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 13, "6")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_86
	    return PGKEY_WORLD_86;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 13, "7")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_87
	    return PGKEY_WORLD_87;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 13, "8")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_88
	    return PGKEY_WORLD_88;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 13, "9")) {	/* PGKEY_WORLD_8 removed */
#ifdef PGKEY_WORLD_89
	    return PGKEY_WORLD_89;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_WORLD_9(char *name, int len, int arg)
{
    switch (name[13 + 0]) {
    case '\0':
	if (strEQ(name + 13, "")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_9
	    return PGKEY_WORLD_9;
#else
	    goto not_there;
#endif
	}
    case '0':
	if (strEQ(name + 13, "0")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_90
	    return PGKEY_WORLD_90;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 13, "1")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_91
	    return PGKEY_WORLD_91;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 13, "2")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_92
	    return PGKEY_WORLD_92;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 13, "3")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_93
	    return PGKEY_WORLD_93;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 13, "4")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_94
	    return PGKEY_WORLD_94;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 13, "5")) {	/* PGKEY_WORLD_9 removed */
#ifdef PGKEY_WORLD_95
	    return PGKEY_WORLD_95;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGKEY_W(char *name, int len, int arg)
{
    if (7 + 5 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[7 + 5]) {
    case '0':
	if (strEQ(name + 7, "ORLD_0")) {	/* PGKEY_W removed */
#ifdef PGKEY_WORLD_0
	    return PGKEY_WORLD_0;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_1(name, len, arg);
    case '2':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_2(name, len, arg);
    case '3':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_3(name, len, arg);
    case '4':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_4(name, len, arg);
    case '5':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_5(name, len, arg);
    case '6':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_6(name, len, arg);
    case '7':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_7(name, len, arg);
    case '8':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_8(name, len, arg);
    case '9':
	if (!strnEQ(name + 7,"ORLD_", 5))
	    break;
	return constant_PGKEY_WORLD_9(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGK(char *name, int len, int arg)
{
    if (3 + 3 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[3 + 3]) {
    case '0':
	if (strEQ(name + 3, "EY_0")) {	/* PGK removed */
#ifdef PGKEY_0
	    return PGKEY_0;
#else
	    goto not_there;
#endif
	}
    case '1':
	if (strEQ(name + 3, "EY_1")) {	/* PGK removed */
#ifdef PGKEY_1
	    return PGKEY_1;
#else
	    goto not_there;
#endif
	}
    case '2':
	if (strEQ(name + 3, "EY_2")) {	/* PGK removed */
#ifdef PGKEY_2
	    return PGKEY_2;
#else
	    goto not_there;
#endif
	}
    case '3':
	if (strEQ(name + 3, "EY_3")) {	/* PGK removed */
#ifdef PGKEY_3
	    return PGKEY_3;
#else
	    goto not_there;
#endif
	}
    case '4':
	if (strEQ(name + 3, "EY_4")) {	/* PGK removed */
#ifdef PGKEY_4
	    return PGKEY_4;
#else
	    goto not_there;
#endif
	}
    case '5':
	if (strEQ(name + 3, "EY_5")) {	/* PGK removed */
#ifdef PGKEY_5
	    return PGKEY_5;
#else
	    goto not_there;
#endif
	}
    case '6':
	if (strEQ(name + 3, "EY_6")) {	/* PGK removed */
#ifdef PGKEY_6
	    return PGKEY_6;
#else
	    goto not_there;
#endif
	}
    case '7':
	if (strEQ(name + 3, "EY_7")) {	/* PGK removed */
#ifdef PGKEY_7
	    return PGKEY_7;
#else
	    goto not_there;
#endif
	}
    case '8':
	if (strEQ(name + 3, "EY_8")) {	/* PGK removed */
#ifdef PGKEY_8
	    return PGKEY_8;
#else
	    goto not_there;
#endif
	}
    case '9':
	if (strEQ(name + 3, "EY_9")) {	/* PGK removed */
#ifdef PGKEY_9
	    return PGKEY_9;
#else
	    goto not_there;
#endif
	}
    case 'A':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_A(name, len, arg);
    case 'B':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_B(name, len, arg);
    case 'C':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_C(name, len, arg);
    case 'D':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_D(name, len, arg);
    case 'E':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_E(name, len, arg);
    case 'F':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_F(name, len, arg);
    case 'G':
	if (strEQ(name + 3, "EY_GREATER")) {	/* PGK removed */
#ifdef PGKEY_GREATER
	    return PGKEY_GREATER;
#else
	    goto not_there;
#endif
	}
    case 'H':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_H(name, len, arg);
    case 'I':
	if (strEQ(name + 3, "EY_INSERT")) {	/* PGK removed */
#ifdef PGKEY_INSERT
	    return PGKEY_INSERT;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_K(name, len, arg);
    case 'L':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_L(name, len, arg);
    case 'M':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_M(name, len, arg);
    case 'N':
	if (strEQ(name + 3, "EY_NUMLOCK")) {	/* PGK removed */
#ifdef PGKEY_NUMLOCK
	    return PGKEY_NUMLOCK;
#else
	    goto not_there;
#endif
	}
    case 'P':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_P(name, len, arg);
    case 'Q':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_Q(name, len, arg);
    case 'R':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_R(name, len, arg);
    case 'S':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_S(name, len, arg);
    case 'T':
	if (strEQ(name + 3, "EY_TAB")) {	/* PGK removed */
#ifdef PGKEY_TAB
	    return PGKEY_TAB;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_U(name, len, arg);
    case 'W':
	if (!strnEQ(name + 3,"EY_", 3))
	    break;
	return constant_PGKEY_W(name, len, arg);
    case 'a':
	if (strEQ(name + 3, "EY_a")) {	/* PGK removed */
#ifdef PGKEY_a
	    return PGKEY_a;
#else
	    goto not_there;
#endif
	}
    case 'b':
	if (strEQ(name + 3, "EY_b")) {	/* PGK removed */
#ifdef PGKEY_b
	    return PGKEY_b;
#else
	    goto not_there;
#endif
	}
    case 'c':
	if (strEQ(name + 3, "EY_c")) {	/* PGK removed */
#ifdef PGKEY_c
	    return PGKEY_c;
#else
	    goto not_there;
#endif
	}
    case 'd':
	if (strEQ(name + 3, "EY_d")) {	/* PGK removed */
#ifdef PGKEY_d
	    return PGKEY_d;
#else
	    goto not_there;
#endif
	}
    case 'e':
	if (strEQ(name + 3, "EY_e")) {	/* PGK removed */
#ifdef PGKEY_e
	    return PGKEY_e;
#else
	    goto not_there;
#endif
	}
    case 'f':
	if (strEQ(name + 3, "EY_f")) {	/* PGK removed */
#ifdef PGKEY_f
	    return PGKEY_f;
#else
	    goto not_there;
#endif
	}
    case 'g':
	if (strEQ(name + 3, "EY_g")) {	/* PGK removed */
#ifdef PGKEY_g
	    return PGKEY_g;
#else
	    goto not_there;
#endif
	}
    case 'h':
	if (strEQ(name + 3, "EY_h")) {	/* PGK removed */
#ifdef PGKEY_h
	    return PGKEY_h;
#else
	    goto not_there;
#endif
	}
    case 'i':
	if (strEQ(name + 3, "EY_i")) {	/* PGK removed */
#ifdef PGKEY_i
	    return PGKEY_i;
#else
	    goto not_there;
#endif
	}
    case 'j':
	if (strEQ(name + 3, "EY_j")) {	/* PGK removed */
#ifdef PGKEY_j
	    return PGKEY_j;
#else
	    goto not_there;
#endif
	}
    case 'k':
	if (strEQ(name + 3, "EY_k")) {	/* PGK removed */
#ifdef PGKEY_k
	    return PGKEY_k;
#else
	    goto not_there;
#endif
	}
    case 'l':
	if (strEQ(name + 3, "EY_l")) {	/* PGK removed */
#ifdef PGKEY_l
	    return PGKEY_l;
#else
	    goto not_there;
#endif
	}
    case 'm':
	if (strEQ(name + 3, "EY_m")) {	/* PGK removed */
#ifdef PGKEY_m
	    return PGKEY_m;
#else
	    goto not_there;
#endif
	}
    case 'n':
	if (strEQ(name + 3, "EY_n")) {	/* PGK removed */
#ifdef PGKEY_n
	    return PGKEY_n;
#else
	    goto not_there;
#endif
	}
    case 'o':
	if (strEQ(name + 3, "EY_o")) {	/* PGK removed */
#ifdef PGKEY_o
	    return PGKEY_o;
#else
	    goto not_there;
#endif
	}
    case 'p':
	if (strEQ(name + 3, "EY_p")) {	/* PGK removed */
#ifdef PGKEY_p
	    return PGKEY_p;
#else
	    goto not_there;
#endif
	}
    case 'q':
	if (strEQ(name + 3, "EY_q")) {	/* PGK removed */
#ifdef PGKEY_q
	    return PGKEY_q;
#else
	    goto not_there;
#endif
	}
    case 'r':
	if (strEQ(name + 3, "EY_r")) {	/* PGK removed */
#ifdef PGKEY_r
	    return PGKEY_r;
#else
	    goto not_there;
#endif
	}
    case 's':
	if (strEQ(name + 3, "EY_s")) {	/* PGK removed */
#ifdef PGKEY_s
	    return PGKEY_s;
#else
	    goto not_there;
#endif
	}
    case 't':
	if (strEQ(name + 3, "EY_t")) {	/* PGK removed */
#ifdef PGKEY_t
	    return PGKEY_t;
#else
	    goto not_there;
#endif
	}
    case 'u':
	if (strEQ(name + 3, "EY_u")) {	/* PGK removed */
#ifdef PGKEY_u
	    return PGKEY_u;
#else
	    goto not_there;
#endif
	}
    case 'v':
	if (strEQ(name + 3, "EY_v")) {	/* PGK removed */
#ifdef PGKEY_v
	    return PGKEY_v;
#else
	    goto not_there;
#endif
	}
    case 'w':
	if (strEQ(name + 3, "EY_w")) {	/* PGK removed */
#ifdef PGKEY_w
	    return PGKEY_w;
#else
	    goto not_there;
#endif
	}
    case 'x':
	if (strEQ(name + 3, "EY_x")) {	/* PGK removed */
#ifdef PGKEY_x
	    return PGKEY_x;
#else
	    goto not_there;
#endif
	}
    case 'y':
	if (strEQ(name + 3, "EY_y")) {	/* PGK removed */
#ifdef PGKEY_y
	    return PGKEY_y;
#else
	    goto not_there;
#endif
	}
    case 'z':
	if (strEQ(name + 3, "EY_z")) {	/* PGK removed */
#ifdef PGKEY_z
	    return PGKEY_z;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGMOD_C(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "APS")) {	/* PGMOD_C removed */
#ifdef PGMOD_CAPS
	    return PGMOD_CAPS;
#else
	    goto not_there;
#endif
	}
    case 'T':
	if (strEQ(name + 7, "TRL")) {	/* PGMOD_C removed */
#ifdef PGMOD_CTRL
	    return PGMOD_CTRL;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGMOD_L(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ALT")) {	/* PGMOD_L removed */
#ifdef PGMOD_LALT
	    return PGMOD_LALT;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 7, "CTRL")) {	/* PGMOD_L removed */
#ifdef PGMOD_LCTRL
	    return PGMOD_LCTRL;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 7, "META")) {	/* PGMOD_L removed */
#ifdef PGMOD_LMETA
	    return PGMOD_LMETA;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "SHIFT")) {	/* PGMOD_L removed */
#ifdef PGMOD_LSHIFT
	    return PGMOD_LSHIFT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGMOD_M(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'E':
	if (strEQ(name + 7, "ETA")) {	/* PGMOD_M removed */
#ifdef PGMOD_META
	    return PGMOD_META;
#else
	    goto not_there;
#endif
	}
    case 'O':
	if (strEQ(name + 7, "ODE")) {	/* PGMOD_M removed */
#ifdef PGMOD_MODE
	    return PGMOD_MODE;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGMOD_R(char *name, int len, int arg)
{
    switch (name[7 + 0]) {
    case 'A':
	if (strEQ(name + 7, "ALT")) {	/* PGMOD_R removed */
#ifdef PGMOD_RALT
	    return PGMOD_RALT;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (strEQ(name + 7, "CTRL")) {	/* PGMOD_R removed */
#ifdef PGMOD_RCTRL
	    return PGMOD_RCTRL;
#else
	    goto not_there;
#endif
	}
    case 'M':
	if (strEQ(name + 7, "META")) {	/* PGMOD_R removed */
#ifdef PGMOD_RMETA
	    return PGMOD_RMETA;
#else
	    goto not_there;
#endif
	}
    case 'S':
	if (strEQ(name + 7, "SHIFT")) {	/* PGMOD_R removed */
#ifdef PGMOD_RSHIFT
	    return PGMOD_RSHIFT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGMO(char *name, int len, int arg)
{
    if (4 + 2 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 2]) {
    case 'A':
	if (strEQ(name + 4, "D_ALT")) {	/* PGMO removed */
#ifdef PGMOD_ALT
	    return PGMOD_ALT;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (!strnEQ(name + 4,"D_", 2))
	    break;
	return constant_PGMOD_C(name, len, arg);
    case 'L':
	if (!strnEQ(name + 4,"D_", 2))
	    break;
	return constant_PGMOD_L(name, len, arg);
    case 'M':
	if (!strnEQ(name + 4,"D_", 2))
	    break;
	return constant_PGMOD_M(name, len, arg);
    case 'N':
	if (strEQ(name + 4, "D_NUM")) {	/* PGMO removed */
#ifdef PGMOD_NUM
	    return PGMOD_NUM;
#else
	    goto not_there;
#endif
	}
    case 'R':
	if (!strnEQ(name + 4,"D_", 2))
	    break;
	return constant_PGMOD_R(name, len, arg);
    case 'S':
	if (strEQ(name + 4, "D_SHIFT")) {	/* PGMO removed */
#ifdef PGMOD_SHIFT
	    return PGMOD_SHIFT;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGME(char *name, int len, int arg)
{
    if (4 + 10 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[4 + 10]) {
    case 'F':
	if (strEQ(name + 4, "MDAT_NEED_FREE")) {	/* PGME removed */
#ifdef PGMEMDAT_NEED_FREE
	    return PGMEMDAT_NEED_FREE;
#else
	    goto not_there;
#endif
	}
    case 'U':
	if (strEQ(name + 4, "MDAT_NEED_UNMAP")) {	/* PGME removed */
#ifdef PGMEMDAT_NEED_UNMAP
	    return PGMEMDAT_NEED_UNMAP;
#else
	    goto not_there;
#endif
	}
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_PGM(char *name, int len, int arg)
{
    switch (name[3 + 0]) {
    case 'E':
	return constant_PGME(name, len, arg);
    case 'O':
	return constant_PGMO(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant_P(char *name, int len, int arg)
{
    if (1 + 1 >= len ) {
	errno = EINVAL;
	return 0;
    }
    switch (name[1 + 1]) {
    case 'B':
	if (strEQ(name + 1, "GBIND_ANY")) {	/* P removed */
#ifdef PGBIND_ANY
	    return PGBIND_ANY;
#else
	    goto not_there;
#endif
	}
    case 'C':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PGC(name, len, arg);
    case 'D':
	if (strEQ(name + 1, "GDEFAULT")) {	/* P removed */
#ifdef PGDEFAULT
	    return PGDEFAULT;
#else
	    goto not_there;
#endif
	}
    case 'F':
	if (strEQ(name + 1, "GFONT_ANY")) {	/* P removed */
#ifdef PGFONT_ANY
	    return PGFONT_ANY;
#else
	    goto not_there;
#endif
	}
    case 'K':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PGK(name, len, arg);
    case 'M':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PGM(name, len, arg);
    case 'R':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PGR(name, len, arg);
    case 'T':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PGT(name, len, arg);
    case '_':
	if (!strnEQ(name + 1,"G", 1))
	    break;
	return constant_PG_(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static double
constant(char *name, int len, int arg)
{
    errno = 0;
    switch (name[0 + 0]) {
    case 'N':
	if (strEQ(name + 0, "NULL")) {	/*  removed */
#ifdef NULL
	    return (IV)NULL;
#else
	    goto not_there;
#endif
	}
    case 'P':
	return constant_P(name, len, arg);
    case '_':
	return constant__(name, len, arg);
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

/***************************************** Glue functions ******************/

MODULE = PicoGUI		PACKAGE = PicoGUI		

PROTOTYPES: ENABLE

double
constant(sv,arg)
    PREINIT:
	STRLEN		len;
    INPUT:
	SV *		sv
	char *		s = SvPV(sv, len);
	int		arg
    CODE:
	RETVAL = constant(s,len,arg);
    OUTPUT:
	RETVAL

void
_pgInit()
    CODE:
    	{
	   char *temp = NULL;
    	   pgInit(0,&temp);
	}

pghandle
pgRegisterApp(type,name)
        short int type
	const char *name
    CODE:
        RETVAL = pgRegisterApp(type,name,0);
    OUTPUT:
        RETVAL

void
pgGetEvent()
    PPCODE:
    	{
	   struct pgEvent *evt;
	   dXSTARG;
	   
	   evt = pgGetEvent();
	   XSprePUSH;
	   XPUSHs(newSVpvn("type",4));
	   XPUSHs(newSViv(evt->type));
	   XPUSHs(newSVpvn("from",4));
	   XPUSHs(newSViv(evt->from));
	   switch (evt->type & PG_EVENTCODINGMASK) {
	    case PG_EVENTCODING_XY:
	      XPUSHs(newSVpvn("w",1));
	      XPUSHs(newSViv(evt->e.size.w));
	      XPUSHs(newSVpvn("h",1));
	      XPUSHs(newSViv(evt->e.size.h));
	      break;
	    case PG_EVENTCODING_KBD:
	      XPUSHs(newSVpvn("mods",4));
	      XPUSHs(newSViv(evt->e.kbd.mods));
	      XPUSHs(newSVpvn("key",3));
	      XPUSHs(newSViv(evt->e.kbd.key));
	      break;
	    case PG_EVENTCODING_PNTR:
	      XPUSHs(newSVpvn("x",1));
	      XPUSHs(newSViv(evt->e.pntr.x));
	      XPUSHs(newSVpvn("y",1));
	      XPUSHs(newSViv(evt->e.pntr.y));
	      XPUSHs(newSVpvn("btn",3));
	      XPUSHs(newSViv(evt->e.pntr.btn));
	      XPUSHs(newSVpvn("chbtn",5));
	      XPUSHs(newSViv(evt->e.pntr.chbtn));
	      break;
	    case PG_EVENTCODING_DATA:
	      XPUSHs(newSVpvn("data",4));
	      XPUSHs(newSVpvn(evt->e.data.pointer,evt->e.data.size));
	      break;
	    default:
	      XPUSHs(newSVpvn("param",5));
	      XPUSHs(newSViv(evt->e.param));
	   }
	}
	      
int
pgMessageDialog(title,text,flags=0)
	const char *title
	const char *text
	unsigned long flags

const char *
pgErrortypeString(errortype)
        unsigned short errortype
	
void
pgFlushRequests()

void
pgUpdate()

void
pgSubUpdate(widget)
	pghandle widget
	
void
pgRegisterOwner(resource)
        int resource
	
void
pgUnregisterOwner(resource)
	int resource
	
void
pgSendKeyInput(type,key,mods)
	unsigned long type
	unsigned short key
	unsigned short mods
	
void
pgSendPointerInput(type,x,y,btn)
	unsigned long type
	unsigned short x
	unsigned short y
	unsigned short btn
	
void
pgSetVideoMode(xres=0,yres=0,bpp=0,flagmode=PG_FM_ON,flags=0)
	unsigned short xres
	unsigned short yres
	unsigned short bpp
	unsigned short flagmode
	unsigned long flags
	
void
pgDelete(object)
        pghandle object
	
void
pgFocus(widget)
        pghandle widget
	
pghandle
pgNewWidget(type,rship=0,parent=0)
        short int type
	short int rship
	pghandle parent

pghandle
pgNewPopup(width=0,height=0)
	int width
	int height
	
pghandle
pgNewPopupAt(x=0,y=0,width=0,height=0)
	int x
	int y
	int width
	int height
	
long
pgGetWidget(widget,property)
	pghandle widget
	short property
	
pghandle
pgNewString(str)
	const char *str
	
char *
pgGetString(string)
	pghandle string
	
void
pgReplaceText(widget,str)
        pghandle widget
	const char *str
	
pghandle
pgNewFont(name=0,size=0,style=0)
	const char *name
	short size
	unsigned long style
	
void
pgSetPayload(object,payload=0)
        pghandle object
	unsigned long payload
	
unsigned long
pgGetPayload(object)
	pghandle object
	
void
pgEnterContext()

void
pgLeaveContext()

int
pgMenuFromString(items)
	char *items
	
