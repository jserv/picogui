/* $Id$
 * 
 * tuxphone.h - Defines the socket interface to control the phone remotely
 *              using tuxphone.
 * 
 */

#ifndef __H_TUXPHONE
#define __H_TUXPHONE

/* ---- Incoming client requests ---- */

#define OPCODE_REGISTER_EVENTS   0x00

typedef struct {
  short opcode;
  short mode;
  long events;
} PKT_REGISTER;

#define MODE_REGISTER_EVENT 0
#define MODE_UNREGISTER_EVENT 1

#define OPCODE_DIAL              0x01

typedef struct {
  short opcode;
  char digits[32];
} PKT_DIAL;

typedef union {
  
  struct {
    short opcode;
  } header;

  PKT_REGISTER reg;
  PKT_DIAL dial;

} CLIENT_PACKET;

/* ---- Outgoing event packets ---- */

#define OPCODE_CLIENT_EVENT   0x02

typedef struct {
  short opcode;
  short type;
  short ring_state;
} PKT_RING_EVENT;

#define RING_OFF 0
#define RING_ON  1

typedef struct {
  short opcode;
  short type;

  char name[17];
  char number[17];
  char month[2];
  char day[2];
  char hour[2];
  char minute[2];

  char qualifier;
} PKT_CID_EVENT;
  
typedef struct {
  short opcode;
  short type;

  short state;
  short destination;
} PKT_HOOK_EVENT;

#define HOOK_OFF 0
#define HOOK_ON  1

#define HOOK_HANDSET 0
#define HOOK_SPEAKER 1

typedef struct {
  short opcode;
  short type;

  short state;
  char key; 
} PKT_ONHOOK_DIAL_EVENT;

typedef struct {
  short opcode;
  short type;

  short state;
  char key; 
} PKT_DIAL_EVENT;

#define KEY_RELEASE 0
#define KEY_PRESS   1

typedef union {
  
  struct {
    short opcode;
    short type;
  } header;

  PKT_RING_EVENT ring;
  PKT_CID_EVENT cid;
  PKT_HOOK_EVENT hook;
  PKT_ONHOOK_DIAL_EVENT ohdial;
  PKT_DIAL_EVENT dial;
} PKT_EVENT;

/* ---- Outgoing client events ---- */

#define RING_EVENT        0
#define CID_EVENT         1
#define HOOK_EVENT        2
#define ONHOOK_DIAL_EVENT 3
#define DIAL_EVENT        4

#define MAX_EVENTS   5

/* ---- Masks used to select events for individual clients ---- */

#define RING_EVENT_MASK        (1 << RING_EVENT)
#define CID_EVENT_MASK         (1 << CID_EVENT)
#define HOOK_EVENT_MASK        (1 << HOOK_EVENT)
#define ONHOOK_DIAL_EVENT_MASK (1 << ONHOOK_DIAL_EVENT)

#define ALL_EVENT_MASK  (~0L)

#define MAX_CLIENTS 24

#define TUXPHONE_PORT 2630   /* Named after the IS2630 */

#endif /* __H_TUXPHONE */
