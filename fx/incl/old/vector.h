#ifndef _VECTOR_H
#define _VECTOR_H

#include <math.h>

struct rect_t
{
  float x, y;
};

struct polar_t
{
  float angle, radius;
};

class Vector
{
 public:
  float x;
  float y;
  float z;

  Vector operator+(const Vector& V2) const
    {
      return Vector (x + V2.x, y + V2.y, z + V2.z);
    };

  Vector operator-(const Vector& V2) const
    {
      return Vector (x - V2.x, y - V2.y, z - V2.z);
    };

  Vector operator-() const
    {
      return Vector (-x, -y, -z);
    };

  Vector operator/(const float b) const
    {
      return Vector (x / b, y / b, z / b);
    };

  Vector operator*(const float b) const
    {
      return Vector (x * b, y * b, z * b);
    };

  Vector operator*(const Vector& V2) const
    {
      return Vector (x * V2.x, y * V2.y, z * V2.z);
    };

  Vector operator=(const Vector& V2)
  {
    x = V2.x;
    y = V2.y;
    z = V2.z;
    return *this;
  };

  Vector operator^(const Vector& V2) const
    {// cross product -> a x b = { y1 * z2 - z1 * y2 , z1 * x2 - x1 * z2 , x1 * y2 - y1 * x2 }
      return Vector (y * V2.z - z * V2.y, z * V2.x - x * V2.z, x * V2.y - y * V2.z);
    };

  float operator%(const Vector& V2) const
    {// dot product -> a * b = Ax * Bx + Ay * By + Az * Bz;
      return (x * V2.x + y * V2.y + z * V2.z);
    };// (implies that both vectors are normalized)

  Vector operator~() const
    {
      float length = sqrt (*this % *this);
      return Vector (x / length, y / length, z / length);
    };

  float length () const
    {
      return sqrt (x * x + y * y + z * z);
    };

  Vector(const Vector& otherV)
    {
      x = otherV.x;
      y = otherV.y;
      z = otherV.z;
    };

  Vector(const point3d& pt)
    {
      x = pt.x;
      y = pt.y;
      z = pt.z;
    };

  Vector(float ix, float iy, float iz)
    {
      x = ix;
      y = iy;
      z = iz;
    };

  Vector() { x = y = z = 0.0F; };
};

class Matrix44
{
 public:
  float elem[4][4];

  Vector operator*(const Vector& V) const
    {
      Vector b;
      b.x = elem[0][0] * V.x + elem[0][1] * V.y + elem[0][2] * V.z + elem[0][3];
      b.y = elem[1][0] * V.x + elem[1][1] * V.y + elem[1][2] * V.z + elem[1][3];
      b.z = elem[2][0] * V.x + elem[2][1] * V.y + elem[2][2] * V.z + elem[2][3];
      return b;
    };

  point3d operator*(const point3d& V) const
    {
      point3d b;
      b.x = elem[0][0] * V.x + elem[0][1] * V.y + elem[0][2] * V.z + elem[0][3];
      b.y = elem[1][0] * V.x + elem[1][1] * V.y + elem[1][2] * V.z + elem[1][3];
      b.z = elem[2][0] * V.x + elem[2][1] * V.y + elem[2][2] * V.z + elem[2][3];
      return b;
    };

  void translationMatrix(const Vector& a)
    {
      identity();
      elem[0][3] = -a.x;
      elem[1][3] = -a.y;
      elem[2][3] = -a.z;
    };

  void scalingMatrix(const Vector& a)
    {
      clear();
      elem[0][0] = a.x;
      elem[1][1] = a.y;
      elem[2][2] = a.z;
      elem[3][3] = 1.0;
    };

  /*	void rotationMatrix(axis3d axis, float angle)
	{
	identity();
	int i, j;
	float c = cos (angle), s = sin (angle);
	i = (axis % 3) + 1;
	j = i-- % 3;
	elem[i][i] = c;
	elem[i][j] = s;
	elem[j][j] = c;
	elem[j][i] = -s;
	};*/

  void rotationMatrix(const Vector& a)
    {
      float sx = sin(a.x), sy = sin(a.y), sz = sin(a.z),
	cx = cos(a.x), cy = cos(a.y), cz = cos(a.z);
      /*		elem[0][0] = (cy * cz) - (sx * sy * sz); elem[0][1] = -cx * sz; elem[0][2] = (sy * cz) + (cy * sx * sz); elem[0][3] = 0.0;
			elem[1][0] = (cy * sz) + (sx * sy * cz); elem[1][1] = cx * cz; elem[1][2] = (sy * sz) - (cy * cz * sx); elem[1][3] = 0.0;
			elem[2][0] = -sy * cx; elem[2][1] = sx; elem[2][2] = cy * cx; elem[2][3] = 0.0;
			elem[3][0] = 0.0; elem[3][1] = 0.0; elem[3][2] = 0.0; elem[3][3] = 1.0;*/
      elem[0][0] = (cy * cz) + (sx * sy * sz); elem[0][1] = cx * sz; elem[0][2] = -(sy * cz) + (cy * sx * sz); elem[0][3] = 0.0;
      elem[1][0] = -(cy * sz) + (sx * sy * cz); elem[1][1] = cx * cz; elem[1][2] = (sy * sz) + (cy * cz * sx); elem[1][3] = 0.0;
      elem[2][0] = sy * cx; elem[2][1] = -sx; elem[2][2] = cy * cx; elem[2][3] = 0.0;
      elem[3][0] = 0.0; elem[3][1] = 0.0; elem[3][2] = 0.0; elem[3][3] = 1.0;
    };

  void transformationMatrix(const Vector& translation, const Vector& scaling, const Vector& rotation)
    {
      Matrix44 m1, m2, m3;
      m1.scalingMatrix(scaling);
      m2.rotationMatrix(rotation);
      m3.translationMatrix(translation);
      identity();
      *this = *this * m1 * m2 * m3;
    };

  void PerspectiveTransforms(float FOV, float Znear, float Zfar)
    {
      clear();
      elem[0][0] = sin(FOV / 2.0);
      elem[2][2] = elem[0][0] / (1.0 - (Znear / Zfar));
      elem[2][3] = elem[0][0];
      elem[3][2] = -Zfar * elem[0][0] * Znear / (Zfar - Znear);
      elem[0][0] = cos(FOV / 2.0);
      elem[1][1] = elem[0][0];
    };

  void transpose()
    {
      int i,j;
      Matrix44 M;
      for (i = 0; i < 4; i++)
      	for (j = 0; j < 4; j++)
	  M.elem[i][j] = elem[j][i];
      for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	  elem[i][j] = M.elem[i][j];
    };

  void identity()
    {
      elem[0][0] = 1.0; elem[0][1] = 0.0; elem[0][2] = 0.0; elem[0][3] = 0.0;
      elem[1][0] = 0.0; elem[1][1] = 1.0; elem[1][2] = 0.0; elem[1][3] = 0.0;
      elem[2][0] = 0.0; elem[2][1] = 0.0; elem[2][2] = 1.0; elem[2][3] = 0.0;
      elem[3][0] = 0.0; elem[3][1] = 0.0; elem[3][2] = 0.0; elem[3][3] = 1.0;
    };

  Matrix44 operator*(const Matrix44& B) const
    {
      Matrix44 m;
      int i, j;
      for (i = 0; i < 4; i++)
      	for (j = 0; j < 4; j++)
	  m.elem[i][j] = elem[i][0] * B.elem[0][j] + elem[i][1] * B.elem[1][j]
	    + elem[i][2] * B.elem[2][j] + elem[i][3] * B.elem[3][j];
      return m;
    };

  void clear()
    {
      elem[0][0] = 0.0; elem[0][1] = 0.0; elem[0][2] = 0.0; elem[0][3] = 0.0;
      elem[1][0] = 0.0; elem[1][1] = 0.0; elem[1][2] = 0.0; elem[1][3] = 0.0;
      elem[2][0] = 0.0; elem[2][1] = 0.0; elem[2][2] = 0.0; elem[2][3] = 0.0;
      elem[3][0] = 0.0; elem[3][1] = 0.0; elem[3][2] = 0.0; elem[3][3] = 0.0;
    };

  Matrix44()
    {
      clear();
    };
};

inline point3d makeP3D(float x, float y, float z)
{
  point3d p;
  p.x = x;
  p.y = y;
  p.z = z;
  return p;
};

#endif
