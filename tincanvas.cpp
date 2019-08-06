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
#include "tincanvas.h"
#include "boundrect.h"
#include "octagon.h"
#include "ldecimal.h"

using namespace std;

TinCanvas::TinCanvas(QWidget *parent):QWidget(parent)
{
  int i,j,rgb;
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
  setBrush(Qt::red);
  setPen(Qt::NoPen);
  setMinimumSize(40,30);
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
  QRectF before(ballPos.getx()-10.5,ballPos.gety()-10.5,21,21);
  ballPos=lis.move()+xy(10,10);
  QRectF after(ballPos.getx()-10.5,ballPos.gety()-10.5,21,21);
  update(before.toAlignedRect());
  update(after.toAlignedRect());
  //cout<<ldecimal(ballPos.getx(),0.1)<<','<<ldecimal(ballPos.gety(),0.1)<<endl;
}

void TinCanvas::setSize()
{
  windowCenter=xy(width(),height())/2.;
  lis.resize(width()-20,height()-20);
}

void TinCanvas::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  painter.setBrush(brush);
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.drawEllipse(QPointF(ballPos.getx(),ballPos.gety()),10,10);
}

void TinCanvas::resizeEvent(QResizeEvent *event)
{
  setSize();
  QWidget::resizeEvent(event);
}