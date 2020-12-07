/******************************************************/
/*                                                    */
/* bezier3d.h - 3d Bézier splines                     */
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

#ifndef BEZIER3D_H
#define BEZIER3D_H
#include <vector>
#include "point.h"
#include "quaternion.h"

class bezier3d
{
private:
  std::vector<xyz> controlpoints;
public:
  bezier3d(xyz kra,xyz con1,xyz con2,xyz fam);
  bezier3d(xyz kra,int bear0,double slp0,double slp1,int bear1,xyz fam);
  bezier3d();
  int size() const; // number of Bézier segments
  void close();
  bool isopen();
  std::vector<xyz> operator[](int n);
  friend bezier3d operator+(const bezier3d &l,const bezier3d &r); // concatenates, not adds
  bezier3d& operator+=(const bezier3d &r);
  void rotate(Quaternion q);
};

double bez3destimate(xy kra,int bear0,double len,int bear1,xy fam);
#endif
