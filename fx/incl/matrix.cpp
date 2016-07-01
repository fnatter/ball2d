#include "matrix.h"
#include <stdio.h>
#include <conio.h>

Matrix::Matrix(int i_rows, int i_cols)
{
  rows = i_rows;
  columns = i_cols;
  int i;
  elems = (double**)malloc(sizeof(double*) * rows);
  for (i = 0; i < rows; i++)
    elems[i] = (double*)malloc(sizeof(double) * columns);
  setzero();
  // array[x] = *(array + x)
  // elems[r][c] = *(elems[r] + c)
};

Matrix::Matrix(Matrix& otherMatrix)
{
  rows = otherMatrix.rows;
  columns = otherMatrix.columns;
  int i, j;
  elems = (double**)malloc(sizeof(double*) * rows);
  for (i = 0; i < rows; i++)
    {
      elems[i] = (double*)malloc(sizeof(double) * columns);
      for (j = 0; j < columns; j++)
	elems[i][j] = otherMatrix.elems[i][j];
    }
};

Matrix::~Matrix()
{
  int i;
  for (i = 0; i < rows; i++)
    free(elems[i]);
  free(elems);
};

void Matrix::transpose()
{
  Matrix temp(*this);
  int i, j;
  for (i = 0; i < rows; i++)
    free(elems[i]);
  free(elems);
  i = rows;
  rows = columns;
  columns = i;
  elems = (double**)malloc(sizeof(double*) * rows);
  for (i = 0; i < rows; i++)
    elems[i] = (double*)malloc(sizeof(double) * columns);
  for (i = 0; i < rows; i++)
    for (j = 0; j < columns; j++)
      elems[i][j] = temp.elems[j][i];
};

void Matrix::setzero()
{
  int i, j;
  for (i = 0; i < rows; i++)
    for (j = 0; j < columns; j++)
      elems[i][j] = 0.0;
};

void Matrix::print_tm()
{
  int i, j;
  printf("\n");
  for (i = 0; i < rows; i++)
    {
      if (i == 0)
	printf("É");
      else if (i == rows - 1)
	printf("È");
      else printf("º");
      for (j = 0; j < columns; j++)
	printf(" %10.2lf", elems[i][j]);
      if (i == 0)
	printf("»\n");
      else if (i == rows - 1)
	printf("¼\n");
      else printf("º\n");
    }
  printf("\n");
};

void Matrix::ero_type1(int row1, int row2)
{
  double temp;
  int i;
  for (i = 0; i < columns; i++)
    {
      temp = elems[row1][i];
      elems[row1][i] = elems[row2][i];
      elems[row2][i] = temp;
    }
};

void Matrix::ero_type2(int row, double factor)
{
  int i;
  for (i = 0; i < columns; i++)
    elems[row][i] = elems[row][i] * factor;
};

void Matrix::ero_type3(int dest_row, int source_row, double factor)
{
  int i;
  double temp;
  for (i = 0; i < columns; i++)
    {
      temp = elems[source_row][i] * factor; // this is needed! otherwise there won't be enough precision!
      elems[dest_row][i] = elems[dest_row][i] + temp;
    }
};

void Matrix::makeREF() /* function sometimes fails because of inaccuracy in math!
                          i.e.: there is x*10^(-15) where there should be 0! */
{
  int i, j, k, p;
  k = 0; // k is the first row of the partition
 start:
  // return if the partition is equal to zero
  for (i = k, j = 0; i < rows; i++)
    if (row_is_zero(i))
      j++;
  if (j == rows - k)
    return;
  //find first nonzero column (p) in the partition (rows >= k)
  p = 0;
  do
    {
      j = 0;
      for (i = k; i < rows; i++)
	if (elems[i][p] != 0.0)
	  j++;
      p++;
    } while (j == 0); // j is the number of nonzero entries in column p of the partiting containing all rows except rows < k
  p--;
  //find first row in the partition that has a nonzero entry in column p
  i = k;
  while (elems[i][p] == 0.0)
    i++;
  // change rows such that this row is first in the partition, ero, type 1
  ero_type1(k, i);
  // use ero, type II, so that the leading entry of the first row of the partition is equal to 1
  ero_type2(k, 1.0 / elems[k][p]); // leading entry of column k is elems[k][p]
  //reduce all other entries of column p (this time of the whole Matrix) to zero by subtracting appropriate multiples of first row in the partition from other rows of the Matrix, use ero, type 3
  i = 0;
  while (i < rows && !(i == k && k == rows - 1))
    {// subtract a multiple of row k from row i such that elems[i][p] = 0
      if (i == k) //  we can't change the first row of the partition itself!
	i++;
      ero_type3(i, k, -elems[i][p]/elems[k][p]); // elems[i][p] + elems[k][p] * factor = 0 <=> factor = -elem[i][p] / elem[k][p]
      i++;
    }
  k++;
  print_tm();
  getch();
  // the first k rows are excluded
  if (k < rows)
    goto start;
};

bool Matrix::column_is_zero(int column)
{
  int i;
  for (i = 0; i < rows; i++)
    if (elems[i][column] != 0.0)
      return false;
  return true;
};

bool Matrix::row_is_zero(int row)
{
  int i;
  for (i = 0; i < columns; i++)
    if (elems[row][i] != 0.0)
      return false;
  return true;
};

bool Matrix::is_zero()
{
  int i, j;
  for (i = 0; i < rows; i++)
    for (j = 0; j < columns; j++)
      if (elems[i][j] != 0.0)
	return false;
  return true;
};

int main()
{
  Matrix test(5,4); // rows, columns of system matrix
  test.elems[0][0] = 20.0;
  test.elems[0][1] = 40.0;
  test.elems[0][2] = 10.0;
  test.elems[0][3] = 20.0;
  test.elems[1][0] = 30.0;
  test.elems[1][1] = 10.0;
  test.elems[1][2] = 10.0;
  test.elems[1][3] = 18.0;
  test.elems[2][0] = 10.0;
  test.elems[2][1] = 10.0;
  test.elems[2][2] = 15.0;
  test.elems[2][3] = 12.0;
  test.elems[3][0] = 25.0;
  test.elems[3][1] = 20.0;
  test.elems[3][2] = 40.0;
  test.elems[3][3] = 30.0;
  test.elems[4][0] = 15.0;
  test.elems[4][1] = 20.0;
  test.elems[4][2] = 25.0;
  test.elems[4][3] = 20.0;
  test.print_tm();
  test.makeREF();
  test.print_tm();
}
