/******************************************************/
/*                                                    */
/* spiral.h - Cornu or Euler spirals                  */
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
#ifndef SPIRAL_H
#define SPIRAL_H

#include <cmath>
#include "point.h"
#include "segment.h"
#include "arc.h"

/* Clothance is the derivative of curvature with respect to distance
 * along the curve.
 */
xy cornu(double t); //clothance=2
xy cornu(double t,double curvature,double clothance);
double spiralbearing(double t,double curvature,double clothance);
int ispiralbearing(double t,double curvature,double clothance);
double spiralcurvature(double t,double curvature,double clothance);
void cornustats();

class spiralarc: public segment
/* station() ignores the x and y coordinates of start and end.
 * mid, midbear, etc. must be set so that the station() values
 * match start and end. This can take several iterations.
 *
 * d and s: d is the bearing at the end - the bearing at the start.
 * s is the sum of the start and end bearings - twice the chord bearing
 * (in setdelta) or - twice the midpoint bearing (in _setdelta).
 */
{
private:
  xy mid;
  double cur,clo,len;
  int midbear;
  double _diffarea(double totcur,double totclo);
public:
  spiralarc();
  spiralarc(const segment &b);
  spiralarc(const arc &b);
  spiralarc(xyz kra,xyz fam);
  spiralarc(xyz kra,double c1,double c2,xyz fam);
  spiralarc(xyz kra,int sbear,double c1,double c2,double length,double famElev);
  spiralarc(xyz kra,xy mij,xyz fam,int mbear,double curvature,double clothance,double length);
  spiralarc(xyz pnt,double curvature,double clothance,int bear,double startLength,double endLength);
  virtual int type();
  virtual double in(xy pnt);
  virtual xy pointOfIntersection();
  virtual double tangentLength(int which);
  virtual double diffarea();
  spiralarc operator-() const;
  double length() const
  {
    return len;
  }
  virtual int bearing(double along) const
  {
    return midbear+ispiralbearing(along-len/2,cur,clo);
  }
  int startbearing() const
  {
    return midbear+ispiralbearing(-len/2,cur,clo);
  }
  int endbearing() const
  {
    return midbear+ispiralbearing(len/2,cur,clo);
  }
  virtual double curvature(double along) const
  {
    return cur+clo*(along-len/2);
  }
  virtual double radius(double along) const
  {
    return 1/curvature(along);
  }
  virtual int getdelta()
  {
    return radtobin(cur*len);
  }
  virtual int getdelta2()
  {
    return startbearing()+endbearing()-2*dir(xy(start),xy(end));
  }
  virtual double clothance()
  {
    return clo;
  }
  virtual xyz station(double along) const;
  virtual xy center();
  virtual double sthrow();
  /* "throw" is a reserved word.
   * The throw is the minimum distance between the circles (one of which may be a line)
   * that osculate the ends of the spiral arc. It is also called the offset, but
   * offset() will create a segment or arc that is parallel to the given one.
   * Trying to offset a spiral arc will throw an error.
   */
  virtual bool isCurly();
  virtual bool isTooCurly();
  void _setdelta(int d,int s=0);
  void _setcurvature(double startc,double endc);
  void _fixends(double p);
  void split(double along,spiralarc &a,spiralarc &b);
  virtual void lengthen(int which,double along);
  void setdelta(int d,int s=0);
  virtual void setcurvature(double startc,double endc);
  bool valid()
  {
    return (std::isfinite(cur) && std::isfinite(clo) && std::isfinite(len));
  }
};

#endif
