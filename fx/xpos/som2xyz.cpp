#include "fifo99.h"
#include "gformats.h"
#include "object3d.h"
#include "fileio.h"
#include "palettes.h"
#include "vbetwo.h"
#include <stdio.h>

int main(int noargs, char** args)
{
  printf("Compiler for Small Object Models (.SOM)\n");
  if (noargs < 3)
    {
      printf("SYNTAX: SOM2XYZ [infile.SOM] [outfile.XYZ]");
      return 0;
    }
  if (!fexists(args[1]))
    {
      printf("\nCan't find %s.\n", args[1]);
      return 1;
    }
  char szpalfile[100];
  if (!fexists("xpos.cfg"))
    {
      printf("Can't find xpos.cfg.");
      return 1;
    }
  int i, j, k;
  SafeIO config("xpos.cfg", "rb");
  fscanf(config.getFILEObject(), "res: %d, %d\n", &i, &j);
  fscanf(config.getFILEObject(), "hsr: %s\n", szpalfile);
  fscanf(config.getFILEObject(), "lightsource_dragging: %s\n", szpalfile);
  fscanf(config.getFILEObject(), "autosave_the_.wld: %s\n", szpalfile);
  fscanf(config.getFILEObject(), "shading_palette: %s\n", szpalfile);
  config.close();
  if (!fexists(szpalfile))
    {
      printf("Can't find standard shading palette %s.\n", szpalfile);
      return 1;
    }
  int yes, no;
  if (fexists(args[2]))
    {
      //	NO_Cursor();
      printf("\nFile %s already exists. Overwrite ? (J/N)\n", args[2]);
      yes = no = 0;
      installkeyhandler();
      while (!yes && !no)
	{
	  yes = getkey(KEY_J);
	  no = getkey(KEY_N);
	}
      restorekeyhandler();
      //	Prev_Cursor();
      if (no)
	return 1;
    }
  SafeIO SOMfile(args[1], "rt");
  SafeIO XYZfile(args[2], "wb");
  XYZheader head;
  head.XYZid[0] = 'X';
  head.XYZid[1] = 'Y';
  head.XYZid[2] = 'Z';
  head.XYZid[3] = 'f';
  head.XYZid[4] = 'x';
  fscanf(SOMfile.getFILEObject(), "%s\n", head.ObjectName);
  fscanf(SOMfile.getFILEObject(), "%d %d %d\n", &head.nVertices, &head.nFaces, &head.nTextures);
  XYZfile.write(&head, sizeof(XYZheader), 1);
  printf("Mesh has %d Vertices, %d Faces...\n", head.nVertices, head.nFaces);
  printf("\tConverting %s to %s...\n", args[1], args[2]);
  char tstr[1000];
  char* pic, *the_pal;
  loadpalette(szpalfile, &the_pal);
  XYZTextureheader_8 texhead;
  for (i = 0; i < head.nTextures; i++)
    {
      fscanf(SOMfile.getFILEObject(), "%s\n", tstr);
      if (!fexists(tstr))
	{
	  printf("Can't find texture %s.\n", tstr);
	  return 1;
	}
      tstr[strlen(tstr)] = '\0';
      loadspr_256(tstr, &pic, TRANSLATE_PAL, the_pal);
      if (pic == NULL)
	{
	  printf("Can't load texture %s.\n", tstr);
	  return 1;
	}
      int width = BITMAP_WIDTH(pic);
      k = 0;
      for (j = 0; j < 32; j++)
	{
	  if (width & (1 << j))
	    k = j;
	}
      if (width != 1 << k) // if it isn't exactly aligned on a bit position...
	width = 1 << (k + 1); // k is highest "1"-bit in previous width
      int height = (int)((width/(float)BITMAP_WIDTH(pic)) * (float)BITMAP_HEIGHT(pic)); // scale height proportially
      printf("scaling texture %s from (%d x %d) to (%d x %d).\n", tstr, BITMAP_WIDTH(pic), BITMAP_HEIGHT(pic), width, height);
      scaleimage(&pic, width, height);
      texhead.picture_size = (BITMAP_HEIGHT(pic) * BITMAP_WIDTH(pic)) + 4;
      XYZfile.write(&texhead, sizeof(texhead), 1);
      XYZfile.writeb(pic, texhead.picture_size);
      xfree(pic);
    }
  xfree(the_pal);
  point3d vertex;
  for (i = 0; i < head.nVertices; i++)
    {
      fscanf(SOMfile.getFILEObject(), "%f %f %f\n", &vertex[VEC_X], &vertex[VEC_Y], &vertex[VEC_Z]);
      XYZfile.write(&vertex, sizeof(vertex), 1);
    }
  short indices[6];
  XYZFaceheader facehead;
  short int temp, colors[3];
  for (i = 0; i < head.nFaces; i++)
    {
      fscanf(SOMfile.getFILEObject(), "%hd:", &temp);
      facehead.nVertices = (char)temp;
      if (facehead.nVertices == 3)
	fscanf(SOMfile.getFILEObject(), "%hd %hd %hd %hd %hd %hd %hd\n", indices, indices + 1, indices + 2, colors, colors + 1, colors + 2, &temp);
      else
	if (facehead.nVertices == 4)
	  fscanf(SOMfile.getFILEObject(), "%hd %hd %hd %hd %hd %hd %hd %hd\n", indices, indices + 1, indices + 2, indices + 3, colors, colors + 1, colors + 2, &temp);
	else
	  if (facehead.nVertices == 5)
	    fscanf(SOMfile.getFILEObject(), "%hd %hd %hd %hd %hd %hd %hd %hd %hd\n", indices, indices + 1, indices + 2, indices + 3, indices + 4, colors, colors + 1, colors + 2, &temp);
	  else
	    if (facehead.nVertices == 6)
	      fscanf(SOMfile.getFILEObject(), "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd\n", indices, indices + 1, indices + 2, indices + 3, indices + 4, indices + 5, colors, colors + 1, colors + 2, &temp);
	    else
	      {
                printf("Face %i has too many vertices: %i.", i, facehead.nVertices);
		return 1;
	      }
      facehead.LocalTextureIndex = temp;
      facehead.Face_basecolor_red = (unsigned char)colors[0];
      facehead.Face_basecolor_green = (unsigned char)colors[1];
      facehead.Face_basecolor_blue = (unsigned char)colors[2];
      XYZfile.write(&facehead, sizeof(facehead), 1);
      XYZfile.write(indices, sizeof(short), facehead.nVertices);
    }
  return 0;
}
