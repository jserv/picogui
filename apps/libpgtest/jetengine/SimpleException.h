/* Abstract base class for simple exceptions that can only be dumped to stderr */

#ifndef _H_SIMPLEEXCEPTION
#define _H_SIMPLEEXCEPTION

class SimpleException {
 public:
  virtual void show(void) = 0;
};

#endif /* _H_SIMPLEEXCEPTION */
