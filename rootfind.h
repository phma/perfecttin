/******************************************************/
/*                                                    */
/* rootfind.h - root-finding methods                  */
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

double invquad(double x0,double y0,double x1,double y1,double x2,double y2);

class brent
{
public:
  double init(double x0,double y0,double x1,double y1,bool intmode=false);
  double step(double y);
  bool finished()
  {
    return !((side&3)%3);
  }
  void setdebug(bool dbg)
  {
    debug=dbg;
  }
private:
  double a,fa,b,fb,c,fc,d,fd,x;
  int side;
  bool mflag,imode,debug;
  bool between(double s);
};

class Newton
{
public:
  double init(double x0,double y0,double z0,double x1,double y1,double z1);
  // z0 is the derivative at x0.
  double step(double y,double z);
  bool finished()
  {
    return done;
  }
  void setdebug(bool dbg)
  {
    debug=dbg;
  }
private:
  double a,fa,da,b,fb,db,x;
  bool done,debug;
  int nodec;
};
