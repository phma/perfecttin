/******************************************************/
/*                                                    */
/* matrix.h - matrices                                */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 * 
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PerfectTIN. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATRIX_H
#define MATRIX_H
#include <vector>

#define BYTERMS 104.51076499576490995918
/* square root of 10922.5, which is the root-mean-square of a random byte
 * doubled and offset to center
 */

#define matrixmismatch 6
// operation on matrices is impossible because the sizes don't match

struct rowsult
{
  int pivot; // which column was selected for the pivot, or -1 if the rows are all 0
  int flags; // which of the three elementary row operations were done
  double detfactor; // these are multiplied together to compute the determinant
};

class matrix
{
protected:
  unsigned rows,columns;
  double *entry;
  rowsult rowop(matrix &b,int row0,int row1,int piv);
  double _determinant();
  bool findpivot(matrix &b,int row,int column);
public:
  matrix();
  matrix(unsigned r,unsigned c);
  matrix(const matrix &b);
  ~matrix();
  void resize(unsigned newrows,unsigned newcolumns);
  unsigned getrows()
  {
    return rows;
  }
  unsigned getcolumns()
  {
    return columns;
  }
  void setzero();
  void setidentity();
  void dump();
  matrix &operator=(const matrix &b);
  double *operator[](unsigned row);
  matrix operator+(matrix& b);
  matrix operator-(matrix& b);
  matrix operator*(matrix& b);
  double trace();
  matrix transpose();
  matrix transmult();
  void swaprows(unsigned r0,unsigned r1);
  void swapcolumns(unsigned c0,unsigned c1);
  void gausselim(matrix &b);
  void randomize_c();
  double determinant();
  operator std::vector<double>() const;
};

matrix invert(matrix m);
matrix rowvector(const std::vector<double> v);
matrix columnvector(const std::vector<double> v);
#endif
