/*  $Id$
 *
 *  edemo - a PicoGUI demo
 *
 *  Author: Daniele Pizzoni - Ascensit s.r.l. - Italy
 *  tsho@ascensit.com - auouo@tin.it
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
 */

#include <picogui.h>
#include <edemo.h>
#include <special.h>

/* special data */
static pghandle wIndicatorBar;
static int iIndicatorValue;
#define INDSTEP 5

#define IDLEMAX 10
static pgidlehandler idle[IDLEMAX];
static int idleidx = 0;

/* special widget & event handlers */
int hThemeList1(struct edemoUI* interface, pghandle widget)
{
    themebar(widget, "fullthemes/", &interface->themeContext);
    return 0;
}


int hThemeList2(struct edemoUI* interface, pghandle widget)
{
    themebar(widget, "cursorthemes/", &interface->themeContext);
    return 0;
}

int hThemeList3(struct edemoUI* interface, pghandle widget)
{
    themebar(widget, "miscthemes/", &interface->themeContext);
    return 0;
}

int hThemeList4(struct edemoUI* interface, pghandle widget)
{
    themebar(widget, "bgthemes/", &interface->themeContext);
    return 0;
}

int evtThemeDefault(struct pgEvent *evt)
{
    struct edemoUI *interface = (struct edemoUI *)evt->extra;

    /* make it children of the main context!! */
    if (interface->themeContext != 0)
	pgDeleteHandleContext(interface->themeContext);

    pgSetContext(interface->mainContext);
    interface->themeContext = pgEnterContext();
    pgSetContext(interface->pageContext);

    themebar_clear();

    return 1;
}


int hThemeDefault(struct edemoUI* interface, pghandle widget)
{
    pgBind(widget, PG_WE_ACTIVATE, evtThemeDefault, (void *)interface);
    return 0;
}


int hTerminal(struct edemoUI* interface, pghandle widget)
{
    pterm(widget);
    return 0;
}

int hIndicatorBar(struct edemoUI* interface, pghandle widget)
{
    wIndicatorBar = widget;
    iIndicatorValue = 30;
    pgSetWidget(widget, PG_WP_VALUE, iIndicatorValue, 0);
    return 0;
}

int evtIndicatorUp(struct pgEvent *evt)
{
    iIndicatorValue += INDSTEP;
    if (iIndicatorValue > 100)
	iIndicatorValue = 100;

    pgSetWidget((pghandle)evt->extra, PG_WP_VALUE, iIndicatorValue, 0);
    return 1;
}

int hIndicatorUp(struct edemoUI* interface, pghandle widget)
{
    pgBind(widget, PG_WE_ACTIVATE, &evtIndicatorUp, (void *)wIndicatorBar);
    return 0;
}

int evtIndicatorDown(struct pgEvent *evt)
{
    iIndicatorValue -= INDSTEP;
    if (iIndicatorValue < 0)
	iIndicatorValue = 0;

    pgSetWidget((pghandle)evt->extra, PG_WP_VALUE, iIndicatorValue, 0);
    return 1;
}

int hIndicatorDown(struct edemoUI* interface, pghandle widget)
{
    pgBind(widget, PG_WE_ACTIVATE, &evtIndicatorDown, (void *)wIndicatorBar);
    return 0;
}

int hCanvas1(struct edemoUI* interface, pghandle widget)
{
    canvastst(widget);
    return 0;
}

int hCanvas2(struct edemoUI* interface, pghandle widget)
{
    chaos_fire(widget);
    return 0;
}

int hDialogs(struct edemoUI* interface, pghandle widget)
{
    dialogdemo(widget);
    return 0;
}

int hMenu(struct edemoUI* interface, pghandle widget)
{
    menutest(widget);
    return 0;
}

int hAppBar(struct edemoUI* interface, pghandle widget)
{
    appbar(widget, "apps/");
    printf("appbar!\n");
    return 0;
}

int evtAppBarKillall(struct pgEvent *evt)
{
    appbar_killall();
    return 1;
}


int hAppBarKillall(struct edemoUI* interface, pghandle widget)
{
    pgBind(widget, PG_WE_ACTIVATE, evtAppBarKillall, NULL);
    return 0;
}


/* list of known special widgets */
struct edemoHandle handles[] = {
     [0] = {"EDHThemeList1",    hThemeList1},
     [1] = {"EDHThemeList2",    hThemeList2},
     [2] = {"EDHThemeList3",    hThemeList3},
     [3] = {"EDHThemeList4",    hThemeList4},
     [4] = {"EDHThemeDefault",  hThemeDefault},
     [5] = {"EDHTerminal",      hTerminal},
     [6] = {"EDHIndicatorBar",  hIndicatorBar},
     [7] = {"EDHIndicatorUp",   hIndicatorUp},
     [8] = {"EDHIndicatorDown", hIndicatorDown},
     [9] = {"EDHCanvas1",       hCanvas1},
    [10] = {"EDHCanvas2",      hCanvas2},
    [11] = {"EDHDialogs",      hDialogs},
    [12] = {"EDHMenu",         hMenu},
    [13] = {"EDHAppbar",       hAppBar},
    [14] = {"EDHAppbarKillall",hAppBarKillall},
    [15] = {NULL, NULL}  /* last one {NULL, NULL} */
};


/* parse the widget tree for special widgets */
void parseSpecialWidgets(struct edemoUI* interface)
{
    int i = 0;
    pghandle widget;

    while (handles[i].sName) {
/* 	printf("index: %d, name: %s, action: %p\n", i, handles[i].sName, handles[i].action); */
/* 	printf("found widget: %s\n", handles[i].sName); */
	if ((widget = pgFindWidget(handles[i].sName)) && (handles[i].action != NULL))
	    handles[i].action(interface, widget);
	i++;
    }
}

static void idleHandler(void)
{
    int i = idleidx;

    while (--i > -1)
	(*(idle[i]))();
}

void idleHandlerAdd (pgidlehandler func)
{
    if (idleidx == IDLEMAX)
	return;

    idle[idleidx++] = func;

    if (idleidx == 1)
	pgSetIdle(50, idleHandler);
}


void idleDelete(void)
{
    idleidx = 0;
    pgSetIdle(0, NULL);
}
