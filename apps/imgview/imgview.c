/* little image viewer demo */

#include <picogui.h>

int main(int argc, char *argv[])
{
   pghandle bitmap;
   int w,h;
   
   pgInit(argc,argv);
   
   pgRegisterApp(PG_APP_NORMAL,"Image Viewer",
		 PG_APPSPEC_HEIGHT,200,
		 0);
   
   bitmap = pgNewBitmap(pgFromFile(argv[1]));
   
   pgSizeBitmap(&w,&h,bitmap);
   printf("Bitmap is %dx%d pixels\n",w,h);
   
   pgNewWidget(PG_WIDGET_BITMAP,0,0);
   pgSetWidget(0,
	       PG_WP_SIDE,PG_S_ALL,
	       PG_WP_BITMAP,bitmap,
	       0);
   
   pgEventLoop();
   return 0;
}





/* The End */
