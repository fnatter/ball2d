#include "vbetwo.h"
#include "palettes.h"
#include "grtext.h"
#include "camera.h"
#include "vecmat.h"
#include "fileio.h"
#include "fifo99.h"
#include "object3d.h"
#include "world3d.h"
#include "lcstdlib.h" // itoa, xmalloc/xfree

#define unit            10.0F


void drawaxes()
{
  point3d P1, P2;
  initVec(P1, -unit, 0.0F, 0.0F);
  initVec(P2, unit, 0.0F, 0.0F);
  draw3dline(P1, P2, NEAREST[CCWHITE]);
  initVec(P1, 0.0F, -unit, 0.0F);
  initVec(P2, 0.0F, unit, 0.0F);
  draw3dline(P1, P2, NEAREST[CCYELLOW]);
  initVec(P1, 0.0F, 0.0F, -unit);
  initVec(P2, 0.0F, 0.0F, unit);
  draw3dline(P1, P2, NEAREST[CCORANGE]);
};

void saveWLDconfig(const char* szfilename, float FOV, WorldHeader& wldhead, WorldMesh* meshinfo)
{
  assignVec(wldhead.Directional_Light_Source, enlightenment);
  assignVec(wldhead.CameraInitialXAxis, cur_cam.XAxis);
  assignVec(wldhead.CameraInitialYAxis, cur_cam.YAxis);
  assignVec(wldhead.CameraInitialZAxis, cur_cam.ZAxis);
  assignVec(wldhead.CameraInitialPosition, cur_cam.pos);
  wldhead.CameraInitialFOV = FOV;
  int i;
  SafeIO worldf(szfilename, "wb");
  worldf.writeb(&wldhead, sizeof(wldhead));
  for (i = 0; i < totalObjects; i++)
    {
      assignVec(meshinfo[i].InitialPosition, objects[i].geometric_center);
      assignVec(meshinfo[i].CenterofRotation, objects[i].HomeSweetHome);
      initVec(meshinfo[i].RotationalEnergy, objects[i].Xan, objects[i].Yan, objects[i].Zan);
      meshinfo[i].NewDiameter = objects[i].bounding_Sphere_radius * 2.0F;
      worldf.writeb(meshinfo + i, sizeof(WorldMesh));
    }
  worldf.close();
};

int main(int nargs, char** args)
{
  if (nargs != 2)
    {
      printf("SYNTAX:\tXPOS [world.wld]\n");
      return 0;
    }
  if (!fexists(args[1]))
    {
      printf("Can't find %s.\n", args[1]);
      return 1;
    }
  SafeIO worldf(args[1], "rb");
  WorldHeader wldhead;
  worldf.readb(&wldhead, sizeof(wldhead));
  int i, j;
  char str[100];
  assignVec(enlightenment, wldhead.Directional_Light_Source);
  WorldMesh* meshinfo =
    (WorldMesh*)xmalloc(sizeof(WorldMesh) * wldhead.nMeshes);
  for (i = 0; i < wldhead.nMeshes; i++)
    worldf.readb(meshinfo + i, sizeof(WorldMesh));
  char* pal;
  int x_resolution, y_resolution;
  if (!fexists("xpos.cfg"))
    {
      printf("xpos.cfg not found!\n");
      return 1;
    }
  SafeIO config("xpos.cfg", "rt");
  fscanf(config.getFILEObject(), "res: %i, %i\n",
	 &x_resolution, &y_resolution);
  fscanf(config.getFILEObject(), "hsr: %s\n", str);
  if (stricmp(str, "yes") == 0)
    do_hsr = true;
  else
    if (stricmp(str, "no") == 0)
      do_hsr = false;
    else
      {
	printf("Invalid command - %s. please enter 'yes' or 'no'
for doing hidden surface removal in xpos.cfg.\n", str);
	return 1;
      }
  fscanf(config.getFILEObject(), "lightsource_dragging: %s\n", str);
  if (stricmp(str, "yes") == 0)
    do_lsdrag = true;
  else
    if (stricmp(str, "no") == 0)
      do_lsdrag = false;
    else
      {
	printf("Invalid command - %s. please enter 'yes' or 'no' 'for Lightsource = camera's ZAxis' in xpos.cfg.\n", str);
	return 1;
      }
  bool autosaveWLD;
  fscanf(config.getFILEObject(), "autosave_the_.wld: %s\n", str);
  if (stricmp(str, "yes") == 0)
    autosaveWLD = true;
  else
    if (stricmp(str, "no") == 0)
      autosaveWLD = false;
    else
      {
	printf("Invalid command - %s. please enter 'yes' or 'no' (for automatic saving of configuration in .wld file) in xpos.cfg.\n", str);
	return 1;
      }
  fscanf(config.getFILEObject(), "shading_palette: %s\n", str);
  if (!fexists(str))
    {
      printf("Shading palette %s not found.\n", str);
      return 1;
    }
  loadpalette(str, &pal);

  config.close();

  initXPOS(wldhead.nMeshes, pal, meshinfo);
  if (!openVBE(x_resolution, y_resolution))
    handleError(ErrorMessage("Couldn't open mode (change xpos.cfg).", "", x_resolution, y_resolution));
  initCamera(wldhead); // must be done AFTER opening VBE mode!!!
  setpal(pal);
  updatecolors();
  xfree(pal);
  bool showaxes = false;
  float FOV = wldhead.CameraInitialFOV;
  float planesize = cur_cam.screen_plane_width;
  settextsize(8, 8);
  installkeyhandler();
  int frames = 0;
  int CObjFacePoolstart, CObjFacePoolend;
  while (!peekkey(KEY_ESC))
    {
      if (peekkey(KEY_A))
	immer_der_Nase_nach(-0.2F);
      if (peekkey(KEY_Y))
	immer_der_Nase_nach(0.2F);
      if (peekkey(KEY_PAD4))
	yaw(0.0174F);
      if (peekkey(KEY_PAD6))
	yaw(-0.0174F);
      if (peekkey(KEY_PAD2))
	pitch(-0.0174F);
      if (peekkey(KEY_PAD8))
	pitch(0.0174F);
      if (peekkey(KEY_PAD7))
	roll(-0.0174F);
      if (peekkey(KEY_PAD9))
	roll(0.0174F);
      if (getkey(KEY_BACK))
	{
	  negateVec(cur_cam.ZAxis);
	  createMatrices();
	}
      if (peekkey(KEY_F))
	{
	  if (getkey(KEY_PLUS) && FOV < 180.0F - 0.0174)
	    setFOV(FOV += 0.0174F);
	  if (getkey(KEY_MINUS) && FOV >= 0.0174F)
	    setFOV(FOV -= 0.0174F);
	}
      if (peekkey(KEY_P))
	{
	  if (getkey(KEY_PLUS))
	    set_screen_plane_size(planesize += 0.5);
	  if (getkey(KEY_MINUS))
	    set_screen_plane_size(planesize -= 0.5);
	}
      if (peekkey(KEY_O))
	{
	  if (getkey(KEY_PLUS))
	    {
	      if (cobj < totalObjects - 1)
		cobj++;
	    }
	  if (getkey(KEY_MINUS))
	    {
	      if (cobj > 0)
		cobj--;
	    }
	}
      if (peekkey(KEY_X))
	{
	  if (peekkey(KEY_PLUS))
	    objects[cobj].Xan += 0.0174;
	  if (peekkey(KEY_MINUS))
	    objects[cobj].Xan -= 0.0174;
	}
      if (peekkey(KEY_Y))
	{
	  if (peekkey(KEY_PLUS))
	    objects[cobj].Yan += 0.0174;
	  if (peekkey(KEY_MINUS))
	    objects[cobj].Yan -= 0.0174;
	}
      if (peekkey(KEY_Z))
	{
	  if (peekkey(KEY_PLUS))
	    objects[cobj].Zan += 0.0174;
	  if (peekkey(KEY_MINUS))
	    objects[cobj].Zan -= 0.0174;
	}
      if (peekkey(KEY_S))
	{
	  if (peekkey(KEY_PLUS))
	    scale(objects[cobj], 1.01, 1.01, 1.01);
	  if (peekkey(KEY_MINUS))
	    scale(objects[cobj], 0.99, 0.99, 0.99);
	}
      if (peekkey(KEY_N))
	{
	  if (peekkey(KEY_PLUS))
	    CLIP_NEAR += 0.1F;
	  if (peekkey(KEY_MINUS))
	    CLIP_NEAR -= 0.1F;
	}
      if (getkey(KEY_SPACE))
	showaxes = !showaxes;
      if (getkey(KEY_Q))
	saveScreen("Screen.SPR");
      if (getkey(KEY_H))
	do_hsr = !do_hsr;
      if (getkey(KEY_D))
	do_lsdrag = !do_lsdrag;
      if (getkey(KEY_W))
	saveWLDconfig(args[1], FOV, wldhead, meshinfo);
      for (i = 0; i < totalObjects; i++)
	{
	  orbit(objects[i]);
	  transform(objects[i]);
	  resetTransform(objects[i]);
	}

      transform_to_clip();// transforms Vertices to Clip-Space
//  set_clipping_flags();
      reflect_on_it();    // shading + backface-culling
      clip_and_project(); // clipping + final Clip-Space -> 2D -transformation

      prepare_face_sorting(); // transforms (visible-)face-GC's to Clip-Space

      //    quicksort_faces(0, totalvisibleFaces - 1); // sorts array of SortItems (SortingElements) "back-to-front" (according to decreasing z of Objects in Camera-Space)
      //    bubblesort_faces();
      //    simple_bubblesort_faces();
      exchangesort_faces();

      clearoffscreenmem();

      CObjFacePoolstart = objects[cobj].FacePoolStartIndex;
      CObjFacePoolend = objects[cobj].header.nFaces + CObjFacePoolstart;
      for (i = 0; i < totalvisibleFaces; i++)
	{
	  j = SortingElements[i].index;
	  switch (facepool[j].render_type)
	    {
	    case FACE_TYPE_WIREFRAME:
	      drawwireframe(facepool + j);
	      break;
	    case FACE_TYPE_FLAT:
	      drawflatpoly(facepool + j);
	      break;
	    case FACE_TYPE_GOURAUD:
	      drawgouraudpoly(facepool + j);
	      break;
	    case FACE_TYPE_TEXTURED:
	      drawlineartexturepoly(facepool + j, textures + facepool[j].texIndex);
	      break;
	    case FACE_TYPE_GOURAUD_TEXTURED:
	      drawtexturegouraudpoly(facepool + j, textures + facepool[j].texIndex);
	      break;
	    }
	  if (j >= CObjFacePoolstart && j < CObjFacePoolend && showaxes)
	    brandmarkFace(facepool + j);
	}

      if (showaxes)
	{
	  drawaxes();
	  floatstr(180.0 * objects[cobj].Xan / M_PI, str);
	  showstrxy(0, 0, "E[cobj] X=", NEAREST[CCGRAY]);
	  showstr(str, NEAREST[CCWHITE]);
	  floatstr(180.0 * objects[cobj].Yan / M_PI, str);
	  showstr(" Y=", NEAREST[CCGRAY]);
	  showstr(str, NEAREST[CCWHITE]);
	  floatstr(180.0 * objects[cobj].Zan / M_PI, str);
	  showstr(" Z=", NEAREST[CCGRAY]);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" GC[cobj] X=", NEAREST[CCGRAY]);
	  floatstr(objects[cobj].geometric_center[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Y=", NEAREST[CCGRAY]);
	  floatstr(objects[cobj].geometric_center[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Z=", NEAREST[CCGRAY]);
	  floatstr(objects[cobj].geometric_center[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("\t", 0);
	  showstrxy(0, 10, "direc.Light X=", NEAREST[CCWHITE]);
	  floatstr(enlightenment[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Y=", NEAREST[CCWHITE]);
	  floatstr(enlightenment[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Z=", NEAREST[CCWHITE]);
	  floatstr(enlightenment[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  floatstr(180.0 * FOV / M_PI, str);
	  showstr(" FOV = ", NEAREST[CCYELLOW]);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" CObject=", NEAREST[CCYELLOW]);
	  itoa(cobj, str, 10);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" AvgDia[cobj]=", NEAREST[CCYELLOW]);
	  floatstr(objects[cobj].bounding_Sphere_radius * 2.0F, str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("\t", 0);
	  showstrxy(0, 20, "CamXAxis X=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.XAxis[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Y=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.XAxis[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Z=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.XAxis[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" YAxis X=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.YAxis[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Y=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.YAxis[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Z=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.YAxis[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" ZAxis X=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.ZAxis[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Y=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.ZAxis[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("Z=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.ZAxis[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("\t", 0);
	  showstrxy(0, 30, "basecolor[0]: ", NEAREST[CCYELLOW]);
	  itoa(facepool[0].basecolor, str, 10);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" flatshade[0]: ", NEAREST[CCYELLOW]);
	  itoa(facepool[0].flatshade, str, 10);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" visible faces: ", NEAREST[CCYELLOW]);
	  itoa(totalvisibleFaces, str, 10);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" ProjectionPlaneWidth = ", NEAREST[CCYELLOW]);
	  floatstr(planesize, str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr("\t", 0);
	  showstrxy(0, 40, "CamPos X=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.pos[VEC_X], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Y=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.pos[VEC_Y], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" Z=", NEAREST[CCYELLOW]);
	  floatstr(cur_cam.pos[VEC_Z], str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstr(" near: Z = ", NEAREST[CCYELLOW]);
	  floatstr(CLIP_NEAR, str);
	  showstr(str, NEAREST[CCWHITE]);
	  showstrxy(0, 50, "", 0);
	  if (do_hsr)
	    showstr(" hsr enabled", NEAREST[CCWHITE]);
	  else
	    showstr(" hsr disabled", NEAREST[CCWHITE]);
	  if (do_lsdrag)
	    showstr(" lsdrag enabled", NEAREST[CCWHITE]);
	  else
	    showstr(" lsdrag disabled", NEAREST[CCWHITE]);
/*
	  showstr(" cobj clipping flags: ", NEAREST[CCYELLOW]);
	  if (objects[cobj].clipping_flags & CLIPPING_FLAG_LEFT)
	    showstr(" left", NEAREST[CCWHITE]);
	  if (objects[cobj].clipping_flags & CLIPPING_FLAG_RIGHT)
	    showstr(" right", NEAREST[CCWHITE]);
	  if (objects[cobj].clipping_flags & CLIPPING_FLAG_TOP)
	    showstr(" top", NEAREST[CCWHITE]);
	  if (objects[cobj].clipping_flags & CLIPPING_FLAG_BOTTOM)
	    showstr(" bottom", NEAREST[CCWHITE]);
	  if (objects[cobj].clipping_flags & CLIPPING_FLAG_NEAR)
	    showstr(" near", NEAREST[CCWHITE]);
	  itoa(objects[cobj].clipping_flags, str, 2);
	  showstr(str, NEAREST[CCYELLOW]);
	  showstr("\t", 0);
*/
	}
      flipoffscreenmem();
      frames++;
    }
  restorekeyhandler();
  if (autosaveWLD)
    saveWLDconfig(args[1], FOV, wldhead, meshinfo);
  xfree(meshinfo);
  printf("Now trashing xpos...\n");
  trashXPOS();
  printf("Now closing down VBE...\n");
  closeVBE();
  printf("\nFrames: %i. bye.\n", frames);
};
