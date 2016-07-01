#include "camera.h"
#include "vbetwo.h" // for xres/yres
#include "world3d.h"
#include "quitbug.h"

#include <stdio.h> // fprintf/fopen

/*
  most of the projection and clipping-space stuff using
  matrices is taken from a basic 3d-tutorial by Patrich Reilly
  which you can find here:
  http://home1.gte.net/reillyp/
  (it's the best I've ever found)
  I didn't use any code, but I did use each of the Matrices,
  (because I didn't understand all that's behind it)
  */

void pitch(float Xan)
{
  Matrix4x4 M;
  RotationMat(M, cur_cam.XAxis[VEC_X] * Xan,
	      cur_cam.XAxis[VEC_Y] * Xan, cur_cam.XAxis[VEC_Z] * Xan);
  MulVecMat(M, cur_cam.XAxis);
  MulVecMat(M, cur_cam.YAxis);
  MulVecMat(M, cur_cam.ZAxis);
  createMatrices();
};

void yaw(float Yan)
{
  Matrix4x4 M;
  RotationMat(M, cur_cam.YAxis[VEC_X] * Yan, cur_cam.YAxis[VEC_Y] * Yan, cur_cam.YAxis[VEC_Z] * Yan);
  MulVecMat(M, cur_cam.XAxis);
  MulVecMat(M, cur_cam.YAxis);
  MulVecMat(M, cur_cam.ZAxis);
  createMatrices();
};

void roll(float Zan)
{
  Matrix4x4 M;
  RotationMat(M, cur_cam.ZAxis[VEC_X] * Zan, cur_cam.ZAxis[VEC_Y] * Zan, cur_cam.ZAxis[VEC_Z] * Zan);
  MulVecMat(M, cur_cam.XAxis);
  MulVecMat(M, cur_cam.YAxis);
  MulVecMat(M, cur_cam.ZAxis);
  createMatrices();
};

void rotateinWorld(float Xan, float Yan, float Zan)
{// rotates and orients cur_camera's +Z pointing to the origin of the world system
  // any idea ?
};

void initCamera(WorldHeader& wldhead)
{
  cur_cam.dist = tan(wldhead.CameraInitialFOV / 2.0F); // can't use setFOV because it calls createMatrices!
  assignVec(cur_cam.XAxis, wldhead.CameraInitialXAxis);
  assignVec(cur_cam.YAxis, wldhead.CameraInitialYAxis);
  assignVec(cur_cam.ZAxis, wldhead.CameraInitialZAxis);
  assignVec(cur_cam.pos, wldhead.CameraInitialPosition);
  cur_cam.mid_rasterX = (xres - 0.5F) / 2.0F;
  cur_cam.mid_rasterY = (yres - 0.5F) / 2.0F;
  cur_cam.screen_plane_width = 25.0F;
  cur_cam.screen_plane_height = cur_cam.screen_plane_width * 0.75F;
  createMatrices();
};

void createMatrices()
{
  IdentityMat(cur_cam.WorldtoCamera);
  cur_cam.WorldtoCamera[0][0] = cur_cam.XAxis[VEC_X];
  cur_cam.WorldtoCamera[0][1] = cur_cam.XAxis[VEC_Y];
  cur_cam.WorldtoCamera[0][2] = cur_cam.XAxis[VEC_Z];
  cur_cam.WorldtoCamera[0][3] = -dotProd(cur_cam.pos, cur_cam.XAxis);
  cur_cam.WorldtoCamera[1][0] = cur_cam.YAxis[VEC_X];
  cur_cam.WorldtoCamera[1][1] = cur_cam.YAxis[VEC_Y];
  cur_cam.WorldtoCamera[1][2] = cur_cam.YAxis[VEC_Z];
  cur_cam.WorldtoCamera[1][3] = -dotProd(cur_cam.pos, cur_cam.YAxis);
  cur_cam.WorldtoCamera[2][0] = cur_cam.ZAxis[VEC_X];
  cur_cam.WorldtoCamera[2][1] = cur_cam.ZAxis[VEC_Y];
  cur_cam.WorldtoCamera[2][2] = cur_cam.ZAxis[VEC_Z];
  cur_cam.WorldtoCamera[2][3] = -dotProd(cur_cam.pos, cur_cam.ZAxis);
  IdentityMat(cur_cam.CameratoClip);
  cur_cam.CameratoClip[0][0] = 2.0F / cur_cam.screen_plane_width;
  cur_cam.CameratoClip[1][1] = 2.0F / cur_cam.screen_plane_height;
  cur_cam.CameratoClip[2][2] = 1.0F / cur_cam.dist;
  combineMat(cur_cam.WorldtoCamera, cur_cam.CameratoClip, cur_cam.WorldtoClip);
};

rasterpoint project_point(Vector3D Worldpt, bool clip)
{
  rasterpoint screen;
  Vector3D ClipP;
  MulVecMat(cur_cam.WorldtoClip, Worldpt, ClipP);
  if ((CLIPPED_LEFT(ClipP) || CLIPPED_RIGHT(ClipP) || CLIPPED_TOP(ClipP) || CLIPPED_BOTTOM(ClipP) || CLIPPED_NEAR(ClipP)) && clip)
    {
      screen.x = 0;
      screen.y = 0;
      return screen;
    }
  project_point_ClipSpace(ClipP, &screen.x, &screen.y);
  return screen;
};

void clipline_near(point3d* E1, point3d* E2, bool* exists)
{// clipping against Z = -1
  point3d P1, P2;
  assignVec(P1, *E1);
  assignVec(P2, *E2);
  float T;
  if (CLIPPED_NEAR(P1)) // P1 is invisible
    {
      if (CLIPPED_NEAR(P2)) // P2 is invisible
	{
	  *exists = false;
	  return;
	}
      else   // P1 is invisible, P2 is visible
	calc_intersect_near(P1, P2, E1, &T); // replace E1 with intersection
    }
  else
    {  // P1 is visible
      if (CLIPPED_NEAR(P2)) // P2 is invisible
	calc_intersect_near(P1, P2, E2, &T); // replace E2 with intersection
    }
  *exists = true;
};

void clipline_left(point3d* E1, point3d* E2, bool* exists)
{// clipping against X = -Z
  point3d P1, P2;
  assignVec(P1, *E1);
  assignVec(P2, *E2);
  float T;
  if (CLIPPED_LEFT(P1)) // P1 is invisible
    {
      if (CLIPPED_LEFT(P2)) // P2 is invisible
	{
	  *exists = false;
	  return;
	}
      else   // P1 is invisible, P2 is visible
	calc_intersect_left(P1, P2, E1, &T); // replace E1 with intersection
    }
  else
    {  // P1 is visible
      if (CLIPPED_LEFT(P2)) // P2 is invisible
	calc_intersect_left(P1, P2, E2, &T); // replace E2 with intersection
    }
  *exists = true;
};

void clipline_right(point3d* E1, point3d* E2, bool* exists)
{// clipping against X = Z
  point3d P1, P2;
  assignVec(P1, *E1);
  assignVec(P2, *E2);
  float T;
  if (CLIPPED_RIGHT(P1)) // P1 is invisible
    {
      if (CLIPPED_RIGHT(P2)) // P2 is invisible
	{
	  *exists = false;
	  return;
	}
      else   // P1 is invisible, P2 is visible
	calc_intersect_right(P1, P2, E1, &T); // replace E1 with intersection
    }
  else
    {  // P1 is visible
      if (CLIPPED_RIGHT(P2)) // P2 is invisible
	calc_intersect_right(P1, P2, E2, &T); // replace E2 with intersection
    }
  *exists = true;
};

void clipline_top(point3d* E1, point3d* E2, bool* exists)
{// clipping against Y = -Z
  point3d P1, P2;
  P1 = *E1;
  P2 = *E2;
  float T;
  if (CLIPPED_TOP(P1)) // P1 is invisible
    {
      if (CLIPPED_TOP(P2)) // P2 is invisible
	{
	  *exists = false;
	  return;
	}
      else   // P1 is invisible, P2 is visible
	calc_intersect_top(P1, P2, E1, &T); // replace E1 with intersection
    }
  else
    {  // P1 is visible
      if (CLIPPED_TOP(P2)) // P2 is invisible
	calc_intersect_top(P1, P2, E2, &T); // replace E2 with intersection
    }
  *exists = true;
};

void clipline_bottom(point3d* E1, point3d* E2, bool* exists)
{// clipping against Y = Z
  point3d P1, P2;
  assignVec(P1, *E1);
  assignVec(P2, *E2);
  float T;
  if (CLIPPED_BOTTOM(P1)) // P1 is invisible
    {
      if (CLIPPED_BOTTOM(P2)) // P2 is invisible
	{
	  *exists = false;
	  return;
	}
      else   // P1 is invisible, P2 is visible
	calc_intersect_bottom(P1, P2, E1, &T); // replace E1 with intersection
    }
  else
    {  // P1 is visible
      if (CLIPPED_BOTTOM(P2)) // P2 is invisible
	calc_intersect_bottom(P1, P2, E2, &T); // replace E2 with intersection
    }
  *exists = true;
};

void draw3dline(point3d WorldE1, point3d WorldE2, unsigned char color)
{
  bool exists;
  point3d E1, E2;

  MulVecMat(cur_cam.WorldtoClip, WorldE1, E1);
  MulVecMat(cur_cam.WorldtoClip, WorldE2, E2);

  clipline_near(&E1, &E2, &exists);
  if (!exists)
    return;
  clipline_left(&E1, &E2, &exists);
  if (!exists)
    return;
  clipline_right(&E1, &E2, &exists);
  if (!exists)
    return;
  clipline_top(&E1, &E2, &exists);
  if (!exists)
    return;
  clipline_bottom(&E1, &E2, &exists);
  if (!exists)
    return;
  int x1, x2, y1, y2;

#ifdef CLIPPING_CHECKS
  if (CLIPPED_LEFT(E1) || CLIPPED_LEFT(E2)) {
    fprintf(stderr, "E1 = (%10.5e; %10.5e; %10.5e)\n",(double)E1[VEC_X],
	    (double)E1[VEC_Y], (double)E1[VEC_Z]);
    fprintf(stderr, "E2 = (%10.5e; %10.5e; %10.5e)\n",(double)E2[VEC_X],
	    (double)E2[VEC_Y], (double)E2[VEC_Z]);
    if (CLIPPED_LEFT(E1))
      fprintf(stderr, "it's E1!\n");
    //    handleError(ErrorMessage("points not clipped left"));
  }
  if (CLIPPED_RIGHT(E1) || CLIPPED_RIGHT(E2))
    handleError(ErrorMessage("points not clipped right"));
  if (CLIPPED_TOP(E1) || CLIPPED_TOP(E2))
    handleError(ErrorMessage("points not clipped top"));
  if (CLIPPED_BOTTOM(E1) || CLIPPED_BOTTOM(E2))
    handleError(ErrorMessage("points not clipped bottom"));
  if (CLIPPED_NEAR(E1) || CLIPPED_NEAR(E2))
    handleError(ErrorMessage("points not clipped near"));
#endif

  project_point_ClipSpace(E1, &x1, &y1);
  project_point_ClipSpace(E2, &x2, &y2);


#ifdef CLIPPING_CHECKS
  if (x1 < 0 || x2 < 0)
    handleError(ErrorMessage("line clipping doesn't work: x < 0 (X1, X2)",
			     "", x1, x2));
  if (y1 < 0 || y2 < 0)
    handleError(ErrorMessage("line clipping doesn't work: y < 0 (Y1, Y2)",
			     "", y1, y2));
    if (x1 >= xres || x2 >= xres)
      handleError(ErrorMessage("line clipping doesn't work: x >= xres (X1, X2)"
			       , "", x1, x2));
    if (y1 >= yres || y2 >= yres)
      handleError(ErrorMessage("line clipping doesn't work: y >= yres (Y1, Y2)"
			       , "", y1, y2));
#endif
    line(x1, y1, x2, y2, color);
};
