#include "vbetwo.h"
#include "gformats.h"
#include <conio.h>

int main()
{
  openVBE(640, 480);
  char* pic;
  loadspr_256("pat1.spr", &pic, TRANSLATE_PAL);
  clearoffscreenmem();
  clipputimage(xres - 10, 100, pic);
  flipoffscreenmem();
  getch();
  xfree(pic);
  closeVBE();
}
