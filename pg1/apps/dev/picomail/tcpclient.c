#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>


FILE *
tcp_connect (char *host, int port)
{
  struct sockaddr_in serv_sock_addr;
  int socket_fd;
  FILE *connfp;

  bzero ((char *) &serv_sock_addr, sizeof (serv_sock_addr));
  serv_sock_addr.sin_family = AF_INET;
  serv_sock_addr.sin_addr.s_addr = inet_addr (host);
  serv_sock_addr.sin_port = htons (port);

  if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf ("tcp_connect: Opening socket failed...");
      exit (1);
    }


  if (connect (socket_fd, (struct sockaddr *) &serv_sock_addr,
	       sizeof (serv_sock_addr)) < 0)
    {
      printf ("tcp_connect: Connection failed...");
      exit (1);
    }

  if ((connfp = fdopen (socket_fd, "r+")) < 0)
    {
      printf ("tcp_connect: Could not open filepointer to socket...\n");
      exit (1);
    }

  return connfp;
}

void
tcp_disconnect (FILE * connfp)
{
  fclose (connfp);
}
