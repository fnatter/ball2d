#ifndef __MATRIX_H

class Matrix
{
 public:
  int rows, columns;
  double** elems;

  void setzero();
  void print_tm();
  void ero_type1(int row1, int row2);
  void ero_type2(int row, double factor);
  void ero_type3(int dest_row, int source_row, double factor);
  void makeREF();
  bool column_is_zero(int column);
  bool row_is_zero(int row);
  bool is_zero();
  void transpose();
  Matrix& operator*(Matrix& rightM);
  Matrix& operator+(Matrix& rightM);
  Matrix& operator-(Matrix& rightM);
  void operator=(Matrix& rightM);
  Matrix& operator*(double scalar);

  Matrix(int i_rows, int i_cols);
  Matrix(Matrix& otherMatrix);
  ~Matrix();
};

#endif
