/******************************************************/
/*                                                    */
/* point.h - classes for points                       */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 *
 * PerfectTIN is free software: you can redistribute it and/or modify
 * modify it under the terms of the GNU Lesser General Public License as
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

#ifndef POINT_H
#define POINT_H
#include <string>
#include <fstream>
#include <vector>

class xyz;
class latlong;

class xy
{
public:
  xy(double e,double n);
  xy(xyz point);
  xy();
  double east() const;
  double north() const;
  double getx() const;
  double gety() const;
  double length() const;
  bool isfinite() const;
  bool isnan() const;
  double dirbound(int angle);
  void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
  friend xy operator+(const xy &l,const xy &r);
  friend xy operator+=(xy &l,const xy &r);
  friend xy operator-=(xy &l,const xy &r);
  friend xy operator-(const xy &l,const xy &r);
  friend xy operator-(const xy &r);
  friend xy operator*(const xy &l,double r);
  friend xy operator*(double l,const xy &r);
  friend xy operator/(const xy &l,double r);
  friend xy operator/=(xy &l,double r);
  friend bool operator!=(const xy &l,const xy &r);
  friend bool operator==(const xy &l,const xy &r);
  friend xy turn90(xy a);
  friend xy turn(xy a,int angle);
  friend double dist(xy a,xy b);
  friend int dir(xy a,xy b);
  friend double dot(xy a,xy b);
  friend double area3(xy a,xy b,xy c);
  friend class triangle;
  friend class point;
  friend class xyz;
  friend class qindex;
protected:
  double x,y;
};

extern const xy nanxy;

class xyz
{
public:
  xyz(double e,double n,double h);
  xyz();
  xyz(xy en,double h);
  double east() const;
  double north() const;
  double elev() const;
  double getx() const;
  double gety() const;
  double getz() const;
  bool isfinite() const;
  bool isnan() const;
  double length();
  void raise(double height);
  void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
  virtual void roscat(xy tfrom,int ro,double sca,xy tto);
  void setelev(double h)
  {
    z=h;
  }
  friend class xy;
  friend class point;
  friend class triangle;
  friend double dist(xyz a,xyz b);
  friend double dot(xyz a,xyz b);
  friend xyz cross(xyz a,xyz b);
  friend bool operator==(const xyz &l,const xyz &r);
  friend bool operator!=(const xyz &l,const xyz &r);
  friend xyz operator/(const xyz &l,const double r);
  friend xyz operator*=(xyz &l,double r);
  friend xyz operator/=(xyz &l,double r);
  friend xyz operator+=(xyz &l,const xyz &r);
  friend xyz operator-=(xyz &l,const xyz &r);
  friend xyz operator*(const xyz &l,const double r);
  friend xyz operator*(const double l,const xyz &r);
  friend xyz operator*(const xyz &l,const xyz &r); // cross product
  friend xyz operator+(const xyz &l,const xyz &r);
  friend xyz operator-(const xyz &l,const xyz &r);
  friend xyz operator-(const xyz &r);
protected:
  double x,y,z;
};

extern const xyz nanxyz;

class edge;
class triangle;
class pointlist;

class point: public xyz
{
public:
  using xyz::roscat;
  using xyz::_roscat;
  //xy pagepos; //used when dumping a lozenge in PostScript
  point();
  point(double e,double n,double h);
  point(xy pnt,double h);
  point(xyz pnt);
  point(const point &rhs);
  //~point();
  const point& operator=(const point &rhs);
  friend class edge;
  friend void rotate(pointlist &pl,int n);
  friend void movesideways(pointlist &pl,double sw);
  friend void moveup(pointlist &pl,double sw);
  friend void enlarge(pointlist &pl,double sw);
  edge *line; // a line incident on this point in the TIN. Used to arrange the lines in order around their endpoints.
  edge *edg(triangle *tri);
  // tri.a->edg(tri) is the side opposite tri.b
  int valence();
  std::vector<edge *> incidentEdges();
  edge *isNeighbor(point *pnt);
  void insertEdge(edge *edg);
};
#endif
