#define NUMBER 257
#define PROPERTY 258
#define THOBJ 259
#define STRING 260
#define UNKNOWNSYM 261
#define OBJ 262
typedef union {
  unsigned long num;
  unsigned short propid;
  unsigned short thobjid;
  struct {
    unsigned long loader;
    unsigned long propid;
    unsigned long data;
  } propval;
  struct propnode *prop;
  struct objectnode *obj;
} YYSTYPE;
extern YYSTYPE yylval;
