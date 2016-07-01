#ifndef __LCSTDLIB_H
#define __LCSTDLIB_H

#include <stdlib.h> // for size_t

/*
  these are defined here, because in djdev-2.02, these prototypes
  are left implicit
 */

extern "C" {
void xfree(void * buf);
void* xmalloc(size_t len);
}

#endif

