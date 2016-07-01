#ifndef __CAMERA_H
#define __CAMERA_H

#include "vecmat.h"
#include "vbetwo.h" // for decl. of rasterpoint
#include <math.h> // 'tan'

/*
  lots of matrix-transformation stuff is taken from
  a tutorial on 3d-graphics (http://home1.gte.net/reillyp/)
  (no code, though)
  */

extern float CLIP_NEAR, CLIP_FAR; // globals

struct WorldHeader; // fwd decl.

#define  Lightsource    Vector3D

struct Camera
{
  Vector3D pos;     // in World Space
  Vector3D XAxis;   //      ""       , unit!
  Vector3D YAxis;   //      ""       , unit!
  Vector3D ZAxis;   //      ""       , unit!
  float dist;     // distance between user & screen in World Coords
  // == near Z clipping plane (ie: 1)
  float screen_plane_width, screen_plane_height;
  // not raster- more like width & height of monitor (ie: 25.0, 18.75)
  float mid_rasterX, mid_rasterY;
  Matrix4x4 WorldtoCamera, CameratoClip, WorldtoClip;
};

extern Camera cur_cam;

void createMatrices();
rasterpoint project_point(Vector3D pt, bool clip = true);
void initCamera(WorldHeader& wldhead);

void pitch(float Xan);
void yaw(float Yan);
void roll(float Zan);
void rotateinWorld(float Xan, float Yan, float Zan);

void clipline_near(point3d* E1, point3d* E2, bool* exists);
void clipline_left(point3d* E1, point3d* E2, bool* exists);
void clipline_right(point3d* E1, point3d* E2, bool* exists);
void clipline_top(point3d* E1, point3d* E2, bool* exists);
void clipline_bottom(point3d* E1, point3d* E2, bool* exists);
void draw3dline(point3d WorldE1, point3d WorldE2, unsigned char color);

inline void calc_intersect_far(point3d P1, point3d P2, point3d* Intersect)
{ // finding intersection of line with Z = CLIP_FAR
  float T = (CLIP_FAR - P1[VEC_Z]) / (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_X] = P1[VEC_X] + T * (P2[VEC_X] - P1[VEC_X]);
  (*Intersect)[VEC_Y] = P1[VEC_Y] + T * (P2[VEC_Y] - P1[VEC_Y]);
  (*Intersect)[VEC_Z] = CLIP_FAR;
};

inline void calc_intersect_near(point3d P1, point3d P2, point3d* Intersect, float* T)
{ // finding intersection of line with Z = CLIP_NEAR (usually -1)
  *T = (CLIP_NEAR - P1[VEC_Z]) / (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_X] = P1[VEC_X] + *T * (P2[VEC_X] - P1[VEC_X]);
  (*Intersect)[VEC_Y] = P1[VEC_Y] + *T * (P2[VEC_Y] - P1[VEC_Y]);
  (*Intersect)[VEC_Z] = CLIP_NEAR;
};

inline void calc_intersect_left(point3d P1, point3d P2, point3d* Intersect, float* T)
{ // finding intersection of line with X = -Z
  *T = (P1[VEC_X] + P1[VEC_Z]) / (P1[VEC_Z] - P2[VEC_Z] - P2[VEC_X] + P1[VEC_X]);
  (*Intersect)[VEC_Y] = P1[VEC_Y] + *T * (P2[VEC_Y] - P1[VEC_Y]);
  (*Intersect)[VEC_Z] = P1[VEC_Z] + *T * (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_X] = -((*Intersect)[VEC_Z]);
};

inline void calc_intersect_right(point3d P1, point3d P2, point3d* Intersect, float* T)
{ // finding intersection of line with X = Z
  *T = (P1[VEC_X] - P1[VEC_Z]) / (P2[VEC_Z] - P1[VEC_Z] - P2[VEC_X] + P1[VEC_X]);
  (*Intersect)[VEC_Y] = P1[VEC_Y] + *T * (P2[VEC_Y] - P1[VEC_Y]);
  (*Intersect)[VEC_Z] = P1[VEC_Z] + *T * (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_X] = (*Intersect)[VEC_Z];
};

inline void calc_intersect_bottom(point3d P1, point3d P2, point3d* Intersect, float* T)
{ // finding intersection of line with Y = Z
  *T = (P1[VEC_Y] - P1[VEC_Z]) / (P2[VEC_Z] - P1[VEC_Z] - P2[VEC_Y] + P1[VEC_Y]);
  (*Intersect)[VEC_X] = P1[VEC_X] + *T * (P2[VEC_X] - P1[VEC_X]);
  (*Intersect)[VEC_Z] = P1[VEC_Z] + *T * (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_Y] = (*Intersect)[VEC_Z];
};

inline void calc_intersect_top(point3d P1, point3d P2, point3d* Intersect, float* T)
{ // finding intersection of line with Y = -Z
  *T = (P1[VEC_Y] + P1[VEC_Z]) / (P1[VEC_Z] - P2[VEC_Z] - P2[VEC_Y] + P1[VEC_Y]);
  (*Intersect)[VEC_X] = P1[VEC_X] + *T * (P2[VEC_X] - P1[VEC_X]);
  (*Intersect)[VEC_Z] = P1[VEC_Z] + *T * (P2[VEC_Z] - P1[VEC_Z]);
  (*Intersect)[VEC_Y] = -((*Intersect)[VEC_Z]);
};

inline void set_screen_plane_size(float size)
{
  cur_cam.screen_plane_width = size;
  cur_cam.screen_plane_height = size * 0.75;
  createMatrices();
};

inline void immer_der_Nase_nach(float dist)
{
  cur_cam.pos[VEC_X] += cur_cam.ZAxis[VEC_X] * dist;
  cur_cam.pos[VEC_Y] += cur_cam.ZAxis[VEC_Y] * dist;
  cur_cam.pos[VEC_Z] += cur_cam.ZAxis[VEC_Z] * dist;
  createMatrices();
};

inline void setFOV(float iFOV)
{
  if (iFOV > 0.0F && iFOV < 180.0F)
    {
      cur_cam.dist = tan(iFOV / 2.0F);
      createMatrices();
    }
};

inline void slide(float dist)
{
  cur_cam.pos[VEC_X] += cur_cam.XAxis[VEC_X] * dist;
  cur_cam.pos[VEC_Y] += cur_cam.XAxis[VEC_Y] * dist;
  cur_cam.pos[VEC_Z] += cur_cam.XAxis[VEC_Z] * dist;
  createMatrices();
};

inline void vertical_men(float dist)
{
  cur_cam.pos[VEC_X] += cur_cam.YAxis[VEC_X] * dist;
  cur_cam.pos[VEC_Y] += cur_cam.YAxis[VEC_Y] * dist;
  cur_cam.pos[VEC_Z] += cur_cam.YAxis[VEC_Z] * dist;
  createMatrices();
};

inline void project_point_ClipSpace(float* ClipP, int* rasterx, int* rastery)
{// M-O-D-I-F-I-E-S ClipP, DOES N-O-T CLIP
  ClipP[VEC_X] /= ClipP[VEC_Z];
  ClipP[VEC_X] = (ClipP[VEC_X] + 1.0F) * cur_cam.mid_rasterX;
  ClipP[VEC_Y] /= ClipP[VEC_Z];
  ClipP[VEC_Y] = (ClipP[VEC_Y] + 1.0F) * cur_cam.mid_rasterY;
  *rasterx = (int)(ClipP[VEC_X]);
  *rastery = (int)(ClipP[VEC_Y]);
};

#endif
