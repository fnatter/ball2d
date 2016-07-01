#include "vecmat.h"

//#define TEST_VECMAT
//#define unit       10.0F

/*
  the idea of using these pointer-structures to represent
  vectors is slightly inspired from a book called
  "fast algorithms in 3d-graphics" -- the book is really crappy
  though, I only bought it because it promised to explain the
  math behind all this (which it really didn't much :-()
 */


void MulVecMat(Matrix4x4& M, float* V)
{
  float tempv[3];
  tempv[VEC_X] = M[0][0] * V[VEC_X] + M[0][1] * V[VEC_Y] + M[0][2] * V[VEC_Z] + M[0][3];
  tempv[VEC_Y] = M[1][0] * V[VEC_X] + M[1][1] * V[VEC_Y] + M[1][2] * V[VEC_Z] + M[1][3];
  tempv[VEC_Z] = M[2][0] * V[VEC_X] + M[2][1] * V[VEC_Y] + M[2][2] * V[VEC_Z] + M[2][3];
  memcpy(V, tempv, sizeof(Vector3D));
};

void MulVecMat(Matrix4x4&M, float* V, float* res)
{
  res[VEC_X] = M[0][0] * V[VEC_X] + M[0][1] * V[VEC_Y] + M[0][2] * V[VEC_Z] + M[0][3];
  res[VEC_Y] = M[1][0] * V[VEC_X] + M[1][1] * V[VEC_Y] + M[1][2] * V[VEC_Z] + M[1][3];
  res[VEC_Z] = M[2][0] * V[VEC_X] + M[2][1] * V[VEC_Y] + M[2][2] * V[VEC_Z] + M[2][3];
};

void RotationMat(Matrix4x4& M, float Xan, float Yan, float Zan)
{
  float sx = sin(Xan), sy = sin(Yan), sz = sin(Zan),
    cx = cos(Xan), cy = cos(Yan), cz = cos(Zan);
  M[0][0] = (cy * cz) + (sx * sy * sz); M[0][1] = cx * sz; M[0][2] = -(sy * cz) + (cy * sx * sz); M[0][3] = 0.0F;
  M[1][0] = -(cy * sz) + (sx * sy * cz); M[1][1] = cx * cz; M[1][2] = (sy * sz) + (cy * cz * sx); M[1][3] = 0.0F;
  M[2][0] = sy * cx; M[2][1] = -sx; M[2][2] = cy * cx; M[2][3] = 0.0F;
  M[3][0] = 0.0F; M[3][1] = 0.0F; M[3][2] = 0.0F; M[3][3] = 1.0F;
};

void combineMat(Matrix4x4& A, Matrix4x4& B, Matrix4x4& res)
{
  int i, j;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      res[i][j] = A[i][0] * B[0][j] + A[i][1] * B[1][j]
	+ A[i][2] * B[2][j] + A[i][3] * B[3][j];
};

void combineMat(Matrix4x4& A, Matrix4x4& B) //"A *= B"
{
  int i, j;
  Matrix4x4 res;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      res[i][j] = A[i][0] * B[0][j] + A[i][1] * B[1][j]
	+ A[i][2] * B[2][j] + A[i][3] * B[3][j];
  assignMat(A, res);
};

#ifdef TEST_VECMAT

#include "vbetwo.h"
#include "keyinp.h"
#include "grtext.h"
#include "camera.h"
#include "palettes.h"
#include "world3d.h"

char str[31];
float Xan, Yan, Zan, Xtransl, Ytransl, Ztransl, Xscale, Yscale, Zscale;

void showvector(const Vector3D& V, unsigned char color, const Vector3D& tail)
{
  Vector3D v;
  assignVec(v, V);
  addVec(v, tail);
  floatstr(v[VEC_X], str);
  showstr("X = ", color + 1);
  showstr(str, color);
  showstr(" Y = ", color + 1);
  floatstr(v[VEC_Y], str);
  showstr(str, color);
  showstr(" Z = ", color + 1);
  floatstr(v[VEC_Z], str);
  showstr(str, color);
  showstr(" length = ", color + 1);
  floatstr(VecMagnitude(V), str);
  showstr(str, color);
  showstr("\t\t", 0);
  draw3dline(tail, v, color);
};

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

int main()
{
  openVBE(800, 600);
  setClipRect(0, 0, xres - 1, 500);
  float FOV = M_PI * 165.0F / 180.0F;
  WorldHeader wldhead;
  wldhead.CameraInitialFOV = FOV;
  initVec(wldhead.CameraInitialXAxis, 1.0F, 0.0F, 0.0F);
  initVec(wldhead.CameraInitialYAxis, 0.0F, 1.0F, 0.0F);
  initVec(wldhead.CameraInitialZAxis, 0.0F, 0.0F, 1.0F);
  initVec(wldhead.CameraInitialPosition, 0.0F, 0.0F, 5.0F);
  initCamera(wldhead);
  installkeyhandler();
  settextsize(8);
  GRTEXT_BGColor = 0;
  char shiftp = 0, changed = 1, showaddsub = 1,
    showproducts = 1, shownormalized = 1;
  Vector3D v1 = {10.0F, 1.0F, -5.0F}, v2 = {2.0F, -3.0F, 4.0F}, resv, tailv = { 0.0F, 0.0F, 0.0F };
  Matrix4x4 transforms, tempM;
  float cinc = unit / 100.0F;
  Xan = Yan = Zan = Xtransl = Ytransl = Ztransl = 0.0F;
  Xscale = Yscale = Zscale = 1.0F;
  clearoffscreenmem();
  while (!getkey(KEY_ESC))
    {
      if (peekkey(KEY_X))
	{
	  if (peekkey(KEY_R))
	    {
	      if (getkey(KEY_PLUS))
		Xan += cinc;
	      if (getkey(KEY_MINUS))
		Xan -= cinc;
	    }
	  else
	    if (peekkey(KEY_S))
	      {
		if (getkey(KEY_PLUS))
		  Xscale += 0.25;
		if (getkey(KEY_MINUS))
		  Xscale -= 0.25;
	      }
	    else
	      if (peekkey(KEY_T))
		{
		  if (getkey(KEY_PLUS))
		    Xtransl += 1;
		  if (getkey(KEY_MINUS))
		    Xtransl -= 1;
		}
	      else	{
		if (getkey(KEY_PLUS))
		  if (shiftp) v2[VEC_X] += cinc;
		  else v1[VEC_X] += cinc;
		if (getkey(KEY_MINUS))
		  if (shiftp) v2[VEC_X] -= cinc;
		  else v1[VEC_X] -= cinc;
	      }
	}
      if (peekkey(KEY_Y))
	{
	  if (peekkey(KEY_R))
	    {
	      if (getkey(KEY_PLUS))
		Yan += cinc;
	      if (getkey(KEY_MINUS))
		Yan -= cinc;
	    }
	  if (peekkey(KEY_S))
	    {
	      if (getkey(KEY_PLUS))
		Yscale += 0.25;
	      if (getkey(KEY_MINUS))
		Yscale -= 0.25;
	    }
	  else if (peekkey(KEY_T))
	    {
	      if (getkey(KEY_PLUS))
		Ytransl += 1;
	      if (getkey(KEY_MINUS))
		Ytransl -= 1;
	    }
	  else	{
	    if (getkey(KEY_PLUS))
	      if (shiftp) v2[VEC_Y] += cinc;
	      else v1[VEC_Y] += cinc;
	    if (getkey(KEY_MINUS))
	      if (shiftp) v2[VEC_Y] -= cinc;
	      else v1[VEC_Y] -= cinc;
	  }
	}
      if (peekkey(KEY_Z))
	{
	  if (peekkey(KEY_R))
	    {
	      if (getkey(KEY_PLUS))
		Zan += cinc;
	      if (getkey(KEY_MINUS))
		Zan -= cinc;
	    }
	  else
	    if (peekkey(KEY_S))
	      {
		if (getkey(KEY_PLUS))
		  Zscale += 0.25;
		if (getkey(KEY_MINUS))
		  Zscale -= 0.25;
	      }
	    else
	      if (peekkey(KEY_T))
		{
		  if (getkey(KEY_PLUS))
		    Ztransl += 1;
		  if (getkey(KEY_MINUS))
		    Ztransl -= 1;
		}
	      else	{
		if (getkey(KEY_PLUS))
		  if (shiftp) v2[VEC_Z] += cinc;
		  else v1[VEC_Z] += cinc;
		if (getkey(KEY_MINUS))
		  if (shiftp) v2[VEC_Z] -= cinc;
		  else v1[VEC_Z] -= cinc;
	      }
	}
      if (getkey(KEY_D))
	{
	  TranslationMat(transforms, Xtransl, Ytransl, Ztransl);
	  RotationMat(tempM, Xan, Yan, Zan);
	  combineMat(transforms, tempM);
	  ScalingMat(tempM, Xscale, Yscale, Zscale);
	  combineMat(transforms, tempM);
	  MulVecMat(transforms, v1);
	  MulVecMat(transforms, v2);
	}
      if (getkey(KEY_A))
	showaddsub ^= 1;
      if (getkey(KEY_P))
	showproducts ^= 1;
      if (getkey(KEY_N))
	shownormalized ^= 1;
      if (peekkey(KEY_F))
	immer_der_Nase_nach(-0.5);
      if (peekkey(KEY_B))
	immer_der_Nase_nach(0.5);
      if (peekkey(KEY_PAD4))
	if (peekkey(KEY_RSHIFT))
	  slide(-1.0F);
	else
	  yaw(0.0174);
      if (peekkey(KEY_PAD6))
	if (peekkey(KEY_RSHIFT))
	  slide(1.0F);
	else
	  yaw(-0.0174);
      if (peekkey(KEY_PAD2))
	if (peekkey(KEY_RSHIFT))
	  vertical_men(1.0F);
	else
	  pitch(-0.0174);
      if (peekkey(KEY_PAD8))
	if (peekkey(KEY_RSHIFT))
	  vertical_men(-1.0F);
	else
	  pitch(0.0174);
      if (getkey(KEY_BACK))
	{
	  negateVec(cur_cam.ZAxis);
	  createMatrices();
	}
      //	showtransforms();
      drawaxes();
      showstrxy(0, yres - 120, "V1 = ", NEAREST[CCRED]);
      showvector(v1, NEAREST[CCRED], tailv);
      showstrxy(0, yres - 110, "V2 = ", NEAREST[CCGREEN]);
      showvector(v2, NEAREST[CCGREEN], tailv);
      if (showaddsub)
	{
	  showstrxy(0, yres - 100, "V1 + V2 = ", NEAREST[CCYELLOW]);
	  addVec(v1, v2, resv);
	  showvector(resv, NEAREST[CCYELLOW], tailv);
	  showstrxy(0, yres - 90, "V1 - V2 = ", NEAREST[CCORANGE]);
	  subVec(v1, v2, resv);
	  showvector(resv, NEAREST[CCORANGE], v2);
	  showstrxy(0, yres - 80, "V2 - V1 = ", NEAREST[CCVIOLET]);
	  subVec(v2, v1, resv);
	  showvector(resv, NEAREST[CCVIOLET], v1);
	}
      if (showproducts)
	{
	  showstrxy(0, yres - 70, "V1 x V2 = ", NEAREST[CCCYAN]);
	  crossProd(v1, v2, resv);
	  showvector(resv, NEAREST[CCCYAN], tailv);
	  showstrxy(0, yres - 60, "V2 x V1 = ", NEAREST[CCBROWN]);
	  crossProd(v2, v1, resv);
	  showvector(resv, NEAREST[CCBROWN], tailv);
	  showstrxy(0, yres - 50, "V1 . V2 = ", NEAREST[CCWHITE]);
	  floatstr(dotProd(v1, v2), str);
	  showstr(str, NEAREST[CCWHITE]);
	}
      if (shownormalized)
	{
	  showstrxy(0, yres - 40, "V1 normalized = ", NEAREST[CCDARKRED]);
	  assignVec(resv, v1);
	  normVec(resv);
	  showvector(resv, NEAREST[CCDARKRED], tailv);
	  showstrxy(0, yres - 30, "V2 normalized = ", NEAREST[CCDARKGREEN]);
	  assignVec(resv, v2);
	  normVec(resv);
	  showvector(resv, NEAREST[CCDARKGREEN], tailv);
	}
      shiftp = (peekkey(KEY_LSHIFT) || peekkey(KEY_RSHIFT));
      flipoffscreenmem();
      clearoffscreenmem();
    }
  closeVBE();
  restorekeyhandler();
  return 0;
};

#endif


