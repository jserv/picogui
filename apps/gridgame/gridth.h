#ifdef THEMESOURCE
#define PROPDECL(name,id) prop name=id;
#else
#define PROPDECL(name,id) /*empty*/
#endif

#define SCALE 100

#define BGEVEN PGTH_P_USER
#define BGODD PGTH_P_USER+1
#define PIECE PGTH_P_USER+2
#define GRIDGAME_USER PGTH_P_USER+1000

PROPDECL(bgeven,BGEVEN)
PROPDECL(bgodd,BGODD)
PROPDECL(piece,PIECE)
