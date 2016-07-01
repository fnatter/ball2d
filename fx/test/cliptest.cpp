#include "vbetwo.h"
#include "fifo99.h"
#include "palettes.h"
#include <stdlib.h>

inline int slow_random (int lower, int upper)
{ // returns random_number in the interval [lower; upper[
  return (int)(((upper - lower) * (float)rand() / RAND_MAX) + lower);
};

int main()
{
  if (!openVBE(640, 480))
    return 1;
  installkeyhandler();
  clearoffscreenmem();
  rectangle(50, 50, 540, 380, NEAREST[CCYELLOW]);
  setClipRect(50, 50, 540, 380);
  int x1, y1;
  while (!peekanykey())
    {
      x1 = slow_random(0, xres);
      y1 = slow_random(0, yres);
      clipline(x1, y1, slow_random(x1, xres), slow_random(y1, yres), slow_random(0, 256));
      flipoffscreenmem();
    }
  restorekeyhandler();
  closeVBE();
  return 0;
}
