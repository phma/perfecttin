/******************************************************/
/*                                                    */
/* qindex.cpp - quad index to tin                     */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of Decisite.
 * 
 * Decisite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Decisite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decisite. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include "ps.h"
#include "qindex.h"
#include "relprime.h"

/* The index enables quickly finding a triangle containing a given point.
 * x and y are the bottom left corner. side is always a power of 2,
 * and x and y are multiples of side/16.
 * The four subsquares are arranged as follows:
 * +-------+-------+
 * |       |       |
 * |   2   |   3   |
 * |       |       |
 * +-------+-------+
 * |       |       |
 * |   0   |   1   |
 * |       |       |
 * +-------+-------+
 * A square is subdivided if there are at least three points of the TIN
 * in it. A point is considered to be in a square if it is on its
 * bottom or left edge, but not if it is on its top or right edge.
 *
 * After constructing the tree of squares, the program assigns to each
 * leaf square the triangle containing its center, proceeding in
 * Hilbert-curve order.
 *
 * When constructing a TIN from bare triangles, the quad index is used to
 * keep track of points as they are entered into the pointlist.
 */
using namespace std;

#if defined(_WIN32) || defined(__CYGWIN__)
// Linux and BSD have this function in the library; Windows doesn't.
double significand(double x)
{
  int dummy;
  return frexp(x,&dummy)*2;
}
#endif

qindex::qindex()
{
  int i;
  x=y=side=0;
  for (i=0;i<4;i++)
    sub[i]=0;
}

qindex::~qindex()
{
  clear();
}

int qindex::size()
{
  if (sub[3])
    return sub[0]->size()+sub[1]->size()+sub[2]->size()+sub[3]->size()+1;
  else
    return 1;
}

xy qindex::middle()
{return xy(x+side/2,y+side/2);
 }

int qindex::quarter(xy pnt,bool clip)
{
  int xbit,ybit,i;
  xbit=pnt.x>=x+side/2;
  if (std::isnan(pnt.x) || ((!clip) && (pnt.x>=x+side || pnt.x<x)))
    xbit=-1;
  ybit=pnt.y>=y+side/2;
  if (std::isnan(pnt.y) || ((!clip) && (pnt.y>=y+side || pnt.y<y)))
    ybit=-1;
  i=(ybit<<1)|xbit;
  return i;
}

triangle *qindex::findt(xy pnt,bool clip)
{
  int i;
  xy inpnt=pnt;
  i=quarter(pnt,clip);
  if (i<0)
    return NULL; // point is outside square
  else if (!sub[3])
    return tri->findt(pnt,clip); // square is undivided
  else 
    return sub[i]->findt(pnt,clip);
}

point *qindex::findp(xy pont,bool clip)
{
  int i;
  xy inpnt=pont;
  point *ret=nullptr;
  i=quarter(pont,clip);
  if (i<0)
    ret=nullptr; // point is outside square
  else if (!sub[3])
    for (i=0;i<3;i++)
    {
      if (pnt[i] && pont==xy(*pnt[i]))
	ret=pnt[i];
    }
  else 
    ret=sub[i]->findp(pont,clip);
  return ret;
}

void qindex::insertPoint(point *pont,bool clip)
/* The qindex must have already been split, with a vector including the xy
 * of this point. If not, there may already be three points in the leaf,
 * in which case it does nothing.
 */
{
  int i;
  xy inpnt=*pont;
  i=quarter(*pont,clip);
  if (i<0)
    ; // point is outside square
  else if (!sub[3])
    for (i=0;i<3;i++)
    {
      if (!pnt[i] || xy(*pont)==xy(*pnt[i]))
      {
	if (pnt[i] && pont->elev()!=pnt[i]->elev())
	  throw samePoints;
	else
	  pnt[i]=pont;
	break;
      }
    }
  else 
    sub[i]->insertPoint(pont,clip);
}

void qindex::sizefit(vector<xy> pnts)
/* Computes size, x, and y such that size is a power of 2, x and y are multiples
 * of size/16, and all points are in the resulting square.
 */
{
  double minx=HUGE_VAL,miny=HUGE_VAL,maxx=-HUGE_VAL,maxy=-HUGE_VAL;
  int i;
  for (i=0;i<pnts.size();i++)
  {
    if (pnts[i].east()>maxx)
      maxx=pnts[i].east();
    if (pnts[i].east()<minx)
      minx=pnts[i].east();
    if (pnts[i].north()>maxy)
      maxy=pnts[i].north();
    if (pnts[i].north()<miny)
      miny=pnts[i].north();
  }
  if (maxy<=miny && maxx<=minx)
    side=0;
  else
  {
    side=(maxx+maxy-minx-miny)/2;
    side/=significand(side);
    x=minx-side;
    y=miny-side;
    while (x+side<=maxx || y+side<=maxy)
    {
      side*=2;
      x=(rint((minx+maxx)/side*8)-8)*side/16;
      y=(rint((miny+maxy)/side*8)-8)*side/16;
    }
  }
}

void qindex::split(vector<xy> pnts)
/* Splits qindex so that each leaf has at most three points.
 * When reading a file of unlabeled triangles, each point occurs as many
 * times as triangles have it as corner, 6 on average. These dupes must
 * be removed.
 */
{
  vector<xy> subpnts[4];
  int h,i,j,n,q,nancnt=0,sz=pnts.size();
  if (pnts.size()>18) // 3 points per leaf * 6 copies of each point
    for (i=1;i<sz;i++)
      if (pnts[i]==pnts[i-1])
      {
	pnts[i-1]=xy(NAN,NAN);
	nancnt++;
      }
      else;
  else
    for (i=1;i<sz;i++)
      for (j=0;j<i;j++)
	if (pnts[i]==pnts[j])
	{
	  pnts[j]=xy(NAN,NAN);
	  nancnt++;
	}
  if (sz-nancnt>3)
  {
    h=relprime(sz);
    for (i=n=0;i<sz;i++,n=(n+h)%sz)
    {
      q=quarter(pnts[n]);
      if (q>=0)
	subpnts[q].push_back(pnts[n]);
    }
    //if (nancnt)
      //cout<<"Deleted "<<nancnt<<" duplicates out of "<<sz<<endl;
    for (i=0;i<4;i++)
    {
      sub[i]=new qindex;
      sub[i]->x=x+(i&1)*(side/2);
      sub[i]->y=y+(i>>1)*(side/2);
      sub[i]->side=side/2;
      sub[i]->split(subpnts[i]);
    }
  }
}

void qindex::clear()
{
  int i;
  if (sub[3])
    for (i=0;i<4;i++)
    {
      delete(sub[i]);
      sub[i]=NULL;
    }
}

void qindex::clearLeaves()
/* Leaves the tree structure intact, but clears the leaves (pointers to
 * points or triangles). This is used when making a TIN of bare triangles,
 * after inserting all the points into the pointlist and before making
 * the index point to triangles.
 */
{
  int i;
  if (sub[3])
    for (i=0;i<4;i++)
      sub[i]->clearLeaves();
  else
    for (i=0;i<3;i++)
      pnt[i]=nullptr;
}

void qindex::draw(PostScript &ps,bool root)
{
  int i;
  if (root) // if this is the root of the tree, draw its border
  {
    ps.line2p(xy(x,y),xy(x+side,y));
    ps.line2p(xy(x+side,y),xy(x+side,y+side));
    ps.line2p(xy(x+side,y+side),xy(x,y+side));
    ps.line2p(xy(x,y+side),xy(x,y));
  }
  if (sub[3])
  {
    for (i=0;i<4;i++)
      sub[i]->draw(ps,false);
    ps.line2p(xy(x,y+side/2),xy(x+side,y+side/2));
    ps.line2p(xy(x+side/2,y),xy(x+side/2,y+side));
  }
}

vector<qindex*> qindex::traverse(int dir)
/* 0 0231 1002 569a
 * 1 0132 0113 478b
 * 2 3201 3220 32dc
 * 3 3102 2331 01ef
 */
{
  vector<qindex*> chain,subchain;
  int i,j,subdirs[4],parts[4];
  if (sub[3])
  {
    subdirs[0]=dir^1;
    subdirs[1]=subdirs[2]=dir&3;
    subdirs[3]=dir^2;
    parts[0]=(dir&2)*3/2;
    parts[1]=2-(dir&1);
    parts[2]=3-parts[0];
    parts[3]=3-parts[1];
    for (i=0;i<4;i++)
    {
      subchain=sub[parts[i]]->traverse(subdirs[i]);
      for (j=0;j<subchain.size();j++)
	chain.push_back(subchain[j]);
    }
  }
  else
    chain.push_back(this);
  return chain;
}

void qindex::settri(triangle *starttri)
{
  int i,j;
  triangle *thistri;
  vector<qindex*> chain;
  chain=traverse();
  thistri=starttri;
  for (i=0;i<chain.size();i++)
  {
    thistri=thistri->findt(chain[i]->middle(),true);
    chain[i]->tri=thistri;
  }
}

set<triangle *> qindex::localTriangles(xy center,double radius,int max)
/* Returns up to max pointers to triangles, the leaves of the tree whose centers
 * are within radius of center. If there are more than max in the circle, returns
 * a single nullptr. Used when drawing the TIN, to avoid looping through the
 * entire map of edges when drawing a small part of the TIN.
 *
 * This will not return all triangles that are within the circle. The set of all
 * corners of these triangles can still miss a point. But the set of all edges
 * incident on these points should include all the edges in the circle, except
 * maybe some near the boundary of the circle.
 */
{
  int i;
  set<triangle *> list,sublist;
  set<triangle *>::iterator j;
  if (max<0)
    list.insert(nullptr);
  else if (sub[3])
    if (dist(middle(),center)<=radius+side/M_SQRT2)
      for (i=0;i<4;i++)
      {
	sublist=sub[i]->localTriangles(center,radius,max);
	if (sublist.count(nullptr))
	{
	  max=-1;
	  list=sublist;
	}
	else
	{
	  max+=list.size();
	  for (j=sublist.begin();j!=sublist.end();j++)
	    list.insert(*j);
	  max-=list.size();
	}
      }
    else; // the square is outside the circle, do nothing
  else if (tri && dist(middle(),center)<=radius)
    list.insert(tri);
  return list;
}
