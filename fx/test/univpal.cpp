#include "palettes.h"
#include <stdio.h>

#define ONE_7           1.0F / 7.0F
#define ONE_8           1.0F / 8.0F

float one_edgeof6[6] = {ONE_7, ONE_7 * 2.0F, ONE_7 * 3.0F, ONE_7 * 4.0F, ONE_7 * 5.0F, ONE_7 * 6.0F};
float one_edgeof7[7] = {ONE_8, ONE_8 * 2.0F, ONE_8 * 3.0F, ONE_8 * 4.0F, ONE_8 * 5.0F, ONE_8 * 6.0F, ONE_8 * 7.0F};

int main()
{
  char pal[0x300];
  float r, g, b;
  int i, j, k, palidx = 0;
  // first 6 * 6 * 7 colors (252)
  for (i = 0; i < 7; i++)
    {
      g = one_edgeof7[i];
      for (j = 0; j < 6; j++)
	{
	  r = one_edgeof6[j];
	  for (k = 0; k < 6; k++)
	    {
	      b = one_edgeof6[k];
	      pal[palidx++] = (int)(r * 63.0F);
	      pal[palidx++] = (int)(g * 63.0F);
	      pal[palidx++] = (int)(b * 63.0F);
	    }
	}
    }
  // use red, green, blue & white for 4 last colors
  pal[palidx++] = 63;
  pal[palidx++] = 0;
  pal[palidx++] = 0;
  pal[palidx++] = 0;
  pal[palidx++] = 63;
  pal[palidx++] = 0;
  pal[palidx++] = 0;
  pal[palidx++] = 0;
  pal[palidx++] = 63;
  pal[palidx++] = 63;
  pal[palidx++] = 63;
  pal[palidx++] = 63;
  printf("palidx = %i\n", palidx);
  savepalette("univ.pal", pal);
}
