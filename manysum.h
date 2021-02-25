/******************************************************/
/*                                                    */
/* manysum.h - add many numbers                       */
/*                                                    */
/******************************************************/
/* Copyright 2019,2021 Pierre Abbat.
 * This file is part of PerfectTIN.
 *
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License and Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and Lesser General Public License along with PerfectTIN. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef MANYSUM_H
#define MANYSUM_H
#include <map>
#include <vector>
#include <cmath>
/* Adds together many numbers (like millions) accurately.
 * pairwisesum takes an array or vector with the numbers already computed.
 * manysum stores numbers as they are computed in a buffer, then calls pairwisesum
 * when the buffer is full, and stores the result in the next buffer.
 * See matrix.cpp and spiral.cpp for examples of pairwisesum.
 */

class manysum
{
private:
  size_t count;
  double stage0[8192],stage1[8192],stage2[8192],
         stage3[8192],stage4[4096];
public:
  manysum();
  void clear();
  double total();
  manysum& operator+=(double x);
  manysum& operator-=(double x);
};

double pairwisesum(double *a,unsigned n);
double pairwisesum(std::vector<double> &a);
long double pairwisesum(long double *a,unsigned n);
long double pairwisesum(std::vector<long double> &a);
#endif
