//#define TEST_GF

#include "gformats.h"
#include "vbetwo.h"
#include "fileio.h"
#include "palettes.h"
#include "lcstdlib.h" // xfree/xmalloc

DIB_fileheader GF_fheader;
DIB_infoheader GF_iheader;

int createcolorlist(char* bitmap, char* colorlist)
{
  int i, colors = 0;
  bool counted[0x100];
  for (i = 0; i < 0x100; i++)
    counted[i] = false;
  int dx = BITMAP_WIDTH(bitmap),
    dy = BITMAP_HEIGHT(bitmap);
  unsigned char cur_color;
  for (i = 0; i < (dx * dy); i++)
    {
      cur_color = bitmap[4 + i];
      if (counted[cur_color] == false)
	{
	  colorlist[colors++] = cur_color;
	  counted[cur_color] = true;
	}
    }
  return colors;
};

char* loadspr_256(const char* szfilename, char** bitmap, PAL_MODE palmode, char* destPAL)
{
  if (!fexists(szfilename))
    {
      *bitmap = NULL;
      return NULL;
    }
  SafeIO sprite(szfilename, "rb");
  char *destpal = NULL; // suppresses a warning
  char *DIpal = (char*)xmalloc(0x300);
  int size = sprite.size(), i;
  *bitmap = (char*)xmalloc(size - 0x300L);
  sprite.readb(DIpal, 0x300);
  char* nctable = NULL; // suppresses a warning
  sprite.readb(*bitmap, 4);
  int lefttoload = size - 0x300 - 4;
  if ((BITMAP_WIDTH(*bitmap) * BITMAP_HEIGHT(*bitmap)) != lefttoload)
    handleError(ErrorMessage("Error reading sprite [STRPARAM]: data size doesn't correspond to WIDTH [PARAM1] & HEIGHT [PARAM2] information !", szfilename, BITMAP_WIDTH(*bitmap), BITMAP_HEIGHT(*bitmap)));
  sprite.readb((*bitmap) + 4, lefttoload);
  sprite.close();
  if (palmode == TRANSLATE_PAL)
    {
      if (destPAL == NULL)
	{
	  destpal = (char*)xmalloc(0x300);
	  getpal(destpal);
	  nctable = findtransitiontable(destpal, DIpal);
	  xfree(destpal);
	}
      else
	nctable = findtransitiontable(destPAL, DIpal);
      for (i = 4; i < 4 + lefttoload; i++) // the UBYTE-cast is necessary!!
	(*bitmap)[i] = nctable[(unsigned char)((*bitmap)[i])];
      xfree(DIpal);
      xfree(nctable);
      return NULL;
    }
  else
    return DIpal;
};

void savespr_256(const char* szfilename, char* bitmap, char* associatedPAL)
{
  SafeIO future_sprite(szfilename, "wb");
  char* associatedpal;
  if (associatedPAL == NULL)
    {
      associatedpal = (char*)xmalloc(0x300);
      getpal(associatedpal);
      future_sprite.writeb(associatedpal, 0x300);
      xfree(associatedpal);
    }
  else
    future_sprite.writeb(associatedPAL, 0x300);
  int lefttosave = (BITMAP_WIDTH(bitmap) * BITMAP_HEIGHT(bitmap)) + 4;
  future_sprite.writeb(bitmap, lefttosave);
  future_sprite.close();
};

char* loadDIB_256(const char* szfilename, char** bitmap, PAL_MODE palmode, char* destPAL)
{
  if (!fexists(szfilename) || DIB_FILE_CANT_HANDLE)
    {
      *bitmap = NULL;
      return NULL;
    }
  SafeIO deeyebe(szfilename, "rb");
  unsigned long i, j, k;
  char buffer[4];
  rgbQuad onecolor;
  char* destpal = NULL; // suppresses warning
  if (palmode == TRANSLATE_PAL)
    {
      if (destPAL == NULL)
	{
	  destpal = (char*)xmalloc(0x300);
	  getpal(destpal);
	}
      else
	destpal = destPAL;
    }
  char *nctable = NULL, *DIpal = NULL; // supresses warnings
  *bitmap = Imageblock(GF_iheader.width, GF_iheader.height);
  (*bitmap)[0L] = LOW_BYTE(GF_iheader.width);
  (*bitmap)[1L] = HIGH_BYTE(GF_iheader.width);
  (*bitmap)[2L] = LOW_BYTE(GF_iheader.height);
  (*bitmap)[3L] = HIGH_BYTE(GF_iheader.height);
  GF_iheader.bmsize = GF_iheader.width * GF_iheader.height;
  int stuff;
  if (GF_iheader.bpp == 8)
    {
      DIpal = (char*)xmalloc(0x300);
      deeyebe.goto_BEG(sizeof(GF_fheader) + GF_iheader.structsize);
      for (j = 0; j < 0x100; j++)
	{
	  deeyebe.read(&onecolor, sizeof(onecolor), 1);
	  DIpal[j * 3] = COLOR_8to6(onecolor.red);
	  DIpal[(j * 3) + 1] = COLOR_8to6(onecolor.green);
	  DIpal[(j * 3) + 2] = COLOR_8to6(onecolor.blue);
	}
      if (palmode == TRANSLATE_PAL)
	nctable = findtransitiontable(destpal, DIpal);
      else
	{
	  nctable = (char*)xmalloc(0x100);
	  for (i = 0; i < 0x100; i++)
	    nctable[i] = i;
	}
      deeyebe.goto_BEG(GF_fheader.offsetbits);
      stuff = ( (GF_iheader.width % 4) == 0) ? (0) : (4 - (GF_iheader.width % 4) );
      for (i = 0; i < GF_iheader.height; i++)
	{
	  for (j = 0; j < GF_iheader.width; j++)
	    {
	      k = 4 + ((GF_iheader.height - i - 1) * GF_iheader.width) + j;
	      buffer[0] = deeyebe.getchar();
	      (*bitmap)[k] = nctable[buffer[0]];
	    }
	  deeyebe.goto_CUR(stuff);
	}
      if (palmode == TRANSLATE_PAL)
	xfree(DIpal);
      xfree(nctable);
    }
  if (GF_iheader.bpp == 24)
    {
      deeyebe.goto_BEG(GF_fheader.offsetbits);
      stuff = ( ((GF_iheader.width * 3) % 4) == 0) ? (0) : (4 - ((GF_iheader.width * 3) % 4) );
      for (i = 0; i < GF_iheader.height; i++)
	{
	  for (j = 0; j < GF_iheader.width; j++)
	    {
	      k = 4 + ((GF_iheader.height - i - 1) * GF_iheader.width) + j;
	      deeyebe.readb(buffer, 3);
	      (*bitmap)[k] = findnearestcolor_8(destpal, (unsigned char)buffer[2], (unsigned char)buffer[1], (unsigned char)buffer[0]);
	    }
	  deeyebe.goto_CUR(stuff);
	}
    }
  deeyebe.close();
  if (destPAL == NULL)
    xfree(destpal);
  if (palmode == USE_PAL && GF_iheader.bpp == 8)
    return DIpal;
  else
    return NULL;
};

void saveDIB_256(const char* szfilename, char* bitmap, char* associatedPAL)
{
  SafeIO future_DIB(szfilename, "wb");
  GF_iheader.width = BITMAP_WIDTH(bitmap);
  GF_iheader.height = BITMAP_HEIGHT(bitmap);
  GF_iheader.colorsused = 0x100L;
  GF_iheader.colorsimportant = 0x100L;
  GF_iheader.bpp = 8;
  GF_iheader.bmsize = (GF_iheader.width * GF_iheader.height);
  GF_fheader.type = 0x4D42;
  GF_fheader.offsetbits = (unsigned long)(sizeof(GF_fheader) + sizeof(GF_iheader) + (sizeof(rgbQuad) << (GF_iheader.bpp)));
  GF_fheader.filesize = GF_fheader.offsetbits + GF_iheader.bmsize;
  GF_fheader.reserved = 0x00000000L;
  future_DIB.write(&GF_fheader, sizeof(GF_fheader), 1);
  GF_iheader.structsize = (long)(sizeof(GF_iheader));
  GF_iheader.planes = 1;
  GF_iheader.compression = 0x00000000L;
  GF_iheader.Xpixelspermeter = 0x00000000L;       // 0xD0NT
  GF_iheader.Ypixelspermeter = 0x00000000L;       // 0xKN0W
  future_DIB.write(&GF_iheader, sizeof(GF_iheader), 1);
  int i;
  char currentpal[0x300];
  getpal(currentpal);
  char* associatedpal;
  if (associatedPAL == NULL)
    {
      associatedpal = (char*)xmalloc(0x300);
      getpal(associatedpal);
    }
  else
    associatedpal = associatedPAL;
  rgbQuad onecolor;
  onecolor.reserved = 0;
  for (i = 0; i < 0x100; i++)
    {
      onecolor.red   = COLOR_6to8(associatedpal[PAL_RED(i)]);
      onecolor.green = COLOR_6to8(associatedpal[PAL_GREEN(i)]);
      onecolor.blue  = COLOR_6to8(associatedpal[PAL_BLUE(i)]);
      future_DIB.write(&onecolor, sizeof(onecolor), 1);
    }
  if (associatedPAL == NULL)
    xfree(associatedpal);
  char padding_bytes[3];
  padding_bytes[0] = 0x00;
  padding_bytes[1] = 0x00;
  padding_bytes[2] = 0x00;
  int stuff = ((GF_iheader.width % 4) == 0) ? (0) : (4 - (GF_iheader.width % 4));
  while (GF_iheader.height > 0)
    {
      future_DIB.writeb(bitmap + 4 + ((GF_iheader.height - 1) * GF_iheader.width), GF_iheader.width);
      if (stuff != 0) // otherwise it'll truncate the file ??
	future_DIB.writeb(padding_bytes, stuff);
      GF_iheader.height--;
    }
  future_DIB.close();
};

void check_DIB(const char* szfilename)
{
  if (!fexists(szfilename))
    {
      GF_DIB_type = DIB_FILE_DOES_NOT_EXIST;
      return;
    }
  SafeIO bitmap(szfilename, "rb"); // i hope it'll be closed when the destructor is invoked
  bitmap.read(&GF_fheader, sizeof(GF_fheader), 1);
  if (GF_fheader.type != 0x4D42)
    {
      GF_DIB_type = DIB_FILE_NO_DIB;
      return;
    }
  bitmap.read(&GF_iheader, sizeof(GF_iheader), 1);
  if (GF_iheader.compression != 0)
    {
      GF_DIB_type = DIB_FILE_COMPRESSED;
      return;
    }
  if (GF_iheader.bpp == 1)
    {
      GF_DIB_type = DIB_FILE_MONO;
      return;
    }
  if (GF_iheader.bpp == 4)
    {
      GF_DIB_type = DIB_FILE_4;
      return;
    }
  if (GF_iheader.bpp == 8)
    {
      GF_DIB_type = DIB_FILE_8;
      return;
    }
  if (GF_iheader.bpp == 16)
    {
      GF_DIB_type = DIB_FILE_16;
      return;
    }
  if (GF_iheader.bpp == 24)
    {
      GF_DIB_type = DIB_FILE_24;
      return;
    }
  if (GF_iheader.bpp == 32)
    {
      GF_DIB_type = DIB_FILE_32;
      return;
    }
  GF_DIB_type = DIB_FILE_UNKNOWN;
};

#ifdef TEST_GF

#define slow_random(lower,upper) ((int) ( ((upper-lower) * (float)rand() / RAND_MAX) + lower) )

#define szfilenamespr   "D:\\TC\\FX\\MAKEBM\\screen.spr"
//#define szfilenamedib     "D:\\TC\\FX\\MAKEBM\\pmaas.bmp"

#include "keyinp.h"
#include <stdio.h>

int main(int noargs, char** args)
{
  if (noargs < 2 || noargs == 3 || noargs > 5)
    {
      printf("SYNTAX: GFORMATS [xres] [yres] [filename.bmp | filename.spr] [palette[.pal]]\n");
      return 0;
    }
  int resx, resy;
  char* szfilename;
  char *pic, *DIpal, *destpal; // destpal will be allocated when calling loadpalette!
  if (noargs == 2)
    {
      resx = 640;
      resy = 480;
      szfilename = args[1];
      if (!fexists(szfilename))
	{
	  printf("Can't find %s.\n", szfilename);
	  return 0;
	}
      loadpalette("default.pal", &destpal);
    }
  else
    {
      resx = atoi(args[1]);
      resy = atoi(args[2]);
      szfilename = args[3];
      if (!fexists(szfilename))
	{
	  printf("Can't find %s.\n", szfilename);
	  return 0;
	}
      if (noargs == 5)
	loadpalette(args[4], &destpal);
      else
	loadpalette("default.pal", &destpal);
    }
  check_DIB(szfilename);
  printf("loading %s...\n", szfilename);
  if (GF_DIB_type == DIB_FILE_8)
    DIpal = loadDIB_256(szfilename, &pic, USE_PAL);
  else if (GF_DIB_type == DIB_FILE_24)
    loadDIB_256(szfilename, &pic, TRANSLATE_PAL, destpal);
  else if (GF_DIB_type == DIB_FILE_NO_DIB)
    DIpal = loadspr_256(szfilename, &pic, USE_PAL);
  // we can be sure since the first two bytes of a sprite couldn't be 0x4d42 (would mean 19778x? !!!)
  else
    {
      printf("Couldn't load DIB %s:\n", szfilename);
      switch (GF_DIB_type)
	{
	DIB_FILE_MONO: printf("File is a monochrome bitmap.\n"); break;
	DIB_FILE_4: printf("File is a 16-color bitmap.\n"); break;
	DIB_FILE_16: printf("File is a 16bit bitmap.\n"); break;
	DIB_FILE_32: printf("File is a 32bit bitmap.\n"); break;
	DIB_FILE_COMPRESSED: printf("File is a compressed bitmap.\n"); break;
	DIB_FILE_UNKNOWN: printf("File is of unknown type.\n"); break;
	}
      return 0;
    }
  if (!openVBE(640, 480))
    return 1;
  if (GF_DIB_type == DIB_FILE_8 || GF_DIB_type == DIB_FILE_NO_DIB)
    {
      setpal(DIpal);
      xfree(DIpal);
    }
  installkeyhandler();
  float wid, hei;
  wid = (float)BITMAP_WIDTH(pic);
  hei = (float)BITMAP_HEIGHT(pic);
  float x, y, dx, dy;
  x = 0.0F;
  y = 0.0F;
  dx = (float)slow_random(0, 100) / 10.0F;
  dy = (float)slow_random(0, 100) / 10.0F;
  dx = 2.0F;
  dy = -2.0F;
  while	(!getkey(KEY_ESC))
    {
      x += dx;
      y += dy;
      if (x < 0.0F)
	{
	  x = 0.0F;
	  dx = -dx;
	}
      if (y < 0.0F)
	{
	  y = 0.0F;
	  dy = -dy;
	}
      if (x > xres - wid)
	{
	  x = xres - wid;
	  dx = -dx;
	}
      if (y > yres - hei)
	{
	  y = yres - hei;
	  dy = -dy;
	}
      clearoffscreenmem();
      putimage((int)x, (int)y, pic);
      flipoffscreenmem();
    }
  xfree(pic);
  xfree(destpal);
  restorekeyhandler();
  closeVBE();
  return 0;
}

#endif
