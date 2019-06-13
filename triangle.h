/******************************************************/
/*                                                    */
/* triangle.h - triangles                             */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of Decisite.
 * 
 * Decisite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Decisite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decisite. If not, see <http://www.gnu.org/licenses/>.
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
  void setneighbor(triangle *neigh);
  void setnoneighbor(edge *neigh);
  double areaCoord(xy pnt,point *v);
  double elevation(xy pnt);
  void flatten();
  bool inTolerance(double tolerance);
  void setError(double tolerance);
  void unsetError();
  xyz gradient3(xy pnt);
  xy gradient(xy pnt);
  triangleHit hitTest(xy pnt);
  bool in(xy pnt);
  bool inCircle(xy pnt,double radius);
  bool iscorner(point *v);
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
  edge *checkBentContour();
private:
};

double deriv0(std::vector<double> xsect);
double deriv1(std::vector<double> xsect);
double deriv2(std::vector<double> xsect);
double deriv3(std::vector<double> xsect);
double paravertex(std::vector<double> xsect);

#endif
