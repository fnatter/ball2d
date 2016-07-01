#include "vbetwo.h"
#include "fifo99.h"
#include "grtext.h"
#include "function.h"
#include "gformats.h"
#include "palettes.h"
#include "fileio.h"
#include "camera.h"
#include "world3d.h"
#include "object3d.h"
#include <stdio.h>
#include <string.h>
#include "lcstdlib.h" // xmalloc/xfree
#include <math.h>
#include <pc.h>
#include <keys.h>

#ifndef DJ_RELEASE
#define SAFE_KEYS
#endif

// input storage for console
#define MAX_CONSOLE_LINES                 500
#define MAX_CONSOLE_LINE_LEN              256
#define CONSOLE_FRAME_WIDTH_X             28
#define CONSOLE_FRAME_WIDTH_Y             22
#define CONSOLE_FONT_WIDTH                8
#define CONSOLE_FONT_HEIGHT               8
#define CONSOLE_CELL_HEIGHT               10
#define SCROLL_STEP                       0.05
#define unit                              10.0F

float FOV, planesize;
float MINX = -10.0F, MAXX = 10.0F, MINY = -10.0F, MAXY = 10.0F;
int steps = 20;

bool showaxes = true, twobaseplanes = false;
char* pal;

struct console_Line
{
  char szline[MAX_CONSOLE_LINE_LEN];
  bool editable;
};

char* console_pic, *bgpic, *temppic;
console_Line console_Lines[MAX_CONSOLE_LINES];
int console_FirstLine = 0, CharsPerLine,
  VisibleConsoleLines, cursor_x = 0, cursor_y = 0,
  console_Height, console_Top;

#define WRAP(x)       ((x) % MAX_CONSOLE_LINES)

const unsigned char boundsidentifycolor = CCDARKGRAY, axescolor = CCGRAY,
  msgcol = CCWHITE, consolecolor = CCWHITE,
  cursorcolor = CCYELLOW, intersectioncolor = CCORANGE,
  zeroscolor = CCBLUE, tpcolor = CCRED, vertexcolor = CCBROWN,
  maxcolor = CCCYAN, mincolor = CCVIOLET;

struct Function3D
{
  Knot parseTreeRoot;
  char szFunction[MAX_EXPR_LEN];
  char basecolor, type;
};

Function3D fn;
int maxdrawx, maxdrawy;

void prepare_frame();
void respond_to_Console(const char* str, int* cmdline);
void set_new_function(const char* szfn);
void drawframe();
void console();
void handleInput(char* szcommand, int* cmdline);
void drawaxes();
void handleCommand(char* szcmd, char* szparamlist, int* cmdline);

inline double funcz(double x, double y)
{
  VARIABLE_X = x;
  VARIABLE_Y = y;
  return evaluate_expression(&fn.parseTreeRoot);
};

void drawframe()
{
  int i;
  switch(fn.type)
    {
    case FACE_TYPE_WIREFRAME:
      for (i = 0; i < totalvisibleFaces; i++)
	drawwireframe(facepool + SortingElements[i].index);
      break;
    case FACE_TYPE_FLAT:
      for (i = 0; i < totalvisibleFaces; i++)
	drawflatpoly(facepool + SortingElements[i].index);
      break;
    case FACE_TYPE_GOURAUD:
      for (i = 0; i < totalvisibleFaces; i++)
	drawgouraudpoly(facepool + SortingElements[i].index);
      break;
    case FACE_TYPE_TEXTURED:
      for (i = 0; i < totalvisibleFaces; i++)
	drawlineartexturepoly(facepool + SortingElements[i].index, textures);
      break;
    case FACE_TYPE_GOURAUD_TEXTURED:
      for (i = 0; i < totalvisibleFaces; i++)
	drawtexturegouraudpoly(facepool + SortingElements[i].index, textures);
      break;
    }
  if (showaxes)
    drawaxes();
};

void set_new_function(const char* szfn)
{
  int i = 0, j, k;
  while (szfn[i] != '\0')
    { // szFunction is statically allocated
      if (szfn[i] != ' ')
	fn.szFunction[i] = szfn[i];
      i++;
    }
  fn.szFunction[i] = '\0';
  strlwr(fn.szFunction);
  record_expression(fn.szFunction, &fn.parseTreeRoot);
  simplify_expression(&fn.parseTreeRoot);
  knot2expr(&fn.parseTreeRoot, fn.szFunction);
  if (NormalPool != 0)
    xfree(NormalPool);
  if (VerticeNormalPool != 0)
    xfree(VerticeNormalPool);
  if (GCPool != 0)
    xfree(GCPool);
  if (facepool != 0)
    xfree(facepool);
  if (AvgZCameraSpacePool != 0)
    xfree(AvgZCameraSpacePool);
  if (VerticeShades != 0)
    xfree(VerticeShades);
  if (ClipPool != 0)
    xfree(ClipPool);
  if (SortingElements != 0)
    xfree(SortingElements);
  totalFaces = steps * steps;
  totalVertices = (steps + 1) * (steps + 1);
  if (twobaseplanes)
    {
      totalFaces *= 2;
      totalVertices *= 2;
    }
  NormalPool = (point3d*)xmalloc(sizeof(point3d) * totalFaces);
  ClipPool = (point3d*)xmalloc(sizeof(point3d) * totalVertices);
  VerticePool = (point3d*)xmalloc(sizeof(point3d) * totalVertices);
  VerticeNormalPool = (point3d*)xmalloc(sizeof(point3d) * totalVertices);
  SortingElements = (SortItem*)xmalloc(sizeof(SortItem) * totalFaces);
  VerticeShades = (int*)xmalloc(sizeof(int) * totalVertices);
  facepool = (Face*)xmalloc(sizeof(Face) * totalFaces);
  GCPool = (point3d*)xmalloc(sizeof(point3d) * totalFaces);
  AvgZCameraSpacePool = (float*)xmalloc(sizeof(float) * totalFaces);
  float x, y, dx, dy;
  dx = (MAXX - MINX) / (float)steps;
  dy = (MAXY - MINY) / (float)steps;
  char* validflags = (char*)xmalloc(totalVertices * sizeof(char));
  unsigned char invalid_basecolor = findnearestbasecolor_8(pal, 255, 0, 0);
  k = 0;
  for (i = 0; i <= steps; i++)
    {
      for (j = 0; j <= steps; j++)
	{
	  x = MINX + (dx * j);
	  y = MINY + (dy * i);
	  (VerticePool[k])[VEC_X] = x;
	  (VerticePool[k])[VEC_Y] = y;
	  evaluate_Error = EVALUATE_NO_ERROR;
	  (VerticePool[k])[VEC_Z] = (float)funcz(x, y);
	  if (evaluate_Error != EVALUATE_NO_ERROR)
	    validflags[k] = 0;
	  else
	    validflags[k] = 1;
	  if (twobaseplanes)
	    {
	      (VerticePool[k + totalVertices / 2])[VEC_X] =
		(VerticePool[k])[VEC_X];
	      (VerticePool[k + totalVertices / 2])[VEC_Y] =
		(VerticePool[k])[VEC_Y];
	      (VerticePool[k + totalVertices / 2])[VEC_Z] =
		-(VerticePool[k])[VEC_Z];
	      validflags[k + totalVertices / 2] = validflags[k];
	    }
	  k++;
	}
    }
  int v1, v2, v3, v4;
  k = 0;
  for (i = 0; i < steps; i++) // looping along y
    {
      for (j = 0; j < steps; j++) // looping along x
	{// should be listed ACW;+X points right,+Y points up on drawing plane
	  v1 = i * (steps + 1) + j; // pt on the 'lower left'
	  v2 = v1 + 1; // pt on the 'lower right'
	  v3 = v2 + steps + 1;
	  v4 = v1 + steps + 1;
	  if (validflags[v1] && validflags[v2] &&
	      validflags[v3] && validflags[v4])
	    facepool[k].basecolor = fn.basecolor;
	  else
	    facepool[k].basecolor = invalid_basecolor;
	  facepool[k].nVertices = 4;
	  facepool[k].verticeindex[0] = v1;
	  facepool[k].verticeindex[1] = v2;
	  facepool[k].verticeindex[2] = v3;
	  facepool[k].verticeindex[3] = v4;
	  facepool[k].texelsU[0] = 0;
	  facepool[k].texelsV[0] = 0;
	  facepool[k].texelsU[1] = (BITMAP_WIDTH(textures->pic) - 1) << 20;
	  facepool[k].texelsV[1] = 0;
	  facepool[k].texelsU[2] = (BITMAP_WIDTH(textures->pic) - 1) << 20;
	  facepool[k].texelsV[2] = (BITMAP_HEIGHT(textures->pic) - 1) << 20;
	  facepool[k].texelsU[3] = 0;
	  facepool[k].texelsV[3] = (BITMAP_HEIGHT(textures->pic) - 1) << 20;
	  k++;
	}
    }
  if (twobaseplanes)
    {
      k = totalFaces / 2;
      for (i = 0; i < steps; i++) // looping along y
	{
	  for (j = 0; j < steps; j++) // looping along x
	    {
// should be listed ACW;+X points right,+Y points up on drawing plane
	      v1 = i * (steps + 1) + j + totalVertices / 2; //pt on'lower left'
	      v2 = v1 + 1; // pt on the 'lower right'
	      v3 = v2 + steps + 1;
	      v4 = v1 + steps + 1;
	      if (validflags[v1] && validflags[v2]
		  && validflags[v3] && validflags[v4])
		facepool[k].basecolor = fn.basecolor;
	      else
		facepool[k].basecolor = invalid_basecolor;
	      facepool[k].nVertices = 4;
	      facepool[k].verticeindex[0] = v1;
	      facepool[k].verticeindex[1] = v2;
	      facepool[k].verticeindex[2] = v3;
	      facepool[k].verticeindex[3] = v4;
	      facepool[k].texelsU[0] = 0;
	      facepool[k].texelsV[0] = 0;
	      facepool[k].texelsU[1] = (BITMAP_WIDTH(textures->pic) - 1) << 20;
	      facepool[k].texelsV[1] = 0;
	      facepool[k].texelsU[2] = (BITMAP_WIDTH(textures->pic) - 1) << 20;
	      facepool[k].texelsV[2] = (BITMAP_HEIGHT(textures->pic) - 1) << 20;
	      facepool[k].texelsU[3] = 0;
	      facepool[k].texelsV[3] = (BITMAP_HEIGHT(textures->pic) - 1) << 20;
	      k++;
	    }
	}
    }
  for (i = 0; i < totalFaces; i++)
    {
      compute_normal(facepool[i], i);
      compute_geometric_center(facepool[i], i);
    }
  compute_vertice_normals();
  xfree(validflags);
  //compute_geometric_center(objects[0]);
  //compute_bounding_Sphere(objects[0]);
};

void console()
{
  int key, i, j;
  char temp[1024];
  do
    {
      putimage(0, console_Top, console_pic);
      // show previous input
      for (i = console_FirstLine, j = 0; j < VisibleConsoleLines; j++, i++)
	showstrxy(CONSOLE_FRAME_WIDTH_X, console_Top + CONSOLE_FRAME_WIDTH_Y + (j * CONSOLE_CELL_HEIGHT), console_Lines[i].szline, NEAREST[consolecolor]);
      // show cursor
      textcurx = CONSOLE_FRAME_WIDTH_X + (cursor_x * CONSOLE_FONT_WIDTH);
      textcury = console_Top + CONSOLE_FRAME_WIDTH_Y + (cursor_y * CONSOLE_CELL_HEIGHT);
      showchar('_', NEAREST[cursorcolor]);
      flipoffscreenmem();
      key = getkey();
      // handle input
      switch (key)
	{
	case K_Control_S:
	  saveScreen("screen.spr");
	  break;
	case K_Tab:
	  break;
	case K_Up:
	  if (console_FirstLine + cursor_y > 0)
	    {
	      if (cursor_y > 0)
		cursor_y--;
	      else
		{
		  if (console_FirstLine > 0)
		    console_FirstLine--;
		}
	      cursor_x = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	      if (cursor_x >= CharsPerLine)
		cursor_x--;
	    }
	  break;
	case K_Down:
	  if (cursor_y < VisibleConsoleLines - 1)
	    cursor_y++;
	  else
	    {
	      if (console_FirstLine > MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		{
		  console_FirstLine = 0;
		  cursor_y = 0;
		}
	      else
		console_FirstLine++;
	    }
	  cursor_x = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	  if (cursor_x >= CharsPerLine)
	    cursor_x--;
	  break;
	case K_Left:
	  if (cursor_x > 0)
	    cursor_x--;
	  break;
	case K_Right:
	  if (cursor_x < VisibleConsoleLines - 1)
	    cursor_x++;
	  break;
	case K_BackSpace:
	  if (cursor_x > 0 && console_Lines[console_FirstLine + cursor_y].editable == true)
	    {
	      cursor_x--;
	      strcpy(temp, console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine + cursor_y].szline[cursor_x] = '\0';
	      appendString(console_Lines[console_FirstLine + cursor_y].szline,
			   temp + cursor_x + 1);
	    }
	  break;
	case K_Delete:
	  if (console_Lines[console_FirstLine + cursor_y].editable == true &&
	      cursor_x <
	      (int)strlen(console_Lines[console_FirstLine + cursor_y].szline))
	    {
	      strcpy(temp, console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine+cursor_y].szline[cursor_x]='\0';
	      appendString(console_Lines[console_FirstLine + cursor_y].szline,
			   temp + cursor_x + 1);
	    }
	  break;
	case K_QuestionMark:
	  i = console_FirstLine + cursor_y;
	  respond_to_Console("? - Console commands:", &i);
	  respond_to_Console("i.e.: f = sin(x)+sin(y)", &i);
	  respond_to_Console("functions available
for function-definitions are: [arc]sin/cos/tan/cot,
ln/log(base,exp)/abs/sgn/sqrt", &i);
	  respond_to_Console("type = <polygon-draw-type>", &i);
	  respond_to_Console("steps = [1-100], sets the number of polygons
used to approximate the surface", &i);
	  respond_to_Console("fov = <Field-of-View>, where 0.0 <= fov < 180.0"
			     , &i);
	  respond_to_Console("eval(<expression>) to evaluate an expression.
i.e.: eval(sin(3/4*pi))", &i);
	  respond_to_Console("use range(<left>, <right>, <bottom>, <top>)
to change the coordinates of the base plane", &i);
	  for (i -= console_FirstLine + cursor_y; i > 0; i--)
	    {
	      if (cursor_y < VisibleConsoleLines - 1)
		cursor_y++;
	      else
		{
		  if (console_FirstLine >
		      MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		    {
		      console_FirstLine = 0;
		      cursor_y = 0;
		    }
		  else
		    console_FirstLine++;
		}
	    }
	  console_Lines[console_FirstLine + cursor_y].szline[0] = '\0';
	  cursor_x = 0;
	  break;
	case K_Return:
	  i = console_FirstLine + cursor_y;
	  if (console_Lines[i].editable == false)
	    break;
	  console_Lines[i].editable = true;
	  handleInput(console_Lines[i].szline, &i);
	  i = i - console_FirstLine - cursor_y;
	  for (; i > 0; i--)
	    {// skip the lines that are filled with responses
	      if (cursor_y < VisibleConsoleLines - 1)
		cursor_y++;
	      else
		{
		  if (console_FirstLine >
		      MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		    {
		      console_FirstLine = 0;
		      cursor_y = 0;
		    }
		  else
		    console_FirstLine++;
		}
	    }
	  console_Lines[console_FirstLine + cursor_y].szline[0] = '\0';
	  console_Lines[console_FirstLine + cursor_y].editable = true;
	  cursor_x = 0;
	  break;
	default:// std ascii, just add to stream
	  if (cursor_x < CharsPerLine - 1 &&
	      console_Lines[console_FirstLine + cursor_y].editable == true)
	    {
	      i = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine + cursor_y].szline[cursor_x]=
		(char)key;
	      cursor_x++;
	      if (i <= cursor_x)
		console_Lines[console_FirstLine+cursor_y].szline[cursor_x]='\0';
	    }
	  break;
	}
    } while (key != K_Tab && key != K_Escape);
};

void handleInput(char* szcommand, int* cmdline)
{
  int i, j, openpos, closepos;
  i = 0;
  char szcmd[MAX_EXPR_LEN], szparams[MAX_EXPR_LEN];
  i = 0; j = 0;
  while (szcommand[i] != '\0')
    {
      if (szcommand[i] != ' ')
	{
	  szcmd[j] = szcommand[i];
	  j++;
	}
      i++;
    }
  szcmd[j] = '\0';
  strlwr(szcmd);
  i = 0;
  while (szcmd[i] != '=' && szcmd[i] != '\0')
    i++; // check for assignment
  if (szcmd[i] != '\0')
    {
      strcpy(szparams, szcmd + i + 1);
      szcmd[i] = '\0';
      appendString(szcmd, ",");
      appendString(szcmd, szparams);
      strcpy(szparams, szcmd);
      strcpy(szcmd, "set");
      goto pass_command;
    }
  openpos = 0;
  while (szcmd[openpos] != '\0' && szcmd[openpos] != '('
	 && szcmd[openpos] != '[' && szcmd[openpos] != '{')
    openpos++;
  if (szcmd[openpos] == '\0')
    {// command without parameter
    no_param:
      if (strcmp(szcmd, "twobaseplanes") == 0)
	{
	  twobaseplanes = !twobaseplanes;
	  if (twobaseplanes)
	    respond_to_Console("Scanning two base-planes (+/-f(x,y)) enabled.",
			       cmdline);
	  else
	    respond_to_Console("Scanning one base-plane (+f(x,y)).", cmdline);
	  return;
	}
      if (strcmp(szcmd, "eval") == 0 || strcmp(szcmd, "evaluate") == 0)
	{
	  respond_to_Console("Use eval(<expression>) to evaluate
an expression. i.e.: eval(sin(3*pi/4))", cmdline);
	  return;
	}
      strcpy(szparams, "I didn't get it. (cmd = ");
      appendString(szparams, szcmd);
      appendString(szparams, ")");
      respond_to_Console(szparams, cmdline);
      return;
    }
  closepos = openpos;
  while (szcmd[closepos] != '\0')
    closepos++;
  closepos--;
  strcpy(szparams, szcmd + openpos + 1); // backup parameter list
  szparams[strlen(szparams) - 1] = '\0';
  szcmd[openpos] = '\0';
  if (szparams[0] == '\0')
    goto no_param;
 pass_command:
  *cmdline = *cmdline + 1;
  handleCommand(szcmd, szparams, cmdline);
};

void prepare_frame()
{
  transform_to_clip();// transforms Vertices to Clip-Space
  reflect_on_it();    // shading + backface-culling
  clip_and_project(); // clipping + final Clip-Space -> 2D -transformation
  prepare_face_sorting(); // transforms (visible-)face-GC's to Clip-Space
  //quicksort_faces(0, totalvisibleFaces - 1);
  exchangesort_faces();
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

void respond_to_Console(const char* str, int* cmdline)
{
  char temp[MAX_EXPR_LEN];
  strcpy(temp, str);
  int charsleft = strlen(temp);
  while (charsleft > CharsPerLine)
    {
      temp[CharsPerLine] = '\0';
      strcpy(console_Lines[*cmdline].szline, temp);
      charsleft -= strlen(temp);
      console_Lines[*cmdline].editable = false;
      *cmdline = WRAP(*cmdline + 1);
      strcpy(temp, str + strlen(str) - charsleft);
    }
  if (strlen(temp) > 0)
    {
      strcpy(console_Lines[*cmdline].szline, temp);
      console_Lines[*cmdline].editable = false;
      *cmdline = WRAP(*cmdline + 1);
    }
};

void handleCommand(char* szcmd, char* szparamlist, int* cmdline)
{
  char temp1[MAX_EXPR_LEN];
  char temp2[MAX_EXPR_LEN];
  int i, j;
  Knot tempTree;
  double tempval;
  if (strcmp(szcmd, "range") == 0)
    {
      i = 0; j = 0;
      while (szparamlist[i] != '\0')
	{
	  if (szparamlist[i] == ',')
	    j++;
	  i++;
	}
      if (j < 3)
	{
	  respond_to_Console("Too few parameters passed to range(<left>,
<right>, <bottom>, <top>)", cmdline);
	  return;
	}
      if (j > 3)
	{
	  respond_to_Console("Too many parameters passed to range(<left>,
<right>, <bottom>, <top>)", cmdline);
	  return;
	}
      strcpy(temp2, szparamlist);
      strcpy(temp1, temp2);
      tempTree.Parent = 0;
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 1st parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MINX;
      MINX = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      strcpy(temp2, temp1); // temp2 = "right,bottom,top"
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 2nd parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MAXX;
      MAXX = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      strcpy(temp2, temp1); // temp2 = "bottom,top"
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 3rd parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MINY;
      MINY = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      record_expression(temp1, &tempTree);
      VARIABLE_X = MAXY;
      MAXY = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      respond_to_Console("Range is changed.", cmdline);
      set_new_function(fn.szFunction);
      prepare_frame();
      clearoffscreenmem();
      drawframe();
      return;
    }
  if (strcmp(szcmd, "eval") == 0 || strcmp(szcmd, "evaluate") == 0)
    {
      record_Error = RECORD_NO_ERROR;
      tempTree.Parent = 0;
      tempTree.Children[0] = 0;
      record_expression(szparamlist, &tempTree);
      if (record_Error == RECORD_NO_ERROR)
	{
	  knot2expr(&tempTree, temp2);
	  if (depends_on_x(&tempTree))
	    {
	      strcpy(temp1, "What is x ? - ");
	      appendString(temp1, temp2);
	      respond_to_Console(temp1, cmdline);
	      return;
	    }
	  evaluate_Error = EVALUATE_NO_ERROR;
	  tempval = evaluate_expression(&tempTree);
	  freeKnot(&tempTree);
	  if (evaluate_Error != EVALUATE_NO_ERROR)
	    strcpy(temp1, " is not defined.");
	  else
	    sprintf(temp1, " = %G", tempval);
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	}
      else
	respond_to_Console("Error in expression.", cmdline);
      return;
    }
  if (strcmp(szcmd, "set") == 0)
    {
      i = 0;
      while (szparamlist[i] != ',' && szparamlist[i] != '\0')
	i++;
      if (szparamlist[i] == '\0')
	{
	  strcpy(temp1, "Error: Too few parameters in set command - ");
	  appendString(temp1, szparamlist);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      strcpy(temp2, szparamlist + i + 1);
      strcpy(temp1, szparamlist);
      temp1[i] = '\0';
      if (strcmp(temp1, "steps") == 0)
	{
	  sscanf(temp2, "%d", &i);
	  if (i <= 0 || i > 100)
	    {
	      respond_to_Console("Value out of range.", cmdline);
	      return;
	    }
	  steps = i;
	  set_new_function(fn.szFunction);
	  prepare_frame();
	  clearoffscreenmem();
	  drawframe();
	  sprintf(temp1, "steps = %d; nvertices = %d; nfaces = %d",
		  steps, totalFaces, totalVertices);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      if (strcmp(temp1, "type") == 0)
	{
	  if (strcmp(temp2, "wireframe") == 0)
	    fn.type = FACE_TYPE_WIREFRAME;
	  else if (strcmp(temp2, "flat") == 0)
	    fn.type = FACE_TYPE_FLAT;
	  else if (strcmp(temp2, "gouraud") == 0)
	    fn.type = FACE_TYPE_GOURAUD;
	  else if (strcmp(temp2, "textured") == 0)
	    fn.type = FACE_TYPE_TEXTURED;
	  else if (strcmp(temp2, "gouraud+textured") == 0)
	    fn.type = FACE_TYPE_GOURAUD_TEXTURED;
	  else
	    {
	      strcpy(temp1, "Invalid polygon-type - ");
	      appendString(temp1, temp2);
	      respond_to_Console(temp1, cmdline);
	      respond_to_Console("possible values are:
wireframe/flat/gouraud/textured/gouraud+textured", cmdline);
	      return;
	    }
	  for (i = 0; i < totalFaces; i++)
	    facepool[i].render_type = fn.type;
	  clearoffscreenmem();
	  drawframe();
	  return;
	}
      if (strcmp(temp1, "fov") == 0)
	{
	  sscanf(temp2, "%lf", &tempval);
	  if (tempval > 0.0 && tempval < 180.0)
	    {
	      FOV = M_PI * tempval / 180.0;
	      sprintf(temp1, "Field of view = %f degrees", tempval);
	      setFOV(FOV);
	      clearoffscreenmem();
	      prepare_frame();
	      drawframe();
	      respond_to_Console(temp1, cmdline);
	      return;
	    }
	  respond_to_Console("Value out of range.", cmdline);
	}
      if (strcmp(temp1, "f(x,y)") != 0 && strcmp(temp1, "f") != 0
	  && strcmp(temp1, "z") != 0)
	{
	  strcpy(temp2, "Unknown command - ");
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      set_new_function(temp2);
      strcpy(temp1, "f(x,y) = ");
      appendString(temp1, fn.szFunction);
      respond_to_Console(temp1, cmdline);
      prepare_frame();
      clearoffscreenmem();
      drawframe();
      return;
    }
};

int main(int nargs, char** args)
{
  int i, j, k;
  if (!fexists("shadecos.pal"))
    {
      printf("shadecos.pal is missing.\n");
      return 1;
    }
  if (!fexists("console.spr"))
    {
      printf("console.spr is missing.\n");
      return 1;
    }
  loadpalette("shadecos.pal", &pal);
  pal[0] = pal[1] = pal[2] = 0x00;
  loadspr_256("console.spr", &console_pic, TRANSLATE_PAL, pal);
  textures = (Texture*)xmalloc(sizeof(Texture));
  loadspr_256("tex.spr", &textures->pic, TRANSLATE_PAL, pal);
  i = BITMAP_WIDTH(textures->pic);
  k = 0;
  for (j = 0; j < 32; j++)
    {
      if (i & (1 << j))
	k = j;
    }
  if (i != 1 << k) // if it isn't exactly aligned on a bit position...
    {
      i = 1 << (k + 1); // k is highest "1"-bit in previous width
      textures->width_shift = k + 1;
    }
  else
    textures->width_shift = k;
  j = (int)((i/(float)BITMAP_WIDTH(textures->pic))
	    * BITMAP_HEIGHT(textures->pic));
// scale width to 2^textures->width_shift (and height proportionally)
  scaleimage(&textures->pic, i, j);
  FILE* config = fopen("fx3d.cfg", "rb");
  if (config == 0)
    {
      printf("fx3d.cfg not found.\n");
      return 1;
    }
  fscanf(config, "res: %d %d\n", &i, &j);
  fclose(config);
  if (!openVBE(i, j))
    {
      printf("Couldn't enter VBE mode (%i x %i).\n", i, j);
      return 1;
    }
  maxdrawx = xres - 1;
  maxdrawy = yres - 1;
  console_Height = yres / 5;
  console_Top = yres - console_Height - 1;
  settextsize(CONSOLE_FONT_WIDTH, CONSOLE_FONT_HEIGHT);
  CharsPerLine = (xres - 2*CONSOLE_FRAME_WIDTH_X) / CONSOLE_FONT_WIDTH;
  VisibleConsoleLines = (console_Height - 2*CONSOLE_FRAME_WIDTH_Y)
    / CONSOLE_CELL_HEIGHT;
  for (i = 0; i < MAX_CONSOLE_LINES; i++)
    {
      console_Lines[i].szline[0] = '\0';
      for (j = 1; j < MAX_CONSOLE_LINE_LEN; j++)
	console_Lines[i].szline[j] = ' ';
      console_Lines[i].editable = true;
    }
  bgpic = Imageblock(xres, console_Height);
  temppic = Imageblock(xres, console_Height);
  scaleimage(&console_pic, xres, console_Height);
  memcpy(temppic, console_pic, (xres * console_Height) + 4);

  /* initialize camera */
  WorldHeader wldhead;

  initVec(wldhead.CameraInitialPosition, 0.0F, 0.0F, 5.0F);
  initVec(wldhead.CameraInitialXAxis, 1.0F, 0.0F, 0.0F);
  initVec(wldhead.CameraInitialYAxis, 0.0F, 1.0F, 0.0F);
  initVec(wldhead.CameraInitialZAxis, 0.0F, 0.0F, 1.0F);
  wldhead.CameraInitialFOV = M_PI * 170.0F / 180.0F;
  initCamera(wldhead);

  /* initialize other parts of xpos */
  do_hsr = false;
  do_lsdrag = true;
  planesize = 25.0F;
  FOV = wldhead.CameraInitialFOV;
  totalObjects = 1;
  totalTextures = 1;
  objects = (Polyhedron*)xmalloc(sizeof(Polyhedron));
  CLIP_NEAR = -1.0F;
  CLIP_FAR = -10000.0F;
  init_shadinglookups(pal);
  setpal(pal);
  updatecolors();
  facepool = 0;
  NormalPool = 0;
  VerticeShades = 0;
  ClipPool = 0;
  VerticeNormalPool = 0;
  AvgZCameraSpacePool = 0;
  GCPool = 0;
  SortingElements = 0;
  fn.basecolor = findnearestbasecolor_8(pal, 255, 255, 255);
  fn.type = FACE_TYPE_GOURAUD;

  /* initialize mesh */
  set_new_function("sin(x+cos(x*y))");

  changeCharacter(42, 0, 0, 0,  8 | 16, 8 | 16, 0, 0, 0); // change '*'

  /* main loop starts here ****************************  */

#ifndef SAFE_KEYS
  installkeyhandler();
  while (!getkey(KEY_ESC))
#else
    int key = 0;
  while (key != K_Escape)
#endif
    {
#ifdef SAFE_KEYS
      if (kbhit())
	key = getkey();
      else
	key = 0;
      if (key == 97)
	immer_der_Nase_nach(-0.1F);
      if (key == 121)
	immer_der_Nase_nach(0.1F);
      if (key == 331)
	yaw(0.0174F);
      if (key == 333)
	yaw(-0.0174F);
      if (key == 336)
	pitch(-0.0174F);
      if (key == 328)
       	pitch(0.0174F);
      /*
      if (peekkey(KEY_PAD7))
	roll(-0.0174F);
      if (peekkey(KEY_PAD9))
	roll(0.0174F);
      if (key == 8)
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
      if (peekkey(KEY_N))
	{
	  if (peekkey(KEY_PLUS))
	    CLIP_NEAR += 0.1F;
	  if (peekkey(KEY_MINUS))
	    CLIP_NEAR -= 0.1F;
	}*/
#else
      if (peekkey(KEY_A))
	immer_der_Nase_nach(-0.1F);
      if (peekkey(KEY_Y))
	immer_der_Nase_nach(0.1F);
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
      if (peekkey(KEY_N))
	{
	  if (peekkey(KEY_PLUS))
	    CLIP_NEAR += 0.1F;
	  if (peekkey(KEY_MINUS))
	    CLIP_NEAR -= 0.1F;
	}
#endif
      prepare_frame();
      clearoffscreenmem();
      drawframe();
      flipoffscreenmem();
#ifndef SAFE_KEYS
      if (getkey(KEY_TAB))
#else
      if (key == K_Tab)
#endif
	{// temppic holds current console image
#ifndef SAFE_KEYS
	  restorekeyhandler();
#endif
	  for (VARIABLE_X = 0.0F; VARIABLE_X <= 1.0F;
	       VARIABLE_X += SCROLL_STEP)
	    {
	      j = (int)(console_Height * VARIABLE_X);
	      clipputimage(0, yres - 2 - j, temppic);
	      flipoffscreenmem();
	    }
	  console();
	  getimage(0, console_Top, xres, console_Height, temppic);
	  clearoffscreenmem();
	  drawframe();
	  getimage(0, console_Top, xres, console_Height, bgpic);
	  for (VARIABLE_X = 0.0F; VARIABLE_X <= 1.0F;
	       VARIABLE_X += SCROLL_STEP)
	    {
	      putimage(0, console_Top, bgpic);
	      j = (int)((console_Height - 2) * VARIABLE_X);
	      clipputimage(0, console_Top + j, temppic);
	      flipoffscreenmem();
	    }
#ifndef SAFE_KEYS
	  clearallkeys();
	  installkeyhandler();
#endif
	}
    }
#ifndef SAFE_KEYS
  restorekeyhandler();
#endif
  closeVBE();
  xfree(pal);
  xfree(console_pic);
  xfree(bgpic);
  xfree(temppic);
  trashXPOS();
  freeKnot(&fn.parseTreeRoot);
}
