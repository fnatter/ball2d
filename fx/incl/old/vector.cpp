//#define TEST_VECTORS

#include "vector.h"
//#include "figures.def"
#include "vbetwo.h"
#include "palettes.h"
#include "object3d.h"
#include "grtext.h"

rect_t makerect(float angle, float radius)
{
  rect_t result;
  result.x = ( cos (angle) * radius );
  result.y = ( sin (angle) * radius );
  return result;
};

rect_t makerect(polar_t pol)
{
  return makerect(pol.angle, pol.radius);
};

polar_t makepolar (float x, float y)
{
  //x = 0.0,                               ####|####
  //	y > 0.0 => ret 90                  #II#|##I#
  //	y < 0.0 => ret 270                 ####|####
  //	y == 0.0 => angle not defined      ----+----
  //x > 0.0,                               ####|####
  //	y < 0.0 => add 360 to tan-result   #III|#IV#
  //	y >= 0.0 => no add-on              ####|####
  //x < 0.0
  //	add 180 to tan- result
  //Quadr II,III adjusted,
  //I,IV directly from tangent
  polar_t result;
  result.radius = sqrt ( (x * x) + (y * y) );
  if (x == 0.0) result.angle = ((y > 0.0) ? (M_PI_2) : (3.0 * M_PI_2));
  else
    {
      result.angle = atan (y / x);
      if (x < 0.0) result.angle += M_PI;
      else if (y < 0.0) result.angle += M_PI * 2;
    }
  int rnd1 = (int)(x + 0.5F),
    rnd2 = (int)(y + 0.5F);
  if (rnd1 == 0 && rnd2 == 0) result.angle = 0; // NOT DEFINED!!!!!
  return result;
};

polar_t makepolar(rect_t rect)
{
  return makepolar(rect.x, rect.y);
};


#ifdef TEST_VECTORS

#include "vbetwo.h"
#include "keyinp.h"
#include "camera.h"

float Xan, Yan, Zan, Xscale, Yscale, Zscale, Xtransl, Ytransl, Ztransl;
char str[20];

void showtransformsgm()
{
  floatstr(Xan, str, 2);
  showstrxy(0, yres - 20, "Transforms: Xan = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Yan, str);
  showstr(" Yan = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Zan, str);
  showstr(" Zan = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Xscale, str);
  showstr("  Xscale = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Yscale, str);
  showstr(" Yscale = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Zscale, str);
  showstr(" Zscale = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Xtransl, str);
  showstr("  Xtransl = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Ytransl, str);
  showstr(" Ytransl = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  floatstr(Ztransl, str);
  showstr(" Ztransl = ", NEAREST[CCORANGE]);
  showstr(str, NEAREST[CCYELLOW]);
  showstr("\t\t", 0);
};

const float unit = 10.0;

void showvectorgm(const Vector& V, Camera& cam, unsigned char color, Vector tail = Vector(0.0F, 0.0F, 0.0F))
{
  Vector v(V + tail);
  if (v.x > unit) v.x = unit;
  if (v.x < -unit) v.x = -unit;
  if (v.y > unit) v.y = unit;
  if (v.y < -unit) v.y = -unit;
  if (v.z > unit) v.z = unit;
  if (v.z < -unit) v.z = -unit;
  floatstr(v.x, str);
  showstr("X = ", color + 1);
  showstr(str, color);
  showstr(" Y = ", color + 1);
  floatstr(v.y, str);
  showstr(str, color);
  showstr(" Z = ", color + 1);
  floatstr(v.z, str);
  showstr(str, color);
  showstr(" length = ", color + 1);
  floatstr((V - tail).length(), str);
  showstr(str, color);
  showstr("\t\t", 0);
  draw3dline(cam, makeP3D(tail.x, tail.y, tail.z), makeP3D(v.x, v.y, v.z), color);
};

void drawaxes(Camera& cam)
{
  draw3dline(cam, makeP3D(-unit, 0.0F, 0.0F), makeP3D(unit, 0.0F, 0.0F), NEAREST[CCWHITE]);
  draw3dline(cam, makeP3D(0.0F, -unit, 0.0F), makeP3D(0.0F, unit, 0.0F), NEAREST[CCGRAY]);
  draw3dline(cam, makeP3D(0.0F, 0.0F, -unit), makeP3D(0.0F, 0.0F, unit), NEAREST[CCDARKGRAY]);
};

int main()
{
  openVBE(800, 600);
  setClipRect(0, 0, xres - 1, 500);
  float FOV = M_PI * 190.0F / 180.0F;
  WorldHeader wldhead;
  wldhead.CameraInitialFOV = FOV;
  wldhead.CameraInitialXAxis.x = 1.0F;
  wldhead.CameraInitialXAxis.y = 0.0F;
  wldhead.CameraInitialXAxis.z = 0.0F;
  wldhead.CameraInitialYAxis.x = 0.0F;
  wldhead.CameraInitialYAxis.y = 1.0F;
  wldhead.CameraInitialYAxis.z = 0.0F;
  wldhead.CameraInitialZAxis.x = 0.0F;
  wldhead.CameraInitialZAxis.y = 0.0F;
  wldhead.CameraInitialZAxis.z = 1.0F;
  wldhead.CameraInitialPosition.x = 0.0F;
  wldhead.CameraInitialPosition.y = 0.0F;
  wldhead.CameraInitialPosition.z = 10.0F;
  Camera cam(wldhead);
  installkeyhandler();
  settextsize(8);
  GRTEXT_BGColor = 0;
  char shiftp = 0, changed = 1, showaddsub = 1, showproducts = 1;
  Vector v1(10.0, 1.0, -5.0), v2(2.0, -3.0, 4.0), resv;
  Matrix44 transforms;
  float cinc = unit / 100.0;
  Xan = Yan = Zan = Xtransl = Ytransl = Ztransl = 0.0;
  Xscale = Yscale = Zscale = 1.0;
  char pal[0x300];
  createlinearshadepal(pal);
  setpal(pal);
  init_shadinglookups(pal);
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
		  if (shiftp) v2.x += cinc;
		  else v1.x += cinc;
		if (getkey(KEY_MINUS))
		  if (shiftp) v2.x -= cinc;
		  else v1.x -= cinc;
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
	      if (shiftp) v2.y += cinc;
	      else v1.y += cinc;
	    if (getkey(KEY_MINUS))
	      if (shiftp) v2.y -= cinc;
	      else v1.y -= cinc;
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
		  if (shiftp) v2.z += cinc;
		  else v1.z += cinc;
		if (getkey(KEY_MINUS))
		  if (shiftp) v2.z -= cinc;
		  else v1.z -= cinc;
	      }
	}
      if (getkey(KEY_D))
	{
	  transforms.transformationMatrix(Vector(Xtransl, Ytransl, Ztransl), Vector(Xscale, Yscale, Zscale), Vector(Xan, Yan, Zan));
	  v1 = transforms * v1;
	  v2 = transforms * v2;
	}
      if (getkey(KEY_A))
	showaddsub ^= 1;
      if (getkey(KEY_P))
	showproducts ^= 1;
      if (peekkey(KEY_F))
	cam.immer_der_Nase_nach(-0.5);
      if (peekkey(KEY_B))
	cam.immer_der_Nase_nach(0.5);
      if (peekkey(KEY_PAD4))
	if (peekkey(KEY_RSHIFT))
	  cam.translateinCameraWorld(-1.0F, 0.0F, 0.0F);
	else
	  cam.yaw(0.0174);
      if (peekkey(KEY_PAD6))
	if (peekkey(KEY_RSHIFT))
	  cam.translateinCameraWorld(1.0F, 0.0F, 0.0F);
	else
	  cam.yaw(-0.0174);
      if (peekkey(KEY_PAD2))
	if (peekkey(KEY_RSHIFT))
	  cam.translateinCameraWorld(0.0F, -1.0F, 0.0F);
	else
	  cam.pitch(-0.0174);
      if (peekkey(KEY_PAD8))
	if (peekkey(KEY_RSHIFT))
	  cam.translateinCameraWorld(0.0F, 1.0F, 0.0F);
	else
	  cam.pitch(0.0174);
      if (getkey(KEY_BACK))
	{
	  cam.ZAxis = -cam.ZAxis;
	  cam.createMatrices();
	}
      showtransformsgm();
      drawaxes(cam);
      showstrxy(0, yres - 120, "V1 = ", NEAREST[CCRED]);
      showvectorgm(v1, cam, NEAREST[CCRED]);
      showstrxy(0, yres - 110, "V2 = ", NEAREST[CCGREEN]);
      showvectorgm(v2, cam, NEAREST[CCGREEN]);
      if (showaddsub)
	{
	  showstrxy(0, yres - 100, "V1 + V2 = ", NEAREST[CCYELLOW]);
	  showvectorgm(v1 + v2, cam, NEAREST[CCYELLOW]);
	  showstrxy(0, yres - 90, "V1 - V2 = ", NEAREST[CCORANGE]);
	  showvectorgm(v1 - v2, cam, NEAREST[CCORANGE], v2);
	  showstrxy(0, yres - 80, "V2 - V1 = ", NEAREST[CCVIOLET]);
	  showvectorgm(v2 - v1, cam, NEAREST[CCVIOLET], v1);
	}
      if (showproducts)
	{
	  showstrxy(0, yres - 70, "V1 x V2 = ", NEAREST[CCCYAN]);
	  showvectorgm(v1 ^ v2, cam, NEAREST[CCCYAN]);
	  showstrxy(0, yres - 60, "V2 x V1 = ", NEAREST[CCBROWN]);
	  showvectorgm(v2 ^ v1, cam, NEAREST[CCBROWN]);
	  showstrxy(0, yres - 50, "V1 . V2 = ", NEAREST[CCWHITE]);
	  floatstr(v1 % v2, str);
	}
      showstr(str, NEAREST[CCWHITE]);
      showstrxy(0, yres - 40, "V1 normalized = ", NEAREST[CCDARKRED]);
      showvectorgm(~v1, cam, NEAREST[CCDARKRED]);
      showstrxy(0, yres - 30, "V2 normalized = ", NEAREST[CCDARKGREEN]);
      showvectorgm(~v2, cam, NEAREST[CCDARKGREEN]);
      shiftp = (peekkey(KEY_LSHIFT) || peekkey(KEY_RSHIFT));
      flipoffscreenmem();
      clearoffscreenmem();
    }
  closeVBE();
  try_free_shadinglookups();
  restorekeyhandler();
  return 0;
}

#endif
