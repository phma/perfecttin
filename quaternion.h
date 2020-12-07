/******************************************************/
/*                                                    */
/* quaternion.h - quaternions                         */
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

#ifndef QUATERNION_H
#define QUATERNION_H

class Quaternion;

#include "point.h"

class Quaternion
{
public:
  Quaternion();
  Quaternion(double r);
  Quaternion(double r,double i,double j,double k);
  Quaternion(double r,xyz i);
  double getcomp(int n);
  double getreal();
  xyz getimag();
  double normsq();
  double norm();
  void normalize();
  Quaternion conj();
  Quaternion inv();
  xyz rotate(xyz vec);
  friend bool operator!=(const Quaternion &l,const Quaternion &r);
  friend bool operator==(const Quaternion &l,const Quaternion &r);
  friend Quaternion operator+(const Quaternion &l,const Quaternion &r);
  friend Quaternion operator-(const Quaternion &l,const Quaternion &r);
  friend Quaternion operator*(const Quaternion &l,const Quaternion &r);
  friend Quaternion operator*(const Quaternion &l,double r);
  friend Quaternion operator/(const Quaternion &l,double r);
  // operator/ for two quaternions is not defined.
  // Multiply by the reciprocal on the appropriate side.
  Quaternion& operator+=(double r);
  Quaternion& operator-=(double r);
  Quaternion& operator*=(double r);
  Quaternion& operator/=(double r);
  friend Quaternion versor(xyz vec);
  friend Quaternion versor(xyz vec,int angle);
  friend Quaternion versor(xyz vec,double angle);
protected:
  double w,x,y,z;
};

Quaternion randomVersor();
#endif
