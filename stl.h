/******************************************************/
/*                                                    */
/* stl.h - stereolithography (3D printing) export     */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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
#ifndef STL_H
#define STL_H
#include <array>
#include <vector>
#include "point.h"
#include "config.h"

#define P3S_ABSOLUTE 0
#define P3S_RECTANGULAR 1
// TODO: handle circular and hexagonal

struct StlTriangle
{
  xyz normal,a,b,c;
  std::string attributes;
  StlTriangle();
  StlTriangle(xyz A,xyz B,xyz C);
};

struct Printer3dSize
{
  int shape;
  double x,y,z; // all in millimeters
  unsigned scaleNum,scaleDenom;
  double minBase;
};

double hScale(pointlist &ptl,Printer3dSize &pri,int ori);
int turnFitInPrinter(pointlist &ptl,Printer3dSize &pri);
std::vector<StlTriangle> stlMesh(Printer3dSize &pri,bool roundScale,bool feet);
void writeStlBinary(std::ostream &file,std::vector<StlTriangle> &mesh);
void writeStlText(std::ostream &file,std::vector<StlTriangle> &mesh);

#endif
