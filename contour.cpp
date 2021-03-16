/******************************************************/
/*                                                    */
/* contour.cpp - generates contours                   */
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

/* After finding extrema, generate contours. The method for generating contours
 * as polylines is as follows:
 * 1. Between two corners (i.e. points in the TIN, corners of triangles) in the TIN
 *    that includes the extrema, find a point on the edge that has the given elevation.
 *    Join these points with line segments.
 * 2. Change the line segments to spiralarcs to form a smooth curve.
 * 3. Add points to each spiralarc, staying within the triangle, until the elevation
 *    all along each spiralarc is within the tolerance or the spiralarc is shorter
 *    than the tolerance.
 */
#include <iostream>
#include <cassert>
#include <cstring>
#include "pointlist.h"
#include "contour.h"
#include "relprime.h"
#include "manysum.h"
#include "ldecimal.h"
using namespace std;

float splittab[65]=
{
  0.2113,0.2123,0.2134,0.2145,0.2156,0.2167,0.2179,0.2191,0.2204,0.2216,0.2229,0.2244,0.2257,
  0.2272,0.2288,0.2303,0.2319,0.2337,0.2354,0.2372,0.2390,0.2410,0.2430,0.2451,0.2472,0.2495,
  0.2519,0.2544,0.2570,0.2597,0.2625,0.2654,0.2684,0.2716,0.2750,0.2786,0.2823,0.2861,0.2902,
  0.2945,0.2990,0.3038,0.3088,0.3141,0.3198,0.3258,0.3320,0.3386,0.3454,0.3527,0.3605,0.3687,
  0.3773,0.3862,0.3955,0.4053,0.4153,0.4256,0.4362,0.4469,0.4577,0.4684,0.4792,0.4897,0.5000
};

ContourInterval::ContourInterval()
{
  interval=1;
  fineRatio=1;
  coarseRatio=5;
  relativeTolerance=0.5;
}

ContourInterval::ContourInterval(double unit,int icode,bool fine)
/* icode encodes the medium interval, as follows:
 * -3 0.1
 * -2 0.2
 * -1 0.5
 * 0  1
 * 1  2
 * 2  5
 * 3  10
 * If fine is true, the fine contour interval is enabled.
 */
{
  setInterval(unit,icode,fine);
  relativeTolerance=0.5;
}

void ContourInterval::setInterval(double unit,int icode,bool fine)
{
  int i;
  fineRatio=1;
  while (icode>1)
  {
    icode-=3;
    unit*=10;
  }
  while (icode<-1)
  {
    icode+=3;
    unit*=0.1;
  }
  switch (icode)
  {
    case -1:
      interval=unit/2;
      coarseRatio=4;
      if (fine)
        interval/=fineRatio=5;
      break;
    case 0:
      interval=unit;
      coarseRatio=5;
      if (fine)
        interval/=fineRatio=5;
      break;
    case 1:
      interval=unit*2;
      coarseRatio=5;
      if (fine)
        interval/=fineRatio=4;
      break;
  }
}

void ContourInterval::setIntervalRatios(double i,int f,int c)
// For restoring settings when program starts
{
  if (i>0 && f>0 && f<10 && c>0 && c<10)
  {
    interval=i;
    fineRatio=f;
    coarseRatio=c;
  }
}

string ContourInterval::valueString(double unit,bool precise)
/* Returns the value of mediumInterval with 1 or 7 significant digits.
 * Uses 7 digits if precise to distinguish which foot it was set in.
 */
{
  return ldecimal(mediumInterval()/unit,mediumInterval()/unit/M_SQRT_10/(precise?1e6:1));
}

int ContourInterval::contourType(double elev)
/* Returns the sum of two numbers:
 * 0 for fine contours, 256 for medium contours, 512 for coarse (index) contours.
 * 0 for contours that are not subdivisions of coarser contours, 4, 5, 8, 10,
 * 12, 15, or 16 for those that are.
 */
{
  int iElev,ret,rem;
  iElev=rint(elev/interval);
  ret=0;
  rem=iElev%fineRatio;
  if (rem<0)
    rem+=fineRatio;
  if (rem)
    ret+=rem*20/fineRatio;
  else
  {
    ret+=256;
    iElev/=fineRatio;
    rem=iElev%coarseRatio;
    if (rem<0)
      rem+=coarseRatio;
    if (rem)
      ret+=rem*20/coarseRatio;
    else
      ret+=256;
  }
  return ret;
}

void ContourInterval::writeXml(ostream &ofile)
{
  ofile<<"<ContourInterval interval=\""<<ldecimal(interval);
  ofile<<"\" fineRatio=\""<<fineRatio;
  ofile<<"\" coarseRatio=\""<<coarseRatio;
  ofile<<"\"/>"<<endl;
}

void DirtyTracker::init(int n)
{
  dirt.clear();
  dirt.resize(n,1);
}

bool DirtyTracker::isDirty(int n)
{
  if (n>=0 && n<dirt.size())
    return dirt[n];
  else
    return false;
}

void DirtyTracker::markDirty(int n,int spread)
{
  int i,start,end,sz=dirt.size();
  start=n-spread;
  if (start<0)
    start+=sz;
  end=start+2*spread;
  for (i=start;sz && i<=end;i++)
    dirt[i%sz]=1;
}

void DirtyTracker::markClean(int n)
{
  if (n>=0 && n<dirt.size())
    dirt[n]=0;
}

void DirtyTracker::erase(int n)
{
  if (n>=0 && n<dirt.size())
  {
    memmove(&dirt[n],&dirt[n+1],dirt.size()-n-1);
    dirt.pop_back();
  }
}

float splitpoint(double leftclamp,double rightclamp,double tolerance)
/* If the values at the clamp points indicate that the curve may be out of tolerance,
 * returns the point to split it at, as a fraction of the length. If not, returns 0.
 * tolerance must be positive.
 */
{
  bool whichbig;
  double ratio;
  float sp;
  if (std::isnan(leftclamp))
    sp=CCHALONG;
  else if (std::isnan(rightclamp))
    sp=1-CCHALONG;
  else if (fabs(leftclamp)*27>tolerance*23 || fabs(rightclamp)*27>tolerance*23)
  {
    whichbig=fabs(rightclamp)>fabs(leftclamp);
    ratio=whichbig?(leftclamp/rightclamp):(rightclamp/leftclamp);
    sp=splittab[(int)rint((ratio+1)*32)];
    if (whichbig)
      sp=1-sp;
  }
  else
    sp=0;
  return sp;
}

vector<edge *> contstarts(pointlist &pts,double elev)
{
  vector<edge *> ret;
  edge *ep;
  int sd,io;
  triangle *tri;
  int i,j;
  //cout<<"Exterior edges:";
  for (io=0;io<2;io++)
    for (i=0;i<pts.edges.size();i++)
    {
      ep=&pts.edges[i];
      if (io==ep->isinterior())
      {
	tri=ep->tria;
	if (!tri)
	  tri=ep->trib;
	assert(tri);
	//cout<<' '<<i;
	sd=tri->subdir(ep);
	if (tri->crosses(sd,elev) && (io || tri->upleft(sd)))
	{
	  //cout<<(char)(j+'a');
	  ret.push_back(ep);
	}
      }
    }
  //cout<<endl;
  return ret;
}

void mark(edge *ep)
{
  ep->mark(0);
}

bool ismarked(edge *ep)
{
  return ep->ismarked(0);
}

polyline trace(edge *edgep,double elev)
{
  polyline ret(elev);
  int subedge,subnext,i;
  edge *prevedgep;
  bool wasmarked;
  xy lastcept,thiscept,firstcept;
  triangle *tri,*ntri;
  tri=edgep->tria;
  ntri=edgep->trib;
  if (tri==nullptr || !tri->upleft(tri->subdir(edgep)))
    tri=ntri;
  mark(edgep);
  firstcept=lastcept=tri->contourcept(tri->subdir(edgep),elev);
  if (firstcept.isnan())
  {
    cerr<<"Tracing STARTS on Nan"<<endl;
    return ret;
  }
  //if (fabs(elev-0.21)<0.0000001) // debugging in testcontour
    //cout<<"Starting "<<ldecimal(firstcept.getx())<<' '<<ldecimal(firstcept.gety())<<endl;
  ret.insert(firstcept);
  do
  {
    prevedgep=edgep;
    subedge=tri->subdir(edgep);
    i=0;
    do
    {
      subnext=tri->proceed(subedge,elev);
      if (subnext>=0)
      {
	if (subnext==subedge)
	  cerr<<"proceed failed! "<<ret.size()<<endl;
	subedge=subnext;
	thiscept=tri->contourcept(subedge,elev);
	if (thiscept.isfinite())
	{
	  if (thiscept!=lastcept)
          {
            //if (fabs(elev-0.21)<0.0000001) // debugging in testcontour
              //cout<<"Interior "<<ldecimal(thiscept.getx())<<' '<<ldecimal(thiscept.gety())<<endl;
	    ret.insert(thiscept);
          }
	  //else
	    //cerr<<"Repeated contourcept: "<<edgep<<' '<<ret.size()<<endl;
          /* "Repeated contourcept" was originally put in to look for contour
           * tracing bugs. After fixing these bugs, I ran bezitopo on home.asc
           * and got this message. It turned out to be caused by tracing the
           * contour with elevation 0 through point 1, whose elevation is 0.
           * Tracing it through a triangle where point 1 is a local maximum
           * produced two (or more) consecutive occurrences of (0,0), thus
           * triggering the message. The contour is traced correctly; it's
           * not a bug.
           */
	  lastcept=thiscept;
	}
	else
	  cerr<<"NaN contourcept"<<endl;
      }
    } while (subnext>=0 && ++i<256);
    //cout<<"after loop "<<subedge<<' '<<subnext<<endl;
    edgep=tri->edgepart(subedge);
    //cout<<"Next edgep "<<edgep<<endl;
    if (edgep==prevedgep)
    {
      cout<<"Edge didn't change"<<endl;
      subedge=tri->subdir(edgep);
      subnext=tri->proceed(subedge,elev);
      if (subnext>=0)
      {
	if (subnext==subedge)
	  cout<<"proceed failed!"<<endl;
	subedge=subnext;
	//ret.insert(tri->contourcept(subedge,elev));
      }
    }
    if (edgep==0)
    {
      ntri=nullptr;
      cout<<"Tracing stopped in middle of a triangle "<<ret.size()<<endl;
      subedge=tri->subdir(prevedgep);
      subnext=tri->proceed(subedge,elev);
    }
    else
    {
      wasmarked=ismarked(edgep);
      if (!wasmarked)
      {
	thiscept=tri->contourcept(tri->subdir(edgep),elev);
	if (thiscept!=lastcept && thiscept!=firstcept && thiscept.isfinite())
        {
          //if (fabs(elev-0.21)<0.0000001) // debugging in testcontour
            //cout<<"Exterior "<<ldecimal(thiscept.getx())<<' '<<ldecimal(thiscept.gety())<<endl;
	  ret.insert(thiscept);
        }
	lastcept=thiscept;
      }
      mark(edgep);
      ntri=edgep->othertri(tri);
    }
    if (ntri)
      tri=ntri;
  } while (ntri && !wasmarked);
  if (!ntri)
    ret.open();
  return ret;
}

void rough1contour(pointlist &pl,double elev)
{
  vector<edge *> cstarts;
  polyline ctour;
  int j;
  cstarts=contstarts(pl,elev);
  pl.clearmarks();
  for (j=0;j<cstarts.size();j++)
    if (!ismarked(cstarts[j]))
    {
      ctour=trace(cstarts[j],elev);
      ctour.dedup();
      ctour.setlengths();
      pl.contours.push_back(ctour);
    }
}

void roughcontours(pointlist &pl,double conterval)
/* Draws contours consisting of line segments.
 * The perimeter must be present in the triangles.
 * Do not attempt to draw contours in the Mariana Trench with conterval
 * less than 5 µm or of Chomolungma with conterval less than 4 µm. It will fail.
 */
{
  array<double,2> tinlohi;
  int i;
  pl.contours.clear();
  tinlohi=pl.lohi();
  for (i=floor(tinlohi[0]/conterval);i<=ceil(tinlohi[1]/conterval);i++)
    rough1contour(pl,i*conterval);
}

double contourError(pointlist &pl,polyline &contour)
{
  int i;
  vector<double> segError;
  for (i=0;i<contour.size();i++)
    segError.push_back(pl.contourError(contour.getsegment(i)));
  return pairwisesum(segError);
}

double contourError(pointlist &pl,double elev,xy start,xy end)
{
  return pl.contourError(segment(xyz(start,elev),xyz(end,elev)));
}

double bendiness(xy a,xy b,xy c,double tolerance)
/* Like contourError, this has dimensions of volume.
 * You have the line segments ab and bc in a contour.
 * Compute the area of the ellipse with foci at a and c passing through b,
 * then multiply by the square of the tolerance divided by ac.
 */
{
  double ab=dist(a,b),bc=dist(b,c),ac=dist(a,c);
  double majorAxis,minorAxis,area;
  majorAxis=ab+bc;
  minorAxis=sqrt(sqr(majorAxis)-sqr(ac));
  if (!(minorAxis>0)) // in case roundoff produces sqrt(neg)
    minorAxis=0;
  area=majorAxis*minorAxis*M_PI/4;
  return area*sqr(tolerance)/ac;
}

double totalBendiness(polyline &p,double tolerance)
{
  int first,last;
  int i;
  vector<double> bends;
  if (p.isopen())
    first=1; // The bendiness of the 0th endpoint is not defined.
  else
    first=0;
  last=p.size()-1;
  if (first==0 && last<2) // Bendiness of 1- and 2- point closed contours is ∞ or NaN,
    last=-1;		  // and they'll be deleted anyway.
  for (i=first;i<=last;i++)
  {
    bends.push_back(bendiness(p.getEndpoint(i-1),p.getEndpoint(i),
			      p.getEndpoint(i+1),tolerance));
    if (!isfinite(bends.back()))
      cout<<".\b";
  }
  return pairwisesum(bends);
}

void checkContour(pointlist &pl,polyline &contour,double tolerance)
{
  int i,j,ilen;
  double len,along,err;
  segment seg;
  for (i=0;i<contour.size();i++)
  {
    seg=contour.getsegment(i);
    len=seg.length();
    ilen=lrint(len);
    for (j=0;j<ilen;j++)
    {
      along=len*j/ilen;
      err=pl.elevation(seg.station(along))-seg.elev(along);
      if (fabs(err)>tolerance)
	cout<<"Segment "<<i<<" of contour out of tolerance at station "<<along<<endl;
    }
  }
}

void prune1contour(pointlist &pl,double tolerance,int i)
/* Removes points from the ith contour, as long as it stays within tolerance.
 * If the resulting contour is closed and has only two points, it should be deleted.
 */
{
  static int n=0;
  int j,sz,origsz;
  array<double,2> lohiElev;
  polyline change;
  double e=pl.contours[i].getElevation();
  PostScript ps;
  BoundRect br;
  DirtyTracker dt;
  bool debugging=false;
  origsz=sz=pl.contours[i].size();
  //cout<<"Contour "<<i<<" error before "<<contourError(pl,pl.contours[i]);
  //cout<<" bendiness "<<totalBendiness(pl.contours[i],tolerance)<<endl;
  if (dist(pl.contours[i].getstart(),xyz(3533477.583,5383594.186,410))<0.002)
    debugging=true; // This ground contour in pc_ex1 crosses a car after pruning.
  if (debugging)
  {
    ps.open("prune.ps");
    ps.setpaper(papersizes["A4 portrait"],0);
    ps.prolog();
    br.include(&pl);
  }
  dt.init(sz);
  for (j=0;j<sz;j++)
  {
    n=(n+relprime(sz))%sz;
    if ((n || !pl.contours[i].isopen()) && pl.contours[i].size()>2 && dt.isDirty(n))
    {
      if (debugging && sz==7068 && n==4362)
	cout<<"Examining endpoint "<<n<<", about to clip car\n";
      change.clear();
      change.insert(pl.contours[i].getEndpoint(n-1));
      change.insert(pl.contours[i].getEndpoint(n));
      change.insert(pl.contours[i].getEndpoint(n+1));
      lohiElev=pl.lohi(change);
      dt.markClean(n); // It's been checked, no need to recheck
      if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
      {
	j=0;
	dt.markDirty(n,1);
	dt.erase(n);
	pl.contours[i].erase(n);
	sz--;
	if (debugging && sz<=7100)
	{
	  ps.startpage();
	  ps.setscale(br);
	  ps.spline(pl.contours[i].approx3d(0.1/ps.getscale()));
	  ps.endpage();
	  checkContour(pl,pl.contours[i],tolerance);
	}
      }
    }
  }
  pl.contours[i].setlengths();
  //cout<<"        "<<i<<" error after "<<contourError(pl,pl.contours[i]);
  //cout<<" bendiness "<<totalBendiness(pl.contours[i],tolerance)<<endl;
}

void smooth1contour(pointlist &pl,double tolerance,int i)
{
  static int n=0;
  int j,sz,origsz;
  int whichNew=0;
  array<double,2> lohiElev;
  polyline change;
  xy a,b,c; // current endpoints; b is the nth
  xy p,q,r,s; // points to try changing the nth endpoint to
  double e=pl.contours[i].getElevation();
  double errCurrent,errForward,errBackward,errNewSeg,errStraighter,errBendier,errBest;
  origsz=sz=pl.contours[i].size();
  for (j=0;j<sz;j++)
  {
    n=(n+relprime(sz))%sz;
    if (n || !pl.contours[i].isopen())
    {
      a=pl.contours[i].getEndpoint(n-1);
      b=pl.contours[i].getEndpoint(n);
      c=pl.contours[i].getEndpoint(n+1);
      p=(a+2*b)/3;
      q=(2*b+c)/3;
      r=(p+q)/2;
      s=2*b-r;
      errForward=errBackward=errNewSeg=errStraighter=errBendier=INFINITY;
      errCurrent=contourError(pl,e,a,b)+contourError(pl,e,b,c)
		 +bendiness(a,b,c,tolerance);
      change.clear();
      change.insert(a);
      change.insert(b);
      change.insert(q);
      lohiElev=pl.lohi(change);
      if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
	errForward=contourError(pl,e,a,q)+contourError(pl,e,q,c)
		   +bendiness(a,q,c,tolerance);
      change.clear();
      change.insert(c);
      change.insert(b);
      change.insert(p);
      lohiElev=pl.lohi(change);
      if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
	errBackward=contourError(pl,e,a,p)+contourError(pl,e,p,c)
		    +bendiness(a,p,c,tolerance);
      //change.clear();
      //change.insert(p);
      //change.insert(b);
      //change.insert(q);
      //lohiElev=pl.lohi(change);
      //if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
	//errNewSeg=contourError(pl,e,a,p)+contourError(pl,e,p,q)+contourError(pl,e,q,c);
      change.clear();
      change.insert(a);
      change.insert(b);
      change.insert(c);
      change.insert(r);
      lohiElev=pl.lohi(change);
      if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
	errStraighter=contourError(pl,e,a,r)+contourError(pl,e,r,c)
		      +bendiness(a,r,c,tolerance);
      change.clear();
      change.insert(a);
      change.insert(b);
      change.insert(c);
      change.insert(s);
      lohiElev=pl.lohi(change);
      if (lohiElev[0]>=e-tolerance && lohiElev[1]<=e+tolerance)
	errBendier=contourError(pl,e,a,s)+contourError(pl,e,s,c)
		   +bendiness(a,s,c,tolerance);
      errBest=errCurrent;
      if (errForward<errBest)
      {
	errBest=errForward;
	whichNew=1;
      }
      if (errBackward<errBest)
      {
	errBest=errBackward;
	whichNew=2;
      }
      // skip new segment for now
      if (errStraighter<errBest)
      {
	errBest=errStraighter;
	whichNew=4;
      }
      if (errBendier<errBest)
      {
	errBest=errBendier;
	whichNew=5;
      }
      if (whichNew && (errCurrent-errBest)*16777216>errCurrent)
      {
	j=0;
	switch (whichNew)
	{
	  case 1:
	    pl.contours[i].replace(q,n);
	    break;
	  case 2:
	    pl.contours[i].replace(p,n);
	    break;
	  case 4:
	    pl.contours[i].replace(r,n);
	    break;
	  case 5:
	    pl.contours[i].replace(s,n);
	    break;
	}
      }
    }
  }
  pl.contours[i].setlengths();
}

void smoothcontours(pointlist &pl,double conterval,bool spiral,bool log)
{
  int i;
  PostScript ps;
  double we,ea,so,no;
  ofstream logfile;
  we=pl.dirbound(0);
  so=pl.dirbound(DEG90);
  ea=-pl.dirbound(DEG180);
  no=-pl.dirbound(DEG270);
  //we=443479;
  //so=164112;
  //ea=443486;
  //no=164119;
  if (log)
  {
    ps.open("smoothcontours.ps");
    ps.setpaper(papersizes["A4 portrait"],0);
    ps.prolog();
  }
  for (i=0;i<pl.contours.size();i++)
  {
    cout<<"smoothcontours "<<i<<'/'<<pl.contours.size()<<" elev "<<i*conterval<<" \r";
    cout.flush();
  }
  if (log)
  {
    ps.trailer();
    ps.close();
  }
}
