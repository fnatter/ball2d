#include "gformats.h"
#include "vbetwo.h"
#include "palettes.h"
#include <pc.h>
#include <stdio.h>
#include <math.h>
#include <keys.h>

const float MAX_ANGLE = M_PI * 6.0F;

char* pic;
int width, height;
const int max_offx = 5, max_offy = 5;

#define pic_pixel(x,y) pic[4 + ((y) * width) + (x)]

int main(int argc, char** args) {
  if (argc < 2) {
    printf("The first argument must be the name of a sprite file.\n");
    return 1;
  } 
  char* pal = loadspr_256(args[1], &pic, USE_PAL);
  if (pic == 0) {
    printf("%s doesn't exist!\n", args[1]);
    return 1;
  }
  width = BITMAP_WIDTH(pic);
  height = BITMAP_HEIGHT(pic);
  openVBE(800, 600, 8);
  int offx, offy;
  float dalphax, dalphay, alphax, alphay;
  dalphax = MAX_ANGLE / width;
  dalphay = MAX_ANGLE / height;
  int px = (xres - width) / 2, py = (yres - height) / 2;
  setpal(pal);
  int i, j;
  alphax = 0.0F;
  alphay = 0.0F;
  int key;
  while (key != K_Escape) {
    if (kbhit())
      key = getkey();
    for (i = 0; i < 1; i++)
      clearoffscreenmem();
    float startalphay = alphay, scale;
    for (i = 0; i < height; i++) {
      alphax += dalphax;
      offx = (int)(sin(alphax) * max_offx + 0.5F);
      alphay = startalphay;
      scale = width / (float)(width + offx);
      for (j = 0; j < width + offx; j++) {
	alphay += dalphay;
	offy = (int)(sin(alphay) * max_offy + 0.5F);
	putpixel(px + j, py + offy + i, pic_pixel((int)(scale * j + 0.5F), i));
      }
    }
    for (i = 0; i < 1; i++)
      flipoffscreenmem();

  }
  closeVBE();
  xfree(pic);
}

