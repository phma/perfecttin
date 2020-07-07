/******************************************************/
/*                                                    */
/* octagon.h - bound the points with an octagon       */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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
#include <array>
#include "pointlist.h"

struct BoundBlockResult
{
  BoundRect orthogonal,diagonal;
  double high,low;
  bool ready;
};

struct BoundBlockTask
{
  BoundBlockTask();
  xyz *dots;
  int numDots,ori;
  BoundBlockResult *result;
};

extern pointlist net;
extern double clipLow,clipHigh;
extern std::array<double,2> areadone;
void setMutexArea(double area);
double estimatedDensity();
double makeOctagon();
int mtxSquare(xy pnt);
