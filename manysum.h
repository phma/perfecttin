/******************************************************/
/*                                                    */
/* manysum.h - add many numbers                       */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
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
 * Each number is put in a bucket numbered by its exponent returned by frexp().
 * If the sum of the number and what's in the bucket is too big to fit
 * in the bucket, it is put into the next bucket up. If the sum is too small,
 * it is not put into the next bucket down, as it has insignificant bits
 * at the low end. This can happen only when adding numbers of opposite sign,
 * which does not happen when computing volumes, as positive and negative
 * volumes are added separately so that compaction of dirt can be computed.
 * 
 * manysum is suited to sums where the number of addends is large and not known
 * in advance and computing each one takes a long time. If the number of addends
 * is known, or a bound is known, in advance and the addends are calculated
 * quickly, it is better to allocate an array and use pairwise summation.
 * See matrix.cpp and spiral.cpp for examples.
 */

class manysum
{
private:
  std::map<int,double> bucket;
  static int cnt;
public:
  void clear();
  void prune();
  double total();
  void dump();
  manysum& operator+=(double x);
  manysum& operator-=(double x);
};

double pairwisesum(double *a,unsigned n);
double pairwisesum(std::vector<double> &a);
long double pairwisesum(long double *a,unsigned n);
long double pairwisesum(std::vector<long double> &a);
#endif
