
typedef enum { PGL_STOREPREF, PGL_GETPREF, PGL_LOADPREFS, PGL_APPLETINSTALLED } pglMessageType;

typedef struct{
  pglMessageType messageType;
  unsigned short senderLen, keyLen, dataLen;
  char *data;
} pglMessage;

struct pgmemdata pglBuildMessage(pglMessageType type, char *senderName, char *key, char *data);

char *pglGetMessageData(pglMessage *message, unsigned short offset);
