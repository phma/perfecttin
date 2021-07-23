/******************************************************/
/*                                                    */
/* arc.h - horizontal circular arcs                   */
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

#ifndef ARC_H
#define ARC_H
#include "segment.h"
#include "angle.h"
#include "point.h"

class arc: public segment
{
private:
  int delta; // angle subtended - "delta" is a surveying term
  double rchordbearing; // chordbearing in radians
public:
  arc();
  arc(xyz kra,xyz fam);
  arc(xyz kra,xyz mij,xyz fam);
  arc(xyz kra,xyz fam,int d);
  virtual void setdelta(int d,int s=0);
  virtual void setcurvature(double startc,double endc);
  virtual double radius(double along) const
  {
    return chordlength()/sinhalf(delta)/2;
  }
  virtual double curvature(double along) const
  {
    return 2*sinhalf(delta)/chordlength();
  }
  virtual xy pointOfIntersection();
  virtual double tangentLength(int which);
  virtual double diffarea();
  int startbearing() const
  {
    return chordbearing()-delta/2;
  }
  int endbearing() const
  {
    return chordbearing()+delta/2;
  }
  virtual int getdelta()
  {
    return delta;
  }
  virtual int getdelta2()
  {
    return 0;
  }
  virtual double in(xy pnt);
  double length() const;
  virtual double epsilon() const;
  virtual xyz station(double along) const;
  virtual int bearing(double along) const;
  virtual xy center();
  virtual bool isCurly();
  virtual bool isTooCurly();
  void split(double along,arc &a,arc &b);
  virtual void lengthen(int which,double along);
};

#endif
