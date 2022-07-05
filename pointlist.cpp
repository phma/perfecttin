/******************************************************/
/*                                                    */
/* pointlist.cpp - list of points                     */
/*                                                    */
/******************************************************/
/* Copyright 2019-2022 Pierre Abbat.
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
#include <cfloat>
#include "angle.h"
#include "pointlist.h"
#include "ldecimal.h"
#include "threads.h"
#include "triop.h"

using namespace std;

const bool loudTinConsistency=false;

int lhash(segment s)
{
  return (int)llrint(4294967296*log(s.length()));
}

void pointlist::clear()
{
  wingEdge.lock();
  qinx.clear();
  pieceDraw.clear();
  trianglePaint.clear();
  contours.clear();
  triangles.clear();
  revtriangles.clear();
  edges.clear();
  points.clear();
  revpoints.clear();
  convexHull.clear();
  edgePool.clear();
  trianglePool.clear();
  swishFactor=0;
  setDirty(false);
  currentContours=nullptr;
  wingEdge.unlock();
}

void pointlist::clearTin()
{
  wingEdge.lock();
  triangles.clear();
  revtriangles.clear();
  edges.clear();
  wingEdge.unlock();
}

void pointlist::setDirty(bool d)
{
  dirty=d;
  //cout<<"Pointlist is "<<(d?"dirty":"clean")<<endl;
}

int pointlist::size()
{
  return points.size();
}

void pointlist::clearmarks()
{
  map<int,edge>::iterator e;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.clearmarks();
}

void pointlist::unsetCurrentContours()
{
  int i;
  for (i=0;currentContours && i<currentContours->size();i++)
    deletePieces((*currentContours)[i],-1);
  currentContours=nullptr;
}

void pointlist::setCurrentContours(ContourInterval &ci)
/* Selects which contours are currently being worked on (traced, pruned, etc.).
 * If a set of contours with this contour interval does not exist, creates it.
 */
{
  int i;
  for (i=0;currentContours && i<currentContours->size();i++)
    deletePieces((*currentContours)[i],-1);
  currentContours=&contours[ci];
  for (i=0;currentContours && i<currentContours->size();i++)
    insertPieces((*currentContours)[i],-1);
}

void pointlist::deleteCurrentContours()
{
  int i;
  map<ContourInterval,vector<polyspiral> >::iterator j;
  for (i=0;currentContours && i<currentContours->size();i++)
    deletePieces((*currentContours)[i],-1);
  for (j=contours.begin();j!=contours.end();++j)
    if (&j->second==currentContours)
    {
      setDirty(true);
      contours.erase(j);
      break;
    }
  currentContours=nullptr;
}

vector<ContourInterval> pointlist::contourIntervals()
{
  vector<ContourInterval> ret;
  map<ContourInterval,vector<polyspiral> >::iterator i;
  for (i=contours.begin();i!=contours.end();++i)
    ret.push_back(i->first);
  return ret;
}

map<ContourLayer,int> pointlist::contourLayers()
{
  map<ContourInterval,vector<polyspiral> >::iterator i;
  int j;
  map<ContourLayer,int>::iterator k;
  map<ContourLayer,int> ret;
  ContourLayer cl;
  for (i=contours.begin();i!=contours.end();++i)
  {
    cl.ci=i->first;
    for (j=0;j<i->second.size();j++)
    {
      cl.tp=cl.ci.contourType(i->second[j].getElevation());
      ret[cl]=0;
    }
  }
  /* Start at layer 3. Layer 0 is reserved by the DXF format, layer 1 is for
   * the TIN, and layer 2 is for the boundary.
   */
  for (k=ret.begin(),j=3;k!=ret.end();++k,++j)
    k->second=j;
  return ret;
}

void pointlist::nipPiece()
// Call only when pieceMutex is locked.
{
  ++pieceInx;
  if (contourPieces.count(pieceInx) && contourPieces[pieceInx].size()==0)
    contourPieces.erase(pieceInx);
}

void pointlist::nipPieces()
// If called 20 times a second, this clears all empty hash buckets in a day.
{
  int i,j;
  for (i=0;i<35;i++)
  {
    pieceMutex.lock();
    for (j=0;j<71;j++)
      nipPiece();
    pieceMutex.unlock();
  }
}

void pointlist::insertContourPiece(spiralarc s,int thread)
{
  ContourPiece piece;
  int i=0;
  int inx;
  vector<ContourPiece> *pcList;
  // clip is true in case the piece starts just outside the TIN because of roundoff
  triangle *tri=findt(s.getstart(),true);
  bool found=false;
  inx=lhash(s);
  piece.s=s;
  if (tri)
    piece.tris.push_back(tri);
  while (tri && !tri->in(s.getend()))
  {
    tri=tri->nextalong(s);
    if (tri)
      piece.tris.push_back(tri);
    i++;
    if (i>4 && piece.tris.back()==piece.tris[piece.tris.size()-3])
      tri=nullptr;
  }
  pieceMutex.lock();
  pcList=&contourPieces[inx];
  for (i=0;i<pcList->size();i++)
    if ((*pcList)[i].s==s)
      found=true;
  //cout<<"Insert ("<<s.getstart().getx()<<','<<s.getstart().gety()<<")-(";
  //cout<<s.getend().getx()<<','<<s.getend().gety()<<") Found "<<found<<endl;
  if (!found)
    pcList->push_back(piece);
  nipPiece();
  pieceMutex.unlock();
  while (!lockTriangles(thread,piece.tris))
    sleepms(thread);
  for (i=0;i<piece.tris.size();i++)
    piece.tris[i]->crossingPieces.push_back(inx);
  unlockTriangles(thread);
  for (i=0;i<piece.tris.size();i++)
    net.trianglePaint.enqueue(piece.tris[i],thread);
}

void pointlist::deleteContourPiece(spiralarc s,int thread)
{
  ContourPiece piece;
  int i,j;
  int inx;
  vector<ContourPiece> *pcList;
  int found=-1;
  inx=lhash(s);
  pieceMutex.lock();
  pcList=&contourPieces[inx];
  for (i=0;i<pcList->size();i++)
    if ((*pcList)[i].s==s)
      found=i;
  //cout<<"Delete ("<<s.getstart().getx()<<','<<s.getstart().gety()<<")-(";
  //cout<<s.getend().getx()<<','<<s.getend().gety()<<") Found "<<found<<endl;
  if (found>=0)
  {
    piece=(*pcList)[found];
    swap((*pcList)[found],pcList->back());
    pcList->resize(pcList->size()-1);
  }
  //else
    //cout<<"Not found\n";
  nipPiece();
  pieceMutex.unlock();
  if (!pcList->size())
  { // This may leave some crossingPieces in case of hash collisions.
    while (!lockTriangles(thread,piece.tris))
      sleepms(thread);
    for (i=0;i<piece.tris.size();i++)
      for (j=0;j<piece.tris[i]->crossingPieces.size();j++)
	if (piece.tris[i]->crossingPieces[j]==inx)
	{
	  swap(piece.tris[i]->crossingPieces[j],piece.tris[i]->crossingPieces.back());
	  piece.tris[i]->crossingPieces.resize(piece.tris[i]->crossingPieces.size()-1);
	}
    unlockTriangles(thread);
  }
  for (i=0;i<piece.tris.size();i++)
    net.trianglePaint.enqueue(piece.tris[i],thread);
}

vector<ContourPiece> pointlist::getContourPieces(int inx)
{
  vector<ContourPiece> ret;
  pieceMutex.lock();
  ret=contourPieces[inx];
  pieceMutex.unlock();
  return ret;
}

void pointlist::insertPieces(polyspiral ctour,int thread)
{
  int i;
  for (i=0;i<ctour.size();i++)
    insertContourPiece(ctour.getspiralarc(i),thread);
}

void pointlist::deletePieces(polyspiral ctour,int thread)
{
  int i;
  for (i=0;i<ctour.size();i++)
    deleteContourPiece(ctour.getspiralarc(i),thread);
}

int pointlist::statsPieces()
{
  map<int,vector<ContourPiece> >::iterator i;
  vector<int> histo;
  int j,total=0;
  for (i=contourPieces.begin();i!=contourPieces.end();++i)
  {
    while (histo.size()<=i->second.size())
      histo.push_back(0);
    histo[i->second.size()]++;
  }
  for (j=0;j<histo.size();j++)
  {
    cout<<histo[j]<<" buckets with "<<j<<" pieces\n";
    total+=j*histo[j];
  }
  cout<<"total "<<contourPieces.size()<<" buckets, "<<total<<" pieces\n";
  return total;
}

void pointlist::eraseEmptyContours()
{
  vector<polyspiral> nonempty;
  int i;
  for (i=0;i<(*currentContours).size();i++)
    if ((*currentContours)[i].size()>2 || (*currentContours)[i].isopen())
      nonempty.push_back((*currentContours)[i]);
    else
      deletePieces((*currentContours)[i],-1);
  nonempty.shrink_to_fit();
  swap((*currentContours),nonempty);
}

int pointlist::isSmoothed(const segment &seg)
/* Returns 0 for a piece of a rough contour, 1 for a piece of a pruned
 * contour, and 2 for a piece of a smoothed contour. It may return wrong
 * answers, so try several pieces.
 *
 * A rough contour piece is always a segment with its midpoint in a triangle
 * and both ends on edges of the same triangle. The midpoint and both ends are
 * at the same elevation.
 *
 * A pruned contour piece is a segment whose ends are on edges at the same
 * elevation, but usually they aren't in the same triangle, and usually the
 * midpoint is not at the same elevation as the endpoints.
 *
 * A smoothed contour piece is a spiralarc whose ends and midpoint usually
 * are at three different elevations. Its ends are usually not on edges.
 */
{
  triangle *tribeg=findt(seg.getstart(),true);
  triangle *trimid=findt(seg.midpoint(),true);
  triangle *triend=findt(seg.getend(),true);
  double begElev=tribeg->elevation(seg.getstart());
  double midElev=trimid->elevation(seg.midpoint());
  double endElev=triend->elevation(seg.getend());
  xy midgrad=trimid->gradient(seg.midpoint());
  double hToler=seg.epsilon();
  double vToler=hToler*midgrad.length()+fabs(midElev)*DBL_EPSILON;
  int ret=0;
  if (!trimid->onEdge(seg.getend(),hToler) || !trimid->onEdge(seg.getstart(),hToler))
    ret=1;
  if (fabs(begElev-midElev)>vToler || fabs(midElev-endElev)>vToler)
    ret=1;
  if (!triend->onEdge(seg.getend(),hToler) || !tribeg->onEdge(seg.getstart(),hToler))
    ret=2;
  if (fabs(endElev-begElev>vToler))
    ret=2;
  return ret;
}

int pointlist::isNextPieceSmoothed()
// Returns -1 if the map entry is empty, otherwise as isSmoothed.
{
  int i,sm,ret=-1,startInx;
  pieceMutex.lock();
  startInx=pieceInx;
  do
  {
    pieceInx+=0x69969669;
    if (pieceInx==startInx)
      break;
  } while (!contourPieces.count(pieceInx));
  for (i=0;i<contourPieces[pieceInx].size();i++)
  {
    sm=isSmoothed(contourPieces[pieceInx][i].s);
    if (sm>ret)
      ret=sm;
  }
  pieceMutex.unlock();
  return ret;
}

bool pointlist::checkTinConsistency()
{
  bool ret=true;
  int i,n,nInteriorEdges=0,nNeighborTriangles=0,turn1;
  double a;
  long long totturn;
  ptlist::iterator p;
  vector<int> edgebearings;
  edge *ed;
  for (p=points.begin();p!=points.end();++p)
  {
    ed=p->second.line;
    if (ed==nullptr || (ed->a!=&p->second && ed->b!=&p->second))
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" line pointer is wrong.\n";
    }
    edgebearings.clear();
    do
    {
      if (ed)
	ed=ed->next(&p->second);
      if (ed)
	edgebearings.push_back(ed->bearing(&p->second));
    } while (ed && ed!=p->second.line && edgebearings.size()<=edges.size());
    if (edgebearings.size()>=edges.size())
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" next pointers do not return to line pointer.\n";
    }
    for (totturn=i=0;i<edgebearings.size();i++)
    {
      turn1=(edgebearings[(i+1)%edgebearings.size()]-edgebearings[i])&(DEG360-1);
      totturn+=turn1;
      if (turn1==0)
      {
	ret=false;
	if (loudTinConsistency)
	  cerr<<"Point "<<p->first<<" has two equal bearings.\n";
      }
    }
    if (totturn!=(long long)DEG360) // DEG360 is construed as positive when cast to long long
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" bearings do not wind once counterclockwise.\n";
    }
  }
  for (i=0;i<edges.size();i++)
  {
    if (edges[i].isinterior())
      nInteriorEdges++;
    if ((edges[i].tria!=nullptr)+(edges[i].trib!=nullptr)!=1+edges[i].isinterior())
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Edge "<<i<<" has wrong number of adjacent triangles.\n";
    }
    if (edges[i].tria)
    {
      a=n=0;
      if (edges[i].tria->a==edges[i].a || edges[i].tria->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->a);
      if (edges[i].tria->b==edges[i].a || edges[i].tria->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->b);
      if (edges[i].tria->c==edges[i].a || edges[i].tria->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->c);
      if (n!=2)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle a does not have edge as a side.\n";
      }
      if (a>=0)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle a is on the wrong side.\n";
      }
    }
    if (edges[i].trib)
    {
      a=n=0;
      if (edges[i].trib->a==edges[i].a || edges[i].trib->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->a);
      if (edges[i].trib->b==edges[i].a || edges[i].trib->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->b);
      if (edges[i].trib->c==edges[i].a || edges[i].trib->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->c);
      if (n!=2)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle b does not have edge as a side.\n";
      }
      if (a<=0)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle b is on the wrong side.\n";
      }
    }
  }
  for (i=0;i<triangles.size();i++)
  {
    if (triangles[i].aneigh)
    {
      nNeighborTriangles++;
      if ( triangles[i].aneigh->iscorner(triangles[i].a) ||
          !triangles[i].aneigh->iscorner(triangles[i].b) ||
          !triangles[i].aneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor a is wrong.\n";
      }
    }
    if (triangles[i].bneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].bneigh->iscorner(triangles[i].a) ||
           triangles[i].bneigh->iscorner(triangles[i].b) ||
          !triangles[i].bneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor b is wrong.\n";
      }
    }
    if (triangles[i].cneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].cneigh->iscorner(triangles[i].a) ||
          !triangles[i].cneigh->iscorner(triangles[i].b) ||
           triangles[i].cneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor c is wrong.\n";
      }
    }
  }
  if (nInteriorEdges*2!=nNeighborTriangles)
  {
    ret=false;
    if (loudTinConsistency)
      cerr<<"Interior edges and neighbor triangles don't match.\n";
  }
  return ret;
}

void pointlist::addpoint(int numb,point pnt,bool overwrite)
{int a;
 if (points.count(numb))
    if (overwrite)
       points[a=numb]=pnt;
    else
       {if (numb<0)
           {a=points.begin()->first-1;
            if (a>=0)
               a=-1;
            }
        else
           {a=points.rbegin()->first+1;
            if (a<=0)
               a=1;
            }
        points[a]=pnt;
        }
 else
    points[a=numb]=pnt;
 revpoints[&(points[a])]=a;
 }

int pointlist::addtriangle(int n,int thread)
{
  int i;
  int newTriNum=triangles.size();
  if (thread>=0)
    lockNewTriangles(thread,n);
  for (i=0;i<n;i++)
  {
    triangles[newTriNum+i].sarea=0;
    revtriangles[&triangles[newTriNum+i]]=newTriNum+i;
  }
  return newTriNum;
}

void pointlist::insertHullPoint(point *newpnt,point *prec)
/* Inserts a point into the convex hull, which usually has much fewer points
 * than a path across the middle of the TIN. It is used to paint the space
 * around the TIN in the GUI.
 */
{
  int i;
  convexHull.push_back(newpnt);
  for (i=convexHull.size()-2;i>-1 && convexHull[i]!=prec;i--)
    swap(convexHull[i],convexHull[i+1]);
}

int pointlist::closestHullPoint(xy pnt)
{
  int i,ret=-1;
  double d,closeDist=INFINITY;
  for (i=0;i<convexHull.size();i++)
    if ((d=dist((xy)*convexHull[i],pnt))<closeDist)
    {
      ret=i;
      closeDist=d;
    }
  return ret;
}

double pointlist::distanceToHull(xy pnt)
/* pnt should be outside the hull. This is used to compute how big a scale
 * can be drawn in the corner of the window.
 */
{
  int n=closestHullPoint(pnt);
  int sz=convexHull.size();
  xy a,b,c;
  double ret=NAN;
  if (sz && n>=0)
  {
    a=(xy)*convexHull[(n+sz-1)%sz];
    b=(xy)*convexHull[n];
    c=(xy)*convexHull[(n+1)%sz];
    ret=dist(b,pnt);
    if (isinsector(dir(b,pnt)-dir(b,a),0xf00ff00f) && pldist(pnt,b,a)<ret)
      ret=pldist(pnt,b,a);
    if (isinsector(dir(b,pnt)-dir(b,c),0xf00ff00f) && pldist(pnt,c,b)<ret)
      ret=pldist(pnt,c,b);
  }
  return ret;
}

bool pointlist::validConvexHull()
/* Deflection angles around the convex hull must be positive (else it isn't convex)
 * and no more than 45° (because it starts as an equiangular octagon).
 */
{
  int i;
  bool ret=true;
  int deflectionAngle;
  int sz=convexHull.size();
  for (i=0;i<sz;i++)
  {
    deflectionAngle=foldangle(dir((xy)*convexHull[(i+1)%sz],(xy)*convexHull[i])-
			      dir((xy)*convexHull[i],(xy)*convexHull[(i+sz-1)%sz]));
    //cout<<i<<' '<<ldecimal(bintodeg(deflectionAngle))<<endl;
    if (deflectionAngle<1 || deflectionAngle>DEG45+1)
      ret=false;
  }
  return ret;
}

vector<int> pointlist::valencyHistogram()
{
  int j;
  ptlist::iterator i;
  edge *e;
  point *p;
  vector<int> ret;
  wingEdge.lock_shared();
  for (i=points.begin();i!=points.end();++i)
  {
    p=&i->second;
    e=p->line;
    for (j=0;j==0 || e!=p->line;j++)
      e=e->next(p);
    if (ret.size()<=j)
      ret.resize(j+1);
    ret[j]++;
  }
  wingEdge.unlock_shared();
  return ret;
}

bool pointlist::shouldWrite(int n,int flags,bool contours)
/* Called from a function that exports a TIN (except in STL format) to export
 * some of the triangles. The flags come from a ThreadAction (see threads.h).
 * If bit 0 is set, write empty triangles.
 * If bit 1 is set and there is a boundary, write only triangles whose centroid
 * is in the boundary.
 * If bit 2 is set and contours is true, don't write any triangles.
 */
{
  bool ret=triangles[n].dots.size() || (flags&1);
  if (contours && (flags&4))
    ret=false;
  if (boundary.size() && (flags&2))
    ret=ret && boundary.in(triangles[n].centroid());
  return ret;
}

void pointlist::makeqindex()
{
  vector<xy> plist;
  ptlist::iterator i;
  qinx.clear();
  for (i=points.begin();i!=points.end();++i)
    plist.push_back(i->second);
  qinx.sizefit(plist);
  qinx.split(plist);
  if (triangles.size())
    qinx.settri(&triangles[0]);
}

void pointlist::updateqindex()
/* Use this when you already have a quad index, split to cover all the points,
 * but the leaves don't point to the right triangles because you've flipped
 * some edges.
 */
{
  if (triangles.size())
    qinx.settri(&triangles[0]);
}

double pointlist::elevation(xy location)
{
  triangle *t;
  t=qinx.findt(location);
  if (t)
    return t->elevation(location);
  else
    return NAN;
}

xy pointlist::gradient(xy location)
{
  triangle *t;
  t=qinx.findt(location);
  if (t)
    return t->gradient(location);
  else
    return nanxy;
}

double pointlist::dirbound(int angle)
/* angle=0x00000000: returns least easting.
 * angle=0x20000000: returns least northing.
 * angle=0x40000000: returns negative of greatest easting.
 */
{
  ptlist::iterator i;
  double bound=HUGE_VAL,turncoord;
  double s=sin(angle),c=cos(angle);
  for (i=points.begin();i!=points.end();++i)
  {
    turncoord=i->second.east()*c+i->second.north()*s;
    if (turncoord<bound)
      bound=turncoord;
  }
  return bound;
}

triangle *pointlist::findt(xy pnt,bool clip)
{
  return qinx.findt(pnt,clip);
}

void pointlist::roscat(xy tfrom,int ro,double sca,xy tto)
{
  xy cs=cossin(ro);
  ptlist::iterator j;
  for (j=points.begin();j!=points.end();++j)
    j->second._roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
}
