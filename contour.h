/******************************************************/
/*                                                    */
/* contour.h - generates contours                     */
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

#ifndef CONTOUR_H
#define CONTOUR_H
#include <vector>
#include "polyline.h"
#include "ps.h"
#define CCHALONG 0.30754991027012474516361707317
// This is sqrt(4/27) of the way from 0.5 to 0. See clampcubic.
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
  double fineInterval()
  {
    return interval;
  };
  double mediumInterval()
  {
    return interval*fineRatio;
  };
  double coarseInterval()
  {
    return interval*fineRatio*coarseRatio;
  };
  int contourType(double elev);
  void writeXml(std::ostream &ofile);
private:
  double interval;
  int fineRatio,coarseRatio;
};

float splitpoint(double leftclamp,double rightclamp,double tolerance);
std::vector<edge *> contstarts(pointlist &pts,double elev);
polyline trace(edge *edgep,double elev);
polyline intrace(triangle *tri,double elev);
bool ismarked(uintptr_t ep);
void rough1contour(pointlist &pl,double elev);
void roughcontours(pointlist &pl,double conterval);
void smooth1contour(pointlist &pl,double conterval,int i,bool spiral,PostScript &ps,
                    double we,double ea,double so,double no);
void smoothcontours(pointlist &pl,double conterval,bool spiral=true,bool log=false);
#endif
