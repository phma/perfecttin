/******************************************************/
/*                                                    */
/* ps.cpp - PostScript output                         */
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
 * 
 * Literal PostScript code in this file, which is written to Decisite's
 * output, is in the public domain.
 */
#include <cstdio>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#include <iomanip>
#include <unistd.h>
#include "ldecimal.h"
#include "config.h"
#include "ps.h"
#include "point.h"
#include "pointlist.h"
using namespace std;

#define PAPERRES 0.004

char rscales[]={10,12,15,20,25,30,40,50,60,80};
const double PSPoint=25.4/72;
map<string,papersize> papersizes=
/* These mean the physical orientation of the paper in the printer. If you
 * want to print in landscape, but the paper is portrait in the printer,
 * set pageorientation to 1.
 */
{
  {"A4 portrait",{210000,297000}},
  {"A4 landscape",{297000,210000}},
  {"US Letter portrait",{215900,279400}},
  {"US Letter landscape",{279400,215900}},
  {"US Legal portrait",{215900,355600}},
  {"US Legal landscape",{355600,215900}}
};

int fibmod3(int n)
{
  int i,a,b;
  for (i=a=0,b=1;a<n;i++)
  {
    b+=a;
    a=b-a;
  }
  return (a==n)?(i%3):-1;
}

bool isstraight(vector<xyz> seg)
{
  int i;
  double toler;
  bool ret=true;
  toler=dist(seg[0],seg.back())*1e-9+(seg[0].length()+seg.back().length())*1e-15;
  /* 1e-9, not 1e-15, because segments of bezier3d are generated by
   * segment::approx3d using start and end bearings rounded to the
   * nearest π/1073741824. Edges of triangles are drawn with curveto
   * because they are curved vertically.
   */
  for (i=1;i<seg.size()-1;i++)
    if (dist(seg[i]-seg[i-1],seg[i+1]-seg[i])>toler)
      ret=false;
  return ret;
}

PostScript::PostScript()
{
  oldr=oldg=oldb=NAN;
  paper=xy(210,297);
  scale=1;
  orientation=pages=0;
  indocument=inpage=inlin=false;
  psfile=nullptr;
}

PostScript::~PostScript()
{
  if (psfile)
    close();
}

void PostScript::setpaper(papersize pap,int ori)
/* ori is 0 for no rotation, 1 for 90° rotation, making portrait
 * into landscape and vice versa. Do this before each page,
 * or before calling prolog if all pages are the same size.
 */
{
  paper=xy(pap.width/1e3,pap.height/1e3);
  pageorientation=ori;
}

double PostScript::aspectRatio()
// Returns >1 for landscape, <1 for portrait.
{
  if (pageorientation&1)
    return paper.gety()/paper.getx();
  else
    return paper.getx()/paper.gety();
}

void PostScript::open(string psfname)
{
  if (psfile)
    close();
  psfile=new ofstream(psfname);
}

bool PostScript::isOpen()
{
  return psfile!=nullptr;
}

void PostScript::prolog()
{
  if (psfile && !indocument)
  {
    *psfile<<"%!PS-Adobe-3.0\n%%BeginProlog\n%%%%Pages: (atend)"<<endl;
    *psfile<<"%%BoundingBox: 0 0 "<<rint(paper.getx()/PSPoint)<<' '<<rint(paper.gety()/PSPoint)<<endl;
    *psfile<<"\n/. % ( x y )\n{ newpath 0.1 0 360 arc fill } bind def\n\n";
    *psfile<<"/- % ( x1 y1 x2 y2 )\n\n{ newpath moveto lineto stroke } bind def\n\n";
    *psfile<<"/c. % ( str )\n{ dup stringwidth -2 div exch -2 div exch\n"<<
            "3 2 roll 2 index 2 index rmoveto show rmoveto } bind def\n\n";
    *psfile<<"/mmscale { 720 254 div dup scale } bind def\n";
    *psfile<<"/col { setrgbcolor } def\n";
    *psfile<<"/n { newpath } def\n";
    *psfile<<"/m { moveto } def\n";
    *psfile<<"/l { lineto } def\n";
    *psfile<<"/c { curveto } def\n";
    *psfile<<"/s { stroke } def\n";
    *psfile<<"/af { arc fill } def\n";
    *psfile<<"%%EndProlog"<<endl;
    indocument=true;
    pages=0;
  }
}

void PostScript::startpage()
{
  if (psfile && indocument && !inpage)
  {
    ++pages;
    *psfile<<"%%Page: "<<pages<<' '<<pages<<"\n<< /PageSize [";
    *psfile<<paper.getx()*36e1/127<<' '<<paper.gety()*36e1/127<<"] >> setpagedevice\n";
    *psfile<<"gsave mmscale 0.1 setlinewidth\n";
    *psfile<<paper.getx()/2<<' '<<paper.gety()/2<<' '<<"translate ";
    *psfile<<(pageorientation&3)*90<<" rotate ";
    *psfile<<paper.getx()/-2<<' '<<paper.gety()/-2<<' '<<"translate"<<endl;
    *psfile<<"/Helvetica findfont 3 scalefont setfont"<<endl;
    oldr=oldg=oldb=NAN;
    inpage=true;
  }
}

void PostScript::endpage()
{
  if (psfile && indocument && inpage)
  {
    *psfile<<"grestore showpage"<<endl;
    inpage=false;
  }
}

void PostScript::trailer()
{
  if (inpage)
    endpage();
  if (psfile && indocument)
  {
    *psfile<<"%%BeginTrailer\n%%Pages: "<<pages<<"\n%%EndTrailer"<<endl;
    indocument=false;
  }
}

void PostScript::close()
{
  if (indocument)
    trailer();
  delete(psfile);
  psfile=nullptr;
}

void PostScript::setPointlist(pointlist &plist)
{
  pl=&plist;
}

int PostScript::getPages()
{
  return pages;
}

double PostScript::xscale(double x)
{
  return scale*(x-modelcenter.east())+paper.getx()/2;
}

double PostScript::yscale(double y)
{
  return scale*(y-modelcenter.north())+paper.gety()/2;
}

string PostScript::escape(string text)
{
  string ret;
  int ch;
  while (text.length())
  {
    ch=text[0];
    if (ch=='(' || ch==')')
      ret+='\\';
    ret+=ch;
    text.erase(0,1);
  }
  return ret;
}

void PostScript::setcolor(double r,double g,double b)
{
  if (r!=oldr || g!=oldg || b!=oldb)
  {
    *psfile<<fixed<<setprecision(3)<<r<<' '<<g<<' '<<b<<" col"<<endl;
    oldr=r;
    oldg=g;
    oldb=b;
  }
}

void PostScript::setscale(double minx,double miny,double maxx,double maxy,int ori)
/* To compute minx etc. using dirbound on e.g. a pointlist pl:
 * minx=pl.dirbound(-ori);
 * miny=pl.dirbound(DEG90-ori);
 * maxx=-pl.dirbound(DEG180-ori);
 * maxy=-pl.dirbound(DEG270-ori);
 */
{
  double xsize,ysize,papx,papy;
  int i;
  orientation=ori;
  modelcenter=xy(minx+maxx,miny+maxy)/2;
  xsize=fabs(minx-maxx);
  ysize=fabs(miny-maxy);
  papx=paper.getx();
  papy=paper.gety();
  if (pageorientation&1)
    swap(papx,papy);
  for (scale=1;scale*xsize/10<papx && scale*ysize/10<papy;scale*=10);
  for (;scale*xsize/80>papx*0.9 || scale*ysize/80>papy*0.9;scale/=10);
  for (i=0;i<9 && (scale*xsize/rscales[i]>papx*0.9 || scale*ysize/rscales[i]>papy*0.9);i++);
  scale/=rscales[i];
  *psfile<<"% minx="<<minx<<" miny="<<miny<<" maxx="<<maxx<<" maxy="<<maxy<<" scale="<<scale<<endl;
}

void PostScript::setscale(BoundRect br)
{
  setscale(br.left(),br.bottom(),br.right(),br.top(),br.getOrientation());
}

double PostScript::getscale()
{
  return scale;
}

void PostScript::dot(xy pnt,string comment)
{
  assert(psfile);
  pnt=turn(pnt,orientation);
  if (isfinite(pnt.east()) && isfinite(pnt.north()))
  {
    *psfile<<ldecimal(xscale(pnt.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt.north()),PAPERRES)<<" .";
    if (comment.length())
      *psfile<<" %"<<comment;
    *psfile<<endl;
  }
}

void PostScript::circle(xy pnt,double radius)
{
  assert(psfile);
  pnt=turn(pnt,orientation);
  if (isfinite(pnt.east()) && isfinite(pnt.north()))
    *psfile<<ldecimal(xscale(pnt.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt.north()),PAPERRES)
    <<" n "<<ldecimal(scale*radius,PAPERRES)<<" 0 360 af %"
    <<ldecimal(radius*radius,radius*radius/1000)<<endl;
}

void PostScript::line(edge lin,int num,bool colorfibaster,bool directed)
/* Used in bezitest to show the 2D TIN before the 3D triangles are constructed on it.
 * In bezitopo, use spline(lin.getsegment().approx3d(x)) to show it in 3D.
 */
{
  xy mid,disp,base,ab1,ab2,a,b;
  char *rgb;
  a=*lin.a;
  b=*lin.b;
  a=turn(a,orientation);
  b=turn(b,orientation);
  if (lin.delaunay())
    if (colorfibaster)
      switch (fibmod3(abs(pl->revpoints[lin.a]-pl->revpoints[lin.b])))
      {
	case -1:
	  setcolor(0.3,0.3,0.3);
	  break;
	case 0:
	  setcolor(1,0.3,0.3);
	  break;
	case 1:
	  setcolor(0,1,0);
	  break;
	case 2:
	  setcolor(0.3,0.3,1);
	  break;
      }
    else
      setcolor(0,0,1);
  else
    setcolor(0,0,0);
  if (directed)
  {
    disp=b-a;
    base=xy(disp.north()/40,disp.east()/-40);
    ab1=a+base;
    ab2=a-base;
    *psfile<<"n "<<xscale(b.east())<<' '<<yscale(b.north())<<" m "<<xscale(ab1.east())<<' '<<yscale(ab1.north())<<" l "<<xscale(ab2.east())<<' '<<yscale(ab2.north())<<" l closepath fill"<<endl;
  }
  else
    *psfile<<xscale(a.east())<<' '<<yscale(a.north())<<' '<<xscale(b.east())<<' '<<yscale(b.north())<<" -"<<endl;
  mid=(a+b)/2;
  //fprintf(psfile,"%7.3f %7.3f m (%d) show\n",xscale(mid.east()),yscale(mid.north()),num);
}

void PostScript::line2p(xy pnt1,xy pnt2)
{
  pnt1=turn(pnt1,orientation);
  pnt2=turn(pnt2,orientation);
  if (isfinite(pnt1.east()) && isfinite(pnt1.north()) && isfinite(pnt2.east()) && isfinite(pnt2.north()))
    *psfile<<ldecimal(xscale(pnt1.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt1.north()),PAPERRES)
    <<' '<<ldecimal(xscale(pnt2.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt2.north()),PAPERRES)<<" -"<<endl;
}

void PostScript::startline()
{
  assert(psfile);
  *psfile<<"n"<<endl;
}

void PostScript::lineto(xy pnt)
{
  assert(psfile);
  pnt=turn(pnt,orientation);
  *psfile<<ldecimal(xscale(pnt.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt.north()),PAPERRES)<<(inlin?" l":" m");
  *psfile<<endl;
  inlin=true;
}

void PostScript::endline(bool closed)
{
  assert(psfile);
  if (closed)
    *psfile<<"closepath ";
  *psfile<<"s"<<endl;
  inlin=false;
}

void PostScript::widen(double factor)
{
  *psfile<<"currentlinewidth "<<ldecimal(factor)<<" mul setlinewidth"<<endl;
}

void PostScript::write(xy pnt,string text)
{
  pnt=turn(pnt,orientation);
  *psfile<<ldecimal(xscale(pnt.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt.north()),PAPERRES)
  <<" m ("<<escape(text)<<") show"<<endl;
}

void PostScript::centerWrite(xy pnt,string text)
{
  pnt=turn(pnt,orientation);
  *psfile<<ldecimal(xscale(pnt.east()),PAPERRES)<<' '<<ldecimal(yscale(pnt.north()),PAPERRES)
  <<" m ("<<escape(text)<<") c."<<endl;
}

void PostScript::comment(string text)
{
  *psfile<<'%'<<text<<endl;
}
