#ifndef __OBJECT3D_H
#define __OBJECT3D_H


#include "camera.h"
#include "quitbug.h"
#include "vecmat.h"
#include "world3d.h"
#include "xpos.h"
#include <stdio.h>

#define FACE_DISPLAY_WIREFRAME            0x00
#define FACE_DISPLAY_SHADED               0x01
#define FACE_DISPLAY_FLAT                 0x02 // forces flat-shading
#define FACE_DISPLAY_GOURAUD              0x03 // forces gouraud-shading
#define FACE_DISPLAY_TEXTURED             0x04
#define FACE_DISPLAY_SHADED_TEXTURE       0x05


inline void resetTransform(Polyhedron& Obj)
{
  IdentityMat(Obj.transforms);
  IdentityMat(Obj.nonaffinetransforms);
};

void orbit(Polyhedron& Obj); // rotates around 'HomeSweetHome'
void rotate(Polyhedron& Obj); // rotates around itself
void scale(Polyhedron& Obj, float scaleX, float scaleY, float scaleZ);
void translate(Polyhedron& Obj, float trlX, float trlY, float trlZ);
void transform(Polyhedron& Obj);
void loadXYZ(Polyhedron& Obj, WorldMesh& meshinfo, char* THE_pal);
void compute_geometric_center(Polyhedron& Obj);
void compute_bounding_Sphere(Polyhedron& Obj);

#endif
