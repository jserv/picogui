/*A little prog for creating pics.h from the .pbm files. Takes one
argument (the filename) and outputs an "unsigned char xxxx[] = { ....}"
on stdout for pgFromMemory to use. Usage: ./pbm_dump xxx.pbm >> pics.h */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  unsigned char buf[2], *p;
  int fd, j = -1;

  fd = open (argv[1], O_RDONLY);
  if (fd == -1)
    exit (-1);

  for (p = argv[1]; *p; p++)
    if (*p == '.')
      *p = '_';

  printf ("unsigned char %s[] = {\n", argv[1]);
  while (read (fd, buf, 1) > 0)
    {
      if (j++ != -1)
	{
	  if (j > 11)
	    {
	      printf (",\n");
	      j = 0;
	    }
	  else
	    printf (", ");
	}
      printf ("0x%.2x", (int) buf[0]);
    }
  printf (" };\n\n");
  return 0;
}
