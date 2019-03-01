/******************************************************/
/*                                                    */
/* pointlist.h - list of points                       */
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
private:
  std::vector<segment> break0;
public:
  ptlist points;
  revptlist revpoints;
  std::map<int,edge> edges;
  std::map<int,triangle> triangles;
  /* edges and triangles are logically arrays from 0 to size()-1, but are
   * implemented as maps, because they have pointers to each other, and points
   * point to edges, and the pointers would be messed up by moving memory
   * when a vector is resized.
   */
  qindex qinx;
  pointlist();
  void addpoint(int numb,point pnt,bool overwrite=false);
  void clear();
  int size();
  void clearmarks();
  void clearTin();
  bool checkTinConsistency();
  int1loop toInt1loop(std::vector<point *> ptrLoop);
  std::vector<point *> fromInt1loop(int1loop intLoop);
  intloop boundary();
  int readCriteria(std::string fname,Measure ms);
  void setgradient(bool flat=false);
  void findedgecriticalpts();
  void findcriticalpts();
  void addperimeter();
  void removeperimeter();
  triangle *findt(xy pnt,bool clip=false);
  // the following methods are in tin.cpp
private:
  void dumpedges();
  void dumpnext_ps(PostScript &ps);
public:
  void dumpedges_ps(PostScript &ps,bool colorfibaster);
  void splitBreaklines();
  int checkBreak0(edge &e);
  bool shouldFlip(edge &e);
  bool tryStartPoint(PostScript &ps,xy &startpnt);
  int1loop convexHull();
  int flipPass(PostScript &ps,bool colorfibaster);
  void maketin(std::string filename="",bool colorfibaster=false);
  void makegrad(double corr);
  void maketriangles();
  void makeqindex();
  void updateqindex();
  void makeBareTriangles(std::vector<std::array<xyz,3> > bareTriangles);
  void triangulatePolygon(std::vector<point *> poly);
  void makeEdges();
  void fillInBareTin();
  double totalEdgeLength();
  double elevation(xy location);
  double dirbound(int angle);
  std::array<double,2> lohi();
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
};

#endif
