#include <picogui.h>

#include "gridth.h"

#define MAXPLAYERS 4

struct gridgame {
  const char *name, *theme;
  const char * const *themes;
  int width, height;	/* currently fixed; some games may implement resizing */
  int players;
  void (*init)(void);
  void (*drag)(pgu x1, pgu y1, pgu x2, pgu y2);
  void (*cleanup)(void);
};

typedef struct squarestatus_struct {
  int player, bricktype, selected;
} squarestatus;

typedef struct gridpos_struct {
  pgu x, y;
} gridpos;

/* Determines if a coordinate is on the board */
int ggisvalid(gridpos square);
/* Determines if a given square is selected, for multisquare select games */
int ggisselected(gridpos square);
/* Selects a square */
void ggselect(gridpos square);
/* Deselects a square */
void ggdeselect(gridpos square);
/* Moves a brick of type newstatus */
void ggmove(gridpos from, gridpos to, squarestatus newstatus);
/* Sets a square */
void ggset(gridpos pos, squarestatus status);
/* Finds the last selected square. None if a square has been deselected since */
gridpos findselected(void);
/* Gets the status of a square (player and brick type) */
squarestatus gggetstatus(gridpos square);
/* Displays a status string */
void ggstatusline(const char *msg);
