#ifndef RFB_H
#define RFB_H

#define HAVE_PTHREADS

/*
 * rfb.h - header file for RFB DDX implementation.
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#if(defined __cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "keysym.h"

/* TODO: this stuff has to go into autoconf */
typedef unsigned char CARD8;
typedef unsigned short CARD16;
typedef unsigned int CARD32;
typedef CARD32 Pixel;
/* typedef CARD32 KeySym; */
typedef unsigned long KeySym;
#define SIGNED signed
/* for some strange reason, "typedef signed char Bool;" yields a four byte
   signed int on IRIX, but only for rfbserver.o!!! */
#ifdef Bool
#undef Bool
#endif
#define Bool signed char
#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE -1

#include "rfbproto.h"

#ifdef __linux__
#include <endian.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <machine/endian.h>
#define _BYTE_ORDER BYTE_ORDER
#define _LITTLE_ENDIAN LITTLE_ENDIAN
#elif defined (__SVR4) && defined (__sun) /* Solaris */
#include <sys/types.h>
#if defined(__sparc)
  /* SPARC here (big endian) */
#define _BYTE_ORDER 4321
#elif defined(__i386)
#define _BYTE_ORDER 1234
#else
#error Solaris 2.5.1 had ppc support did it not? :-)
#endif
#undef Bool
#define Bool char
#undef SIGNED
#define SIGNED
#include <sys/types.h>
/* typedef unsigned int pthread_t; */
#elif defined(WIN32)
#define _LITTLE_ENDIAN 1234
#define _BYTE_ORDER _LITTLE_ENDIAN
#undef Bool
#define Bool int
#else
#include <sys/endian.h>
#endif

#ifndef _BYTE_ORDER
#define _BYTE_ORDER __BYTE_ORDER
#endif

#if !defined(_LITTLE_ENDIAN) && defined(__LITTLE_ENDIAN)
#define _LITTLE_ENDIAN __LITTLE_ENDIAN
#endif

#ifdef WIN32
#include <sys/timeb.h>
#include <winsock.h>
#undef SOCKET
#define SOCKET int
#else
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#include <sys/time.h>
#include <netinet/in.h>
#define SOCKET int
#endif

#ifndef INADDR_NONE
#define                INADDR_NONE     ((in_addr_t) 0xffffffff)
#endif

#ifdef HAVE_PTHREADS
#include <pthread.h>
#if 0 /* debugging */
#define LOCK(mutex) fprintf(stderr,"%s:%d LOCK(%s,0x%x)\n",__FILE__,__LINE__,#mutex,&(mutex))
#define UNLOCK(mutex) fprintf(stderr,"%s:%d UNLOCK(%s,0x%x)\n",__FILE__,__LINE__,#mutex,&(mutex))
#define MUTEX(mutex) int mutex
#define INIT_MUTEX(mutex) fprintf(stderr,"%s:%d INIT_MUTEX(%s,0x%x)\n",__FILE__,__LINE__,#mutex,&(mutex))
#define TINI_MUTEX(mutex) fprintf(stderr,"%s:%d TINI_MUTEX(%s)\n",__FILE__,__LINE__,#mutex)
#define SIGNAL(cond) fprintf(stderr,"%s:%d SIGNAL(%s)\n",__FILE__,__LINE__,#cond)
#define WAIT(cond,mutex) /* fprintf(stderr,"%s:%d WAIT(%s,%s)\n",__FILE__,__LINE__,#cond,#mutex) */
#define COND(cond)
#define INIT_COND(cond) fprintf(stderr,"%s:%d INIT_COND(%s)\n",__FILE__,__LINE__,#cond)
#define TINI_COND(cond) fprintf(stderr,"%s:%d TINI_COND(%s)\n",__FILE__,__LINE__,#cond)
#define IF_PTHREADS(x)
#else
#define LOCK(mutex) pthread_mutex_lock(&(mutex));
#define UNLOCK(mutex) pthread_mutex_unlock(&(mutex));
#define MUTEX(mutex) pthread_mutex_t (mutex)
#define INIT_MUTEX(mutex) pthread_mutex_init(&(mutex),NULL)
#define TINI_MUTEX(mutex) pthread_mutex_destroy(&(mutex))
#define TSIGNAL(cond) pthread_cond_signal(&(cond))
#define WAIT(cond,mutex) pthread_cond_wait(&(cond),&(mutex))
#define COND(cond) pthread_cond_t (cond)
#define INIT_COND(cond) pthread_cond_init(&(cond),NULL)
#define TINI_COND(cond) pthread_cond_destroy(&(cond))
#define IF_PTHREADS(x) x
#endif
#else
#define LOCK(mutex)
#define UNLOCK(mutex)
#define MUTEX(mutex)
#define INIT_MUTEX(mutex)
#define TINI_MUTEX(mutex)
#define TSIGNAL(cond)
#define WAIT(cond,mutex) this_is_unsupported
#define COND(cond)
#define INIT_COND(cond)
#define TINI_COND(cond)
#define IF_PTHREADS(x)
#endif

/* end of stuff for autoconf */

/* if you use pthreads, but don't define HAVE_PTHREADS, the structs
   get all mixed up. So this gives a linker error reminding you to compile
   the library and your application (at least the parts including rfb.h)
   with the same support for pthreads. */
#ifdef HAVE_PTHREADS
#define rfbInitServer rfbInitServerWithPthreads
#else
#define rfbInitServer rfbInitServerWithoutPthreads
#endif

#define MAX_ENCODINGS 10

struct _rfbClientRec;
struct _rfbScreenInfo;

enum rfbNewClientAction {
	RFB_CLIENT_ACCEPT,
	RFB_CLIENT_ON_HOLD,
	RFB_CLIENT_REFUSE
};

typedef void (*KbdAddEventProcPtr) (Bool down, KeySym keySym, struct _rfbClientRec* cl);
typedef void (*KbdReleaseAllKeysProcPtr) (struct _rfbClientRec* cl);
typedef void (*PtrAddEventProcPtr) (int buttonMask, int x, int y, struct _rfbClientRec* cl);
typedef void (*SetXCutTextProcPtr) (char* str,int len, struct _rfbClientRec* cl);
typedef Bool (*SetTranslateFunctionProcPtr)(struct _rfbClientRec* cl);
typedef Bool (*PasswordCheckProcPtr)(struct _rfbClientRec* cl,const char* encryptedPassWord,int len);
typedef enum rfbNewClientAction (*NewClientHookPtr)(struct _rfbClientRec* cl);
typedef void (*DisplayHookPtr)(struct _rfbClientRec* cl);

typedef struct {
  CARD32 count;
  Bool is16; /* is the data format short? */
  union {
    CARD8* bytes;
    CARD16* shorts;
  } data; /* there have to be count*3 entries */
} rfbColourMap;

/*
 * Per-screen (framebuffer) structure.  There can be as many as you wish,
 * each serving different clients. However, you have to call
 * rfbProcessEvents for each of these.
 */

typedef struct _rfbScreenInfo
{
    int width;
    int paddedWidthInBytes;
    int height;
    int depth;
    int bitsPerPixel;
    int sizeInBytes;

    Pixel blackPixel;
    Pixel whitePixel;

    /* some screen specific data can be put into a struct where screenData
     * points to. You need this if you have more than one screen at the
     * same time while using the same functions.
     */
    void* screenData;
     
    /* additions by libvncserver */

    rfbPixelFormat rfbServerFormat;
    rfbColourMap colourMap; /* set this if rfbServerFormat.trueColour==FALSE */
    const char* desktopName;
    char rfbThisHost[255];

    Bool autoPort;
    int rfbPort;
    SOCKET rfbListenSock;
    int maxSock;
    int maxFd;
    fd_set allFds;

    Bool socketInitDone;
    SOCKET inetdSock;
    Bool inetdInitDone;

    int udpPort;
    SOCKET udpSock;
    struct _rfbClientRec* udpClient;
    Bool udpSockConnected;
    struct sockaddr_in udpRemoteAddr;

    int rfbMaxClientWait;

    PasswordCheckProcPtr passwordCheck;
    void* rfbAuthPasswdData;

    /* this is the amount of milliseconds to wait at least before sending
     * an update. */
    int rfbDeferUpdateTime;
    char* rfbScreen;
    Bool rfbAlwaysShared;
    Bool rfbNeverShared;
    Bool rfbDontDisconnect;
    struct _rfbClientRec* rfbClientHead;

    /* the frameBufferhas to be supplied by the serving process.
     * The buffer will not be freed by 
     */
    char* frameBuffer;
    KbdAddEventProcPtr kbdAddEvent;
    KbdReleaseAllKeysProcPtr kbdReleaseAllKeys;
    PtrAddEventProcPtr ptrAddEvent;
    SetXCutTextProcPtr setXCutText;
    SetTranslateFunctionProcPtr setTranslateFunction;
  
    /* newClientHook is called just after a new client is created */
    NewClientHookPtr newClientHook;
    /* displayHook is called just before a frame buffer update */
    DisplayHookPtr displayHook;

#ifdef HAVE_PTHREADS
    Bool backgroundLoop;
    pthread_t listener_thread;
#endif

} rfbScreenInfo, *rfbScreenInfoPtr;


/*
 * rfbTranslateFnType is the type of translation functions.
 */

typedef void (*rfbTranslateFnType)(char *table, rfbPixelFormat *in,
                                   rfbPixelFormat *out,
                                   char *iptr, char *optr,
                                   int bytesBetweenInputLines,
                                   int width, int height);


/* 
 * vncauth.h - describes the functions provided by the vncauth library.
 */

#define MAXPWLEN 8
#define CHALLENGESIZE 16

extern int vncEncryptAndStorePasswd(char *passwd, char *fname);
extern char *vncDecryptPasswdFromFile(char *fname);
extern void vncRandomBytes(unsigned char *bytes);
extern void vncEncryptBytes(unsigned char *bytes, char *passwd);

/* region stuff */

struct sraRegion;
typedef struct sraRegion* sraRegionPtr;

/*
 * Per-client structure.
 */

typedef void (*ClientGoneHookPtr)(struct _rfbClientRec* cl);

typedef struct _rfbClientRec {
  
    /* back pointer to the screen */
    rfbScreenInfoPtr screen;
  
    /* private data. You should put any application client specific data
     * into a struct and let clientData point to it. Don't forget to
     * free the struct via clientGoneHook!
     *
     * This is useful if the IO functions have to behave client specific.
     */
    void* clientData;
    ClientGoneHookPtr clientGoneHook;

    SOCKET sock;
    char *host;

#ifdef HAVE_PTHREADS
    pthread_t client_thread;
#endif
                                /* Possible client states: */
    enum {
        RFB_PROTOCOL_VERSION,   /* establishing protocol version */
        RFB_AUTHENTICATION,     /* authenticating */
        RFB_INITIALISATION,     /* sending initialisation messages */
        RFB_NORMAL              /* normal protocol messages */
    } state;

    Bool reverseConnection;
    Bool onHold;
    Bool readyForSetColourMapEntries;
    Bool useCopyRect;
    int preferredEncoding;
    int correMaxWidth, correMaxHeight;

    /* The following member is only used during VNC authentication */
    CARD8 authChallenge[CHALLENGESIZE];

    /* The following members represent the update needed to get the client's
       framebuffer from its present state to the current state of our
       framebuffer.

       If the client does not accept CopyRect encoding then the update is
       simply represented as the region of the screen which has been modified
       (modifiedRegion).

       If the client does accept CopyRect encoding, then the update consists of
       two parts.  First we have a single copy from one region of the screen to
       another (the destination of the copy is copyRegion), and second we have
       the region of the screen which has been modified in some other way
       (modifiedRegion).

       Although the copy is of a single region, this region may have many
       rectangles.  When sending an update, the copyRegion is always sent
       before the modifiedRegion.  This is because the modifiedRegion may
       overlap parts of the screen which are in the source of the copy.

       In fact during normal processing, the modifiedRegion may even overlap
       the destination copyRegion.  Just before an update is sent we remove
       from the copyRegion anything in the modifiedRegion. */

    sraRegionPtr copyRegion;	/* the destination region of the copy */
    int copyDX, copyDY;		/* the translation by which the copy happens */

    sraRegionPtr modifiedRegion;

    /* As part of the FramebufferUpdateRequest, a client can express interest
       in a subrectangle of the whole framebuffer.  This is stored in the
       requestedRegion member.  In the normal case this is the whole
       framebuffer if the client is ready, empty if it's not. */

    sraRegionPtr requestedRegion;

    /* The following member represents the state of the "deferred update" timer
       - when the framebuffer is modified and the client is ready, in most
       cases it is more efficient to defer sending the update by a few
       milliseconds so that several changes to the framebuffer can be combined
       into a single update. */

      struct timeval startDeferring;

    /* translateFn points to the translation function which is used to copy
       and translate a rectangle from the framebuffer to an output buffer. */

    rfbTranslateFnType translateFn;
    char *translateLookupTable;
    rfbPixelFormat format;

    /*
     * UPDATE_BUF_SIZE must be big enough to send at least one whole line of the
     * framebuffer.  So for a max screen width of say 2K with 32-bit pixels this
     * means 8K minimum.
     */

#define UPDATE_BUF_SIZE 30000

    char updateBuf[UPDATE_BUF_SIZE];
    int ublen;

    /* statistics */

    int rfbBytesSent[MAX_ENCODINGS];
    int rfbRectanglesSent[MAX_ENCODINGS];
    int rfbLastRectMarkersSent;
    int rfbLastRectBytesSent;
    int rfbFramebufferUpdateMessagesSent;
    int rfbRawBytesEquivalent;
    int rfbKeyEventsRcvd;
    int rfbPointerEventsRcvd;

    /* zlib encoding -- necessary compression state info per client */

    struct z_stream_s compStream;
    Bool compStreamInited;
    CARD32 zlibCompressLevel;

    /* tight encoding -- preserve zlib streams' state for each client */

    z_stream zsStruct[4];
    Bool zsActive[4];
    int zsLevel[4];
    int tightCompressLevel;
    int tightQualityLevel;

    Bool enableLastRectEncoding;   /* client supports LastRect encoding */

    Bool useNewFBSize;             /* client supports NewFBSize encoding */
    Bool newFBSizePending;         /* framebuffer size was changed */

#ifdef BACKCHANNEL
    Bool enableBackChannel;        /* custom channel for special clients */
#endif

    struct _rfbClientRec *prev;
    struct _rfbClientRec *next;

#ifdef HAVE_PTHREADS
    /* whenever a client is referenced, the refCount has to be incremented
       and afterwards decremented, so that the client is not cleaned up
       while being referenced.
       Use the functions rfbIncrClientRef(cl) and rfbDecrClientRef(cl);
    */
    int refCount;
    MUTEX(refCountMutex);
    COND(deleteCond);

    MUTEX(outputMutex);
    MUTEX(updateMutex);
    COND(updateCond);
#endif

} rfbClientRec, *rfbClientPtr;

/*
 * This macro is used to test whether there is a framebuffer update needing to
 * be sent to the client.
 */

#define FB_UPDATE_PENDING(cl)                                              \
     (((cl)->useNewFBSize && (cl)->newFBSizePending) ||                     \
     !sraRgnEmpty((cl)->copyRegion) || !sraRgnEmpty((cl)->modifiedRegion))

/*
 * Macros for endian swapping.
 */

#define Swap16(s) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff))

#define Swap24(l) ((((l) & 0xff) << 16) | (((l) >> 16) & 0xff) | \
                   (((l) & 0x00ff00)))

#define Swap32(l) (((l) >> 24) | \
                   (((l) & 0x00ff0000) >> 8)  | \
                   (((l) & 0x0000ff00) << 8)  | \
                   ((l) << 24))


extern char rfbEndianTest;

#define Swap16IfLE(s) (rfbEndianTest ? Swap16(s) : (s))
#define Swap24IfLE(l) (rfbEndianTest ? Swap24(l) : (l))
#define Swap32IfLE(l) (rfbEndianTest ? Swap32(l) : (l))

/* sockets.c */

extern int rfbMaxClientWait;

extern void rfbInitSockets(rfbScreenInfoPtr rfbScreen);
extern void rfbDisconnectUDPSock(rfbScreenInfoPtr rfbScreen);
extern void rfbCloseClient(rfbClientPtr cl);
extern int ReadExact(rfbClientPtr cl, char *buf, int len);
extern int ReadExactTimeout(rfbClientPtr cl, char *buf, int len,int timeout);
extern int WriteExact(rfbClientPtr cl, const char *buf, int len);
extern void rfbCheckFds(rfbScreenInfoPtr rfbScreen,long usec);
extern int rfbConnect(rfbScreenInfoPtr rfbScreen, char* host, int port);
extern int ConnectToTcpAddr(char* host, int port);
extern int ListenOnTCPPort(int port);
extern int ListenOnUDPPort(int port);

/* rfbserver.c */

extern rfbClientPtr pointerClient;


/* Routines to iterate over the client list in a thread-safe way.
   Only a single iterator can be in use at a time process-wide. */
typedef struct rfbClientIterator *rfbClientIteratorPtr;

extern void rfbClientListInit(rfbScreenInfoPtr rfbScreen);
extern rfbClientIteratorPtr rfbGetClientIterator(rfbScreenInfoPtr rfbScreen);
extern rfbClientPtr rfbClientIteratorNext(rfbClientIteratorPtr iterator);
extern void rfbReleaseClientIterator(rfbClientIteratorPtr iterator);

extern void rfbNewClientConnection(rfbScreenInfoPtr rfbScreen,int sock);
extern rfbClientPtr rfbNewClient(rfbScreenInfoPtr rfbScreen,int sock);
extern rfbClientPtr rfbNewUDPClient(rfbScreenInfoPtr rfbScreen);
extern rfbClientPtr rfbReverseConnection(rfbScreenInfoPtr rfbScreen,char *host, int port);
extern void rfbClientConnectionGone(rfbClientPtr cl);
extern void rfbProcessClientMessage(rfbClientPtr cl);
extern void rfbClientConnFailed(rfbClientPtr cl, char *reason);
extern void rfbNewUDPConnection(rfbScreenInfoPtr rfbScreen,int sock);
extern void rfbProcessUDPInput(rfbScreenInfoPtr rfbScreen);
extern Bool rfbSendFramebufferUpdate(rfbClientPtr cl, sraRegionPtr updateRegion);
extern Bool rfbSendRectEncodingRaw(rfbClientPtr cl, int x,int y,int w,int h);
extern Bool rfbSendUpdateBuf(rfbClientPtr cl);
extern void rfbSendServerCutText(rfbScreenInfoPtr rfbScreen,char *str, int len);
extern Bool rfbSendCopyRegion(rfbClientPtr cl,sraRegionPtr reg,int dx,int dy);
extern Bool rfbSendLastRectMarker(rfbClientPtr cl);
extern Bool rfbSendNewFBSize(rfbClientPtr cl, int w, int h);
extern Bool rfbSendSetColourMapEntries(rfbClientPtr cl, int firstColour, int nColours);
extern void rfbSendBell(rfbScreenInfoPtr rfbScreen);

void rfbGotXCutText(rfbScreenInfoPtr rfbScreen, char *str, int len);

#ifdef BACKCHANNEL
extern void rfbSendBackChannel(rfbScreenInfoPtr s,char* message,int len);
#endif

/* translate.c */

extern Bool rfbEconomicTranslate;

extern void rfbTranslateNone(char *table, rfbPixelFormat *in,
                             rfbPixelFormat *out,
                             char *iptr, char *optr,
                             int bytesBetweenInputLines,
                             int width, int height);
extern Bool rfbSetTranslateFunction(rfbClientPtr cl);
extern Bool rfbSetClientColourMap(rfbClientPtr cl, int firstColour, int nColours);
extern void rfbSetClientColourMaps(rfbScreenInfoPtr rfbScreen, int firstColour, int nColours);

/* auth.c */

extern void rfbAuthNewClient(rfbClientPtr cl);
extern void rfbAuthProcessClientMessage(rfbClientPtr cl);


/* rre.c */

extern Bool rfbSendRectEncodingRRE(rfbClientPtr cl, int x,int y,int w,int h);


/* corre.c */

extern Bool rfbSendRectEncodingCoRRE(rfbClientPtr cl, int x,int y,int w,int h);


/* hextile.c */

extern Bool rfbSendRectEncodingHextile(rfbClientPtr cl, int x, int y, int w,
                                       int h);


/* zlib.c */

/* Minimum zlib rectangle size in bytes.  Anything smaller will
 * not compress well due to overhead.
 */
#define VNC_ENCODE_ZLIB_MIN_COMP_SIZE (17)

/* Set maximum zlib rectangle size in pixels.  Always allow at least
 * two scan lines.
 */
#define ZLIB_MAX_RECT_SIZE (128*256)
#define ZLIB_MAX_SIZE(min) ((( min * 2 ) > ZLIB_MAX_RECT_SIZE ) ? \
			    ( min * 2 ) : ZLIB_MAX_RECT_SIZE )

extern Bool rfbSendRectEncodingZlib(rfbClientPtr cl, int x, int y, int w,
				    int h);


/* tight.c */

#define TIGHT_DEFAULT_COMPRESSION  6

extern Bool rfbTightDisableGradient;

extern int rfbNumCodedRectsTight(rfbClientPtr cl, int x,int y,int w,int h);
extern Bool rfbSendRectEncodingTight(rfbClientPtr cl, int x,int y,int w,int h);

/* cursor handling for the pointer */
extern void defaultPtrAddEvent(int buttonMask,int x,int y,rfbClientPtr cl);

/* stats.c */

extern void rfbResetStats(rfbClientPtr cl);
extern void rfbPrintStats(rfbClientPtr cl);

/* main.c */

extern void rfbLogEnable(int enabled);
extern void rfbLog(const char *format, ...);
extern void rfbLogPerror(const char *str);

void rfbScheduleCopyRect(rfbScreenInfoPtr rfbScreen,int x1,int y1,int x2,int y2,int dx,int dy);
void rfbScheduleCopyRegion(rfbScreenInfoPtr rfbScreen,sraRegionPtr copyRegion,int dx,int dy);

void rfbDoCopyRect(rfbScreenInfoPtr rfbScreen,int x1,int y1,int x2,int y2,int dx,int dy);
void rfbDoCopyRegion(rfbScreenInfoPtr rfbScreen,sraRegionPtr copyRegion,int dx,int dy);

void rfbMarkRectAsModified(rfbScreenInfoPtr rfbScreen,int x1,int y1,int x2,int y2);
void rfbMarkRegionAsModified(rfbScreenInfoPtr rfbScreen,sraRegionPtr modRegion);
void doNothingWithClient(rfbClientPtr cl);
enum rfbNewClientAction defaultNewClientHook(rfbClientPtr cl);

/* to check against plain passwords */
Bool rfbCheckPasswordByList(rfbClientPtr cl,const char* response,int len);

/* functions to make a vnc server */
extern rfbScreenInfoPtr rfbGetScreen(int width,int height,int bitsPerSample,int samplesPerPixel,
				     int bytesPerPixel);
extern void rfbInitServer(rfbScreenInfoPtr rfbScreen);
extern void rfbNewFramebuffer(rfbScreenInfoPtr rfbScreen,char *framebuffer,
 int width,int height, int bitsPerSample,int samplesPerPixel,
 int bytesPerPixel);

extern void rfbScreenCleanup(rfbScreenInfoPtr screenInfo);

/* functions to accept/refuse a client that has been put on hold
   by a NewClientHookPtr function. Must not be called in other
   situations. */
extern void rfbStartOnHoldClient(rfbClientPtr cl);
extern void rfbRefuseOnHoldClient(rfbClientPtr cl);

/* call one of these two functions to service the vnc clients.
 usec are the microseconds the select on the fds waits.
 if you are using the event loop, set this to some value > 0, so the
 server doesn't get a high load just by listening. */

extern void rfbRunEventLoop(rfbScreenInfoPtr screenInfo, long usec, Bool runInBackground);
extern void rfbProcessEvents(rfbScreenInfoPtr screenInfo,long usec);

#endif

#if(defined __cplusplus)
}
#endif
