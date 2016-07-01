#include "fifo99.h"
#include "fileio.h"
#include <stdio.h>
#include <stdlib.h> // atoi
#include <string.h>

/* TODO:
make it work!
*/

int main(int argc, char** args)
{
  if (argc != 6 && argc != 9)
    {
      printf("SYNTAX: JAW2SOM [infile.JAW] [outfile.SOM] [Red] [Green] [Blue] [[AlternateRed] [AlternateGreen] [AlternateBlue]]");
      return 0;
    }
  int col[6];
  col[0] = atoi(args[3]);
  col[1] = atoi(args[4]);
  col[2] = atoi(args[5]);
  if (argc == 9)
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
  FILE* JAWfile = fopen(args[1], "rt");
  FILE* SOMfile = fopen(args[2], "wt");
  int nVertices, nFaces, nTextures = 1;
  int i, j;
  // look for the number of vertices
  char line[10000];
  i = (int)fgets(line, 10000, JAWfile); /* discard line about Lightsource */
  if (i != NULL)
    i = (int)fgets(line, 10000, JAWfile);
  while (i != NULL) {
    j = 0;
    while (line[j] != ':' && line[j] != '\0') {
      j++;
    }
    if (line[j] == '\0') {
      /* vertice-list is finished, so this line should already contain a face-definition,
	 if this isn't so, quit with an error */
      if (line[0] != 't' || line[1] != 'r' || line[2] != 'i') {
	printf("Error in %s: Unknown block following vertice-list (only tri supported).",
	       args[1]);
	return 1;
      }
      break; // stop the while-loop
    }
    line[j] = '\0'; // terminate string at ':', what's left is the index of the vertex
    nVertices = atoi(line); /* last vertex so far */
    i = (int)fgets(line, 10000, JAWfile); // get next line
  }
  nVertices++; // vertice-indices are zero-based
  // now find out how many faces...; 'line' already contains the first line of the faces-block
  nFaces = 0;
  while (i != NULL) {
    nFaces++;
    if (line[0] != 't' || line[1] != 'r' || line[2] != 'i') {
      printf("Error in %s: Unknown block in face-list (only tri supported).",
	     args[1]);
      return 1;
    }
    i = (int)fgets(line, 10000, JAWfile);
  }
  fclose(JAWfile);
  JAWfile = fopen(args[1], "rt");
  printf("Mesh has %i Vertices, %i Faces...\n", nVertices, nFaces);
  fprintf(SOMfile, "noname\n");
  fprintf(SOMfile, "%i %i %i\n", nVertices, nFaces, nTextures);
  fprintf(SOMfile, "texture.spr\n");
  float x, y, z;
  int dummy;
  fgets(line, 10000, JAWfile); // discard Lightsource information
  printf("\tConverting %s to %s... ([%i][%i][%i])\n", args[1], args[2], col[0], col[1], col[2]);
  for (i = 0; i < nVertices; i++)
    {
      fscanf(JAWfile, "%i: %f %f %f\n", &dummy, &x, &y, &z);
      if (dummy != i) {
	printf("Error in %s: Vertice-Index out of range.", args[1]);
	return 1;
      }
      fprintf(SOMfile, "%f %f %f\n", x, y, z);
    }
  int LocalTextureIndex = 0;
  int indices[4];
  bool alternate = true;
  for (i = 0; i < nFaces; i++) {
    fscanf(JAWfile, "tri %i, %i, %i\n", indices, indices + 1, indices + 2);
    if (argc == 6)
      fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1],
	      indices[2], col[0], col[1], col[2], LocalTextureIndex);
    else
      {
	if (alternate)
	  fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1],
		  indices[2], col[0], col[1], col[2], LocalTextureIndex);
	else
	  fprintf(SOMfile, "3:%i %i %i %i %i %i %i\n", indices[0], indices[1],
		  indices[2], col[3], col[4], col[5], LocalTextureIndex);
	alternate = !alternate;
      }
  }
  fclose(JAWfile);
  fclose(SOMfile);
  return 0;
}
