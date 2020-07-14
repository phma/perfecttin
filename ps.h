/******************************************************/
/*                                                    */
/* ps.h - PostScript output                           */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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
#ifndef PS_H
#define PS_H
#include <string>
#include <iostream>
#include <map>
#include "tin.h"
#include "boundrect.h"
class pointlist;

struct papersize
{
  int width,height; // in micrometers
};
extern std::map<std::string,papersize> papersizes;

class PostScript
{
protected:
  std::ostream *psfile;
  int pages;
  bool indocument,inpage,inlin;
  double scale; // paper size is in millimeters, but model space is in meters
  int orientation,pageorientation;
  double oldr,oldg,oldb;
  xy paper,modelcenter;
  pointlist *pl;
public:
  PostScript();
  ~PostScript();
  void setpaper(papersize pap,int ori);
  double aspectRatio();
  void open(std::string psfname);
  bool isOpen();
  void prolog();
  void startpage();
  void endpage();
  void trailer();
  void close();
  void setPointlist(pointlist &plist);
  int getPages();
  double xscale(double x);
  double yscale(double y);
  std::string escape(std::string text);
  void setcolor(double r,double g,double b);
  void setscale(double minx,double miny,double maxx,double maxy,int ori=0);
  void setscale(BoundRect br);
  double getscale();
  void dot(xy pnt,std::string comment="");
  void circle(xy pnt,double radius);
  void line(edge lin,int num,bool colorfibaster,bool directed=false);
  void line2p(xy pnt1,xy pnt2);
  void startline();
  void lineto(xy pnt);
  void endline(bool closed=false,bool fill=false);
  void widen(double factor);
  void write(xy pnt,const std::string &text);
  void centerWrite(xy pnt,const std::string &text);
  void comment(const std::string &text);
};

#endif
