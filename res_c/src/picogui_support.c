#ifdef CONFIG_PICOGUI_SUPPORT

#include <picogui.h>

#include "res_c.h"
#include "confparse.h"
#include "picogui_support.h"

//-------------PicoGUI specific functions----------------------
struct pgmemdata pgFromResource(resResource *resource, char *resourceName){
  struct pgmemdata newData;
  int size;
  void *data = (void *)resGetResource(resource, "Resources", resourceName, &size);

  newData.pointer = data;
  newData.size = size;
  newData.flags = 0;

  return newData;
}

#endif
