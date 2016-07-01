#include "fileio.h"
#include "fifo99.h"
#include "object3d.h"
#include <stdio.h>

int main(int nargs, char** args)
{
  if (nargs != 3)
    {
      printf("SYNTAX:\tPLAYGOD (create world) [any_idea.id] [filename.wld]\n");
      return 0;
    }
  if (!fexists(args[1]))
    {
      printf("Can't find %s.\n", args[1]);
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
  FILE* idea = fopen(args[1], "rt");
  FILE* world = fopen(args[2], "wb");
  WorldHeader wldhead;
  fscanf(idea, "%i\n", &wldhead.nMeshes);
  fscanf(idea, "Light: %f %f %f\n", &wldhead.Directional_Light_Source[VEC_X], &wldhead.Directional_Light_Source[VEC_Y], &wldhead.Directional_Light_Source[VEC_Z]);
  fscanf(idea, "CameraPos: %f %f %f\n", &wldhead.CameraInitialPosition[VEC_X], &wldhead.CameraInitialPosition[VEC_Y], &wldhead.CameraInitialPosition[VEC_Z]);
  fscanf(idea, "CameraXAxis: %f %f %f\n", &wldhead.CameraInitialXAxis[VEC_X], &wldhead.CameraInitialXAxis[VEC_Y], &wldhead.CameraInitialXAxis[VEC_Z]);
  fscanf(idea, "CameraYAxis: %f %f %f\n", &wldhead.CameraInitialYAxis[VEC_X], &wldhead.CameraInitialYAxis[VEC_Y], &wldhead.CameraInitialYAxis[VEC_Z]);
  fscanf(idea, "CameraZAxis: %f %f %f\n", &wldhead.CameraInitialZAxis[VEC_X], &wldhead.CameraInitialZAxis[VEC_Y], &wldhead.CameraInitialZAxis[VEC_Z]);
  fscanf(idea, "FOV: %f\n", &wldhead.CameraInitialFOV);
  if (wldhead.CameraInitialFOV < 0.0F || wldhead.CameraInitialFOV >= 180.0F)
    {
      printf("please change the FieldOfView so that 0.0ø <= FOV < 180.0ø\n");
      return 1;
    }
  wldhead.CameraInitialFOV *= 0.0174533; // convert to RAD
  fwrite(&wldhead, sizeof(wldhead), 1, world);
  int i;
  char str[100];
  WorldMesh wldmesh;
  for (i = 0; i < wldhead.nMeshes; i++)
    {
      fscanf(idea, "%s\n", str);
      str[12] = '\0'; // to be sure...
      strcpy(wldmesh.szMeshfilename, str);
      fscanf(idea, "Pos: %f %f %f\n", &wldmesh.InitialPosition[VEC_X], &wldmesh.InitialPosition[VEC_Y], &wldmesh.InitialPosition[VEC_Z]);
      fscanf(idea, "Origin: %f %f %f\n", &wldmesh.CenterofRotation[VEC_X], &wldmesh.CenterofRotation[VEC_Y], &wldmesh.CenterofRotation[VEC_Z]);
      fscanf(idea, "Energy: %f %f %f\n", &wldmesh.RotationalEnergy[VEC_X], &wldmesh.RotationalEnergy[VEC_Y], &wldmesh.RotationalEnergy[VEC_Z]);
      wldmesh.RotationalEnergy[VEC_X] *= M_PI / 180.0F;
      wldmesh.RotationalEnergy[VEC_Y] *= M_PI / 180.0F;
      wldmesh.RotationalEnergy[VEC_Z] *= M_PI / 180.0F;
      fscanf(idea, "Size: %f\n", &wldmesh.NewDiameter);
      fscanf(idea, "Facetype: %s\n", str);
      if (stricmp(str, "wireframe") == 0)
	wldmesh.face_display_type = FACE_DISPLAY_WIREFRAME;
      else
	if (stricmp(str, "shaded") == 0)
	  wldmesh.face_display_type = FACE_DISPLAY_SHADED;
	else
	  if (stricmp(str, "gouraud") == 0)
            wldmesh.face_display_type = FACE_DISPLAY_GOURAUD;
	  else
	    if (stricmp(str, "flat") == 0)
	      wldmesh.face_display_type = FACE_DISPLAY_FLAT;
	    else
	      if (stricmp(str, "textured") == 0)
		wldmesh.face_display_type = FACE_DISPLAY_TEXTURED;
	      else
		if (stricmp(str, "shaded+textured") == 0)
		  wldmesh.face_display_type = FACE_DISPLAY_SHADED_TEXTURE;
		else
		  {
		    printf("Error in %s at Mesh %s - facetype not specified correctly.\n", args[1], wldmesh.szMeshfilename);
		    return 1;
		  }
      fwrite(&wldmesh, sizeof(wldmesh), 1, world);
    }
  fclose(idea);
  fclose(world);
}
