#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcpclient.h"
#include "configfile.h"
#include "picomail.h"

#define BUFFERSIZE 4048

#define true 1
#define false 0

#define SUCCESS 0
#define FAILURE 1

#define IMAP_WELCOME "* OK"

/* Filepointer for the IMAP connection */
FILE *imapfp;

/* How many messages in the folder? */
int messages = 0;
int connectionLocked = false;

/* Counter for the command id... */
int cmd;
char cmdchar = 'A';

char *imapCurrentHeader;
int imapCurrentHeaderAlloc;


/* Reads a line from the fp. */
char *
getrow ()
{
  char row[BUFFERSIZE];
  char *newrow;

  fgets (row, BUFFERSIZE, imapfp);
  newrow = (char *) malloc (strlen (row) + 1);
  strcpy (newrow, row);

//  printf ("DEBUG RCV: %s", newrow);

  return newrow;
}




/* Generates the current cmdid string... */
void
getcmdid (char *cmdid)
{
  sprintf (cmdid, "%c%03d", cmdchar, cmd);
}




/* Increments the cmdid, sends a command to fp. */
void
sendcmd (char *command)
{
  char cmdid[5];

  if (++cmd >= 1000)
    {
      cmd = 0;
      cmdchar++;
    }

  getcmdid (cmdid);

  fprintf (imapfp, "%s %s\n", cmdid, command);
//  printf ("DEBUG SND: %s %s\n", cmdid, command);
}




/* Sends a command, get all the responses,
 * calls the process function...
 */
int
docmd (char *command, int (*fnt) (char *))
{
  char *buffer;
  char cmdid[5];

  sendcmd (command);
  getcmdid (cmdid);

  do
    {
      buffer = getrow ();
      (fnt) (buffer);
    }
  while (strncmp (buffer, cmdid, 4) != 0);

  return SUCCESS;
}




/* Just checks if the next line on the fp begins with "* OK"... */
int
check_ok ()
{
  char *buffer;

  buffer = getrow ();

  if (strncmp (buffer, IMAP_WELCOME, sizeof (IMAP_WELCOME)))
    return true;
  else
    return false;
}




/* This function looks if the line contains a count of
 * messages -> sets the messages var...
 */
int
mesgcount (char *line)
{
  int i = 0;

  if (strncmp (line + (strlen (line) - 8), "EXISTS", 5) == 0)
    {
      line += 2;
      while (((line[i] >= '0') && (line[i] <= '9')))
	i++;

      line[i] = '\0';
      messages = atoi (line);
      //printf ("Messages Count: %d\n", messages);
    }
  return SUCCESS;
}




/* This function does nothing ;) */
int
donothing (char *line)
{
  return SUCCESS;
}



/* Sends a command, get all the responses,
 * calls the process function...
 */
int
readheaders (char *command)
{
  char *buffer;
  char cmdid[5];

  int messageid = -1;

  char *sender;
  char *date;
  char *subject;

  sender = malloc (BUFFERSIZE);
  date = malloc (80);
  subject = malloc (BUFFERSIZE);

  sendcmd (command);
  getcmdid (cmdid);

  do
    {
      buffer = getrow ();
      if (strncmp (buffer, "* ", 2) == 0)
	{
	  if (messageid > 0)
	    addheader (messageid, sender, subject, date);
	  strcpy (sender, "");
	  strcpy (date, "");
	  strcpy (subject, "");
	  sscanf (buffer, "* %i FLAGS ", &messageid);
	}
      else
	{
	  if (strncmp (buffer, "From: ", 6) == 0)
	    {
	      strncpy (sender, buffer + 6, strlen (buffer) - 7);
	      sender[strlen (buffer) - 7] = '\0';
	    }
	  if (strncmp (buffer, "Subject: ", 9) == 0)
	    {
	      strncpy (subject, buffer + 9, strlen (buffer) - 10);
	      subject[strlen (buffer) - 10] = '\0';
	    }
	  if (strncmp (buffer, "Date: ", 6) == 0)
	    {
	      strncpy (date, buffer + 6, strlen (buffer) - 7);
	      date[strlen (buffer) - 7] = '\0';
	    }
	}

    }
  while (strncmp (buffer, cmdid, 4) != 0);

  free (sender);
  free (date);
  free (subject);

  return SUCCESS;
}



char *
readmessage (char *command)
{
  char *buffer;
  char *message;
  char cmdid[5];

  int allocated;
  int rowno = 0;

  sendcmd (command);
  getcmdid (cmdid);

  allocated = 4048;
  message = malloc (allocated);
  strcpy (message, "");

  while (strncmp ((buffer = getrow ()), cmdid, 4) != 0)
    {
      if ((rowno++ != 0))
	{
	  if (allocated < (strlen (buffer) + strlen (message) + 1))
	    {
	      allocated = strlen (buffer) + strlen (message) + 2048;
	      message = realloc (message, allocated);
	    }
	  strcat (message, buffer);
	}
    }

  message[strlen (message) - 4] = '\0';
  return message;
}




int
domessageheader (char *line)
{
  if ((strncmp (line, "From: ", 6) == 0) ||
      (strncmp (line, "To: ", 4) == 0) ||
      (strncmp (line, "Subject: ", 9) == 0) ||
      (strncmp (line, "Date: ", 6) == 0))
    {
      if (imapCurrentHeaderAlloc <
	  (strlen (line) + strlen (imapCurrentHeader) + 1))
	{
	  imapCurrentHeaderAlloc =
	    strlen (line) + strlen (imapCurrentHeader) + 1024;
	  imapCurrentHeader =
	    realloc (imapCurrentHeader, imapCurrentHeaderAlloc);
	}
      strcat (imapCurrentHeader, line);
    }
  return SUCCESS;
}





/* Login to the IMAP server... */
int
imap_login ()
{
  char *command;
  command = malloc (512);
  sprintf (command, "LOGIN %s %s",
	   get_param_str ("imap", "username", "guest"),
	   get_param_str ("imap", "password", "guest"));
  docmd (command, &donothing);

  return 0;
}




/* Conenct to the IMAP server.
 * Login.
 * Select folder, get message count.
 */
void
imap_init ()
{
  char *command;
  command = malloc (512);
  //printf ("Connection to IMAP...\n");
  imapfp = tcp_connect (get_param_str ("imap", "server", "127.0.0.1"),
			get_param_int ("imap", "port", 143));
  if (!check_ok ())
    exit (2);

  imap_login ();

  sprintf (command, "SELECT %s", get_param_str ("imap", "folder", "INBOX"));
  docmd (command, &mesgcount);
}




/* Check if we have a connection.
 * not -> make one ;)
 */
void
check_connection ()
{
  if (imapfp == NULL)
    imap_init ();
}




/* A function to get a list of messages... */
void
imap_getlist ()
{
  char *command;
  command = malloc (64);
  //printf ("imap_getlist\n");
  check_connection ();
  sprintf (command, "FETCH %d:%d (BODY[HEADER.FIELDS (SUBJECT DATE FROM)])",
	   1, messages);
  readheaders (command);
}




/* Get a message from server... */
char *
imap_getmesg (int mesg)
{
  char *header;
  char *text;
  char *message;
  char *messagebody;

  header = malloc (512);
  text = malloc (512);


  imapCurrentHeaderAlloc = 4048;
  imapCurrentHeader = malloc (imapCurrentHeaderAlloc);
  strcpy (imapCurrentHeader, "");

  sprintf (header, "FETCH %d RFC822.HEADER", mesg);
  sprintf (text, "FETCH %d RFC822.TEXT", mesg);

  check_connection ();
  docmd (header, &domessageheader);
  messagebody = readmessage (text);

  message = malloc (strlen (imapCurrentHeader) + strlen (messagebody) + 6);
  strcpy (message, imapCurrentHeader);
  strcat (message, "~~~\n\n");
  strcat (message, messagebody);

  strcpy (imapCurrentHeader, "");

  free (imapCurrentHeader);
  free (messagebody);

  return message;
}




/* Should I explain? */
void
imap_disconnect ()
{
  docmd ("LOGOUT", &mesgcount);
  fclose (imapfp);
}
