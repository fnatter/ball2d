#ifndef __XPOS_H
#define __XPOS_H

/* 
main header of Xposition, the 3d-system
there is no xpos.cpp-file, declarations and definitions
are split up between object3d.h/cpp, world3d.h/cpp, camera.h/cpp
and vecmat.h/cpp
*/

#include "vecmat.h"

#pragma pack(1)
#define PACKED		__attribute__ ((packed))

#define MAX_PTS_PER_FACE_BEFORE_CLIPPING              4
#define MAX_PTS_PER_FACE                              MAX_PTS_PER_FACE_BEFORE_CLIPPING + 6

#define FACE_TYPE_WIREFRAME         0
#define FACE_TYPE_FLAT              1
#define FACE_TYPE_GOURAUD           2
#define FACE_TYPE_TEXTURED          3
#define FACE_TYPE_GOURAUD_TEXTURED  4

#define CLIPPING_FLAG_LEFT              0x01
#define CLIPPING_FLAG_RIGHT             0x02
#define CLIPPING_FLAG_TOP               0x04
#define CLIPPING_FLAG_BOTTOM            0x08
#define CLIPPING_FLAG_NEAR              0x10
#define CLIPPING_FLAG_FAR               0x20

#define CLIPPED_LEFT(x)                 ((x)[VEC_X] > -(x)[VEC_Z])
#define CLIPPED_RIGHT(x)                 ((x)[VEC_X] < (x)[VEC_Z])
#define CLIPPED_TOP(x)                 ((x)[VEC_Y] > -(x)[VEC_Z])
#define CLIPPED_BOTTOM(x)                 ((x)[VEC_Y] < (x)[VEC_Z])
#define CLIPPED_NEAR(x)                 ((x)[VEC_Z] > CLIP_NEAR)
#define CLIPPED_FAR(x)                 ((x)[VEC_Z] < CLIP_FAR)

struct Scanline
{
  int left PACKED;
  int right PACKED;
  int g_shadeleft PACKED; // 12.20
  int g_shaderight PACKED;// 12.20
  int texelsU_left PACKED;// 12.20
  int texelsV_left PACKED;// 12.20
  int texelsU_right PACKED;//12.20
  int texelsV_right PACKED;//12.20
};

struct Vertex
{
  int x PACKED;
  int y PACKED;
  int shade PACKED; // !!! this must be 32-bit because it will hold a 12.20 fixed. shade!
  int u PACKED; // texture-space references, 12.20 fixed.
  int v PACKED;
};

struct Texture
{
  char* pic PACKED;
  int width_shift PACKED;
};

struct Face
{
  unsigned char nVertices PACKED;
  unsigned char nIntroVerts PACKED;
  int verticeindex[MAX_PTS_PER_FACE_BEFORE_CLIPPING] PACKED; // point indices into Vertice-, Clip- and VerticeNormal-pools
  Vertex verts[MAX_PTS_PER_FACE] PACKED;
  int texelsU[MAX_PTS_PER_FACE] PACKED;
  int texelsV[MAX_PTS_PER_FACE] PACKED;
  unsigned char basecolor PACKED;
  unsigned char flatshade PACKED;
  unsigned char texIndex PACKED;
  unsigned char render_type PACKED;
  unsigned char culled PACKED;
};

struct XYZheader
{
  char XYZid[5] PACKED; // "XYZfx"
  char ObjectName[100];
  int nVertices PACKED;
  int nFaces PACKED;
  int nTextures PACKED;
};

struct XYZTextureheader_8
{
  int picture_size PACKED; // (tex_width * tex_height) + 4
  // here follows the actual texture which is fitted to THE_PAL and is
  // picture_size size (the width being a power of 2)
};

struct XYZFaceheader
{
  char nVertices PACKED;
  unsigned char Face_basecolor_red PACKED;
  unsigned char Face_basecolor_green PACKED;
  unsigned char Face_basecolor_blue PACKED;
  char LocalTextureIndex PACKED;
  // here follows the list of local vertice-indices... (array of 16bit ints)
};

struct WorldHeader
{
  int nMeshes PACKED;
  point3d Directional_Light_Source PACKED;
  point3d CameraInitialPosition PACKED;
  point3d CameraInitialXAxis PACKED;
  point3d CameraInitialYAxis PACKED;
  point3d CameraInitialZAxis PACKED;
  float CameraInitialFOV PACKED; // 0 - 2PI
};

struct WorldMesh
{
  char szMeshfilename[13] PACKED;
  point3d InitialPosition PACKED;
  point3d CenterofRotation PACKED;
  point3d RotationalEnergy PACKED;
  float NewDiameter PACKED; // 0.0 for no scaling
  int face_display_type PACKED;
};

struct SortItem
{
  float AvgZ PACKED;
  int index PACKED;
};

#pragma pack()

struct Polyhedron
{
  XYZheader header;
  point3d* Vertices;
  point3d* VerticeNormals;
  point3d* Geometric_Centers;
  point3d* Normals;
  point3d geometric_center;
  point3d HomeSweetHome; // center of rotation
  float Xan, Yan, Zan; // 'rotational energy'
  float bounding_Sphere_radius;
  bool recalc_Sphere; // flag will be set if Object is scaled
  int FacePoolStartIndex;
  unsigned char clipping_flags;
  Matrix4x4 transforms, nonaffinetransforms;
};

#endif
