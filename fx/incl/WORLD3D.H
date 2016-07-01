#ifndef WORLD3D_H
#define WORLD3D_H

#define MAX_OBJECTS           100
#define MAX_TEXTURES          MAX_OBJECTS // 128 at the most!
#define MAX_VERTICES          20000
#define MAX_FACES             40000
#define MAX_FACES_GOURAUD     200
#define MAX_TEXTURE_VERTS     4

/*
the following double-checks the 3d-clipping,
because there is often some inaccuracy which may
lead to crashes
 */
#define CLIPPING_CHECKS

#include "vecmat.h" // point3d
#include "camera.h" // Lightsource
#include "vbetwo.h" // polygon
#include "xpos.h"

extern int totalObjects, cobj, totalTextures, totalFaces, totalVertices;
extern Polyhedron* objects; // [0..totalObjects[
extern Texture* textures;   // [0..totalTextures[
extern Face* facepool;      // [0..totalFaces[
extern point3d* NormalPool;
extern point3d* GCPool;
extern float* AvgZCameraSpacePool;
extern point3d * VerticeNormalPool; // [0..totalVertices[
extern point3d * VerticePool;
extern point3d * ClipPool;
extern Lightsource enlightenment;
extern int* VerticeShades; // fixed., temporary storage for g_shades
extern bool do_hsr;
extern bool do_lsdrag;
extern SortItem* SortingElements;
extern int totalvisibleFaces;
extern float CLIP_NEAR;
extern float CLIP_FAR;

void initXPOS(int nMeshes, char* THE_pal, WorldMesh* meshinfo);
void trashXPOS();

void compute_normal(Face& face, int FaceIndex);
void compute_geometric_center(Face& face, int FaceIndex);
void interpolate_normal(int VerticePoolIndex);
void compute_vertice_normals();

void transform_to_clip();
void set_clipping_flags();
void clip_and_project();
void reflect_on_it();
void prepare_face_sorting();

void quicksort_faces(int first, int last);
void bubblesort_faces();
void simple_bubblesort_faces();
void exchangesort_faces();

void clippoly_near(unsigned char *nVertices, point3d* introverts, point3d* extroverts);
void clippoly_left(unsigned char *nVertices, point3d* introverts, point3d* extroverts);
void clippoly_right(unsigned char *nVertices, point3d* introverts, point3d* extroverts);
void clippoly_top(unsigned char *nVertices, point3d* introverts, point3d* extroverts);
void clippoly_bottom(unsigned char *nVertices, point3d* introverts, point3d* extroverts);


#endif
