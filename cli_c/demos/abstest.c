/* Little proggie to demo the absolute coordinates -- micah */

#include <picogui.h>

int main(int argc,char **argv) {
   pghandle wLabel;
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Absolute coordinate test",0);
   
   pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   pgNewWidget(PG_WIDGET_LABEL,0,0);

   printf("Coords: %d, %d\n",pgGetWidget(PGDEFAULT,PG_WP_ABSOLUTEX),
	  pgGetWidget(PGDEFAULT,PG_WP_ABSOLUTEY));
   
   printf("pgUpdate();\n");
   pgUpdate();

   printf("Coords: %d, %d\n",pgGetWidget(PGDEFAULT,PG_WP_ABSOLUTEX),
	  pgGetWidget(PGDEFAULT,PG_WP_ABSOLUTEY));
   
   return 0;
}
