#ifdef ENABLE_PICOGUI_SUPPORT

#ifndef PGMEMDAT_NEED_FREE

struct pgmemdata {
  void *pointer;       //!< when null, indicates error
  u32 size;  //!< size in bytes of data block
  int flags;           //!< PGMEMDAT_* flags or'ed together
};
#define PGMEMDAT_NEED_FREE    0x0001   //!< pgmemdata should be free()'d when done
#define PGMEMDAT_NEED_UNMAP   0x0002   //!< pgmemdata should be munmap()'d when done

//------------PicoGUI specific functions-----------------------
struct pgmemdata pgFromResource(resResource *resource, char *resourceName);

#endif
