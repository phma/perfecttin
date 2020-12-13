/******************************************************/
/*                                                    */
/* triangle.h - triangles                             */
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

#ifndef TRIANGLE_H
#define TRIANGLE_H
#include <vector>
#include <array>
#include "cogo.h"
#define M_SQRT_3_4 0.86602540378443864676372317
#define M_SQRT_3 1.73205080756887729352744634
#define M_SQRT_1_3 0.5773502691896257645091487805
#define M_SQRT_432 20.78460969082652752232935609
#define PT_MIN 0
#define PT_FLAT 1
#define PT_MAX 2
#define PT_SLOPE 3
#define PT_SADDLE 4
#define PT_MONKEY 5
#define PT_GRASS 6

#ifndef NDEBUG
class testfunc
{
public:
  double coeff[4];
  testfunc(double cub,double quad,double lin,double con);
  double operator()(double x);
};
double parabinter(testfunc func,double start,double startz,double end,double endz);
#endif

struct triangleHit
{
  triangle *tri;
  edge *edg;
  point *cor;
};

struct ErrorBlockResult
{
  double vError;
  bool ready;
};

struct ErrorBlockTask
{
  ErrorBlockTask();
  xyz *dots;
  int numDots;
  triangle *tri;
  double tolerance;
  ErrorBlockResult *result;
};

void computeErrorBlock(ErrorBlockTask &task);

class triangle
/* A triangle has three corners, arranged as follows:
 *   a
 * b   c
 * The corners are arranged counterclockwise so that the area is always nonnegative
 * (it can be zero in pathological cases such as all the points in a line, but then
 * the elevation and gradient at a point are impossible to compute). It can compute
 * the elevation and gradient at any point, but they are valid only inside the triangle.
 */
{
public:
  point *a,*b,*c; //corners
  double peri,sarea;
  triangle *aneigh,*bneigh,*cneigh;
  double gradmat[2][3]; // to compute gradient from three partial gradients
  std::vector<xyz> dots;
  int flags;
  double aElev,bElev,cElev,vError;
  triangle();
  bool ptValid();
  void setneighbor(triangle *neigh);
  void setnoneighbor(edge *neigh);
  void setEdgeTriPointers();
  double areaCoord(xy pnt,point *v);
  double elevation(xy pnt);
  void flatten();
  bool inTolerance(double tolerance,double minArea);
  void setError(double tolerance);
  void unsetError();
  xyz gradient3(xy pnt);
  xy gradient(xy pnt);
  triangleHit hitTest(xy pnt);
  bool in(xy pnt);
  int quadrant(xy pnt);
  bool inCircle(xy pnt,double radius);
  bool iscorner(point *v);
  point *otherCorner(point *v0,point *v1);
  triangle *nexttoward(xy pnt);
  triangle *findt(xy pnt,bool clip=false);
  double area();
  double perimeter();
  double acicularity();
  xy centroid();
  void setgradmat();
  std::vector<double> xsect(int angle,double offset);
  double spelevation(int angle,double x,double y);
  int pointtype(xy pnt);
  edge *edgepart(int subdir);
  std::array<double,2> lohi();
  int subdir(edge *edgepart);
  int proceed(int subdir,double elevation);
  bool crosses(int subdir,double elevation);
  bool upleft(int subdir);
  xy contourcept(int subdir,double elevation);
  edge *checkBentContour();
private:
};

double deriv0(std::vector<double> xsect);
double deriv1(std::vector<double> xsect);
double deriv2(std::vector<double> xsect);
double deriv3(std::vector<double> xsect);
double paravertex(std::vector<double> xsect);

#endif
