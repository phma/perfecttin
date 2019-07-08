/******************************************************/
/*                                                    */
/* pointlist.h - list of points                       */
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
#ifndef POINTLIST_H
#define POINTLIST_H

#include <map>
#include <string>
#include <vector>
#include <array>
#include <set>
#include "point.h"
#include "tin.h"
#include "triangle.h"
#include "qindex.h"

typedef std::map<int,point> ptlist;
typedef std::map<point*,int> revptlist;

class pointlist
{
public:
  ptlist points;
  revptlist revpoints;
  std::map<int,edge> edges;
  std::map<int,triangle> triangles;
  std::map<triangle*,int> revtriangles;
  /* edges and triangles are logically arrays from 0 to size()-1, but are
   * implemented as maps, because they have pointers to each other, and points
   * point to edges, and the pointers would be messed up by moving memory
   * when a vector is resized.
   */
  qindex qinx;
  void addpoint(int numb,point pnt,bool overwrite=false);
  int addtriangle(int n=1);
  void clear();
  int size();
  void clearTin();
  bool checkTinConsistency();
  triangle *findt(xy pnt,bool clip=false);
  // the following methods are in tin.cpp
private:
  void dumpedges();
  void dumpnext_ps(PostScript &ps);
public:
  void dumpedges_ps(PostScript &ps,bool colorfibaster);
  void maketriangles();
  void makeqindex();
  void updateqindex();
  void makeEdges();
  double totalEdgeLength();
  double elevation(xy location);
  double dirbound(int angle);
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
};

#endif
