/*  $Id$
 *
 *  edemo - a PicoGUI demo
 *
 *  Author: Daniele Pizzoni - Ascensit s.r.l. - Italy
 *  tsho@ascensit.com - auouo@tin.it
 *
 *  Based on picosm (by ?) and pgdemo.py by Micah Dowty
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <picogui.h>

#include <edemo.h>
#include <special.h>

/* ** ** ** ** ** */

/* hard coded filenames defs */
#define MAIN_TMPL "templates/edemo.wt"
#define PAGE_FMT  "templates/epage%02d.wt"
#define PAGE_LEN  26                 /* lenght of the PAGE_FMT string + 1 */

/* event handlers */
int evtPrev(struct pgEvent*);
int evtNext(struct pgEvent*);

/* functions */
void loadPage(int page, struct edemoUI*);

/* ** ** ** ** ** */

/* *  Event Handlers * */
int evtPrev(struct pgEvent *evt)
{
    struct edemoUI *interface = (struct edemoUI *)evt->extra;

    loadPage(interface->pCurr - 1, interface);
    return 1;
}

int evtNext(struct pgEvent *evt)
{
    struct edemoUI *interface = (struct edemoUI *)evt->extra;

    loadPage(interface->pCurr + 1, interface);
    return 1;
}

/* * UI functions * */
struct edemoUI *buildUI(void) {
    struct edemoUI *newUI = (struct edemoUI *)malloc(sizeof(struct edemoUI));

    /* register application */
    if(!(newUI->wApp = pgFindWidget("EDemo")))
        eerror("no EDemo widget in main template");
    
    /* navigation bar */
    if(!(newUI->wNavBar = pgFindWidget("EDNavBar")))
        eerror("no EDNavBar widget in main template");

    /* navbar label */
    if(!(newUI->wLabel = pgFindWidget("EDLabel")))
        eerror("no EDLabel widget in main template");
    
    /* 'previous' button */
    if(!(newUI->wPrev = pgFindWidget("EDPrev")))
        eerror("no EDPrev widget in main template");

    /* 'next' button */
    if(!(newUI->wNext = pgFindWidget("EDNext")))
        eerror("no EDNext widget in main template");

    /* page box */
    if(!(newUI->wBox = pgFindWidget("EDBox")))
        eerror("no EDBox widget in main template");

    return newUI;
}

void bindUI(struct edemoUI *interface) {
    pgBind(interface->wPrev, PG_WE_ACTIVATE, &evtPrev, interface);
    pgBind(interface->wNext, PG_WE_ACTIVATE, &evtNext, interface);
}

int main(int argc, char **argv){
    struct edemoUI *interface;
    char pageName[PAGE_LEN];
    FILE *existCheck;
    int pages;

    pgInit(argc, argv);

    if((existCheck = fopen(MAIN_TMPL, "r"))){
        fclose(existCheck);
        pgDup(pgLoadWidgetTemplate(pgFromFile(MAIN_TMPL)));
    } 
    else
	eerror("no main template file");

    interface = buildUI();
    bindUI(interface);

    /* initialization */
    interface->wCurrPage = 0;
    interface->pCurr = 0;
    interface->pageContext = 0;
    interface->mainContext = pgEnterContext();
    interface->themeContext = pgEnterContext();

    pgSetContext(interface->mainContext);

    /* check pages */
    pages = 1;
    while (1) {
	snprintf(pageName, PAGE_LEN, PAGE_FMT, pages);
	existCheck = fopen(pageName, "r");
	if (existCheck) {
	    fclose(existCheck);
	    pages++;
	}
	else
	    break;
    }
    interface->pTot  = pages - 1;

    /* no pages found */
    if (interface->pTot <= 0)
	eerror("no templates found");

    fprintf(stderr, "pages found: %d\n", interface->pTot);

    /* load first page */
    loadPage(1, interface);

    pgEventLoop();
  
    free(interface);
    return 0;
}

int eerror(char * string)
{
    fprintf(stderr, "error: %s\n", string);
    exit(255);
}

/* The file MUST exist */
void loadPage(int page, struct edemoUI *interface)
{
    char pageName[PAGE_LEN];

    snprintf(pageName, PAGE_LEN, PAGE_FMT, page);
    fprintf(stderr, "loading page: %s\n", pageName);

    idleDelete();

    /* a context for each page, no need to track things to delete :) */
    if (interface->pageContext)
	pgDeleteHandleContext(interface->pageContext);
    interface->pageContext = pgEnterContext();

    /* oh, well... */
    themebar_delete();
    pterm_delete();

/*     if (interface->wCurrPage) { */
/* 	fprintf(stderr, "deleting handle: %p:\n", interface->wCurrPage); */
/* 	pgDelete(interface->wCurrPage); */
/*     } */
    interface->wCurrPage = pgDup(pgLoadWidgetTemplate(pgFromFile(pageName)));

    pgAttachWidget(interface->wBox,
		   PG_DERIVE_INSIDE, 
		   interface->wCurrPage);

    pgReplaceTextFmt(interface->wLabel,
		     "page %d of %d", page, interface->pTot);

    interface->pCurr = page;

    /* enable/disable next/prev buttons */
    if (page == 1)
	pgSetWidget(interface->wPrev, PG_WP_DISABLED, 1, 0);
    else
	pgSetWidget(interface->wPrev, PG_WP_DISABLED, 0, 0);
    
    if (page == interface->pTot)
	pgSetWidget(interface->wNext, PG_WP_DISABLED, 1, 0);
    else
	pgSetWidget(interface->wNext, PG_WP_DISABLED, 0, 0);

    /* Check for special widgets and call corresponding action */
    parseSpecialWidgets(interface);
}
