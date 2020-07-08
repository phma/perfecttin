/******************************************************/
/*                                                    */
/* adjelev.h - adjust elevations of points            */
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

#ifndef ADJELEV_H
#define ADJELEV_H
#include "matrix.h"
#include "triangle.h"

#define AVG_TASK_SIZE 1024
// AVG_TASK_SIZE should be even and must be between 6 and 1301505241.

struct adjustRecord
{
  bool validMatrix;
  double msAdjustment;
};

struct AdjustBlockResult
{
  matrix mtmPart;
  matrix mtvPart;
  double high,low;
  bool ready;
};

struct AdjustBlockTask
{
  AdjustBlockTask();
  triangle *tri;
  std::vector<point *> pnt;
  xyz *dots;
  int numDots;
  AdjustBlockResult *result;
};

std::vector<int> blockSizes(int total);
adjustRecord adjustElev(std::vector<triangle *> tri,std::vector<point *> pnt);
void computeAdjustBlock(AdjustBlockTask &task);
void logAdjustment(adjustRecord rec);
double rmsAdjustment();
void adjustLooseCorners(double tolerance);
void clearLog();
#endif
