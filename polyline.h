/******************************************************/
/*                                                    */
/* polyline.h - polylines                             */
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
#ifndef POLYLINE_H
#define POLYLINE_H

#include <vector>
#include "point.h"
#include "point.h"
#include "arc.h"
#include "bezier3d.h"
#include "spiral.h"

extern int bendlimit;
/* The maximum angle through which a segment of polyspiral can bend. If the bend
 * is greater, it will be replaced with a straight line segment. If angles in
 * contours have extra bulbs, it is too high; if smooth curves in contours have
 * straight line segments, it is too low. Default value is 180°. Setting it to
 * DEG360 will not work; set it to DEG360-1 instead.
 */

/* Polylines and alignments are very similar. The differences are:
 * polylines are in a horizontal plane, whereas alignments have vertical curves;
 * polylines have derived classes with arcs and spirals, whereas alignments
 * always have complete spiral data.
 */
int midarcdir(xy a,xy b,xy c);

class polyline
{
protected:
  double elevation;
  std::vector<xy> endpoints;
  std::vector<double> lengths,cumLengths;
public:
  friend class polyarc;
  friend class polyspiral;
  polyline();
  virtual ~polyline()
  {}
  explicit polyline(double e);
  double getElevation()
  {
    return elevation;
  }
  virtual void clear();
  bool isopen();
  int size();
  segment getsegment(int i);
  xyz getEndpoint(int i);
  xyz getstart();
  xyz getend();
  void dedup();
  virtual bezier3d approx3d(double precision);
  virtual void insert(xy newpoint,int pos=-1);
  virtual void replace(xy newpoint,int pos);
  virtual void erase(int pos);
  virtual void setlengths();
  virtual void open();
  virtual void close();
  virtual double in(xy point);
  double length();
  double getCumLength(int i);
  int stationSegment(double along);
  virtual xyz station(double along);
  virtual double area();
  virtual void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
};

class polyarc: public polyline
{
protected:
  std::vector<int> deltas;
public:
  friend class polyspiral;
  polyarc();
  explicit polyarc(double e);
  polyarc(polyline &p);
  virtual void clear();
  arc getarc(int i);
  virtual bezier3d approx3d(double precision);
  virtual void insert(xy newpoint,int pos=-1);
  virtual void replace(xy newpoint,int pos);
  virtual void erase(int pos);
  void setdelta(int i,int delta);
  virtual void setlengths();
  virtual void open();
  virtual void close();
  virtual double in(xy point);
  virtual xyz station(double along);
  virtual double area();
};

class polyspiral: public polyarc
{
protected:
  std::vector<int> bearings; // correspond to endpoints
  std::vector<int> delta2s;
  std::vector<int> midbearings;
  std::vector<xy> midpoints;
  std::vector<double> clothances,curvatures;
  bool curvy;
public:
  polyspiral();
  explicit polyspiral(double e);
  polyspiral(polyline &p);
  virtual void clear();
  spiralarc getspiralarc(int i);
  virtual bezier3d approx3d(double precision);
  virtual void insert(xy newpoint,int pos=-1);
  virtual void erase(int pos);
  void setbear(int i);
  void setbear(int i,int bear);
  void setspiral(int i);
  void smooth();
  //void setdelta(int i,int delta);
  virtual void setlengths();
  virtual void open();
  virtual void close();
  virtual double in(xy point);
  virtual xyz station(double along);
  virtual double area();
  virtual void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
};

#endif
