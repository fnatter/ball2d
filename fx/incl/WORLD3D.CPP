#include "world3d.h"
#include "object3d.h" // for "warm-up"-functions
#include "palettes.h" // SHADES_PER_COLOR
#include "quitbug.h"
#include <stdlib.h> // xfree/xmalloc

const float VERTICE_SHADE_MUL_CONST = (SHADES_PER_COLOR - 1) * 1047576.0F; // this is done to convert to 12.20

static void warmup_XPOS(char* THE_pal, WorldMesh* meshinfo);

void transform_to_clip()
{
  int i;
  for (i = totalVertices - 1; i >= 0; i--)
    MulVecMat(cur_cam.WorldtoClip, VerticePool[i], ClipPool[i]);
};

void prepare_face_sorting()
{
  int i;
  for (i = totalFaces - 1, totalvisibleFaces = 0; i >= 0; i--)// for face-sorting
    {
      if (!facepool[i].culled)
	{
	  SortingElements[totalvisibleFaces].index = i;
	  MulVecMatZonly(cur_cam.WorldtoCamera, GCPool[i], &SortingElements[totalvisibleFaces].AvgZ);
	  totalvisibleFaces++;
	}
    }
};

/*
  this was an attempt to implement a test if a whole object
  is outside of view, but I quit doing this
 */

#ifndef DJ_RELEASE
void set_clipping_flags()
{
  int i;
  Vector3D gcClipSpace, temp, temp2, temp3, temp4, temp5;
  for (i = totalObjects - 1; i >= 0; i--)
    {
      objects[i].clipping_flags = 0x00;
      MulVecMat(cur_cam.WorldtoCamera, objects[i].geometric_center, temp);
      assignVec(temp2, temp);
      assignVec(temp3, temp);
      assignVec(temp4, temp);
      assignVec(temp5, temp);
      temp[VEC_X] -= objects[i].bounding_Sphere_radius;
      temp2[VEC_X] += objects[i].bounding_Sphere_radius;
      temp3[VEC_Y] -= objects[i].bounding_Sphere_radius;
      temp4[VEC_Y] += objects[i].bounding_Sphere_radius;
      temp5[VEC_Z] -= objects[i].bounding_Sphere_radius;
      MulVecMat(cur_cam.CameratoClip, temp, gcClipSpace);
      if (CLIPPED_LEFT(gcClipSpace))
	objects[i].clipping_flags |= CLIPPING_FLAG_LEFT;
      MulVecMat(cur_cam.CameratoClip, temp2, gcClipSpace);
      if (CLIPPED_RIGHT(gcClipSpace))
	objects[i].clipping_flags |= CLIPPING_FLAG_RIGHT;
      MulVecMat(cur_cam.CameratoClip, temp3, gcClipSpace);
      if (CLIPPED_TOP(gcClipSpace))
	objects[i].clipping_flags |= CLIPPING_FLAG_TOP;
      MulVecMat(cur_cam.CameratoClip, temp4, gcClipSpace);
      if (CLIPPED_BOTTOM(gcClipSpace))
	objects[i].clipping_flags |= CLIPPING_FLAG_BOTTOM;
      MulVecMat(cur_cam.CameratoClip, temp3, gcClipSpace);
      if (CLIPPED_NEAR(gcClipSpace))
	objects[i].clipping_flags |= CLIPPING_FLAG_NEAR;
    }
};
#endif

void reflect_on_it() // shading & backface-culling
{
  int i;
  float dotLight, dotCamera;
  Face* cface;
  Vector3D look, COP;
  assignVec(COP, cur_cam.pos);
  for (i = totalFaces - 1; i >= 0; i--)
    {
      cface = facepool + i;
      cface->culled = 0;
      if (!do_hsr)
	goto no_hsr;
      //      subVec(GCPool[i], COP, look);
      //      normVec(look);
      assignVec(look, cur_cam.ZAxis);
      dotCamera = dotProd(look, NormalPool[i]);
      if (dotCamera < 0.0F)
	{
	  cface->culled = 1;
	  continue;
	}
    no_hsr:
      if (do_lsdrag)
	assignVec(enlightenment, cur_cam.ZAxis);
      dotLight = dotProd(enlightenment, NormalPool[i]);
#ifdef ONLY_CLOSED_POLYHEDRA
      if (dotlight > 0.0F)
	cface->flatshade = (unsigned char)(dotLight * (SHADES_PER_COLOR - 1));
      else
	cface->flatshade = 2;
#else
      if (dotLight < 0.0F && !do_hsr)
	cface->flatshade = (unsigned char)(-dotLight * (SHADES_PER_COLOR - 1));
      else
	cface->flatshade = (unsigned char)(dotLight * (SHADES_PER_COLOR - 1));
#endif
    }
  for (i = totalVertices - 1; i >= 0; i--)
    { // i hope this is faster than including it in the previous loop (-> faster than looping through all vertices at each face)
      dotLight = dotProd(enlightenment, VerticeNormalPool[i]);
#ifdef ONLY_CLOSED_POLYHEDRA
      if (dotLight > 0.0F)
	VerticeShades[i] = (int)(VERTICE_SHADE_MUL_CONST * dotLight);
      else
	VerticeShades[i] = 2 << 20;
#else
      if (dotLight < 0.0F && !do_hsr)
	VerticeShades[i] = (int)(VERTICE_SHADE_MUL_CONST * (-dotLight));
      else
	VerticeShades[i] = (int)(VERTICE_SHADE_MUL_CONST * dotLight);
#endif
      // the VerticeShades will be assigned to the faces in clip_and_project()
    }
};

/*
  the algorithm for polygon-clipping is
  slightly taken from:
Frustrum Clipping by Tom Hammersley (tomh@globalnet.co.uk)
(I don't have a link anymore, you can search or contact him)
 */

void clippoly_near(unsigned char *nVertices, point3d* introverts, point3d* extroverts, int* shadesin, int* shadesout, int* linearuin, int* linearuout, int* linearvin, int* linearvout)
{
  int in1, in2, out;
  in1 = *nVertices - 1;
  out = 0;
  float T;
  for (in2 = 0; in2 < *nVertices; in2++)
    {// note that comments (leaving, entering) are based on perception from in1 to in2
      if (CLIPPED_NEAR(introverts[in1]))
	{
	  if (CLIPPED_NEAR(introverts[in2]))
	    {// TR
	      // don't insert anything
	    }
	  else // leaving
	    {// replace introverts[in2] with intersection
	      calc_intersect_near(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      else
	{
	  if (CLIPPED_NEAR(introverts[in2]))// entering
	    {// replace introverts[in2] with intersection
	      calc_intersect_near(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	    }
	  else
	    {// TA
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      in1 = in2;
    }
  *nVertices = out;
};

void clippoly_left(unsigned char *nVertices, point3d* introverts, point3d* extroverts, int* shadesin, int* shadesout, int* linearuin, int* linearuout, int* linearvin, int* linearvout)
{
  int in1, in2, out;
  in1 = *nVertices - 1;
  out = 0;
  float T;
  for (in2 = 0; in2 < *nVertices; in2++)
    {// note that comments (leaving, entering) are based on perception from in1 to in2
      if (CLIPPED_LEFT(introverts[in1]))
	{
	  if (CLIPPED_LEFT(introverts[in2]))
	    {// TR
	      // don't insert anything
	    }
	  else // leaving
	    {// replace introverts[in2] with intersection
	      calc_intersect_left(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      else
	{
	  if (CLIPPED_LEFT(introverts[in2]))// entering
	    {// replace introverts[in2] with intersection,
	      calc_intersect_left(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	    }
	  else
	    {// TA
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      in1 = in2;
    }
  *nVertices = out;
};

void clippoly_right(unsigned char *nVertices, point3d* introverts, point3d* extroverts, int* shadesin, int* shadesout, int* linearuin, int* linearuout, int* linearvin, int* linearvout)
{
  int in1, in2, out;
  in1 = *nVertices - 1;
  out = 0;
  float T;
  for (in2 = 0; in2 < *nVertices; in2++)
    {// note that comments (leaving, entering) are based on perception from in1 to in2
      if (CLIPPED_RIGHT(introverts[in1]))
	{
	  if (CLIPPED_RIGHT(introverts[in2]))
	    {// TR
	      // don't insert anything
	    }
	  else // leaving
	    {// replace introverts[in2] with intersection
	      calc_intersect_right(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      else
	{
	  if (CLIPPED_RIGHT(introverts[in2]))// entering
	    {// replace introverts[in2] with intersection,
	      calc_intersect_right(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	    }
	  else
	    {// TA
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      in1 = in2;
    }
  *nVertices = out;
};

void clippoly_top(unsigned char *nVertices, point3d* introverts, point3d* extroverts, int* shadesin, int* shadesout, int* linearuin, int* linearuout, int* linearvin, int* linearvout)
{
  int in1, in2, out;
  in1 = *nVertices - 1;
  out = 0;
  float T;
  for (in2 = 0; in2 < *nVertices; in2++)
    {// note that comments (leaving, entering) are based on perception from in1 to in2
      if (CLIPPED_TOP(introverts[in1]))
	{
	  if (CLIPPED_TOP(introverts[in2]))
	    {// TR
	      // don't insert anything
	    }
	  else // leaving
	    {// replace introverts[in2] with intersection
	      calc_intersect_top(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      else
	{
	  if (CLIPPED_TOP(introverts[in2]))// entering
	    {// replace introverts[in2] with intersection,
	      calc_intersect_top(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	    }
	  else
	    {// TA
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      in1 = in2;
    }
  *nVertices = out;
};

void clippoly_bottom(unsigned char *nVertices, point3d* introverts, point3d* extroverts, int* shadesin, int* shadesout, int* linearuin, int* linearuout, int* linearvin, int* linearvout)
{
  int in1, in2, out;
  in1 = *nVertices - 1;
  out = 0;
  float T;
  for (in2 = 0; in2 < *nVertices; in2++)
    {// note that comments (leaving, entering) are based on perception from in1 to in2
      if (CLIPPED_BOTTOM(introverts[in1]))
	{
	  if (CLIPPED_BOTTOM(introverts[in2]))
	    {// TR
	      // don't insert anything
	    }
	  else // leaving
	    {// replace introverts[in2] with intersection
	      calc_intersect_bottom(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      else
	{
	  if (CLIPPED_BOTTOM(introverts[in2]))// entering
	    {// replace introverts[in2] with intersection,
	      calc_intersect_bottom(introverts[in1], introverts[in2], &extroverts[out], &T);
	      shadesout[out] = shadesin[in1] + (int)(T * (shadesin[in2] - shadesin[in1]));
	      linearuout[out] = linearuin[in1] + (int)(T * (linearuin[in2] - linearuin[in1]));
	      linearvout[out] = linearvin[in1] + (int)(T * (linearvin[in2] - linearvin[in1]));
	      out++;
	    }
	  else
	    {// TA
	      assignVec(extroverts[out], introverts[in2]);
	      shadesout[out] = shadesin[in2];
	      linearuout[out] = linearuin[in2];
	      linearvout[out] = linearvin[in2];
	      out++;
	    }
	}
      in1 = in2;
    }
  *nVertices = out;
};

void clip_and_project()
{
  int i, j, k;
  Face* cface;
  point3d temp[MAX_PTS_PER_FACE];
  point3d temp2[MAX_PTS_PER_FACE];
  int shades1[MAX_PTS_PER_FACE];
  int shades2[MAX_PTS_PER_FACE];
  int linearu1[MAX_PTS_PER_FACE];
  int linearv1[MAX_PTS_PER_FACE];
  int linearu2[MAX_PTS_PER_FACE];
  int linearv2[MAX_PTS_PER_FACE];
  for (i = totalFaces - 1; i >= 0; i--)
    {
    nextone:
      cface = facepool + i;
      if (cface->culled)
	continue;
      for (j = 0; j < cface->nVertices; j++)
	{
	  k = cface->verticeindex[j];
	  assignVec(temp[j], ClipPool[k]);
	  shades1[j] = VerticeShades[k];
	  linearu1[j] = cface->texelsU[j];
	  linearv1[j] = cface->texelsV[j];
	}
      cface->nIntroVerts = cface->nVertices;
      clippoly_near(&cface->nIntroVerts, temp, temp2, shades1, shades2, linearu1, linearu2, linearv1, linearv2);
      if (cface->nIntroVerts < 3)
	{
	  cface->culled = 1;
	  i--;
	  if (i >= 0) goto nextone;
	}
      clippoly_left(&cface->nIntroVerts, temp2, temp, shades2, shades1, linearu2, linearu1, linearv2, linearv1);
      if (cface->nIntroVerts < 3)
	{
	  cface->culled = 1;
	  i--;
	  if (i >= 0) goto nextone;
	}
      clippoly_right(&cface->nIntroVerts, temp, temp2, shades1, shades2, linearu1, linearu2, linearv1, linearv2);
      if (cface->nIntroVerts < 3)
	{
	  cface->culled = 1;
	  i--;
	  if (i >= 0) goto nextone;
	}
      clippoly_top(&cface->nIntroVerts, temp2, temp, shades2, shades1, linearu2, linearu1, linearv2, linearv1);
      if (cface->nIntroVerts < 3)
	{
	  cface->culled = 1;
	  i--;
	  if (i >= 0) goto nextone;
	}
      clippoly_bottom(&cface->nIntroVerts, temp, temp2, shades1, shades2, linearu1, linearu2, linearv1, linearv2);
      if (cface->nIntroVerts < 3)
	{
	  cface->culled = 1;
	  i--;
	  if (i >= 0) goto nextone;
	}
      for (j = 0; j < cface->nIntroVerts; j++)
	{
#ifdef CLIPPING_CHECKS
	  if (CLIPPED_LEFT(temp2[j])) { // left
	    cface->culled = 1;
	    break;
	  }
	  if (CLIPPED_RIGHT(temp2[j])) { // right
	    cface->culled = 1;
	    break;
	  }
	  if (CLIPPED_TOP(temp2[j])) { // top
	    cface->culled = 1;
	    break;
	  }
	  if (CLIPPED_BOTTOM(temp2[j])) { // bottom
	    cface->culled = 1;
	    break;
	  }
	  if (CLIPPED_NEAR(temp2[j])) { // near
	    cface->culled = 1;
	    break;
	  }
#endif
	  project_point_ClipSpace(temp2[j], &cface->verts[j].x, &cface->verts[j].y);
	  if ((unsigned)cface->verts[j].x >= (unsigned)xres || (unsigned)cface->verts[j].y >= (unsigned)yres)
	    handleError(ErrorMessage("Points outside screen projected (not 3d-clipped!)", "", cface->verts[j].x, cface->verts[j].y));
	  cface->verts[j].shade = shades2[j];
	  cface->verts[j].u = linearu2[j];
	  cface->verts[j].v = linearv2[j];
	}
    }
};

void vecswap(int a[], int i, int j, int n)
{
  int temp;
  while (n-- > 0)
    {
      temp = a[i];
      a[i] = a[j];
      a[j] = temp;
      i++;
      j++;
    }
};

/*void quicksort3_faces(int a[], int n)
{
int le, lt, gt, ge, r, temp;
int v;
if (n <= 1)
      return;
le = lt = 1;
gt = ge = n - 1;
v = a[0];
for (; ;)
      {
      for (; lt <= gt && a[lt] <= v; lt++)
            if (a[lt] == v)
                  {
                  temp = a[lt];
                  a[lt] = a[le];
                  a[le] = temp;
                  le++;
                  }
      for (; lt <= gt && a[gt] >= v; gt--)
            {
            if (a[gt] == v)
                  {
                  temp = a[gt];
                  a[gt] = a[ge];
                  a[ge] = temp;
                  ge--;
                  }
            }
      if (lt > gt)
            break;
      temp = a[lt];
      a[lt] = a[gt];
      a[gt] = temp;
      lt++;
      gt--;
      }
r = le <? (lt - le);
vecswap(a, 0, lt - r, r);
r = (ge - gt) <? (n - ge - 1);
vecswap(a, lt, n - r, r);
quicksort3(a, lt - le);
quicksort3(a + n - (ge - gt), ge - gt);
};*/


void quicksort_faces(int first, int last)
{// cause GPF's when xfreeing i.e.: SortingElements!!!!!
  int pivot_index, down, up;
  float pivot;
  SortItem temp;
  pivot_index = first;
  pivot = SortingElements[pivot_index].AvgZ;
  down = first;
  up = last;
  while (down < up)
    {
      while (SortingElements[down].AvgZ <= pivot && down <= last)
	down++;
      while (SortingElements[up].AvgZ > pivot && up >= first)
	up--;
      if (up > down)
	{
	  temp = SortingElements[up];
	  SortingElements[up] = SortingElements[down];
	  SortingElements[down] = temp;
	}
    }
  temp = SortingElements[pivot_index];
  SortingElements[pivot_index] = SortingElements[up];
  SortingElements[up] = temp;
  pivot_index = up;
  if (pivot_index > first + 1)
    quicksort_faces(first, pivot_index - 1);
  if (pivot_index < last - 1)
    quicksort_faces(pivot_index + 1, last);
};

void exchangesort_faces()
{//wer ist die sch”nste im ganzen land ? (die Frauen-Version)
  int i, j;
  SortItem temp;
  for (i = 0; i < totalvisibleFaces - 1; i++)
    {
      for (j = i + 1; j < totalvisibleFaces; j++)
	{
	  if (SortingElements[i].AvgZ > SortingElements[j].AvgZ)
	    {
	      temp = SortingElements[i];
	      SortingElements[i] = SortingElements[j];
	      SortingElements[j] = temp;
	    }
	}
    }
};


void bubblesort_faces()
{//wer ist die sch”nste im ganzen land ? (die romantische version)
  int Bound, j, t;
  SortItem temp;
  Bound = totalvisibleFaces - 1;
 begin:
  t = 0;
  for (j = 0; j < Bound; j++)
    {
      if (SortingElements[j].AvgZ > SortingElements[j + 1].AvgZ)
	{
	  temp = SortingElements[j];
	  SortingElements[j] = SortingElements[j + 1];
	  SortingElements[j + 1] = temp;
	  t = j;
	}
    }
  if (t != 0)
    {
      Bound = t;
      goto begin;
    }
};

void simple_bubblesort_faces()
{
  SortItem temp;
  int i, nSwaps;
 begin:
  nSwaps = 0;
  for (i = totalvisibleFaces - 2; i >= 0; i--) // can be reversed
    {
      if (SortingElements[i].AvgZ > SortingElements[i + 1].AvgZ)
	{
	  temp = SortingElements[i];
	  SortingElements[i] = SortingElements[i + 1];
	  SortingElements[i + 1] = temp;
	  nSwaps++;
	}
    }
  if (nSwaps != 0)
    goto begin;
};

void compute_normal(Face& face, int FaceIndex)
{
  Vector3D A, B;
  subVec(VerticePool[face.verticeindex[0]], VerticePool[face.verticeindex[1]], A);
  subVec(VerticePool[face.verticeindex[0]], VerticePool[face.verticeindex[2]], B);
  crossProd(A, B, NormalPool[FaceIndex]);
  normVec(NormalPool[FaceIndex]);
};

static void warmup_XPOS(char* THE_pal, WorldMesh* meshinfo)
{
  int i, j, k;
  CLIP_NEAR = -1.0F;
  CLIP_FAR = -10000.0F;
  printf("\ninitializing Freibad (%i Vertices, %i Faces)...\n", totalVertices, totalFaces);
  printf("\ncalculating normals & face-centers...");
  for (i = 0; i < totalFaces; i++)
    {
      compute_normal(facepool[i], i);
      //	printf("\tcalculated normal of face %i", i + 1);
      compute_geometric_center(facepool[i], i);
      //	printf("\tcalculated geometric center of face %i", i + 1);
      SortingElements[i].index = i;
    }
  printf("\nnow calculating vertice-normals...");
  compute_vertice_normals();
  printf("\nnow calculating bounding spheres and geometric centers (and scaling objects)...");
  for (i = 0; i < totalObjects; i++)
    {
      //	printf("\tnow calculating the geometric center of object %i...", i + 1);
      compute_geometric_center(objects[i]);
      //	printf("\tnow calculating the bounding Sphere of object %i...", i + 1);
      compute_bounding_Sphere(objects[i]);
      resetTransform(objects[i]);
      translate(objects[i], meshinfo[i].InitialPosition[VEC_X] - objects[i].geometric_center[VEC_X], meshinfo[i].InitialPosition[VEC_Y] - objects[i].geometric_center[VEC_Y], meshinfo[i].InitialPosition[VEC_Z] - objects[i].geometric_center[VEC_Z]);
      if (meshinfo[i].NewDiameter != 0.0F)
	{
	  meshinfo[i].NewDiameter /= 2.0F * objects[i].bounding_Sphere_radius;
	  //		printf("\tnow scaling object %i by %f...", i + 1, meshinfo[i].NewDiameter);
	  scale(objects[i], meshinfo[i].NewDiameter, meshinfo[i].NewDiameter, meshinfo[i].NewDiameter);
	}
      transform(objects[i]);
      resetTransform(objects[i]);
      compute_geometric_center(objects[i]);
      compute_bounding_Sphere(objects[i]); // we don't have to, but it's better than doing it real-time
      switch (meshinfo[i].face_display_type)
	{
	case FACE_DISPLAY_SHADED:
	  if (objects[i].header.nFaces < MAX_FACES_GOURAUD)
	    k = FACE_TYPE_GOURAUD;
	  else
	    k = FACE_TYPE_FLAT;
	  break;
	case FACE_DISPLAY_FLAT:
	  k = FACE_TYPE_FLAT;
	  break;
	case FACE_DISPLAY_GOURAUD:
	  k = FACE_TYPE_GOURAUD;
	  break;
	case FACE_DISPLAY_TEXTURED:
	  k = FACE_TYPE_TEXTURED;
	  break;
	case FACE_DISPLAY_SHADED_TEXTURE:
	  k = FACE_TYPE_GOURAUD_TEXTURED;
	  break;
	case FACE_DISPLAY_WIREFRAME:
	  k = FACE_TYPE_WIREFRAME;
	  break;
	default:
	  handleError(ErrorMessage("No valid FACE_DISPLAY_TYPE specified! [initXPOS]"));
	  break;
	}
      for (j = 0; j < objects[i].header.nFaces; j++)
	facepool[objects[i].FacePoolStartIndex + j].render_type = k;
    }
  printf("\nCalculating shading lookups...\n");
  init_shadinglookups(THE_pal);
  cobj = 0;
};

void interpolate_normal(int VerticePoolIndex)
{
  // find adjacent faces, and find the mean value of all the normals
  initVec(VerticeNormalPool[VerticePoolIndex], 0.0F, 0.0F, 0.0F);
  int i, j, facesconnected = 0;
  //printf("\tfinding faces using vertice no %i...", VerticePoolIndex + 1);
  for (i = totalFaces - 1; i >= 0; i--)
    {
      for (j = facepool[i].nVertices - 1; j >= 0; j--)
	{
	  if (facepool[i].verticeindex[j] == VerticePoolIndex)
	    { // face shares our vertice, so accumulate the coordinates of the normal
	      addVec(VerticeNormalPool[VerticePoolIndex], NormalPool[i]);
	      facesconnected++; // overflow??????????
	      // we can go to the next face here...
	      break;
	    }
	}
    }
  if (facesconnected == 0)
    handleError(ErrorMessage("found lonely vertice... call 1-800-LONELY-VERTEX, visit www.lonely-vertexxx.com or edit your mesh file... ([PARAM1])", "", VerticePoolIndex + 1));
  //printf("\tnow dividing by facesconnected (%i)...", facesconnected);
  scaleVec(VerticeNormalPool[VerticePoolIndex], 1.0F / (float)facesconnected);
};

void compute_vertice_normals()
{
  int j;// find the vertice-normal for each vertice
  for (j = totalVertices - 1; j >= 0; j--)
    interpolate_normal(j);
};

void compute_geometric_center(Face& face, int FaceIndex)
{
  int i;
  initVec(GCPool[FaceIndex], 0.0F, 0.0F, 0.0F);
  for (i = face.nVertices - 1; i >= 0; i--) // overflow??????????
    addVec(GCPool[FaceIndex], VerticePool[face.verticeindex[i]]);
  scaleVec(GCPool[FaceIndex], 1.0F / (float)face.nVertices);
};

void initXPOS(int nMeshes, char* THE_pal, WorldMesh* meshinfo)
{
  totalVertices = 0; totalFaces = 0; totalTextures = 0;
  objects = (Polyhedron*)xmalloc(sizeof(Polyhedron) * MAX_OBJECTS);
  textures = (Texture*)xmalloc(sizeof(Texture) * MAX_TEXTURES);
  facepool = (Face*)xmalloc(sizeof(Face) * MAX_FACES);
  GCPool = (point3d*)xmalloc(sizeof(point3d) * MAX_FACES);
  AvgZCameraSpacePool = (float*)xmalloc(sizeof(float) * MAX_FACES);
  NormalPool = (point3d*)xmalloc(sizeof(point3d) * MAX_FACES);
  SortingElements = (SortItem*)xmalloc(sizeof(SortItem) * MAX_FACES);
  VerticeNormalPool = (point3d*)xmalloc(sizeof(point3d) * MAX_VERTICES);
  VerticePool = (point3d*)xmalloc(sizeof(point3d) * MAX_VERTICES);
  ClipPool = (point3d*)xmalloc(sizeof(point3d) * MAX_VERTICES);
  VerticeShades = (int*)xmalloc(sizeof(int) * MAX_VERTICES);
  for (totalObjects = 0; totalObjects < nMeshes; totalObjects++)
    loadXYZ(objects[totalObjects], meshinfo[totalObjects], THE_pal);
  warmup_XPOS(THE_pal, meshinfo);
};

void trashXPOS()
{
  xfree(VerticeShades);
  xfree(VerticeNormalPool);
  xfree(VerticePool);
  xfree(ClipPool);
  xfree(SortingElements);
  xfree(GCPool);
  xfree(AvgZCameraSpacePool);
  xfree(NormalPool);
  xfree(facepool);
  xfree(objects);
  int i;
  for (i = 0; i < totalTextures; i++)
    xfree(textures[i].pic);
  xfree(textures);
  try_free_shadinglookups();
};
