/******************************************************/
/*                                                    */
/* tincanvas.cpp - canvas for drawing TIN             */
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
#include <QPainter>
#include <cmath>
#include "tincanvas.h"
#include "boundrect.h"
#include "octagon.h"
#include "angle.h"
#include "threads.h"
#include "ldecimal.h"

using namespace std;
namespace cr=std::chrono;

QPoint qptrnd(xy pnt)
{
  return QPoint(lrint(pnt.getx()),lrint(pnt.gety()));
}

TinCanvas::TinCanvas(QWidget *parent):QWidget(parent)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
  setMinimumSize(40,30);
  triangleNum=splashScreenTime=0;
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
  int i,sz;
  int thisOpcount=opcount;
  int tstatus=getThreadStatus();
  double r,g,b;
  triangle *tri=nullptr;
  xy gradient,A,B,C;
  xy dartCorners[4];
  xy leftTickmark,rightTickmark,tickmark;
  QPolygonF polygon;
  QPolygon swath;
  QPointF circleCenter;
  QRectF circleBox,textBox;
  string scaleText;
  cr::nanoseconds elapsed=cr::milliseconds(0);
  cr::time_point<cr::steady_clock> timeStart=clk.now();
  xy oldBallPos=ballPos;
  QPainter painter(&frameBuffer);
  QBrush brush;
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.setPen(Qt::NoPen);
  brush.setStyle(Qt::SolidPattern);
  if ((tstatus&0x3ffbfeff)%1048577==0)
    state=(tstatus&0x3ffbfeff)/1048577;
  else
  {
    state=0;
    //cout<<"tstatus "<<((tstatus>>20)&1023)<<':'<<((tstatus>>10)&1023)<<':'<<(tstatus&1023)<<endl;
  }
  if ((state==TH_WAIT || state==TH_PAUSE) && currentAction)
    state=-currentAction;
  if (splashScreenTime)
  {
    if (0==--splashScreenTime)
    {
      net.clear();
      cout<<"Splash screen finished\n";
      splashScreenFinished();
    }
  }
  // Compute the new position of the ball, and update a swath containing the ball's motion.
  ballAngle+=lrint(8388608*sqrt((unsigned)(thisOpcount-lastOpcount)));
  lastOpcount=thisOpcount;
  penPos=(penPos+5)%144;
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
  // Paint a dart (concave quadrilateral) outside the TIN white.
  dartAngle+=PHITURN;
  dartCorners[0]=hypot(width(),height())*0.51/scale*cossin(dartAngle)+worldCenter;
  if ((sz=net.convexHull.size()) && !net.convexHull[0]->isnan())
  {
    i=net.closestHullPoint(dartCorners[0]);
    dartCorners[1]=*net.convexHull[(i+sz-1)%sz];
    dartCorners[2]=*net.convexHull[i];
    dartCorners[3]=*net.convexHull[(i+1)%sz];
  }
  else
  {
    dartCorners[1]=hypot(width(),height())*0.1/scale*cossin(dartAngle+DEG30);
    dartCorners[2]=xy(0,0);
    dartCorners[3]=hypot(width(),height())*0.1/scale*cossin(dartAngle-DEG30);
  }
  for (i=0;i<4;i++)
    polygon<<worldToWindow(dartCorners[i]);
  painter.setBrush(Qt::white);
  painter.drawPolygon(polygon);
  // Paint a circle white as a background for the scale.
  if (maxScaleSize>0)
  {
    circleCenter=worldToWindow(scaleEnd);
    circleBox=QRectF(circleCenter.x()-maxScaleSize*scale,circleCenter.y()-maxScaleSize*scale,
		    maxScaleSize*scale*2,maxScaleSize*scale*2);
    painter.drawEllipse(circleBox);
    // Draw the scale.
    painter.setPen(Qt::black);
    tickmark=turn90(rightScaleEnd-leftScaleEnd)/10;
    leftTickmark=leftScaleEnd+tickmark;
    rightTickmark=rightScaleEnd+tickmark;
    polygon=QPolygonF();
    polygon<<worldToWindow(leftTickmark);
    polygon<<worldToWindow(leftScaleEnd);
    polygon<<worldToWindow(rightScaleEnd);
    polygon<<worldToWindow(rightTickmark);
    painter.drawPolyline(polygon);
    // Write the length of the scale.
    textBox=QRectF(worldToWindow(leftScaleEnd+2*tickmark),worldToWindow(rightScaleEnd));
    scaleText=ldecimal(scaleSize/lengthUnit,scaleSize/lengthUnit/10)+((lengthUnit==1)?" m":" ft");
    painter.drawText(textBox,Qt::AlignCenter,QString::fromStdString(scaleText));
  }
  painter.setPen(Qt::NoPen);
  // Paint some triangles in the TIN in colors depending on their gradient.
  for (;trianglesToPaint && elapsed<cr::milliseconds(20);trianglesToPaint--)
  {
    net.wingEdge.lock_shared();
    if (++triangleNum>=net.triangles.size())
      triangleNum=0;
    if (triangleNum<net.triangles.size())
      tri=&net.triangles[triangleNum];
    else
      tri=nullptr;
    net.wingEdge.unlock_shared();
    if (tri && tri->a)
    {
      net.wingEdge.lock_shared();
      gradient=tri->gradient(tri->centroid());
      A=*tri->a;
      B=*tri->b;
      C=*tri->c;
      net.wingEdge.unlock_shared();
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
      polygon=QPolygon();
      polygon<<worldToWindow(A)<<worldToWindow(B)<<worldToWindow(C);
      painter.drawConvexPolygon(polygon);
    }
    elapsed=clk.now()-timeStart;
  }
  if (tri && !trianglesToPaint)
    update();
  //if (elapsed>cr::milliseconds(50))
    //cout<<"tick got stuck\n";
}

void TinCanvas::startSplashScreen()
{
  int i,j;
  if (splashScreenTime==0 && net.points.size()==0)
  {
    splashScreenTime=SPLASH_TIME;
    cout<<"Starting splash screen\n";
    net.wingEdge.lock();
    for (i=1;i>-2;i--)
      for (j=abs(i)-3;j<4-abs(i);j+=2)
	net.addpoint((j-7*i+11)/2,point(j,i*M_SQRT_3,0));
    for (i=1,j=0;i<10;i++)
      if ((i*i+1)%10)
      {
	net.edges[j].a=&net.points[i];
	net.edges[j].b=&net.points[i+1];
	net.points[i].insertEdge(&net.edges[j]);
	net.points[i+1].insertEdge(&net.edges[j]);
	j++;
      }
    for (i=1;i<4;i++)
    {
      net.edges[j].a=&net.points[i];
      net.edges[j].b=&net.points[i+3];
      net.points[i].insertEdge(&net.edges[j]);
      net.points[i+3].insertEdge(&net.edges[j]);
      j++;
      net.edges[j].a=&net.points[i];
      net.edges[j].b=&net.points[i+4];
      net.points[i].insertEdge(&net.edges[j]);
      net.points[i+4].insertEdge(&net.edges[j]);
      j++;
      net.edges[j].a=&net.points[i+7];
      net.edges[j].b=&net.points[i+3];
      net.points[i+7].insertEdge(&net.edges[j]);
      net.points[i+3].insertEdge(&net.edges[j]);
      j++;
      net.edges[j].a=&net.points[i+7];
      net.edges[j].b=&net.points[i+4];
      net.points[i+7].insertEdge(&net.edges[j]);
      net.points[i+4].insertEdge(&net.edges[j]);
      j++;
    }
    net.wingEdge.unlock();
    splashScreenStarted();
    sizeToFit();
  }
}

void TinCanvas::setSize()
{
  double oldScale=scale;
  xy oldWindowCenter=windowCenter;
  int dx,dy;
  QTransform tf;
  windowCenter=xy(width(),height())/2.;
  lis.resize(width()-20,height()-20);
  sizeToFit();
  if (frameBuffer.isNull())
  {
    frameBuffer=QPixmap(width(),height());
    frameBuffer.fill(Qt::white);
  }
  else
  {
    tf.translate(-oldWindowCenter.getx(),-oldWindowCenter.gety());
    tf.scale(scale/oldScale,scale/oldScale);
    tf.translate(windowCenter.getx(),windowCenter.gety());
    QPixmap frameTemp=frameBuffer.transformed(tf);
    frameBuffer=QPixmap(width(),height());
    frameBuffer.fill(Qt::white);
    QPainter painter(&frameBuffer);
    dx=(width()-frameTemp.width())/2;
    dy=(height()-frameTemp.height())/2;
    QRect rect0(0,0,frameTemp.width(),frameTemp.height());
    QRect rect1(dx,dy,frameTemp.width(),frameTemp.height());
    painter.drawPixmap(rect1,frameTemp,rect0);
    update();
  }
  setScalePos();
  trianglesToPaint=3*net.triangles.size();
  /* It takes three coats of paint to color the framebuffer well
   * after it has been cleared to white.
   */
}

void TinCanvas::setScalePos()
// Set the position of the scale at the lower left or right corner.
{
  xy leftScalePos,rightScalePos,disp;
  bool right=false;
  leftScalePos=windowToWorld(QPointF(width()*0.01,height()*0.99));
  rightScalePos=windowToWorld(QPointF(width()*0.99,height()*0.99));
  maxScaleSize=net.distanceToHull(leftScalePos);
  if (net.distanceToHull(rightScalePos)>maxScaleSize)
  {
    maxScaleSize=net.distanceToHull(rightScalePos);
    right=true;
  }
  scaleSize=lengthUnit;
  while (scaleSize>maxScaleSize)
    scaleSize/=10;
  while (scaleSize<maxScaleSize)
    scaleSize*=10;
  if (scaleSize/2<maxScaleSize)
    scaleSize/=2;
  else if (scaleSize/5<maxScaleSize)
    scaleSize/=5;
  else
    scaleSize/=10;
  if (right)
  {
    scaleEnd=rightScaleEnd=rightScalePos;
    disp=leftScalePos-rightScalePos;
    disp/=disp.length();
    leftScaleEnd=rightScaleEnd+scaleSize*disp;
  }
  else
  {
    scaleEnd=leftScaleEnd=leftScalePos;
    disp=rightScalePos-leftScalePos;
    disp/=disp.length();
    rightScaleEnd=leftScaleEnd+scaleSize*disp;
  }
  if (trianglesToPaint==0)
    trianglesToPaint++; // repaint if not busy painting triangles
  //cout<<(right?"right ":"left ")<<maxScaleSize<<" corner to convex hull "<<scaleSize<<" scale\n";
}

void TinCanvas::setLengthUnit(double unit)
{
  lengthUnit=unit;
  setScalePos();
}

void TinCanvas::paintEvent(QPaintEvent *event)
{
  int i;
  QPainter painter(this);
  QRectF square(ballPos.getx()-10,ballPos.gety()-10,20,20);
  QRectF sclera(ballPos.getx()-10,ballPos.gety()-5,20,10);
  QRectF iris(ballPos.getx()-5,ballPos.gety()-5,10,10);
  QRectF pupil(ballPos.getx()-2,ballPos.gety()-2,4,4);
  QRectF paper(ballPos.getx()-7.07,ballPos.gety()-10,14.14,20);
  QPolygonF octagon;
  double x0,x1,y;
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()+4.14);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()+4.14);
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.drawPixmap(this->rect(),frameBuffer,this->rect());
  switch (state)
  {
    case TH_RUN:
      painter.setBrush(Qt::yellow);
      painter.setPen(Qt::NoPen);
      painter.drawChord(square,lrint(bintodeg(ballAngle)*16),2880);
      painter.setBrush(Qt::blue);
      painter.drawChord(square,lrint(bintodeg(ballAngle+DEG180)*16),2880);
      break;
    case -ACT_WRITE_DXF:
    case -ACT_WRITE_TIN:
    case -ACT_WRITE_CARLSON_TIN:
    case -ACT_WRITE_LANDXML:
    case -ACT_WRITE_PTIN:
      painter.setBrush(Qt::white);
      painter.setPen(Qt::NoPen);
      painter.drawRect(paper);
      painter.setPen(Qt::black);
      for (i=0;i<8;i++)
      {
	y=ballPos.gety()-9+i*18./7;
	x0=ballPos.getx()-6;
	if (penPos/18==i)
	  x1=x0+(i%18+0.5)*2/3;
	else
	  x1=x0+12*(i<penPos/18);
	if (x1>x0)
	  painter.drawLine(QPointF(x0,y),QPointF(x1,y));
      }
      break;
    case -ACT_OCTAGON:
      painter.setBrush(Qt::darkGreen);
      painter.setPen(Qt::NoPen);
      painter.drawPolygon(octagon);
      break;
    case -ACT_LOAD:
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::lightGray);
      painter.drawEllipse(sclera);
      painter.setBrush(Qt::blue);
      painter.drawEllipse(iris);
      painter.setBrush(Qt::black);
      painter.drawEllipse(pupil);
      break;
    case 0:
      painter.setBrush(Qt::lightGray);
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(QPointF(ballPos.getx(),ballPos.gety()),10,10);
      break;
  }
}

void TinCanvas::resizeEvent(QResizeEvent *event)
{
  setSize();
  QWidget::resizeEvent(event);
}
