/* Maximum length of clock format strings */
#define FMTMAX 30

/* Maximum length of clock contents */
#define CLKMAX 50

/* The toolbar */
pghandle pglToolbar;

/* The toolbar's response variable */
char *pglToolbarResponse;

/* Clock settings */
struct clockData {
  /* The clock itself */
  pghandle wClock;

  /* Settings */
  pghandle fClockFont;
  unsigned int flashColon : 1;
  unsigned int enable24hour : 1;
  unsigned int enableSeconds : 1;
  unsigned int enableWeekDay : 1;
  unsigned int enableDay : 1;
  unsigned int enableMonth : 1;
  unsigned int enableYear : 1;

  /* Format strings- these are generated from the above settings
   * by the mungeSettings function
   */
  char fmt1[40];
  char fmt2[40];

} currentClock;

/* Functions */
void loadSettings(void);
void storeSettings(void);
int recieveMessage(struct pgEvent *evt);
int btnDialog(struct pgEvent *btnevt);
void mungeSettings(void);
void updateTime(void);
