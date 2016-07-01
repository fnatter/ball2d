#include "fileio.h"
#include "fifo99.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // atoi

int main(int nargs, char** args)
{
  if (nargs != 6 && nargs != 9)
    {
      printf("SYNTAX: GEM2SOM [infile.GEM] [outfile.SOM] [Red] [Green] [Blue] [[AlternateRed] [AlternateGreen] [AlternateBlue]]");
      return 0;
    }
  int col[6];
  col[0] = atoi(args[3]);
  col[1] = atoi(args[4]);
  col[2] = atoi(args[5]);
  if (nargs == 9)
    {
      col[3] = atoi(args[6]);
      col[4] = atoi(args[7]);
      col[5] = atoi(args[8]);
    }
  if (!fexists(args[1]))
    {
      printf("\nCan't find %s.\n", args[1]);
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
  FILE* GEMfile = fopen(args[1], "rt");
  FILE* SOMfile = fopen(args[2], "wt");
  int nVertices, nFaces, nTextures = 1;
  int i, bla;
  fscanf(GEMfile, "%i %i %i\n", &nVertices, &nFaces, &bla);
  printf("Mesh has %i Vertices, %i Faces...\n", nVertices, nFaces);
  fprintf(SOMfile, "noname\n");
  fprintf(SOMfile, "%i %i %i\n", nVertices, nFaces, nTextures);
  fprintf(SOMfile, "texture.spr\n");
  printf("\tConverting %s to %s... ([%i][%i][%i])", args[1], args[2], col[0], col[1], col[2]);
  float x, y, z;
  for (i = 0; i < nVertices; i++)
    {
      fscanf(GEMfile, "%f %f %f\n", &x, &y, &z);
      fprintf(SOMfile, "%f %f %f\n", x, y, z);
    }
  int LocalTextureIndex = 0, polyverts;
  int indices[6];
  bool alternate = true;
  int col_offset;
  for (i = 0; i < nFaces; i++)
    {
      fscanf(GEMfile, "%i ", &polyverts);
      if (polyverts < 3)
	handleError(ErrorMessage("In GEM-file [STRPARAM]: face has fewer than 3 vertices [PARAM1]", args[1], polyverts));
      if (nargs == 6)
	col_offset = 0;
      else
	{
	  if (alternate)
	    col_offset = 0;
	  else
	    col_offset = 3;
	  alternate = !alternate;
	}
      switch (polyverts)
	{// in GEM's (i hope that applies to all .GEM files), vertices are listed in clockwise-order, i need them ACW, so I'll turn the indices around
	case 3: // note that the indices range from 1..totalVertices, not 0..totalVertices - 1, so subtract one
	  fscanf(GEMfile, "%i %i %i\n", indices + 2, indices + 1, indices + 0);
	  fprintf(SOMfile, "3: %i %i %i %i %i %i %i\n", indices[0] - 1, indices[1] - 1, indices[2] - 1, col[col_offset], col[col_offset + 1], col[col_offset + 2], LocalTextureIndex);
	  break;
	case 4:
	  fscanf(GEMfile, "%i %i %i %i\n", indices + 3, indices + 2, indices + 1, indices + 0);
	  fprintf(SOMfile, "4: %i %i %i %i %i %i %i %i\n", indices[0] - 1, indices[1] - 1, indices[2] - 1, indices[3] - 1, col[col_offset], col[col_offset + 1], col[col_offset + 2], LocalTextureIndex);
	  break;
	case 5:
	  fscanf(GEMfile, "%i %i %i %i %i\n", indices + 4, indices + 3, indices + 2, indices + 1, indices + 0);
	  fprintf(SOMfile, "4: %i %i %i %i %i %i %i %i %i\n", indices[0] - 1, indices[1] - 1, indices[2] - 1, indices[3] - 1, indices[4] - 1, col[col_offset], col[col_offset + 1], col[col_offset + 2], LocalTextureIndex);
	  break;
	case 6:
	  fscanf(GEMfile, "%i %i %i %i %i %i\n", indices + 5, indices + 4, indices + 3, indices + 2, indices + 1, indices + 0);
	  fprintf(SOMfile, "4: %i %i %i %i %i %i %i %i %i %i\n", indices[0], indices[1], indices[2], indices[3], indices[4], indices[5], col[col_offset], col[col_offset + 1], col[col_offset + 2], LocalTextureIndex);
	  break;
	default:
	  printf("Face %i has too many vertices.", i);
	  return 1;
	}
    }
  fclose(GEMfile);
  fclose(SOMfile);
  return 0;
}
