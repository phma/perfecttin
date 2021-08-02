/******************************************************/
/*                                                    */
/* spiral.cpp - Cornu or Euler spirals                */
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

/* The Cornu spiral is a complex-valued function of a real argument,
 * the graph in the complex plane of integral(cis(t²) dt).
 * The curvature at t equals 2t.
 * 
 * Traditionally spirals have been used mostly for transitions between
 * a straight section and a circular arc, i.e. at one end of every spiral
 * arc, t=0. Parkways, though, require a greater variety of spirals.
 * See http://www.fhwa.dot.gov/publications/publicroads/01septoct/spiral.cfm .
 * 
 * I originally wrote the one-argument cornu() function for spiral arcs
 * with one end straight, then wrote the three-argument cornu() function
 * for more general spiral arcs.
 * 
 * A spiral arc takes 6 numbers to specify: x, y, and bearing at each end.
 * Two spiral arcs osculating end-to-end take 8. Given two circles which
 * are not concentric and do not intersect, there are two spiral arcs that
 * connect them and osculate them at the ends; they are mirror images in the
 * line joining the centers. Two circles which do intersect, or which are
 * traveled in opposite directions, can be joined by two spiral arcs,
 * specifying the point where they abut osculating.
 * 
 * http://www.wsdot.wa.gov/publications/manuals/fulltext/m3087/FieldTablesEngSpirals.pdf
 * a is the clothance, in units of 1 degree per (100 ft)². Bezitopo uses the
 * coherent metric unit, 1 rad/m² (aka square diopter). 1 °/(100 ft)² is 
 * 18.786568 mdpt² if the international foot is used and 18.786493 mdpt²
 * if the US survey foot is used.
 * 
 * http://cc4w.net/spiral/SpiralTraining.pdf contains a plat of a stretch of
 * road in Arizona including some spirals connecting two arcs.
 */

#include <vector>
#include <cstdio>
#include <iostream>
#include <cfloat>
#include "spiral.h"
#include "angle.h"
#include "manysum.h"
#include "cogospiral.h"
using namespace std;
#define MAXITER 144
/* The most iterations without losing precision in an actual run is 138.
 * This occurs at the ends of the bendiest curves in testcurly.
 */
#define MAXTOTCUR 0.05
#define MAXTOTCLO 0.01
// When computing area, if the curve exceeds either of these, it will split it.
#define CURLTEST 4
// Number of points to try in the too curly test. 2 doesn't work, but 4 appears to.
vector<int> cornuhisto;

xy cornu(double t)
/* If |t|>=6, it returns the limit points rather than a value with no precision.
 * The largest t useful in surveying is 1.430067.
 */
{
  vector<long double> realparts,imagparts;
  int i;
  long double facpower,rsum,isum,t2,bigpart;
  double precision;
  t2=t*t;
  for (i=0,facpower=t;0.9+facpower!=0.9 || !i;i++)
  {
    realparts.push_back(facpower/(8*i+1));
    facpower*=t2/(4*i+1);
    imagparts.push_back(facpower/(8*i+3));
    facpower*=t2/(4*i+2);
    realparts.push_back(-facpower/(8*i+5));
    facpower*=t2/(4*i+3);
    imagparts.push_back(-facpower/(8*i+7));
    facpower*=t2/(4*i+4);
  }
  if (i>=cornuhisto.size())
    cornuhisto.resize(i+1);
  cornuhisto[i]++;
  for (i=realparts.size()-1,bigpart=0;i>=0;i--)
  {
    if (fabsl(realparts[i])>bigpart)
      bigpart=fabsl(realparts[i]);
    if (fabsl(imagparts[i])>bigpart)
      bigpart=fabsl(imagparts[i]);
  }
  rsum=pairwisesum(realparts);
  isum=pairwisesum(imagparts);
  precision=nextafterl(bigpart,2*bigpart)-bigpart;
  //printf("precision %e\n",precision);
  if (precision>1e-6)
    rsum=isum=sqrt(M_PI/8)*(t/fabs(t));
  return xy(rsum,isum);
}

xy cornu(double t,double curvature,double clothance)
/* Evaluates the integral of cis(clothance×t²/2+curvature×t).
 * 1+(cl×t²/2+cu×t)i-(cl×t²/2+cu×t)²/2-(cl×t²/2+cu×t)³i/6+(cl×t²/2+cu×t)⁴/24+...
 * 1+cl×t²×i/2  +cu×t×i   -cl²×t⁴/8  -cl×cu×t³×2/4  -cu²×t²/2  -cl³×t⁶×i/6/8  -cl²×cu×t⁵×3i/6/4  -cl×cu²×t⁴×3i/6/2  -cu³×t³i/6  +cl⁴×t⁸/24/16  +cl³×cu×t⁷×4/24/8  +cl²×cu²×t⁶6/24/4  +cl×cu³×t⁵×4/24/2  +cu⁴×t⁴/24+...
 * t+cl×t³×i/3/2+cu×t²×i/2-cl²×t⁵/5/8-cl×cu×t⁴×2/4/4-cu²×t³/3/2-cl³×t⁷×i/7/6/8-cl³×cu×t⁶×3i/6/6/4-cl×cu²×t⁵×3i/5/6/2-cu³×t⁴i/4/6+cl⁴×t⁹/9/24/16+cl³×cu×t⁸×4/8/24/8+cl²×cu²×t⁷6/7/24/4+cl×cu³×t⁶×4/6/24/2+cu⁴×t⁵/5/24+...
 * If clothance=0, you get a circle of radius 1/curvature.
 * If curvature=0 and clothance=2, you get cornu(t).
 */
{
  long double cupower[MAXITER+1],clpower[MAXITER+1];
  long double realparts[((MAXITER+2)*(MAXITER+2))/4],imagparts[((MAXITER+2)*(MAXITER+2))/4];
  int i,j,rinx,iinx;
  long double facpower,rsum,isum,t2,bigpart,binom,clotht,term,bigterm=0;
  double precision;
  t2=t*t;
  clotht=clothance*t/2;
  cupower[0]=clpower[0]=1;
  realparts[0]=imagparts[0]=0;
  for (bigpart=i=iinx=rinx=0,facpower=t;(0.9+bigterm!=0.9 || !i) && i<MAXITER;i++)
  {
    for (bigterm=j=0,binom=1;j<=i;j++)
    {
      term=clpower[j]*cupower[i-j]*binom*facpower/(i+j+1);
      //cout<<"i="<<i<<" j="<<j<<" term="<<cupower[j]<<'*'<<clpower[i-j]<<'*'<<binom<<'*'<<facpower<<'/'<<i+j+1<<'='<<term<<endl;
      if (fabsl(term)>bigterm)
	bigterm=fabsl(term);
      if (fabsl(term)>bigpart)
	bigpart=fabsl(term);
      switch (i&3)
      {
	case 0:
	  realparts[rinx++]=term;
	  break;
	case 1:
	  imagparts[iinx++]=term;
	  break;
	case 2:
	  realparts[rinx++]=-term;
	  break;
	case 3:
	  imagparts[iinx++]=-term;
	  break;
      }
      binom=binom*(i-j)/(j+1);
    }
    cupower[i+1]=cupower[i]*curvature;
    clpower[i+1]=clpower[i]*clotht;
    facpower*=t/(i+1);
  }
  /*if (i>=cornuhisto.size())
    cornuhisto.resize(i+1);
  cornuhisto[i]++;*/
  precision=nextafterl(bigpart,2*bigpart)-bigpart;
  // precision is 0 in Valgrind. https://bugs.kde.org/show_bug.cgi?id=197915
  //printf("precision %e\n",precision);
  if (i>=MAXITER-1 && precision<=1e-6)
    cerr<<"cornu needs more iterations"<<endl;
  rsum=pairwisesum(realparts,rinx);
  isum=pairwisesum(imagparts,iinx);
  if (precision>1e-6)
    rsum=isum=nan("cornu");
  return xy(rsum,isum);
}

void cornustats()
{
  int i;
  cout<<"Cornu statistics"<<endl;
  for (i=0;i<cornuhisto.size();i++)
    cout<<i<<' '<<cornuhisto[i]<<endl;
}
/* It should be possible to fit a spiral to be tangent to two given circular
 * or straight curves by successive approximation using these functions.
 */

double spiralbearing(double t,double curvature=0,double clothance=1)
{
  return t*t*clothance/2+t*curvature;
}

int ispiralbearing(double t,double curvature=0,double clothance=1)
{
  return radtobin(t*t*clothance/2+t*curvature);
}

double spiralcurvature(double t,double curvature=0,double clothance=1)
{
  return t*clothance+curvature;
}

spiralarc::spiralarc()
{
  mid=start=end=xyz(0,0,0);
  cur=clo=len=0;
  midbear=0;
}

spiralarc::spiralarc(const segment &b)
{
  start=b.start;
  end=b.end;
  mid=(start+end)/2;
  cur=clo=0;
  len=b.length();
  midbear=atan2i(xy(end-start));
}

spiralarc::spiralarc(const arc &b)
{
  start=b.start;
  end=b.end;
  mid=b.midpoint();
  cur=b.curvature(0);
  clo=0;
  len=b.length();
  midbear=atan2i(xy(end-start));
}

spiralarc::spiralarc(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
  mid=(start+end)/2;
  len=dist(xy(start),xy(end));
  cur=clo=0;
  midbear=atan2i(xy(end-start));
}

spiralarc::spiralarc(xyz kra,double c1,double c2,xyz fam):spiralarc(kra,fam)
{
  setcurvature(c1,c2);
}

spiralarc::spiralarc(xyz kra,int sbear,double c1,double c2,double length,double famElev)
{
  clo=(c2-c1)/length;
  start=kra;
  end=xyz(xy(kra),famElev);
  mid=turn(cornu(length/2,c1,clo),sbear)+xy(kra);
  midbear=sbear+ispiralbearing(length/2,c1,clo);
  cur=spiralcurvature(length/2,c1,clo);
  len=length;
  end=station(length);
}

spiralarc::spiralarc(xyz kra,xy mij,xyz fam,int mbear,double curvature,double clothance,double length)
// This does NO checking and is intended for polyspiral and alignment.
{
  start=kra;
  mid=mij;
  end=fam;
  midbear=mbear;
  cur=curvature;
  clo=clothance;
  len=length;
}

spiralarc::spiralarc(xyz pnt,double curvature,double clothance,int bear,double startLength,double endLength)
// For the spiralarc intersection test.
{
  double midLength=(startLength+endLength)/2;
  start=xyz(turn(cornu(startLength,curvature,clothance),bear),0)+pnt;
  mid=turn(cornu(midLength,curvature,clothance),bear)+xy(pnt);
  end=xyz(turn(cornu(endLength,curvature,clothance),bear),0)+pnt;
  midbear=bear+ispiralbearing(midLength,curvature,clothance);
  cur=spiralcurvature(midLength,curvature,clothance);
  clo=clothance;
  len=endLength-startLength;
}

spiralarc spiralarc::operator-() const
{
  spiralarc ret(end,mid,start,midbear+DEG180,-cur,clo,len);
  return ret;
}

double spiralarc::epsilon() const
{
  return sqrt((sqr(mid.getx())+sqr(mid.gety())+sqr(len))/1.5)*DBL_EPSILON;
}

double spiralarc::in(xy pnt)
{
  double begcur=curvature(0),endcur=curvature(len),maxcur;
  maxcur=fabs(begcur);
  if (fabs(endcur)>maxcur)
    maxcur=fabs(endcur);
  if ((dist(pnt,start)>len && dist(pnt,end)>len) || 
      maxcur*len*len<(xy(start).length()+xy(end).length())*1e-12)
    return 0;
  else if (std::isnan(maxcur*len) || mid.isnan())
    return NAN;
  else if (maxcur*len<2 && pnt==xy(start))
    return bintorot(foldangle(chordbearing()-startbearing()));
  else if (maxcur*len<2 && pnt==xy(end))
    return bintorot(foldangle(endbearing()-chordbearing()));
  else
  {
    spiralarc first,second;
    split(len/2,first,second);
    return in3(pnt,start,mid,end)+first.in(pnt)+second.in(pnt);
  }
}

double spiralarc::_diffarea(double totcur,double totclo)
{
  return len*len*(totcur/12-cub(totcur)/240-totcur*sqr(totclo)/6720);
  /* 6720 is an empirical constant and may be wrong. The true value appears
   * to be somewhere near 6724 or 6740.
   */
}

xy spiralarc::pointOfIntersection()
{
  if (cur==0 && clo==0)
    return mid;
  else
    return intersection(start,startbearing(),end,endbearing());
}

double spiralarc::tangentLength(int which)
{
  if (which==START)
    return distanceInDirection(start,pointOfIntersection(),startbearing());
  if (which==END)
    return distanceInDirection(pointOfIntersection(),end,endbearing());
  return NAN;
}

double spiralarc::diffarea()
{
  double totcur=len*cur,totclo=len*len*clo;
  if ((totcur<MAXTOTCUR && totclo<MAXTOTCLO) || !valid())
    return _diffarea(totcur,totclo);
  else
  {
    spiralarc first,second;
    split(len/2,first,second);
    return area3(start,mid,end)+first.diffarea()+second.diffarea();
  }
}

xyz spiralarc::station(double along) const
{
  double midlong;
  xy relpos;
  midlong=along-len/2;
  relpos=cornu(midlong,cur,clo);
  return xyz(turn(relpos,midbear)+mid,elev(along));
}

xy spiralarc::center()
/* The center of a spiralarc is the center of the circle that osculates its midpoint.
 * Thus turning an arc into a spiralarc does not change its center.
 */
{
  return mid+cossin(midbear+DEG90)/cur;
}

void spiralarc::_setdelta(int d,int s)
{
  cur=bintorad(d)/len;
  clo=4*bintorad(s)/len/len;
}

void spiralarc::_setcurvature(double startc,double endc)
{
  cur=(startc+endc)/2;
  clo=(endc-startc)/len;
}

void spiralarc::_fixends(double p)
{
  xy kra,fam;
  int turnangle;
  double scale;
  kra=station(0);
  fam=station(len);
  turnangle=foldangle(atan2i(end-start)-atan2i(fam-kra)); // don't turn by more than 180° either way
  midbear+=rint(turnangle*p);
  scale=dist(xy(end),xy(start))/dist(fam,kra);
  len*=scale;
  cur/=scale;
  clo/=scale*scale;
  kra=station(0);
  fam=station(len);
  mid+=((end-fam)+(start-kra))/2;
}

void spiralarc::split(double along,spiralarc &a,spiralarc &b)
{
  xyz mida,midb;
  int midbeara,midbearb;
  double cura,curb;
  if (std::isinf(along))
    cerr<<"along=inf"<<endl;
  xyz splitpoint=station(along);
  mida=station(along/2);
  midb=station((along+len)/2);
  midbeara=bearing(along/2);
  midbearb=bearing((along+len)/2);
  cura=curvature(along/2);
  curb=curvature((along+len)/2);
  a.mid=mida;
  a.midbear=midbeara;
  a.cur=cura;
  a.len=along;
  a.start=start;
  b.mid=midb;
  b.midbear=midbearb;
  b.cur=curb;
  b.len=len-along;
  b.end=end;
  a.clo=b.clo=clo;
  a.end=b.start=splitpoint;
  //printf("split: %f,%f\n",a.end.east(),a.end.north());
}

void spiralarc::lengthen(int which,double along)
/* Lengthens or shortens the spiralarc, moving the specified end.
 * Used for extend, trim, trimTwo, and fillet (trimTwo is fillet with radius=0).
 */
{
  double oldSlope,newSlope=slope(along);
  xyz newEnd=station(along);
  xy newMid;
  double newCur;
  int newMidbear;
  if (which==START)
  {
    newMid=station((along+len)/2);
    newCur=curvature((along+len)/2);
    newMidbear=bearing((along+len)/2);
    len-=along;
    mid=newMid;
    cur=newCur;
    midbear=newMidbear;
    start=newEnd;
  }
  if (which==END)
  {
    newMid=station(along/2);
    newCur=curvature(along/2);
    newMidbear=bearing(along/2);
    len=along;
    mid=newMid;
    cur=newCur;
    midbear=newMidbear;
    end=newEnd;
  }
}

void spiralarc::setdelta(int d,int s)
/* Works as long as |d|<=300° and |s|<=253°.
 * For s outside that range, an arithmetic overflow of the expression
 * s+rot may result.
 */
{
  int lastmidbear,chordbear,rot,i;
  double looseness;
  xy lastmid;
  chordbear=chordbearing();
  if (!valid())
  {
    cur=clo=0;
    len=segment::length();
    midbear=chordbearing();
    mid=(start+end)/2;
  }
  i=0;
  do
  {
    rot=2*(chordbear-midbear);
    lastmidbear=midbear;
    lastmid=mid;
    _setdelta(d,s+rot);
    _fixends(1-i/257.);
    i++;
    //cout<<"iter "<<i<<" midbear "<<midbear<<" cur "<<cur<<" clo "<<clo<<endl;
    looseness=DEG60/((len/mid.length())/DBL_EPSILON*2)+1;
    /* When a spiralarc is short (like 10 µm, which appears in some contours)
     * and far from the origin (like 1-2 Mm, due to false easting), the
     * bearing between start and end cannot assume every value, but jumps by
     * a few thousand, or a couple of seconds of arc.
     */
  }
  while ((abs(midbear-lastmidbear)>looseness || dist(mid,lastmid)>1e-6) && i<256);
  if (abs(midbear-lastmidbear)>looseness || dist(mid,lastmid)>1e-6)
    cur=clo=len=NAN;
}

void spiralarc::setcurvature(double startc,double endc)
{
  int lastmidbear,chordbear,rot,i;
  double chordlen;
  xy lastmid;
  chordbear=chordbearing();
  chordlen=segment::length();
  if (!valid())
  {
    cur=clo=0;
    len=chordlen;
    midbear=chordbearing();
    mid=(start+end)/2;
  }
  i=0;
  if (signbit(startc)==signbit(endc) && fabs(startc*chordlen)>2 && fabs(endc*chordlen)>2)
    lastmidbear=midbear+DEG180;
  else
    do
    {
      if (fabs(len*cur)>6.5)
      {
	cur=clo=0;
	len=chordlen;
	midbear=chordbearing();
	mid=(start+end)/2;
      }
      rot=2*(chordbear-midbear);
      lastmidbear=midbear;
      lastmid=mid;
      _setcurvature(startc,endc);
      _fixends(1-i/257.);
      i++;
      //cout<<"iter "<<i<<" midbear "<<midbear<<" cur "<<cur<<" clo "<<clo<<endl;
    }
    while ((abs(midbear-lastmidbear)>1 || dist(mid,lastmid)>1e-6) && i<256);
  if (abs(midbear-lastmidbear)>1 || dist(mid,lastmid)>1e-6)
    cur=clo=len=NAN;
}
