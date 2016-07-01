#include "vbetwo.h"
#include "fileio.h"
#include "palettes.h"
#include "grtext.h"
#include "gformats.h"
#include <pc.h>
#include <keys.h>
#include <stdio.h>
#include <string.h>
#include "lcstdlib.h"
#include <math.h>


bool onedee = false, prompt = false, show_details = false,
  show_direction = true;

#define SCALE_STEP            2.0
#define SCROLL_STEP           2.0

double slow_random(double lower, double upper);
int slow_random_int(int lower, int upper);

int slow_random_int(int lower, int upper)
{ // returns random_number in the interval [lower; upper[
  return (int)(((upper - lower) * (double)rand() / (double)RAND_MAX) + lower);
};

double slow_random(double lower, double upper)
{
  return ((upper - lower) * (double)rand() / (double)RAND_MAX) + lower;
};


const double MIN_RADIUS = 1.0, MAX_RADIUS = 6.0;
const double MIN_VELOCITY = 0.0, MAX_VELOCITY = 2.0, MIN_MASS = 0.1, MAX_MASS = 2.0, SINGLE_STEP = 0.25, SEPARATE_STEP = 0.01;
const double MAX_MOMENTUM = MAX_VELOCITY * MAX_MASS;
const double PERCENT_AFTER_MOVE = 1.0, PERCENT_AFTER_WALL_HIT = 0.9999;
const double MIN_WLD = -100.0, MAX_WLD = 100.0; // squared viewing plane
double left = MIN_WLD, top = MIN_WLD, size = MAX_WLD - MIN_WLD;
char* pal;

class Vector2D
{
public:
  double x;
  double y;

  double scalar_projection_of_this_onto(const Vector2D& v2) const
  {// returns the amount of *this* that is parallel to v2
    return ((*this) * v2) / v2.length();
  };

  void normalize()
  {
    double l = sqrt(x*x + y*y);
    x /= l;
    y /= l;
  };

  Vector2D normalized() const
  {
    double l = sqrt(x*x + y*y);
    return Vector2D(x / l, y / l);
  };

  double length() const
  {
    return sqrt(x*x + y*y);
  };

  Vector2D operator-() const
  {
    return Vector2D(-x, -y);
  };

  Vector2D operator+(const Vector2D& v2) const
  {
    return Vector2D(x + v2.x, y + v2.y);
  };

  Vector2D operator-(const Vector2D& v2) const
  {
    return Vector2D(x - v2.x, y - v2.y);
  };

  Vector2D operator*(const double scalar) const
  {
    return Vector2D(x * scalar, y * scalar);
  };

  double operator*(const Vector2D& v2) const
  { // dot product, implies that both vectors are normalized
    return x * v2.x + y * v2.y;
  };

  Vector2D() {};
  Vector2D(double i_x, double i_y)
  {
    x = i_x;
    y = i_y;
  };

  Vector2D(const Vector2D& anotherv)
  {
    x = anotherv.x;
    y = anotherv.y;
  };

  ~Vector2D() {};
};

class Ball
{
public:
  double wld_x;
  double wld_y;
  double wld_dx;
  double wld_dy;
  double wld_radius;   // up to here it's all world
  int radius;
  int scrx;
  int scry;
  unsigned char color;
  unsigned char contrastindex;
  double mass;
  double velocity;
  double way_to_go;
  bool EOFiteration;


  static bool collision_check(Ball* b1, Ball* b2);
  static void do_Collision(Ball* b1, Ball* b2);

  void random_Position();
  void draw();

  Ball();
  ~Ball() {};
};

class BallManager
{
  int nBalls;
  Ball* balls; // [0; nBalls[

public:
  inline static void map_length(double wld, int* screen)
  {
    wld /= size;
    wld *= ((yres + xres) / 2 -  2);
    *screen = (int)(wld + 0.5);
  };

  inline static void map2screen(double wldx, double wldy, int* scrx, int* scry)
  {
    wldx -= left;
    wldx /= size;
    wldx *= (xres - 2);
    wldy -= top;
    wldy /= size;
    wldy *= (yres - 2);
    *scrx = (int)(wldx + 0.5);
    *scry = (int)(wldy + 0.5);
  };

  void move();
  void draw();
  void recalc_radius();
  double getavgvelocity();
  double getavglinmomentum();

  BallManager(int nballs);
  ~BallManager()
  { delete[] balls; };
};

void show_vector_in_ball(int x, int y, int radius, Vector2D v, unsigned char color);
void show_vector(Vector2D vec, double x, double y, unsigned char color);

// Constructors and Destructors

Ball::Ball()
{
  mass = slow_random(MIN_MASS, MAX_MASS);
  wld_radius = ((MAX_RADIUS - MIN_RADIUS) * (mass - MIN_MASS) / MAX_MASS) + MIN_RADIUS;
  BallManager::map_length(wld_radius, &radius);
  wld_dx = slow_random(-1.0, 1.0);
  if (onedee)
    wld_dy = 0.0;
  else
    wld_dy = slow_random(-1.0, 1.0);
  velocity = slow_random(MIN_VELOCITY, MAX_VELOCITY);
  do
    {
      color = slow_random_int(0, 256);
    } while (pal[PAL_RED(color)] + pal[PAL_GREEN(color)] + pal[PAL_BLUE(color)] < 40);
  contrastindex = findcontrastingindex(pal, color);
};

BallManager::BallManager(int nballs)
{
  nBalls = nballs;
  balls = new Ball[nBalls];
  int i, j;
  bool conflict;
  balls[0].random_Position();
  for (i = 1; i < nBalls; i++)
    {
      conflict = true;
      while (conflict)
        {
          balls[i].random_Position();
          conflict = false;
          for (j = 0; j < i; j++)
            {
              if (Ball::collision_check(balls + i, balls + j))
                conflict = true;
            }
        }
    }
};

// methods

void Ball::random_Position()
{
  wld_x = slow_random(MIN_WLD + wld_radius, MAX_WLD - wld_radius);
  if (onedee)
    wld_y = (MAX_WLD + MIN_WLD) / 2.0;
  else
    wld_y = slow_random(MIN_WLD + wld_radius, MAX_WLD - wld_radius);
};

bool Ball::collision_check(Ball* b1, Ball* b2)
{
  double r1pr2 = b1->wld_radius + b2->wld_radius, dx, dy;
  dx = b2->wld_x - b1->wld_x;
  dy = b2->wld_y - b1->wld_y;
  if (dx*dx + dy*dy <= r1pr2*r1pr2)
    return true;
  else
    return false;
};

inline void Ball::draw()
{
  BallManager::map2screen(wld_x, wld_y, &scrx, &scry);
  clip_fillcircle(scrx, scry, radius, color);
  if (show_direction)
    {
      double len = radius * velocity / MAX_VELOCITY;
      clipline(scrx, scry, scrx + (int)(wld_dx * len), scry + (int)(wld_dy * len), contrastindex);
    }
};

void Ball::do_Collision(Ball* b1, Ball* b2)
{
  double move1 = SEPARATE_STEP * b1->velocity / MAX_VELOCITY,
    move2 = SEPARATE_STEP * b2->velocity / MAX_VELOCITY;
  while (Ball::collision_check(b1, b2))
    { // separation of the colored (just a joke, I'm definitely not a racist)
      b1->wld_x -= b1->wld_dx * move1;
      b1->wld_y -= b1->wld_dy * move1;
      b2->wld_x -= b2->wld_dx * move2;
      b2->wld_y -= b2->wld_dy * move2;
    }
  Vector2D position_b1(b1->wld_x, b1->wld_y), position_b2(b2->wld_x, b2->wld_y);
  Vector2D line_of_sight(position_b2 - position_b1);
  if (show_details)
    show_vector(line_of_sight, b1->wld_x, b1->wld_y, NEAREST[CCWHITE]);
  line_of_sight.normalize();
  Vector2D b1_velocity_before(b1->wld_dx * b1->velocity, b1->wld_dy * b1->velocity);
  Vector2D b2_velocity_before(b2->wld_dx * b2->velocity, b2->wld_dy * b2->velocity);
  double b1_velocity_in_line_of_sight, b2_velocity_in_line_of_sight, b1_velocity_in_line_of_sight_before, b2_velocity_in_line_of_sight_before, b1_change_in_line_of_sight_velocity, b2_change_in_line_of_sight_velocity;
  b1_velocity_in_line_of_sight_before = b1_velocity_before.scalar_projection_of_this_onto(line_of_sight);
  b2_velocity_in_line_of_sight_before = b2_velocity_before.scalar_projection_of_this_onto(line_of_sight);

  b1_velocity_in_line_of_sight = b1_velocity_in_line_of_sight_before * (b1->mass - b2->mass) / (b1->mass + b2->mass) + b2_velocity_in_line_of_sight_before * 2.0 * b2->mass / (b1->mass + b2->mass);
  b2_velocity_in_line_of_sight = b1_velocity_in_line_of_sight_before * 2.0 * b1->mass / (b1->mass + b2->mass) + b2_velocity_in_line_of_sight_before * (b2->mass - b1->mass) / (b2->mass + b1->mass);

  b1_change_in_line_of_sight_velocity = b1_velocity_in_line_of_sight - b1_velocity_in_line_of_sight_before;
  b2_change_in_line_of_sight_velocity = b2_velocity_in_line_of_sight - b2_velocity_in_line_of_sight_before;

  Vector2D b1_velocity_after(b1_velocity_before + (line_of_sight * b1_change_in_line_of_sight_velocity)), b2_velocity_after(b2_velocity_before + (line_of_sight * b2_change_in_line_of_sight_velocity));

  if (show_details)
    {
      /*show_vector_in_ball(b1->scrx, b1->scry, b1->radius, from_b1_to_b2 * b1_velocity_in_line_of_sight_before, NEAREST[CCRED]);
        show_vector_in_ball(b1->scrx, b1->scry, b1->radius, from_b1_to_b2 * b1_velocity_in_line_of_sight, NEAREST[CCGREEN]);
        show_vector_in_ball(b2->scrx, b2->scry, b2->radius, from_b2_to_b1 * b2_velocity_in_line_of_sight_before, NEAREST[CCRED]);
        show_vector_in_ball(b2->scrx, b2->scry, b2->radius, from_b2_to_b1 * b2_velocity_in_line_of_sight, NEAREST[CCGREEN]);*/
      char str[1000];
      showstrxy(b1->scrx - 4, b1->scry - 4, "1", b1->contrastindex);// clipping!!!!!!
      showstrxy(b2->scrx - 4, b2->scry - 4, "2", b2->contrastindex);
      BallManager::map2screen(b1->wld_x, b1->wld_y, &b1->scrx, &b1->scry);
      BallManager::map2screen(b2->wld_x, b2->wld_y, &b2->scrx, &b2->scry);
      showstrxy(b1->scrx - 4, b1->scry - 4, "1", b1->contrastindex);// clipping!!!!!!
      showstrxy(b2->scrx - 4, b2->scry - 4, "2", b2->contrastindex);
      sprintf(str, "Masses: m1 = %G; m2 = %G", b1->mass, b2->mass);
      showstrxy(10, 20, str, NEAREST[CCWHITE]);
      sprintf(str, "Positions: b1 = (%G, %G); b2 = (%G, %G);", b1->wld_x, b1->wld_y, b2->wld_x, b2->wld_y);
      showstrxy(10, 30, str, NEAREST[CCWHITE]);
      sprintf(str, "Line of Sight = (%G, %G);", line_of_sight.x, line_of_sight.y);
      showstrxy(10, 40, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities before collision: vi1 = (%G, %G); vi2 = (%G, %G);", b1_velocity_before.x, b1_velocity_before.y, b2_velocity_before.x, b2_velocity_before.y);
      showstrxy(10, 50, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities before collision in line of sight: vi1' = %G; vi2' = %G", b1_velocity_in_line_of_sight_before, b2_velocity_in_line_of_sight_before);
      showstrxy(10, 60, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities after collision in line of sight: v1' = %G; v2' = %G", b1_velocity_in_line_of_sight, b2_velocity_in_line_of_sight);
      showstrxy(10, 70, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities after collision: v1 = (%G, %G); v2 = (%G, %G);", b1_velocity_after.x, b1_velocity_after.y, b2_velocity_after.x, b2_velocity_after.y);
      showstrxy(10, 80, str, NEAREST[CCWHITE]);
      flipoffscreenmem();
    }

  b1->velocity = b1_velocity_after.length();
  b2->velocity = b2_velocity_after.length();
  b1_velocity_after.normalize();
  b2_velocity_after.normalize();
  b1->wld_dx = b1_velocity_after.x;
  b1->wld_dy = b1_velocity_after.y;
  b2->wld_dx = b2_velocity_after.x;
  b2->wld_dy = b2_velocity_after.y;

  if (prompt)
    getkey();
};

void BallManager::draw()
{
  int i;
  for (i = 0; i < nBalls; i++)
    balls[i].draw();
};

void BallManager::move()
{
  int i, j;
  for (i = 0; i < nBalls; i++)
    {
      balls[i].way_to_go = balls[i].velocity;
      balls[i].EOFiteration = false;
    }
  bool reiterate;
  double thisway;
  Ball* cball;
  do
    {
      for (i = 0; i < nBalls; i++)
        {
          cball = balls + i;
          if (cball->way_to_go > SINGLE_STEP)
            thisway = SINGLE_STEP;
          else
            {
              thisway = cball->way_to_go;
              cball->EOFiteration = true;
            }
          cball->way_to_go -= thisway;
          cball->wld_x += cball->wld_dx * thisway;
          cball->wld_y += cball->wld_dy * thisway;
          if (cball->wld_x + cball->wld_radius > MAX_WLD)
            {
              cball->wld_x = MAX_WLD - cball->wld_radius - SINGLE_STEP;
              cball->wld_dx = -cball->wld_dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = false;
            }
          if (cball->wld_y + cball->wld_radius > MAX_WLD)
            {
              cball->wld_y = MAX_WLD - cball->wld_radius - SINGLE_STEP;
              cball->wld_dy = -cball->wld_dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = false;
            }
          if (cball->wld_x < MIN_WLD + cball->wld_radius)
            {
              cball->wld_x = MIN_WLD + cball->wld_radius + SINGLE_STEP;
              cball->wld_dx = -cball->wld_dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = false;
            }
          if (cball->wld_y < MIN_WLD + cball->wld_radius)
            {
              cball->wld_y = MIN_WLD + cball->wld_radius + SINGLE_STEP;
              cball->wld_dy = -cball->wld_dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = false;
            }
          // check for collisions
          for (j = 0; j < i; j++)
            {
              if (Ball::collision_check(cball, balls + j))
                {
                  Ball::do_Collision(cball, balls + j);
                  // reset way_to_go
                  cball->way_to_go = cball->velocity;
                  balls[j].way_to_go = balls[j].velocity;
                  cball->EOFiteration = false;
                }
            }
        }
      reiterate = false;
      for (i = 0; i < nBalls; i++)
        if (!balls[i].EOFiteration)
          reiterate = true;
    } while (reiterate);
};

void BallManager::recalc_radius()
{
  int i;
  for (i = 0; i < nBalls; i++)
    BallManager::map_length(balls[i].wld_radius, &balls[i].radius);
};

double BallManager::getavgvelocity()
{
  int i;
  double avgvel = 0.0;
  for (i = 0; i < nBalls; i++)
    avgvel += fabs(balls[i].velocity);
  return avgvel / nBalls;
};

double BallManager::getavglinmomentum()
{
  int i;
  double avgmom = 0.0;
  for (i = 0; i < nBalls; i++)
    avgmom += fabs(balls[i].velocity) * balls[i].mass;
  return avgmom / nBalls;
};

void drawborder()
{
  int u, v, u2, v2;
  BallManager::map2screen(MIN_WLD, MIN_WLD, &u, &v);
  BallManager::map2screen(MAX_WLD, MAX_WLD, &u2, &v2);
  clipline(u, v, u, v2, NEAREST[CCWHITE]); // left
  clipline(u, v, u2, v, NEAREST[CCWHITE]); // top
  clipline(u2, v, u2, v2, NEAREST[CCWHITE]); // right
  clipline(u, v2, u2, v2, NEAREST[CCWHITE]); // bottom
};

void show_vector(Vector2D vec, double x, double y, unsigned char color)
{
  int u, v, u2, v2;
  BallManager::map2screen(x, y, &u, &v);
  BallManager::map2screen(x + vec.x, y + vec.y, &u2, &v2);
  clipline(u, v, u2, v2, color);
};

void show_vector_in_ball(int x, int y, int radius, Vector2D v, unsigned char color)
{
  int len;
  BallManager::map_length(v.length(), &len);
  v.normalize();
  clipline(x, y, x + (int)(v.x * len), y + (int)(v.y * len), color);
};

int main(int nargs, char** args)
{
  int key = 0, nBalls = 0, i, j, resx = 800, resy = 600;
  char temp[0x100], temp2[0x100];
  bool bg_tiled = false;
  for (i = 1; i < nargs; i++)
    {
      if (stricmp(args[i], "-prompt") == 0)
        prompt = true;
      else if (stricmp(args[i], "-details") == 0)
        show_details = true;
      else if (stricmp(args[i], "-onedee") == 0)
        onedee = true;
      else if (stricmp(args[i], "-novelocity") == 0)
        show_direction = false;
      else if (stricmp(args[i], "-bgtiled") == 0)
        bg_tiled = true;
      else if (strnicmp(args[i], "-res:", 5) == 0)
        {
          strcpy(temp, args[i] + 5);
          j = 0;
          while (temp[j] != '\0' && temp[j] != 'x')
            j++;
          if (temp[j] == '\0')
            {
              printf("Invalid resolution: %s; use -res:<xres>x<yres>.\n", temp);
              return 1;
            }
          strcpy(temp2, temp + j + 1);
          resy = atoi(temp2);
          temp[j] = '\0';
          resx = atoi(temp);
        }
      else
        nBalls = atoi(args[i]);
    }
  if (nBalls == 0)
    {
      printf("SYNTAX:\tball <number_of_balls> [<-prompt> <-details> <-onedee> <-novelocity> <-bgtiled> <res:<xres>x<yres>>]\n");
      return 0;
    }
  if (!fexists("bg.spr")) {
    printf("Can't open bg.spr!\n");
    return 1;
  }
  if (!openVBE(resx, resy))
    {
      printf("Can't open VBE mode %i x %i x 256.\n", resx, resy);
      return 1;
    }
  char* pic;
  pal = loadspr_256("bg.spr", &pic, USE_PAL);
  if (!bg_tiled)
    scaleimage(&pic, xres, yres);
  else
    {
      i = 0;
      while (i < yres - 1)
        {
          j = 0;
          while (j < xres - 1)
            {
              clipputimage(j, i, pic);
              j += BITMAP_WIDTH(pic);
            }
          i += BITMAP_HEIGHT(pic);
        }
      xfree(pic);
      pic = Imageblock(xres, yres);
      getimage(0, 0, xres, yres, pic);
    }
  setblackpal();
  putbgimage(pic + 4);
  drawborder();
  BallManager* TheBest = new BallManager(nBalls);
  TheBest->draw();
  flipoffscreenmem();
  fadepalinto(pal, NORMAL);
  xfree(pal);
  char str[100];
  int iters = 0;
  while (key != K_Escape)
    {
      if (kbhit())
        key = getkey();
      else
        key = 0;
      switch(key)
        {
        case K_Right:
          if (left + size <= MAX_WLD - SCROLL_STEP)
            left += SCROLL_STEP;
          TheBest->recalc_radius();
          break;
        case K_Down:
          if (top + size <= MAX_WLD - SCROLL_STEP)
            top += SCROLL_STEP;
          TheBest->recalc_radius();
          break;
        case K_Left:
          if (left >= MIN_WLD + SCROLL_STEP)
            left -= SCROLL_STEP;
          TheBest->recalc_radius();
          break;
        case K_Up:
          if (top >= MIN_WLD + SCROLL_STEP)
            top -= SCROLL_STEP;
          TheBest->recalc_radius();
          break;
        case K_Control_L:
          if (left + size <= MAX_WLD - SCALE_STEP && top + size <= MAX_WLD - SCALE_STEP)
            size += SCALE_STEP;
          if (left >= MIN_WLD + SCALE_STEP)
            left -= SCALE_STEP;
          if (top >= MIN_WLD + SCALE_STEP)
            top -= SCALE_STEP;
          TheBest->recalc_radius();
          break;
        case K_Control_S:
          if (size >= 2 * SCALE_STEP)
            size -= SCALE_STEP;
          if (left + size <= MAX_WLD - SCALE_STEP)
            left += SCALE_STEP;
          if (top + size <= MAX_WLD - SCALE_STEP)
            top += SCALE_STEP;
          TheBest->recalc_radius();
          break;
        case K_Control_W:
          saveScreen("screen.spr");
          break;
        }
      iters++;
      TheBest->move();
      putbgimage(pic + 4);
      drawborder();
      TheBest->draw();
      sprintf(str, "iterations: %d; avg vel.: %G m/s", iters, TheBest->getavgvelocity());
      showstrxy(10, 10, str, NEAREST[CCWHITE]);
      flipoffscreenmem();
    }
  xfree(pic);
  fadepalout(FAST);
  closeVBE();
  delete TheBest;
  return 0;
}
