#ifndef __VECMAT_H
#define __VECMAT_H


#define VEC_X		0
#define VEC_Y		1
#define VEC_Z		2
typedef float Vector3D[3];
#define point3d         Vector3D
typedef float Matrix4x4[4][4];


inline void normVec(float* V)
{
  float rec_len = 1.0F / sqrt(V[VEC_X] * V[VEC_X] + V[VEC_Y] * V[VEC_Y] + V[VEC_Z] * V[VEC_Z]);
  V[VEC_X] *= rec_len;
  V[VEC_Y] *= rec_len;
  V[VEC_Z] *= rec_len;
};

inline void addVec(float* A, float* B, float* res)
{
  res[VEC_X] = A[VEC_X] + B[VEC_X];
  res[VEC_Y] = A[VEC_Y] + B[VEC_Y];
  res[VEC_Z] = A[VEC_Z] + B[VEC_Z];
};

inline void addVec(float* A, float* B) // "A += B"
{
  A[VEC_X] += B[VEC_X];
  A[VEC_Y] += B[VEC_Y];
  A[VEC_Z] += B[VEC_Z];
};

inline void subVec(float* A, float* B, float* res)
{
  res[VEC_X] = A[VEC_X] - B[VEC_X];
  res[VEC_Y] = A[VEC_Y] - B[VEC_Y];
  res[VEC_Z] = A[VEC_Z] - B[VEC_Z];
};

inline void subVec(float* A, float* B) // "A -= B"
{
  A[VEC_X] -= B[VEC_X];
  A[VEC_Y] -= B[VEC_Y];
  A[VEC_Z] -= B[VEC_Z];
};

inline void negateVec(float* V)
{
  V[VEC_X] = -V[VEC_X];
  V[VEC_Y] = -V[VEC_Y];
  V[VEC_Z] = -V[VEC_Z];
};

inline void initVec(float* V, float x, float y, float z)
{
  V[VEC_X] = x;
  V[VEC_Y] = y;
  V[VEC_Z] = z;
};

inline float dotProd(float* A, float* B)
{
  return A[VEC_X] * B[VEC_X] + A[VEC_Y] * B[VEC_Y] + A[VEC_Z] * B[VEC_Z];
};

inline void crossProd(float* A, float* B, float* res)
{
  res[VEC_X] = A[VEC_Y] * B[VEC_Z] - A[VEC_Z] * B[VEC_Y];
  res[VEC_Y] = A[VEC_Z] * B[VEC_X] - A[VEC_X] * B[VEC_Z];
  res[VEC_Z] = A[VEC_X] * B[VEC_Y] - A[VEC_Y] * B[VEC_X];
};

inline void scaleVec(float* V, float factor)
{
  V[VEC_X] *= factor;
  V[VEC_Y] *= factor;
  V[VEC_Z] *= factor;
};

inline float VecMagnitude(Vector3D& V)
{
  return sqrt(V[VEC_X] * V[VEC_X] + V[VEC_Y] * V[VEC_Y] + V[VEC_Z] * V[VEC_Z]);
};

inline void assignVec(float* dest, float* source)
{
  memcpy(dest, source, sizeof(Vector3D));
};

inline void IdentityMat(Matrix4x4& M)
{
  M[0][0] = 1.0F; M[1][0] = 0.0F; M[2][0] = 0.0F; M[3][0] = 0.0F;
  M[0][1] = 0.0F; M[1][1] = 1.0F; M[2][1] = 0.0F; M[3][1] = 0.0F;
  M[0][2] = 0.0F; M[1][2] = 0.0F; M[2][2] = 1.0F; M[3][2] = 0.0F;
  M[0][3] = 0.0F; M[1][3] = 0.0F; M[2][3] = 0.0F; M[3][3] = 1.0F;
};

inline void ScalingMat(Matrix4x4& M, float sX, float sY, float sZ)
{
  M[0][0] = sX; M[1][0] = 0.0F; M[2][0] = 0.0F; M[3][0] = 0.0F;
  M[0][1] = 0.0F; M[1][1] = sY; M[2][1] = 0.0F; M[3][1] = 0.0F;
  M[0][2] = 0.0F; M[1][2] = 0.0F; M[2][2] = sZ; M[3][2] = 0.0F;
  M[0][3] = 0.0F; M[1][3] = 0.0F; M[2][3] = 0.0F; M[3][3] = 1.0F;
};

inline void TranslationMat(Matrix4x4& M, float tX, float tY, float tZ)
{
  M[0][0] = 1.0F; M[1][0] = 0.0F; M[2][0] = 0.0F; M[3][0] = 0.0F;
  M[0][1] = 0.0F; M[1][1] = 1.0F; M[2][1] = 0.0F; M[3][1] = 0.0F;
  M[0][2] = 0.0F; M[1][2] = 0.0F; M[2][2] = 1.0F; M[3][2] = 0.0F;
  M[0][3] = tX; M[1][3] = tY; M[2][3] = tZ; M[3][3] = 1.0F;
};

inline void assignMat(Matrix4x4& dest, Matrix4x4& source)
{
  memcpy(dest, source, sizeof(Matrix4x4));
};

inline void MulVecMatZonly(Matrix4x4& M, float* V, float* transformedZ)
{
  *transformedZ = M[2][0] * V[VEC_X] + M[2][1] * V[VEC_Y] + M[2][2] * V[VEC_Z] + M[2][3];
};


void combineMat(Matrix4x4& A, Matrix4x4& B, Matrix4x4& res);
void combineMat(Matrix4x4& A, Matrix4x4& B); //"A *= B"
void MulVecMat(Matrix4x4&M, float* V); // "V = M * V"
void MulVecMat(Matrix4x4&M, float* V, float* res);
void RotationMat(Matrix4x4& M, float Xan, float Yan, float Zan);


#endif
