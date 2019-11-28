/******************************************************/
/*                                                    */
/* cogo.cpp - coordinate geometry                     */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
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

#include <stdarg.h>
#include <stdlib.h>
#include <cmath>
#include <climits>
#include "cogo.h"
#include "random.h"
#include "angle.h"
using namespace std;

int debugdel;

char intstable[3][3][3][3]=
/* NOINT  don't intersect
   ACXBD  intersection is in the midst of both AC and BD
   BDTAC  one end of BD is in the midst of AC
   ACTBD  one end of AC is in the midst of BD
   ACVBD  one end of AC is one end of BD
   COINC  A=C or B=D
   COLIN  all four points are collinear
   IMPOS  impossible, probably caused by roundoff error
   */
//  -     -     -     0     0     0     +     +     +   B
//  -     0     +     -     0     +     -     0     +   D   A C
{ ACXBD,BDTAC,NOINT,BDTAC,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // - -
  ACTBD,ACVBD,NOINT,ACVBD,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // - 0
  NOINT,NOINT,NOINT,NOINT,COINC,NOINT,NOINT,NOINT,NOINT, // - +
  ACTBD,ACVBD,NOINT,ACVBD,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // 0 -
  IMPOS,IMPOS,COINC,IMPOS,COLIN,IMPOS,COINC,IMPOS,IMPOS, // 0 0
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,ACVBD,NOINT,ACVBD,ACTBD, // 0 +
  NOINT,NOINT,NOINT,NOINT,COINC,NOINT,NOINT,NOINT,NOINT, // + -
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,ACVBD,NOINT,ACVBD,ACTBD, // + 0
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,BDTAC,NOINT,BDTAC,ACXBD  // + +
  };

signed char intable[3][3][3][3]=
//  +    +    +    0    0    0    -    -    -  pca
//  +    0    -    +    0    -    +    0    -  pab abc pbc
{    0,   0,   0,   0,-128,-128,   0,-128,-128, //  +   -
     1, 105,   0, 105,-128,-128,   0,-128,-128, //  +   0
     2,   1,   0,   1, 105,   0,   0,   0,   0, //  +   +
     0,   0,   0,   0,-128,-128,   0,-128,-128, //  0   -
  -128,-128,   0,-128,   0,-128,   0,-128,-128, //  0   0
  -128,-128,   0,-128,-128,   0,   0,   0,   0, //  0   +
     0,   0,   0,   0, 105,  -1,   0,  -1,  -2, //  -   -
  -128,-128,   0,-128,-128, 105,   0, 105,  -1, //  -   0
  -128,-128,   0,-128,-128,   0,   0,   0,   0  //  -   +
  };

#define CMPSWAP(m,n,o) if (fabs(m)>fabs(n)) {o=m;m=n;n=o;}
#define EQOPP(m,n) if (m+n==0 && m>0) {m=-m;n=-n;}
double area3(xy a,xy b,xy c)
{
  double surface,temp,xl,xm,xh,yl,ym,yh,area[6];
  xy m;
  // Translate the points near the origin for greater precision.
  xl=a.x;xm=b.x;xh=c.x;
  yl=a.y;ym=b.y;yh=c.y;
  CMPSWAP(xl,xm,surface);
  CMPSWAP(yl,ym,temp);
  CMPSWAP(xm,xh,surface);
  CMPSWAP(ym,yh,temp);
  CMPSWAP(xl,xm,surface);
  CMPSWAP(yl,ym,temp);
  m=xy(xm,ym);
  a-=m;
  b-=m;
  c-=m;
  // Compute the six areas.
  area[0]=a.x*b.y;
  area[1]=-b.x*a.y;
  area[2]=b.x*c.y;
  area[3]=-c.x*b.y;
  area[4]=c.x*a.y;
  area[5]=-a.x*c.y;
  // Sort the six areas into absolute value order for numerical stability.
  CMPSWAP(area[0],area[1],surface); // sorting network
  CMPSWAP(area[2],area[3],temp);
  CMPSWAP(area[4],area[5],surface);
  CMPSWAP(area[0],area[2],temp);
  CMPSWAP(area[1],area[4],surface);
  CMPSWAP(area[3],area[5],temp);
  CMPSWAP(area[0],area[1],surface);
  CMPSWAP(area[2],area[3],temp);
  CMPSWAP(area[4],area[5],surface);
  CMPSWAP(area[1],area[2],temp);
  CMPSWAP(area[3],area[4],surface);
  CMPSWAP(area[2],area[3],temp);
  // Make signs of equal-absolute-value areas alternate.
  EQOPP(area[0],area[5]);
  EQOPP(area[0],area[3]);
  EQOPP(area[4],area[1]);
  EQOPP(area[2],area[5]);
  EQOPP(area[0],area[1]);
  EQOPP(area[2],area[1]);
  EQOPP(area[2],area[3]);
  EQOPP(area[4],area[3]);
  EQOPP(area[4],area[5]);
  surface=(((((area[0]+area[1])+area[2])+area[3])+area[4])+area[5])/2;
  return surface;
}

xy intersection (xy a,xy c,xy b,xy d)
//Intersection of lines ac and bd.
{
  double A,B,C,D;
  A=area3(b,c,d);
  B=area3(c,d,a);
  C=area3(d,a,b);
  D=area3(a,b,c);
  return ((a*A+c*C)+(b*B+d*D))/((A+C)+(B+D));
}

xy intersection (xy a,int aBear,xy b,int bBear)
{
  double length=dist(a,b);
  if (length==0)
    length=a.length();
  if (length==0)
    length=1;
  if ((bBear-aBear)&(DEG180-1))
    return intersection(a,a+length*cossin(aBear),b,b+length*cossin(bBear));
  else
    return(xy(NAN,NAN));
}

#define setmaxabs(a,b) a=(fabs(b)>a)?fabs(b):a

int intstype (xy a,xy c,xy b,xy d,double &maxarea,double &maxcoord)
//Intersection type - one of 81 numbers, not all possible.
{
  double A,B,C,D;
  A=area3(b,c,d);
  B=area3(c,d,a);
  C=area3(d,a,b);
  D=area3(a,b,c);
  maxarea=maxcoord=0;
  setmaxabs(maxarea,A);
  setmaxabs(maxarea,B);
  setmaxabs(maxarea,C);
  setmaxabs(maxarea,B);
  setmaxabs(maxcoord,a.east());
  setmaxabs(maxcoord,a.north());
  setmaxabs(maxcoord,b.east());
  setmaxabs(maxcoord,b.north());
  setmaxabs(maxcoord,c.east());
  setmaxabs(maxcoord,c.north());
  setmaxabs(maxcoord,d.east());
  setmaxabs(maxcoord,d.north());
  return (27*sign(A)+9*sign(C)+3*sign(B)+sign(D));
}

double missDistance (xy a,xy c,xy b,xy d)
/* If the intersection type is NOINT, but is close to ACTBD or BDTAC,
 * this returns the distance one segment has to move to intersect the other.
 * It is used if there is an extra segment in triangle subdivision,
 * because of roundoff error, to determine which is the extra segment.
 */
{
  double A,B,C,D,totarea,aclen,bdlen,ret=0;
  A=area3(b,c,d);
  B=area3(c,d,a);
  C=area3(d,a,b);
  D=area3(a,b,c);
  aclen=dist(a,c);
  bdlen=dist(b,d);
  totarea=(A+B+C+D)/2;
  if (sign(A)*sign(totarea)<0)
    ret+=A/bdlen;
  if (sign(B)*sign(totarea)<0)
    ret+=B/aclen;
  if (sign(C)*sign(totarea)<0)
    ret+=C/bdlen;
  if (sign(D)*sign(totarea)<0)
    ret+=D/aclen;
  return fabs(ret);
}

inttype intersection_type(xy a,xy c,xy b,xy d)
{
  double maxarea,maxcoord;
  int itype=intstype(a,c,b,d,maxarea,maxcoord)+40;
  itype=intstable[itype/27][itype%27/9][itype%9/3][itype%3];
  if (itype==IMPOS && maxarea<maxcoord*maxcoord*1e-15)
    itype=COLIN;
  return (inttype)itype;
}

double in3(xy p,xy a,xy b,xy c)
{
  double maxarea,maxcoord,ret;
  int n=intstype(p,a,b,c,maxarea,maxcoord)+40;
  // abc's sign is wrong, pab's sign is wrong, pbc's sign is right, and pca's sign is wrong.
  ret=intable[n/27][n%27/9][n%9/3][n%3]/2.;
  if (ret==-64 && maxarea<maxcoord*maxcoord*1e-15)
    ret=0;
  if (ret==-64)
    ret=NAN;
  if (ret==52.5)
  {
    if (p==a)
      n=dir(a,c)-dir(a,b);
    if (p==b)
      n=dir(b,a)-dir(b,c);
    if (p==c)
      n=dir(c,b)-dir(c,a);
    ret=bintorot(foldangle(n));
  }
  return ret;
}

bool crossTriangle(xy p,xy q,xy a,xy b,xy c)
{
  return intersection_type(p,q,a,b)==ACXBD
      || intersection_type(p,q,b,c)==ACXBD
      || intersection_type(p,q,c,a)==ACXBD;
}

double pldist(xy a,xy b,xy c)
/* Signed distance from a to the line bc. */
{
  return area3(a,b,c)/dist(b,c)*2;
}

xy rand2p(xy a,xy b)
/* A random point in the circle with diameter ab. */
{
  xy mid((a+b)/2);
  unsigned short n;
  xy pnt;
  double angle=(sqrt(5)-1)*M_PI;
  n=rng.usrandom();
  pnt=xy(cos(angle*n)*sqrt(n+0.5)/256,sin(angle*n)*sqrt(n+0.5)/256);
  pnt=pnt*dist(mid,a)+mid;
  return pnt;
}

bool delaunay(xy a,xy c,xy b,xy d)
/* Returns true if ac satisfies the criterion in the quadrilateral abcd.
 * If false, the edge should be flipped to bd.
 * The computation is based on the theorem that the two diagonals of
 * a quadrilateral inscribed in a circle cut each other into parts
 * whose products are equal. Element 3:35.
 */
{
  xy ints;
  double dista,distc,distb,distd,distac,distbd;
  ints=intersection(a,c,b,d);
  distac=dist(a,c);
  distbd=dist(b,d);
  if (std::isnan(ints.north()))
  {//printf("delaunay:No intersection, distac=%a, distbd=%a\n",distac,distbd);
    return distac<=distbd;
  }
  else
  {
    dista=dist(a,ints);
    distb=dist(b,ints);
    distc=dist(c,ints);
    distd=dist(d,ints);
    if (dista>distac || distc>distac) dista=-dista;
    if (distb>distbd || distd>distbd) distb=-distb;
    if (debugdel && dista*distc>distb*distd)
      printf("delaunay:dista*distc=%a, distb*distd=%a\n",dista*distc,distb*distd);
    if (dista*distc == distb*distd)
      return distac<=distbd;
    else
      return dista*distc<=distb*distd;
  }
}

char inttstr[]="NOINT\0ACXBD\0BDTAC\0ACTBD\0ACVBD\0COINC\0COLIN\0IMPOS";

char *inttype_str(inttype i)
{
  return inttstr+6*i;
}

double distanceInDirection(xy a,xy b,int dir)
{
  return dot(b-a,cossin(dir));
}
