/* This program sets brightness and contrast controls
 * given on the commandline. It only works, of course
 * with drivers that support setting brightness and
 * contrast.
 */

#include <picogui.h>

int main(int argc, char **argv)
{

   pgInit(argc,argv);
   pgDriverMessage(PGDM_BRIGHTNESS,atoi(argv[1]));
   pgDriverMessage(PGDM_CONTRAST,atoi(argv[2]));
   pgFlushRequests();
   return 0;
}
