#include "object3d.h"
#include "vbetwo.h" // because of the functions around the 'shadinglookies'
#include "vecmat.h"
#include "fileio.h"
#include "palettes.h"
#include "gformats.h"
#include "quitbug.h"
#include <stdlib.h> // xmalloc/xfree

void compute_geometric_center(Polyhedron& Obj)
{
  initVec(Obj.geometric_center, 0.0F, 0.0F, 0.0F);
  int i;
  for (i = 0; i < Obj.header.nFaces; i++) // overflow ???
    addVec(Obj.geometric_center, Obj.Geometric_Centers[i]);
  scaleVec(Obj.geometric_center, 1.0F / (float)Obj.header.nFaces);
};

void compute_bounding_Sphere(Polyhedron& Obj)
{
  int i;
  Vector3D dist;
  float len;
  Obj.bounding_Sphere_radius = 0.0F;
  for (i = 0; i < Obj.header.nVertices; i++)
    {
      subVec(Obj.geometric_center, Obj.Vertices[i], dist);
      len = VecMagnitude(dist);
      if (len > Obj.bounding_Sphere_radius)
	Obj.bounding_Sphere_radius = len;
    }
  Obj.recalc_Sphere = false;
};

void translate(Polyhedron& Obj, float trlX, float trlY, float trlZ)
{
  Matrix4x4 T;
  TranslationMat(T, trlX, trlY, trlZ);
  combineMat(Obj.transforms, T);
};

void scale(Polyhedron& Obj, float scaleX, float scaleY, float scaleZ)
{
  Matrix4x4 temp;
  TranslationMat(temp, Obj.geometric_center[VEC_X], Obj.geometric_center[VEC_Y], Obj.geometric_center[VEC_Z]);
  combineMat(Obj.transforms, temp);
  ScalingMat(temp, scaleX, scaleY, scaleZ);
  combineMat(Obj.transforms, temp);
  TranslationMat(temp, -Obj.geometric_center[VEC_X], -Obj.geometric_center[VEC_Y], -Obj.geometric_center[VEC_Z]);
  combineMat(Obj.transforms, temp);
  Obj.recalc_Sphere = true;
};

void orbit(Polyhedron& Obj) // homerun
{
  Matrix4x4 temp;
  TranslationMat(temp, -Obj.HomeSweetHome[VEC_X], -Obj.HomeSweetHome[VEC_Y], -Obj.HomeSweetHome[VEC_Z]);
  combineMat(Obj.transforms, temp);
  RotationMat(temp, Obj.Xan, Obj.Yan, Obj.Zan);
  combineMat(Obj.transforms, temp);
  combineMat(Obj.nonaffinetransforms, temp);
  TranslationMat(temp, Obj.HomeSweetHome[VEC_X], Obj.HomeSweetHome[VEC_Y], Obj.HomeSweetHome[VEC_Z]);
  combineMat(Obj.transforms, temp);
};

void rotate(Polyhedron& Obj)
{
  Matrix4x4 temp;
  TranslationMat(temp, -Obj.geometric_center[VEC_X], -Obj.geometric_center[VEC_Y], -Obj.geometric_center[VEC_Z]);
  combineMat(Obj.transforms, temp);
  RotationMat(temp, Obj.Xan, Obj.Yan, Obj.Zan);
  combineMat(Obj.transforms, temp);
  combineMat(Obj.nonaffinetransforms, temp);
  TranslationMat(temp, Obj.geometric_center[VEC_X], Obj.geometric_center[VEC_Y], Obj.geometric_center[VEC_Z]);
  combineMat(Obj.transforms, temp);
};

void transform(Polyhedron& Obj)
{
  int i;
  for (i = 0; i < Obj.header.nVertices; i++)
    {
      MulVecMat(Obj.transforms, Obj.Vertices[i]);
      MulVecMat(Obj.nonaffinetransforms, Obj.VerticeNormals[i]);
    }
  for (i = 0; i < Obj.header.nFaces; i++)
    {
      MulVecMat(Obj.transforms, Obj.Geometric_Centers[i]);
      MulVecMat(Obj.nonaffinetransforms, Obj.Normals[i]);
    }
  MulVecMat(Obj.transforms, Obj.geometric_center);
  if (Obj.recalc_Sphere)
    compute_bounding_Sphere(Obj);
};

void loadXYZ(Polyhedron& Obj, WorldMesh& meshinfo, char* THE_pal)
{
  if (!fexists(meshinfo.szMeshfilename))
    handleError(ErrorMessage("Couldn't open mesh-file.", meshinfo.szMeshfilename));
  SafeIO mesh(meshinfo.szMeshfilename, "rb");
  mesh.read(&Obj.header, sizeof(Obj.header), 1);
  if (Obj.header.XYZid[0] != 'X' || Obj.header.XYZid[1] != 'Y' || Obj.header.XYZid[2] != 'Z'
      || Obj.header.XYZid[3] != 'f' || Obj.header.XYZid[4] != 'x')
    handleError(ErrorMessage("Mesh file doesn't have a valid XYZ-id.", meshinfo.szMeshfilename));
  if (totalObjects + 1 > MAX_OBJECTS)
    handleError(ErrorMessage("Can't allocate [PARAM1]th object (MAX_OBJECTS = [PARAM2])", meshinfo.szMeshfilename, totalObjects + 1, MAX_OBJECTS));
  if (totalVertices + Obj.header.nVertices > MAX_VERTICES)
    handleError(ErrorMessage("Can't allocate [PARAM1]th object (too many vertices)", meshinfo.szMeshfilename, totalObjects + 1, totalVertices + Obj.header.nVertices));
  if (totalFaces + Obj.header.nFaces > MAX_FACES)
    handleError(ErrorMessage("Can't allocate [PARAM1]th object (too many faces)", meshinfo.szMeshfilename, totalObjects + 1, totalFaces + Obj.header.nFaces));
  if (totalTextures + Obj.header.nTextures > MAX_TEXTURES)
    handleError(ErrorMessage("Can't allocate [PARAM1]th object (too many textures)", meshinfo.szMeshfilename, totalObjects + 1, totalTextures + Obj.header.nTextures));
  printf("\nReading mesh %s (%s)...\n", Obj.header.ObjectName, meshinfo.szMeshfilename);
  char texturebeginindex = totalTextures; // will be added to the textureindex of each face
  int i, j;
  XYZTextureheader_8 texhead;
  for (i = 0; i < Obj.header.nTextures; i++)
    {
      mesh.readb(&texhead, sizeof(texhead));
      textures[totalTextures].pic = (char*)xmalloc(texhead.picture_size);
      mesh.readb(textures[totalTextures].pic, texhead.picture_size);
      j = 0;
      while ((1 << j) != BITMAP_WIDTH(textures[totalTextures].pic))
	j++;
      textures[totalTextures].width_shift = j;
      totalTextures++;
    }
  assignVec(Obj.HomeSweetHome, meshinfo.CenterofRotation);
  Obj.Xan = meshinfo.RotationalEnergy[VEC_X];
  Obj.Yan = meshinfo.RotationalEnergy[VEC_Y];
  Obj.Zan = meshinfo.RotationalEnergy[VEC_Z];
  Obj.Vertices = VerticePool + totalVertices;
  Obj.VerticeNormals = VerticeNormalPool + totalVertices;
  Obj.Normals = NormalPool + totalFaces;
  Obj.Geometric_Centers = GCPool + totalFaces;
  Obj.FacePoolStartIndex = totalFaces;
  const int beginIndex = totalVertices; // will be added to the *local* face-vertice-indices of the mesh to get some global index
  for (i = 0; i < Obj.header.nVertices; i++, totalVertices++)
    {
      //	printf("\nnow reading vertice %i", i);
      mesh.read(VerticePool + totalVertices, sizeof(point3d), 1);
    }
  short indices[MAX_PTS_PER_FACE_BEFORE_CLIPPING];
  XYZFaceheader facehead;
  for (i = 0; i < Obj.header.nFaces; i++, totalFaces++)
    {
      //	printf("\nnow reading face %i", i);
      mesh.readb(&facehead, sizeof(facehead));
      if (facehead.nVertices > MAX_PTS_PER_FACE_BEFORE_CLIPPING)
	handleError(ErrorMessage("Face #[PARAM1] has too many edges [PARAM2]", meshinfo.szMeshfilename, totalFaces, facehead.nVertices));
      facepool[totalFaces].nVertices = facehead.nVertices;
      mesh.read(indices, sizeof(short), facepool[totalFaces].nVertices);
      for (j = 0; j < facepool[totalFaces].nVertices; j++)
	facepool[totalFaces].verticeindex[j] = indices[j] + beginIndex;
      facepool[totalFaces].basecolor = findnearestbasecolor_8(THE_pal, facehead.Face_basecolor_red, facehead.Face_basecolor_green, facehead.Face_basecolor_blue);
      facepool[totalFaces].texIndex = facehead.LocalTextureIndex + texturebeginindex;
      switch(facepool[totalFaces].nVertices)
	{
	case 3:
	  facepool[totalFaces].texelsU[0] = 0;
	  facepool[totalFaces].texelsV[0] = 0;
	  facepool[totalFaces].texelsU[1] = (BITMAP_WIDTH(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  facepool[totalFaces].texelsV[1] = 0;
	  facepool[totalFaces].texelsU[2] = (BITMAP_WIDTH(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  facepool[totalFaces].texelsV[2] = (BITMAP_HEIGHT(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  break;
	case 4:
	  facepool[totalFaces].texelsU[0] = 0;
	  facepool[totalFaces].texelsV[0] = 0;
	  facepool[totalFaces].texelsU[1] = (BITMAP_WIDTH(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  facepool[totalFaces].texelsV[1] = 0;
	  facepool[totalFaces].texelsU[2] = (BITMAP_WIDTH(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  facepool[totalFaces].texelsV[2] = (BITMAP_HEIGHT(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  facepool[totalFaces].texelsU[3] = 0;
	  facepool[totalFaces].texelsV[3] = (BITMAP_HEIGHT(textures[facepool[totalFaces].texIndex].pic) - 1) << 20;
	  break;
	default:
	  if ((meshinfo.face_display_type == FACE_DISPLAY_SHADED_TEXTURE || meshinfo.face_display_type == FACE_DISPLAY_TEXTURED) && (facepool[totalFaces].nVertices > MAX_TEXTURE_VERTS))
	    handleError(ErrorMessage("Some textured face [#PARAM1] has too many vertices: [PARAM2]", meshinfo.szMeshfilename, totalFaces - Obj.FacePoolStartIndex, facepool[totalFaces].nVertices));
	  break;
	}
    }
  mesh.close();
};
