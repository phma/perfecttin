/******************************************************/
/*                                                    */
/* contour.h - generates contours                     */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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

#ifndef CONTOUR_H
#define CONTOUR_H
#include <vector>
#include "polyline.h"
#include "ps.h"
#define CCHALONG 0.30754991027012474516361707317
// This is sqrt(4/27) of the way from 0.5 to 0. See clampcubic in Bezitopo.
#define M_SQRT_10 3.16227766016837933199889354

class pointlist;

class ContourInterval
{
  /* interval is in meters. When interval is in the display unit, they may be:
   * int fine coarse
   *  1    5     4
   *  2    5     5
   *  5    4     5
   *  1    1     5
   *  2    1     5
   *  5    1     4
   * with int multiplied by any power of 10. The coarse interval tells which
   * contours are labeled with elevations, the medium interval tells which
   * contours are completely drawn, and the fine interval, if different from
   * the medium, tells which contours are drawn only on nearly flat ground.
   */
public:
  ContourInterval();
  ContourInterval(double unit,int icode,bool fine);
  double fineInterval() const
  {
    return interval;
  };
  double mediumInterval() const
  {
    return interval*fineRatio;
  };
  double coarseInterval() const
  {
    return interval*fineRatio*coarseRatio;
  };
  double tolerance() const
  {
    return interval*fineRatio*relativeTolerance;
  };
  double getRelativeTolerance() const
  {
    return relativeTolerance;
  };
  void setRelativeTolerance(double tol)
  {
    relativeTolerance=tol;
  }
  void setInterval(double unit,int icode,bool fine);
  void setIntervalRatios(double i,int f,int c);
  std::string valueString(double unit,bool precise=false);
  std::string valueToleranceString() const;
  int contourType(double elev);
  friend bool operator<(const ContourInterval &l,const ContourInterval &r);
  friend bool operator==(const ContourInterval &l,const ContourInterval &r);
  friend bool operator!=(const ContourInterval &l,const ContourInterval &r);
  void writeXml(std::ostream &ofile);
private:
  double interval,relativeTolerance;
  int fineRatio,coarseRatio;
};

struct ContourLayer
/* These correspond to layers when exporting as DXF. Each ContourInterval
 * has a set of 4 or 5 or 2 layers. E.g. if they are 1 m 1/2 contours, either
 * 1 m 1/2 200: 0,5,10,15...
 * 1 m 1/2 104: 1,6,11,16...
 * 1 m 1/2 108: 2,7,12,17...
 * 1 m 1/2 10c: 3,8,13,18...
 * 1 m 1/2 110: 4,9,14,19...
 * or
 * 1 m 1/2 200: 0,5,10,15...
 * 1 m 1/2 100: 1,2,3,4,6...
 */
{
  ContourInterval ci;
  int tp;
  friend bool operator<(const ContourLayer &l,const ContourLayer &r);
  friend bool operator==(const ContourLayer &l,const ContourLayer &r);
  friend bool operator!=(const ContourLayer &l,const ContourLayer &r);
};

class DirtyTracker
{
public:
  void init(int n);
  bool isDirty(int n);
  void markDirty(int n,int spread);
  void markClean(int n);
  void erase(int n);
  void insert(int n);
private:
  std::vector<char> dirt;
};

float splitpoint(double leftclamp,double rightclamp,double tolerance);
std::vector<edge *> contstarts(pointlist &pts,double elev);
polyline trace(edge *edgep,double elev);
polyline intrace(triangle *tri,double elev);
double bendiness(xy a,xy b,xy c,double tolerance);
void rough1contour(pointlist &pl,double elev,int thread);
void roughcontours(pointlist &pl,double conterval);
void checkContour(pointlist &pl,polyspiral &contour,double tolerance);
void prune1contour(pointlist &pl,double tolerance,int i,int thread);
void prunecontours(pointlist &pl,double tolerance);
void smooth1contour(pointlist &pl,double tolerance,int i,int thread);
void smoothcontours(pointlist &pl,double tolerance);
int makeContourIndex();
spiralarc nthPiece(int n);
#endif
