#include "vbetwo.h"
#include "palettes.h"
#include "gformats.h"
#include "fifo99.h"
#include <stdio.h> // printf
#include "lcstdlib.h" // xmalloc/xfree

int main(int nargs, char** args)
{
  if (nargs != 2)
    {
      printf("SYNTAX\tSHADING [filename.spr].\n");
      return 0;
    }
  char* pic, *shadepic, *pal;
  unsigned char* shades[0x100];
  loadpalette("shadelin.pal", &pal);
  loadspr_256(args[1], &pic, TRANSLATE_PAL, pal);
  int i;
  for (i = 0; i < 0x100; i++)
    shades[i] = createshadinglookup(pal, SHADES_PER_COLOR, i);
  int width, height;
  if (pic == NULL)
    return 1;
  if (!openVBE(800, 600))
    return 1;
  setpal(pal);
  xfree(pal);
  installkeyhandler();
  width = 640;
  height = 480;
  scaleimage(&pic, width, height);
  shadepic = Imageblock(width, height);
  memcpy(shadepic, pic, 4);
  int picx = xres / 2 - width / 2, picy = yres / 2 - height / 2;
  int shade = 0;
  bool autoshade = true;
  while (!getkey(KEY_ESC))
    {
      if (getkey(KEY_PLUS))
	shade = (SHADES_PER_COLOR - 1 <? shade + 1);
      if (getkey(KEY_MINUS))
	shade = (0 >? shade - 1);
      if (getkey(KEY_SPACE))
	autoshade = !autoshade;
      if (autoshade)
	shade = (shade + 1) % SHADES_PER_COLOR;
      clearoffscreenmem();
      for (i = 0; i < width * height; i++)
	shadepic[4 + i] = *(shades[(unsigned char)pic[4 + i]] + shade);
      putimage(picx, picy, shadepic);
      flipoffscreenmem();
    }
  xfree(pic);
  xfree(shadepic);
  for (i = 0; i < 0x100; i++)
    xfree(shades[i]);
  restorekeyhandler();
  closeVBE();
}
