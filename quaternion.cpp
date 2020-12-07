/******************************************************/
/*                                                    */
/* quaternion.cpp - quaternions                       */
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

#include <cmath>
#include <iostream>
#include "angle.h"
#include "quaternion.h"
#include "random.h"
using namespace std;

/* Quaternions are used in least-squares adjustment to represent the
 * orientation of a total station. Normally it is oriented in any direction
 * around an axis slightly tilted from the vertical.
 */

Quaternion::Quaternion()
{
  w=x=y=z=0;
}

Quaternion::Quaternion(double r)
{
  w=r;
  x=y=z=0;
}

Quaternion::Quaternion(double r,double i,double j,double k)
{
  w=r;
  x=i;
  y=j;
  z=k;
}

Quaternion::Quaternion(double r,xyz i)
{
  w=r;
  x=i.x;
  y=i.y;
  z=i.z;
}

double Quaternion::getcomp(int n)
{
  switch (n&3)
  {
    case 0:
      return w;
    case 1:
      return x;
    case 2:
      return y;
    case 3:
      return z;
  } // Ignore warning "control reaches end". n&3 cannot be anything else.
}

double Quaternion::getreal()
{
  return w;
}

xyz Quaternion::getimag()
{
  xyz ret(x,y,z);
  return ret;
}

double Quaternion::normsq()
{
  return (w*w+x*x)+(y*y+z*z);
}

double Quaternion::norm()
{
  return sqrt(normsq());
}

void Quaternion::normalize()
{
  operator/=(norm());
}

Quaternion Quaternion::conj()
{
  Quaternion ret(w,-x,-y,-z);
  return ret;
}

Quaternion Quaternion::inv()
{
  return conj()/normsq();
}

xyz Quaternion::rotate(xyz vec)
// TODO; expand the multiplication, as this has some multiplications by 0
/* If *this is (0.5,0.5,0.5,0.5), it rotates vec 120° clockwise as seen
 * from (0,0,0) looking toward (0.5,0.5,0.5); i.e. (x,y,z) becomes (z,x,y).
 */
{
  Quaternion qvec(0,vec);
  qvec=*this*qvec*conj();
  return qvec.getimag();
}
  
bool operator!=(const Quaternion &l,const Quaternion &r)
{
  return l.w!=r.w || l.x!=r.x || l.y!=r.y || l.z!=r.z;
}

bool operator==(const Quaternion &l,const Quaternion &r)
{
  return l.w==r.w && l.x==r.x && l.y==r.y && l.z==r.z;
}

Quaternion operator+(const Quaternion &l,const Quaternion &r)
{
  Quaternion ret;
  ret.w=l.w+r.w;
  ret.x=l.x+r.x;
  ret.y=l.y+r.y;
  ret.z=l.z+r.z;
  return ret;
}

Quaternion operator-(const Quaternion &l,const Quaternion &r)
{
  Quaternion ret;
  ret.w=l.w-r.w;
  ret.x=l.x-r.x;
  ret.y=l.y-r.y;
  ret.z=l.z-r.z;
  return ret;
}

Quaternion operator*(const Quaternion &l,const Quaternion &r)
// THAT DOES NOT COMMUTE :)
{
  Quaternion ret;
  ret.w=(l.w*r.w-l.x*r.x)-(l.y*r.y+l.z*r.z);
  ret.x=(l.w*r.x+l.x*r.w)+(l.y*r.z-l.z*r.y);
  ret.y=(l.w*r.y+l.y*r.w)+(l.z*r.x-l.x*r.z);
  ret.z=(l.w*r.z+l.z*r.w)+(l.x*r.y-l.y*r.x);
  return ret;
}

Quaternion operator*(const Quaternion &l,double r)
{
  Quaternion ret;
  ret.w=l.w*r;
  ret.x=l.x*r;
  ret.y=l.y*r;
  ret.z=l.z*r;
  return ret;
}

Quaternion operator/(const Quaternion &l,double r)
{
  Quaternion ret;
  ret.w=l.w/r;
  ret.x=l.x/r;
  ret.y=l.y/r;
  ret.z=l.z/r;
  return ret;
}

Quaternion& Quaternion::operator+=(double r)
{
  w+=r;
  return *this;
}

Quaternion& Quaternion::operator-=(double r)
{
  w-=r;
  return *this;
}

Quaternion& Quaternion::operator*=(double r)
{
  w*=r;
  x*=r;
  y*=r;
  z*=r;
  return *this;
}

Quaternion& Quaternion::operator/=(double r)
{
  w/=r;
  x/=r;
  y/=r;
  z/=r;
  return *this;
}

/* A versor is a quaternion representing a rotation. versor(vec,angle) returns
 * a rotation around vec by angle. versor(vec) is used in least-squares and
 * returns a rotation as follows:
 * If vec has length ε, it rotates by ε radians.
 * If vec has length 4, it rotates by 180°.
 * If vec has length 16/ε, it rotates by π-ε radians.
 * Rotating by 360°, considered as a path through rotation space, is NOT the
 * same as not rotating, but rotating by 720° IS. That's another reason why
 * 2**32 stands for 720°.
 */
Quaternion versor(xyz vec)
{
  Quaternion ret;
  ret.w=4;
  ret.x=vec.x;
  ret.y=vec.y;
  ret.z=vec.z;
  ret/=ret.normsq()/4;
  ret-=2;
  return ret/2;
}

Quaternion versor(xyz vec,int angle)
{
  Quaternion real(1),imag(0);
  imag.x=vec.x;
  imag.y=vec.y;
  imag.z=vec.z;
  imag.normalize();
  if (imag.normsq()==0)
    angle=0;
  if (std::isinf(imag.normsq()))
  {
    angle=DEG360;
    imag=Quaternion(0);
  }
  return real*coshalf(angle)+imag*sinhalf(angle);
}

Quaternion versor(xyz vec,double angle)
{
  Quaternion real(1),imag(0);
  imag.x=vec.x;
  imag.y=vec.y;
  imag.z=vec.z;
  imag.normalize();
  if (imag.normsq()==0)
    angle=0;
  if (std::isinf(imag.normsq()))
  {
    angle=DEG360;
    imag=Quaternion(0);
  }
  return real*cos(angle/2)+imag*sin(angle/2);
}

Quaternion randomacc=1;

Quaternion randomVersor()
{
  unsigned int r;
  int i,j;
  Quaternion m;
  for (i=j=0;i<5 || j<2;i++)
  {
    r=rng.uirandom();
    m=Quaternion(((r>>0)&255)-127.5,((r>>8)&255)-127.5,((r>>16)&255)-127.5,((r>>24)&255)-127.5);
    randomacc=randomacc*m/128;
    if (m.norm()<=128)
      j++;
    if (randomacc.norm()<0.5)
      randomacc*=2;
    if (randomacc.norm()>2)
      randomacc/=2;
  }
  //cout<<"i="<<i<<" j="<<j<<endl;
  return randomacc/randomacc.norm();
}
