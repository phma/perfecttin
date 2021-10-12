/******************************************************/
/*                                                    */
/* pointlist.h - list of points                       */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#ifndef POINTLIST_H
#define POINTLIST_H

class ContourLayer;

#include <map>
#include <string>
#include <vector>
#include <array>
#include <set>
#include "mthreads.h"
#include "point.h"
#include "tin.h"
#include "triangle.h"
#include "qindex.h"
#include "polyline.h"
#include "contour.h"
#include "unifiro.h"

typedef std::map<int,point> ptlist;
typedef std::map<point*,int> revptlist;

struct ContourPiece
{
  spiralarc s;
  std::vector<triangle *> tris;
};

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
  std::map<ContourInterval,std::vector<polyspiral> > contours;
  std::vector<polyspiral> *currentContours;
  polyline boundary;
  qindex qinx;
  std::vector<point*> convexHull;
  Unifiro<triangle *> trianglePool,trianglePaint;
  Unifiro<edge *> edgePool;
  Unifiro<void *> pieceDraw;
  double swishFactor; // for tracing top or bottom of a point cloud
  time_t conversionTime; // Time when conversion starts, used to identify checkpoint files
  std::shared_mutex wingEdge; // Lock this while changing pointers in the winged edge structure.
  std::map<int,std::vector<ContourPiece> > contourPieces;
  std::mutex pieceMutex;
  int pieceInx;
  void addpoint(int numb,point pnt,bool overwrite=false);
  int addtriangle(int n=1,int thread=-1);
  void insertHullPoint(point *newpnt,point *prec);
  int closestHullPoint(xy pnt);
  double distanceToHull(xy pnt);
  bool validConvexHull();
  std::vector<int> valencyHistogram();
  bool shouldWrite(int n,int flags,bool contours);
  void updateqindex();
  double elevation(xy location);
  xy gradient(xy location);
  double dirbound(int angle);
  void clear();
  int size();
  void clearmarks();
  void clearTin();
  void unsetCurrentContours();
  void setCurrentContours(ContourInterval &ci);
  void deleteCurrentContours();
  std::vector<ContourInterval> contourIntervals();
  std::map<ContourLayer,int> contourLayers();
  void insertContourPiece(spiralarc s,int thread);
  void deleteContourPiece(spiralarc s,int thread);
  std::vector<ContourPiece> getContourPieces(int inx);
  void nipPieces();
  void insertPieces(polyspiral ctour,int thread);
  void deletePieces(polyspiral ctour,int thread);
  int statsPieces();
  void eraseEmptyContours();
  int isSmoothed(const segment &seg);
  int isNextPieceSmoothed();
  bool checkTinConsistency();
  triangle *findt(xy pnt,bool clip=false);
private:
  void nipPiece();
  // the following methods are in tin.cpp
  void dumpedges();
  void dumpnext_ps(PostScript &ps);
public:
  void dumpedges_ps(PostScript &ps,bool colorfibaster);
  void maketriangles();
  void makeqindex();
  void makeEdges();
  double totalEdgeLength();
  std::array<double,2> lohi();
  std::array<double,2> lohi(polyline p,double tolerance);
  double contourError(segment seg);
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
};

#endif
