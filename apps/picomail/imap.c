#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcpclient.h"
#include "configfile.h"

#define BUFFERSIZE 4048

#define true 1
#define false 0

#define SUCCESS 0
#define FAILURE 1

#define IMAP_WELCOME "* OK"
//#define LIST_FLAGS (flags body[header.fields (date from subject)])
//#define LIST_FLAGS FULL

/* Filepointer for the IMAP connection */
FILE *fp;

/* How many messages in the folder? */
int messages = 0;

/* Counter for the command id... */
int cmd;
char cmdchar = 'A';




/* Reads a line from the fp. */
char *
getrow( )
{
    char row[BUFFERSIZE];
    char *newrow;
    
    fgets( row, BUFFERSIZE, fp);
    newrow = (char *) malloc( strlen( row ) + 1 );
    strcpy(newrow, row);
    
    printf("DEBUG RCV: %s", newrow );

    return newrow;
}




/* Generates the current cmdid string... */
void
getcmdid( char * cmdid )
{
    sprintf( cmdid, "%c%03d", cmdchar, cmd );
}




/* Increments the cmdid, sends a command to fp. */
void
sendcmd( char * command )
{
    char cmdid[5];
    
    if ( ++cmd >= 1000 )
    {
        cmd = 0;
        cmdchar++;
    }
    
    getcmdid( cmdid );
    
    fprintf(fp, "%s %s\n", cmdid, command );
    printf("DEBUG SND: %s %s\n", cmdid, command );
}




/* Sends a command, get all the responses,
 * calls the process function...
 */
int
docmd( char * command, int (*fnt)( char * ) )
{
    char *buffer;
    char cmdid[5];
    
    sendcmd( command );
    getcmdid( cmdid );
   
    do
    {
        buffer = getrow();
        (fnt)( buffer );
    }
    while ( strncmp( buffer, cmdid, 4 ) != 0 );

    return SUCCESS;
}




/* Just checks if the next line on the fp begins with "* OK"... */
int
check_ok()
{
    char *buffer;

    buffer = getrow();

    if ( strncmp( buffer, IMAP_WELCOME, sizeof(IMAP_WELCOME) ) )
        return true;
    else
        return false;
}




/* This function looks if the line contains a count of
 * messages -> sets the messages var...
 */
int
mesgcount( char * line )
{
    int i = 0;
    
    if ( strncmp( line+(strlen(line)-8), "EXISTS", 5 ) == 0 )
    {
        line += 2;
        while ( ( ( line[i] >= '0' ) && (line[i] <='9') ) )
            i++;

        line[i] = '\0';
        messages = atoi( line );
        
        printf( "Messages Count: %d\n", messages );
    }
        
    return SUCCESS;
}




/* This function does nothing ;) */
int
donothing( char * line )
{
    return SUCCESS;
}




/* Login to the IMAP server... */
int
imap_login()
{
    char *command;
    sprintf(command, "LOGIN %s %s", get_param_str( "imap", "username", "guest" ),
                                    get_param_str( "imap", "password", "guest" ) );
    docmd( command, &donothing);
    
    return 0;
}




/* Conenct to the IMAP server.
 * Login.
 * Select folder, get message count.
 */
void
imap_init()
{
    char *command;
    
    printf("Connection to IMAP...\n");
    fp = tcp_connect( get_param_str( "imap", "server", "127.0.0.1" ),
                      get_param_int( "imap", "port", 143 ) );
    if (!check_ok())
        exit(2);
    
    imap_login();
    
    sprintf( command, "SELECT %s", get_param_str( "imap", "folder", "INBOX" ) );
    docmd(command, &mesgcount);
}




/* Check if we have a connection.
 * not -> make one ;)
 */
void
check_connection()
{
    if ( fp == NULL )
        imap_init();
}




/* A function to get a list of messages... */
void
imap_getlist()
{
    char *command;
    printf("imap_getlist\n");
    check_connection();
    sprintf( command, "FETCH %d:%d FULL", 1, messages );
    docmd(command, &donothing);
}




/* Get a message from server... */
void
imap_getmesg( int mesg )
{
    char *header;
    char *text;

    sprintf( header, "FETCH %d RFC822.HEADER", mesg );
    sprintf( text, "FETCH %d RFC822.TEXT", mesg );
    printf("imap_getmesg: %d\n", mesg);
    check_connection();
    docmd( header, &donothing );
    docmd( text, &donothing );
}




/* Should I explain? */
void
imap_disconnect()
{
    docmd("LOGOUT", &mesgcount);
    fclose(fp);
}
