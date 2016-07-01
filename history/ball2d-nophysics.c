/* xscreensaver, Copyright (c) 1992, 1995, 1996, 1997, 1998
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include <math.h>
#include "screenhack.h"
#include "alpha.h"
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
# include "xdbe.h"
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */

/*
  TODO:
  - dbuf + reshape (deluxe.c!)
  - physics from good old DJGPP
  - better way of placing the balls, current code might not terminate!
*/

/* typedefs / structs */
typedef double number;

typedef struct _Vector2D {
  number x;
  number y;
} Vector2D;

typedef struct _Ball {
  number x,y;
  number dx,dy;
  number mass;
  number radius;
  unsigned long color;
  double velocity;
  double way_to_go;
  int EOFiteration;
} Ball;

struct state {
  Display *dpy;
  Window window;

  Ball* balls;

  Bool dbuf;
  int delay;
  int count;
  int ncolors;
  XColor *colors;
  GC erase_gc;
  XWindowAttributes xgwa;
  Pixmap b, ba, bb;	/* double-buffer to reduce flicker */

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  Bool dbeclear_p;
  XdbeBackBuffer backb;
# endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
};


/* constants */
const number MINX = -100.0;
const number MAXX = 100.0;
const number MINY = -100.0;
const number MAXY = 100.0;

const double MIN_RADIUS = 1.0, MAX_RADIUS = 6.0;
const double MIN_VELOCITY = 0.0, MAX_VELOCITY = 2.0, MIN_MASS = 0.1, MAX_MASS = 2.0, SINGLE_STEP = 0.25, SEPARATE_STEP = 0.01;
#define MAX_MOMENTUM (MAX_VELOCITY * MAX_MASS)
const double PERCENT_AFTER_MOVE = 1.0, PERCENT_AFTER_WALL_HIT = 0.9999;
const int onedee = 0;

static number
new_number_random (number min, number max) {
  return ( (max-min)*((number)random()/(number)RAND_MAX) ) + min;
}

static int
new_number_random_int (int min,int max) {
  return (int)( (max-min)*((number)random()/(number)RAND_MAX) ) + min;
}

/* Vector class */
static Vector2D
new_vector_random (number min_x, number max_x,
                   number min_y, number max_y) {
  Vector2D ret;
  ret.x = new_number_random (min_x, max_x);
  ret.y = new_number_random (min_y, max_y);
  return ret;
}

static number
Vector2D_length(Vector2D* vec)
{
  return sqrt(vec->x*vec->x + vec->y*vec->y);
}

static void
Vector2D_normalize(Vector2D* vec)
{
  number len = Vector2D_length(vec);
  vec->x /= len;
  vec->y /= len;
}

static Vector2D
Vector2D_normalized(Vector2D* vec)
{
  Vector2D ret;
  ret.x = vec->x;
  ret.y = vec->y;
  Vector2D_normalize(&ret);
  return ret;
}

static void
Vector2D_negate(Vector2D* vec)
{
  vec->x = -vec->x;
  vec->y = -vec->y;
}

static Vector2D
Vector2D_add(Vector2D* vec1, Vector2D* vec2)
{
  Vector2D sum;
  sum.x = vec1->x + vec2->x;
  sum.y = vec1->y + vec2->y;
  return sum;
}

static Vector2D
Vector2D_sub(Vector2D* vec1, Vector2D* vec2)
{
  Vector2D diff;
  diff.x = vec1->x - vec2->x;
  diff.y = vec1->y - vec2->y;
  return diff;
}

static Vector2D
Vector2D_multScalar(Vector2D* vec, number scalar)
{
  Vector2D ret;
  ret.x = vec->x * scalar;
  ret.y = vec->y * scalar;
  return ret;
}

static number
Vector2D_scalarProduct(Vector2D* vec1, Vector2D* vec2)
{
  return vec1->x * vec2->x + vec1->y * vec2->y;
}

static number
Vector2D_scalarProjectioOfVec1OntoVec2(Vector2D* vec1, Vector2D* vec2)
{
  return Vector2D_scalarProduct(vec1, vec2) / Vector2D_length(vec2);
}

static void
map_length(struct state* st, double wld, int* screen)
{
  wld /= (MAXY-MINY);
  /* TODO: scale with mean(width,height)? */
  wld *= (st->xgwa.height-1);
  *screen = (int)(wld + 0.5);
}

static void
map2screen(struct state* st, double wldx, double wldy, int* scrx, int* scry)
{
  wldx -= MINX;
  wldx /= (MAXX-MINX);
  wldx *= (st->xgwa.width-1);

  wldy -= MINY;
  wldy /= (MAXY-MINY);
  wldy *= (st->xgwa.height-1);

  *scrx = (int)(wldx + 0.5);
  *scry = (int)(wldy + 0.5);

  /* 
     deviceX = (int)((ball->position.x-MINX)/(MAXX-MINX) * st->xgwa.width);
     deviceY = (int)((ball->position.y-MINY)/(MAXY-MINY) * st->xgwa.height);
  */
}


/* Ball class */

static void
Ball_init(Ball* ball)
{ 
  ball->mass = new_number_random(MIN_MASS, MAX_MASS);
  ball->radius = ((MAX_RADIUS - MIN_RADIUS) * (ball->mass - MIN_MASS) / MAX_MASS) + MIN_RADIUS;
  /* map_length(wld_radius, &radius); */
  ball->dx = new_number_random(-1.0, 1.0);
  if (onedee)
    ball->dy = 0.0;
  else
    ball->dy = new_number_random(-1.0, 1.0);
  ball->velocity = new_number_random(MIN_VELOCITY, MAX_VELOCITY);
}

static void
Ball_randomizePosition (Ball* ball)
{
  Vector2D position;
  position = new_vector_random (MINX + ball->radius, MAXX - ball->radius,
                                MINY + ball->radius, MAXY - ball->radius);
  ball->x = position.x;
  if (onedee)
    ball->y = (MAXY-MINY) / 2.0;
  else
    ball->y = position.y;
}

static int
Ball_collision_check(Ball* b1, Ball* b2)
{
  double r1pr2, dx, dy;
  r1pr2 = b1->radius + b2->radius;
  dx = b2->x - b1->x;
  dy = b2->y - b1->y;
  if (dx*dx + dy*dy <= r1pr2*r1pr2)
    return 1;
  else
    return 0;
}

static void
Ball_draw (struct state* st, Drawable w, Ball *ball, int ballIdx)
{
  XGCValues gcv;
  unsigned long flags;
  GC gc;
  int deviceX, deviceY, radius;
  
  ball->x += ball->dx;
  ball->y += ball->dy;

  if (ball->x >= MAXX - ball->radius) 
    {
      ball->x = MAXX - ball->radius;
      ball->dx *= -1;
    }

  else if (ball->x <= MINX + ball->radius) 
    {
      ball->x = MINX + ball->radius;
      ball->dx *= -1;
    }

  if (ball->y >= MAXY - ball->radius) 
    {
      ball->y = MAXY - ball->radius;
      ball->dy *= -1;
    }
  else if (ball->y <= MINY + ball->radius) 
    {
      ball->y = MINY + ball->radius;
      ball->dy *= -1;
    }

  map_length(st, ball->radius, &radius);
  map2screen(st, ball->x, ball->y,
             &deviceX, &deviceY);

  flags = GCForeground;
  gcv.cap_style = CapProjecting;
  gcv.join_style = JoinMiter;
  gcv.foreground = st->colors[ballIdx % st->ncolors].pixel;
  /* gcv.foreground = ~0L; */
  flags |= (GCLineWidth | GCCapStyle | GCJoinStyle);
  gc = XCreateGC (st->dpy, w, flags, &gcv);

  XFillArc (st->dpy, w, gc,
            deviceX - radius,
            deviceY - radius,
            radius*2, radius*2,
            0, 360*64);
}

static void *
ball2d_init (Display *dpy, Window window)
{
  struct state *st = (struct state *) calloc (1, sizeof(*st));
  XGCValues gcv;
  int i, j;
  int conflict;

  st->dpy = dpy;
  st->window = window;
  st->count = get_integer_resource (st->dpy, "count", "Integer");
  st->ncolors = get_integer_resource (st->dpy, "ncolors", "Integer");
  st->delay = get_integer_resource (st->dpy, "delay", "Integer");
  st->dbuf = get_boolean_resource (st->dpy, "doubleBuffer", "Boolean");

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  st->dbeclear_p = get_boolean_resource (st->dpy, "useDBEClear", "Boolean");
#endif

# ifdef HAVE_COCOA	/* Don't second-guess Quartz's double-buffering */
  st->dbuf = False;
# endif

  XGetWindowAttributes (st->dpy, st->window, &st->xgwa);

  /* st->transparent_p = get_boolean_resource(st->dpy, "transparent", "Transparent"); */

  st->colors = (XColor *) calloc (sizeof(*st->colors), st->ncolors);

  if (get_boolean_resource(st->dpy, "mono", "Boolean"))
    {
    MONO:
      st->ncolors = 1;
      st->colors[0].pixel = get_pixel_resource(st->dpy, st->xgwa.colormap,
                                           "foreground", "Foreground");
    }
  else
    {
      make_random_colormap (st->dpy, st->xgwa.visual, st->xgwa.colormap,
                            st->colors, &st->ncolors, True, True, 0, True);
      if (st->ncolors < 2)
        goto MONO;
    }

  if (st->dbuf)
    {
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
      if (st->dbeclear_p)
        st->b = xdbe_get_backbuffer (st->dpy, st->window, XdbeBackground);
      else
        st->b = xdbe_get_backbuffer (st->dpy, st->window, XdbeUndefined);
      st->backb = st->b;
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */

      if (!st->b)
        {
          st->ba = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height,st->xgwa.depth);
          st->bb = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height,st->xgwa.depth);
          st->b = st->ba;
        }
    }
  else
    {
      st->b = st->window;
    }

  /*
  st->throbbers = (struct throbber **) calloc (st->count, sizeof(struct throbber *));
  for (i = 0; i < st->count; i++)
    st->throbbers[i] = make_throbber (st, st->b, st->xgwa.width, st->xgwa.height,
                                  st->colors[random() % st->ncolors].pixel);
  */
  st->balls = (Ball*) calloc(st->count, sizeof(Ball));
  for (i = 0; i < st->count; i++)
    { 
      Ball_init(&st->balls[i]);
    }

  Ball_randomizePosition(&st->balls[0]);
  for (i = 1; i < st->count; i++)
    {
      conflict = 1;
      while (conflict)
        {
          Ball_randomizePosition(&st->balls[i]);
          conflict = 0;
          for (j = 0; j < i; j++)
            {
              if (Ball_collision_check(st->balls + i, st->balls + j))
                conflict = 1;
            }
        }
    }


  gcv.foreground = get_pixel_resource (st->dpy, st->xgwa.colormap,
                                       "background", "Background");
  st->erase_gc = XCreateGC (st->dpy, st->b, GCForeground, &gcv);

  if (st->ba) XFillRectangle (st->dpy, st->ba, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);
  if (st->bb) XFillRectangle (st->dpy, st->bb, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);

  return st;
}

static unsigned long
ball2d_draw (Display *dpy, Window window, void *closure)
{
  int i;
  struct state *st = (struct state *) closure;

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  if (!st->dbeclear_p || !st->backb)
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
    XFillRectangle (st->dpy, st->b, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);
  
  /*
  for (i = 0; i < st->count; i++)
    if (throb (st, st->b, st->throbbers[i]) < 0)
      st->throbbers[i] = make_throbber (st, st->b, st->xgwa.width, st->xgwa.height,
                                    st->colors[random() % st->ncolors].pixel);
  */
  for (i = 0; i < st->count; i++)
    {
      Ball_draw(st, st->b, &st->balls[i], i);
    }

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  if (st->backb)
    {
      XdbeSwapInfo info[1];
      info[0].swap_window = st->window;
      info[0].swap_action = (st->dbeclear_p ? XdbeBackground : XdbeUndefined);
      XdbeSwapBuffers (st->dpy, info, 1);
    }
  else
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
    if (st->dbuf)
      {
        XCopyArea (st->dpy, st->b, st->window, st->erase_gc, 0, 0,
                   st->xgwa.width, st->xgwa.height, 0, 0);
        st->b = (st->b == st->ba ? st->bb : st->ba);
      }

  return st->delay;
}

static void
ball2d_reshape (Display *dpy, Window window, void *closure, 
                 unsigned int w, unsigned int h)
{
  struct state *st = (struct state *) closure;
  if (! st->dbuf) {   /* #### more complicated if we have a back buffer... */
    XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
    XClearWindow (dpy, window);
  }
}

static Bool
ball2d_event (Display *dpy, Window window, void *closure, XEvent *event)
{
  return False;
}

static void
ball2d_free (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;
  free(st->balls);
}


static const char *ball2d_defaults [] = {
  ".background:		black",
  ".foreground:		white",
  "*ncolors:		20", /* TODO: bind this to `count' */
  "*delay:		10000",
  "*count:		5",
  "*doubleBuffer:	True",
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  "*useDBE:		True",
  "*useDBEClear:	True",
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
  0
};

static XrmOptionDescRec ball2d_options [] = {
  { "-delay",		".delay",	XrmoptionSepArg, 0 },
  { "-count",		".count",	XrmoptionSepArg, 0 },
  { "-ncolors",		".ncolors",	XrmoptionSepArg, 0 },
  { "-db",		".doubleBuffer", XrmoptionNoArg,  "True"  },
  { "-no-db",		".doubleBuffer", XrmoptionNoArg,  "False" },
  { 0, 0, 0, 0 }
};


XSCREENSAVER_MODULE ("Ball2D", ball2d)

/*
  Local Variables:
  compile-command: "make ball2d"
  End:
*/
