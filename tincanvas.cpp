/******************************************************/
/*                                                    */
/* tincanvas.cpp - canvas for drawing TIN             */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 * 
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PerfectTIN. If not, see <http://www.gnu.org/licenses/>.
 */
#include <QPainter>
#include <cmath>
#include "tincanvas.h"
#include "boundrect.h"
#include "octagon.h"
#include "angle.h"
#include "threads.h"
#include "ldecimal.h"

using namespace std;

QPoint qptrnd(xy pnt)
{
  return QPoint(lrint(pnt.getx()),lrint(pnt.gety()));
}

TinCanvas::TinCanvas(QWidget *parent):QWidget(parent)
{
  int i,j,rgb;
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
  setBrush(Qt::red);
  setPen(Qt::NoPen);
  setMinimumSize(40,30);
  triangleNum=0;
}

void TinCanvas::setPen(const QPen &qpen)
{
  pen=qpen;
}

void TinCanvas::setBrush(const QBrush &qbrush)
{
  brush=qbrush;
}

QPointF TinCanvas::worldToWindow(xy pnt)
{
  pnt.roscat(worldCenter,0,scale,windowCenter);
  QPointF ret(pnt.getx(),height()-pnt.gety());
  return ret;
}

xy TinCanvas::windowToWorld(QPointF pnt)
{
  xy ret(pnt.x(),height()-pnt.y());
  ret.roscat(windowCenter,0,1/scale,worldCenter);
  return ret;
}

void TinCanvas::sizeToFit()
{
  int i;
  double xscale,yscale;
  BoundRect br;
  for (i=0;i<8 && i<net.points.size();i++)
    br.include(net.points[i+1]);
  if (br.left()>=br.right() && br.bottom()>=br.top())
  {
    worldCenter=xy(0,0);
    scale=1;
  }
  else
  {
    worldCenter=xy((br.left()+br.right())/2,(br.bottom()+br.top())/2);
    xscale=width()/(br.right()-br.left());
    yscale=height()/(br.top()-br.bottom());
    if (xscale<yscale)
      scale=xscale;
    else
      scale=yscale;
  }
}

void TinCanvas::tick()
{
  int i;
  int thisOpcount=opcount;
  double r,g,b;
  xy gradient,A,B,C;
  QPolygonF polygon;
  QPolygon swath;
  xy oldBallPos=ballPos;
  QPainter painter(&frameBuffer);
  QBrush brush;
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.setPen(Qt::NoPen);
  brush.setStyle(Qt::SolidPattern);
  ballAngle+=lrint(8388608*sqrt((unsigned)(thisOpcount-lastOpcount)));
  lastOpcount=thisOpcount;
  ballPos=lis.move()+xy(10,10);
  switch ((ballPos.gety()>oldBallPos.gety())*2+(ballPos.getx()>oldBallPos.getx()))
  {
    case 0: // Ball is moving up and left
      swath<<qptrnd(ballPos+xy(-10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,10.5))<<qptrnd(oldBallPos+xy(10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,-10.5))<<qptrnd(ballPos+xy(10.5,-10.5));
      break;
    case 1: // Ball is moving up and right
      swath<<qptrnd(oldBallPos+xy(-10.5,-10.5))<<qptrnd(oldBallPos+xy(-10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,10.5))<<qptrnd(ballPos+xy(10.5,10.5));
      swath<<qptrnd(ballPos+xy(10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,-10.5));
      break;
    case 2: // Ball is moving down and left
      swath<<qptrnd(oldBallPos+xy(10.5,10.5))<<qptrnd(oldBallPos+xy(10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,-10.5));
      swath<<qptrnd(ballPos+xy(-10.5,10.5))<<qptrnd(ballPos+xy(10.5,10.5));
      break;
    case 3: // Ball is moving down and right
      swath<<qptrnd(ballPos+xy(10.5,10.5))<<qptrnd(ballPos+xy(10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,-10.5))<<qptrnd(oldBallPos+xy(-10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,10.5))<<qptrnd(ballPos+xy(-10.5,10.5));
      break;
  }
  update(QRegion(swath));
  for (i=0;i<1;i++)
  {
    if (++triangleNum>=net.triangles.size())
      triangleNum=0;
    if (triangleNum<net.triangles.size() && net.triangles[triangleNum].a)
    {
      gradient=net.triangles[triangleNum].gradient(net.triangles[triangleNum].centroid());
      A=*net.triangles[triangleNum].a;
      B=*net.triangles[triangleNum].b;
      C=*net.triangles[triangleNum].c;
      r=0.5+gradient.north()*0.1294+gradient.east()*0.483;
      g=0.5+gradient.north()*0.3535-gradient.east()*0.3535;
      b=0.5-gradient.north()*0.483 -gradient.east()*0.1294;
      if (r>1)
	r=1;
      if (r<0)
	r=0;
      if (g>1)
	g=1;
      if (g<0)
	g=0;
      if (b>1)
	b=1;
      if (b<0)
	b=0;
      brush.setColor(QColor::fromRgbF(r,g,b));
      painter.setBrush(brush);
      polygon<<worldToWindow(A)<<worldToWindow(B)<<worldToWindow(C);
      painter.drawConvexPolygon(polygon);
    }
  }
}

void TinCanvas::setSize()
{
  windowCenter=xy(width(),height())/2.;
  lis.resize(width()-20,height()-20);
  sizeToFit();
  frameBuffer=QPixmap(width(),height());
}

void TinCanvas::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  QRectF rect(ballPos.getx()-10,ballPos.gety()-10,20,20);
  painter.setBrush(Qt::red);
  painter.setPen(Qt::NoPen);
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.drawPixmap(this->rect(),frameBuffer,this->rect());
  //painter.drawEllipse(QPointF(ballPos.getx(),ballPos.gety()),10,10);
  painter.drawChord(rect,lrint(bintodeg(ballAngle)*16),2880);
  painter.setBrush(Qt::blue);
  painter.drawChord(rect,lrint(bintodeg(ballAngle+DEG180)*16),2880);
}

void TinCanvas::resizeEvent(QResizeEvent *event)
{
  setSize();
  QWidget::resizeEvent(event);
}
