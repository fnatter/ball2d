#include "fifo99.h"
#include "fileio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // atoi

int main(int nargs, char** args)
{
  if (nargs != 6 && nargs != 9)
    {
      printf("SYNTAX: 3D2SOM [infile.3D] [outfile.SOM] [Red] [Green] [Blue] [[AlternateRed] [AlternateGreen] [AlternateBlue]]");
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
  FILE* _3Dfile = fopen(args[1], "rt");
  FILE* SOMfile = fopen(args[2], "wt");
  int nVertices, nFaces, nTextures = 1;
  int i;
  fscanf(_3Dfile, "%i %i\n", &nVertices, &nFaces);
  printf("Mesh has %i Vertices, %i Faces...\n", nVertices, nFaces);
  fprintf(SOMfile, "noname\n");
  fprintf(SOMfile, "%i %i %i\n", nVertices, nFaces, nTextures);
  fprintf(SOMfile, "texture.spr\n");
  float x, y, z, bla, blabla;
  printf("\tConverting %s to %s... ([%i][%i][%i])", args[1], args[2], col[0], col[1], col[2]);
  for (i = 0; i < nVertices; i++)
    {
      fscanf(_3Dfile, "%f %f %f %f %f\n", &x, &y, &z, &bla, &blabla);
      fprintf(SOMfile, "%f %f %f\n", x, y, z);
    }
  int LocalTextureIndex = 0;
  int indices[4];
  bool alternate = true;
  for (i = 0; i < nFaces; i++)
    {
      fscanf(_3Dfile, "%i %i %i\n", indices, indices + 1, indices + 2);
      if (nargs == 6)
	fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1], indices[2], col[0], col[1], col[2], LocalTextureIndex);
      else
	{
	  if (alternate)
	    fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1], indices[2], col[0], col[1], col[2], LocalTextureIndex);
	  else
	    fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1], indices[2], col[3], col[4], col[5], LocalTextureIndex);
	  alternate = !alternate;
	}
    }
  fclose(_3Dfile);
  fclose(SOMfile);
  return 0;
}
