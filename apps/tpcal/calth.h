#ifdef THEMESOURCE
#define PROPDECL(name,id) prop name=id;
#else
#define PROPDECL(name,id) /*empty*/
#endif

#define TARGET PGTH_P_USER

PROPDECL(target,TARGET)
