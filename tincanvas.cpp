/******************************************************/
/*                                                    */
/* tincanvas.cpp - canvas for drawing TIN             */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021,2025 Pierre Abbat.
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
#include <QPainterPath>
#include <QGuiApplication>
#include <cmath>
#include "tincanvas.h"
#include "mainwindow.h"
#include "boundrect.h"
#include "octagon.h"
#include "relprime.h"
#include "angle.h"
#include "color.h"
#include "triop.h"
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
  ciDialog=new ContourIntervalDialog(this);
  timer=new QTimer(this);
  goal=DONE;
  triangleNum=splashScreenTime=dartAngle=ballAngle=0;
  state=TH_WAIT;
  totalContourPieces=0;
  memset(contourColor,0,sizeof(contourColor));
  memset(contourLineType,0,sizeof(contourLineType));
  memset(contourThickness,0,sizeof(contourThickness));
  conterval=tolerance=0;
  scale=1;
  lengthUnit=1;
  maxScaleSize=scaleSize=0;
  progInx=0;
  elevHi=elevLo=0;
  roughContoursValid=pruneContoursValid=smoothContoursValid=false;
  penPos=0;
  lastOpcount=opcount=0;
  lastntri=0;
}

void TinCanvas::repaintSeldom()
// Spends up to 5% of the time repainting during long operations.
{
  if (lastPaintTime.elapsed()>20*lastPaintDuration)
    repaint();
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
  if (splashScreenTime)
    scale=scale*2/3;
}

void TinCanvas::tick()
/* This method, which is called at 20 Hz, updates the display and moves
 * the ball, octagon, closed curve, or invisible thing (depending on what
 * the program is doing) around. Occasionally it freezes. I stopped the
 * program during a freeze and found that the main thread was in lowlevellock.c
 * in the destructor of a vector in tick, which had 42 elements, while the only
 * active worker thread was appending a triangle pointer to a vector of 8
 * pointers. This is not enough to jank it for a few seconds, so most likely
 * Qt was doing something that locked it.
 */
{
  int i,sz,timeLimit;
  int thisOpcount=opcount;
  int tstatus=getThreadStatus();
  int trianglesPainted=0,piecesDrawn=0,piecesToDraw=0;
  bool fromTrianglePaint;
  double splashElev;
  uintptr_t pieceInx;
  vector<ContourPiece> pieces;
  vector<int> crossingPieces;
  vector<triangle *> triPtr;
  bezier3d b3d;
  Color color;
  triangle *tri=(triangle *)5;
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
  QPen pen;
  QPainterPath path;
  vector<xyz> beziseg;
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
    for (i=0;i<10;i++)
    {
      splashElev=sin(splashScreenTime*quadirr[i]/2)*splashScreenTime/SPLASH_TIME;
      net.points[i+1].raise(splashElev-net.points[i+1].elev());
    }
    for (i=0;i<10;i++)
      net.triangles[i].flatten();
    trianglesToPaint+=27;
    if (0==--splashScreenTime)
    {
      net.clear();
      setIdle(BUSY_SPL|BUSY_TIN);
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
  if (state==TH_WAIT || state==TH_PAUSE)
    timeLimit=45;
  else
    timeLimit=20;
  // Draw contour pieces on previously painted triangles.
  while (elapsed<cr::milliseconds(timeLimit))
  {
    pieceInx=(uintptr_t)net.pieceDraw.dequeue();
    if (pieceInx==0)
      break;
    pieces=net.getContourPieces((int)pieceInx);
    for (i=0;i<pieces.size();i++)
    {
      b3d=pieces[i].s.approx3d(1/scale);
      path=QPainterPath();
      color=colorize(contourInterval,pieces[i].s);
      pen.setColor(QColor::fromRgbF(color.fr(),color.fg(),color.fb()));
      painter.setPen(pen);
      beziseg=b3d[0];
      path.moveTo(worldToWindow(beziseg[0]));
      path.cubicTo(worldToWindow(beziseg[1]),worldToWindow(beziseg[2]),worldToWindow(beziseg[3]));
      painter.drawPath(path);
      piecesDrawn++;
    }
  }
  // Paint some triangles in the TIN in colors depending on their gradient or elevation.
  painter.setPen(Qt::NoPen);
  while (tri && elapsed<cr::milliseconds(timeLimit))
  {
    tri=net.trianglePaint.dequeue();
    fromTrianglePaint=tri!=nullptr;
    if (!tri && trianglesToPaint)
    {
      net.wingEdge.lock_shared();
      if (++triangleNum>=net.triangles.size())
	triangleNum=0;
      if (triangleNum<net.triangles.size())
	tri=&net.triangles[triangleNum];
      trianglesToPaint--;
      net.wingEdge.unlock_shared();
    }
    if (tri && tri->a)
    {
      net.wingEdge.lock_shared();
      if (net.triangles.size())
      {
	gradient=tri->gradient(tri->centroid());
	A=*tri->a;
	B=*tri->b;
	C=*tri->c;
      }
      else // tri came from trianglePaint and net was just cleared by thread reading ptin file
	tri=nullptr;
      net.wingEdge.unlock_shared();
      triPtr.clear();
      triPtr.push_back(tri);
      if (net.currentContours && net.currentContours->size())
      {
	while (!lockTriangles(numThreads(),triPtr));
	crossingPieces=tri->crossingPieces;
	unlockTriangles(numThreads());
      }
      ++trianglesPainted;
      if (tri)
	color=colorize(tri);
      if (splashScreenTime)
	color.mix(white,1-(double)splashScreenTime/SPLASH_TIME);
      brush.setColor(QColor::fromRgbF(color.fr(),color.fg(),color.fb()));
      painter.setBrush(brush);
      polygon=QPolygon();
      polygon<<worldToWindow(A)<<worldToWindow(B)<<worldToWindow(C);
      painter.drawConvexPolygon(polygon);
      for (i=0;i<crossingPieces.size();i++)
      {
	net.pieceDraw.enqueue((void *)(crossingPieces[i]+4294967296),0);
	piecesToDraw++;
      }
    }
    elapsed=clk.now()-timeStart;
  }
  if (splashScreenTime>0 && splashScreenTime<=SPLASH_TIME/2)
  {
    pen.setColor(Qt::black);
    pen.setWidthF(scale/20);
    pen.setCapStyle(Qt::FlatCap);
    painter.setPen(pen);
    for (i=0;i<net.edges.size();i++)
      painter.drawLine(worldToWindow(*net.edges[i].a),worldToWindow(*net.edges[i].b));
    painter.setPen(Qt::NoPen);
    brush.setColor(Qt::blue);
    painter.setBrush(brush);
    for (i=1;i<=10;i++)
    {
      painter.drawEllipse(worldToWindow(net.points[i]),scale/8,scale/8);
    }
  }
  if (lastntri && !net.triangles.size())
    frameBuffer.fill();
  if ((trianglesPainted || piecesDrawn) && !trianglesToPaint && !piecesToDraw)
    update();
  //if (elapsed>cr::milliseconds(50))
    //cout<<"tick got stuck\n";
  lastntri=net.triangles.size();
  net.nipPieces();
}

void TinCanvas::startSplashScreen()
{
  int i,j;
  if (splashScreenTime==0 && net.points.size()==0)
  {
    splashScreenTime=SPLASH_TIME;
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
    net.maketriangles();
    net.wingEdge.unlock();
    colorize.setLimits(-0.5,0.5);
    setBusy(BUSY_SPL);
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
  repaintAllTriangles();
}

void TinCanvas::repaintAllTriangles()
{
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

void TinCanvas::selectContourInterval()
{
  ciDialog->set(&contourInterval,lengthUnit);
  if (ciDialog->exec())
  {
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    /* setCurrentContours deletes all the pieces of the old contours and
     * inserts all the pieces of the new contours. This takes a noticeable
     * time, after which all triangles have to be repainted.
     */
    net.setCurrentContours(contourInterval);
    setContourFlags();
    QGuiApplication::restoreOverrideCursor();
    repaintAllTriangles();
    contourSetsChanged();
    contourIntervalChanged(contourInterval);
  }
}

void TinCanvas::setContourInterval(ContourInterval ci)
{
  int i,which=-1;
  vector<ContourInterval> ciList=net.contourIntervals();
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  for (i=0;i<ciList.size();i++)
    if (ciList[i]==ci)
    {
      which=i;
      if (!net.currentContours || ci!=contourInterval)
	net.setCurrentContours(contourInterval=ci);
    }
  if (which<0)
    net.unsetCurrentContours();
  setContourFlags();
  QGuiApplication::restoreOverrideCursor();
  repaintAllTriangles();
  contourIntervalChanged(contourInterval);
}

void TinCanvas::clearContourFlags()
{
  roughContoursValid=pruneContoursValid=smoothContoursValid=false;
}

void TinCanvas::setContourFlags()
{
  int contourState,i,j,n;
  int histo[3];
  totalContourPieces=makeContourIndex();
  n=lrint(sqrt(totalContourPieces));
  for (i=0;i<3;i++)
    histo[i]=0;
  for (i=j=0;i<n;i++)
  {
    contourState=net.isSmoothed(nthPiece(j));
    j=(j+relprime(n))%n;
    if (contourState>=0)
      histo[contourState]++;
  }
  cout<<histo[0]<<' '<<histo[1]<<' '<<histo[2]<<endl;
  roughContoursValid=(histo[0]|histo[1]|histo[2])>0;
  pruneContoursValid=(histo[1]|histo[2])>0;
  smoothContoursValid=histo[2]>0;
}

void TinCanvas::roughContours()
{
  int i;
  ContourTask ctr;
  conterval=contourInterval.fineInterval();
  tolerance=contourInterval.tolerance();
  net.setCurrentContours(contourInterval);
  contourSetsChanged(); // setCurrentContours could make an empty contour set
  setBusy(BUSY_CTR);
  if (goal==DONE)
  {
    goal=ROUGH_CONTOURS;
    timer->start(0);
  }
  if (net.triangles.size())
    tinlohi=net.lohi();
  (*net.currentContours).clear();
  elevLo=floor(tinlohi[0]/conterval);
  elevHi=ceil(tinlohi[1]/conterval);
  progInx=elevLo;
  disconnect(timer,SIGNAL(timeout()),0,0);
  setThreadCommand(TH_ROUGH);
  totalContourPieces=elevHi-elevLo+1;
  for (i=elevLo;i<=elevHi;i++)
    {
      ctr.num=i;
      ctr.size=1;
      ctr.elevation=i*conterval;
      enqueueRough(ctr);
    }
  waitForThreads(TH_ROUGH);
  connect(timer,SIGNAL(timeout()),this,SLOT(rough1Contour()));
}

void TinCanvas::rough1Contour()
{
  if (contourSegmentsDone<totalContourPieces)
  {
    ++progInx;
  }
  else
  {
    disconnect(timer,SIGNAL(timeout()),this,SLOT(rough1Contour()));
    connect(timer,SIGNAL(timeout()),this,SLOT(roughContoursFinish()));
  }
  repaintSeldom();
}

void TinCanvas::roughContoursFinish()
{
  BoundRect br;
  int i;
  disconnect(timer,SIGNAL(timeout()),this,SLOT(roughContoursFinish()));
  switch (goal)
  {
    case ROUGH_CONTOURS:
      goal=DONE;
      timer->stop();
      setIdle(BUSY_CTR);
      contourDrawingFinished();
      setThreadCommand(TH_WAIT);
      break;
    case PRUNE_CONTOURS:
    case SMOOTH_CONTOURS:
      connect(timer,SIGNAL(timeout()),this,SLOT(pruneContours()));
      break;
  }
  roughContoursValid=true;
  pruneContoursValid=false;
  smoothContoursValid=false;
  repaintAllTriangles();
}

void TinCanvas::pruneContours()
{
  int i,sizeRange,lastSizeRange;
  ContourTask ctr;
  setBusy(BUSY_CTR);
  if (goal==DONE)
  {
    goal=PRUNE_CONTOURS;
    timer->start(0);
  }
  progInx=0;
  contourSegmentsDone=totalContourPieces=0;
  ctr.tolerance=tolerance;
  disconnect(timer,SIGNAL(timeout()),0,0);
  if (roughContoursValid && conterval==contourInterval.fineInterval())
  {
    for (i=sizeRange=0;i<net.currentContours->size();i++)
    {
      if ((*net.currentContours)[i].size()>sizeRange)
	sizeRange=(*net.currentContours)[i].size();
      totalContourPieces+=(*net.currentContours)[i].size();
    }
    sizeRange+=2;
    lastSizeRange=sizeRange;
    while (sizeRange)
    {
      for (i=0;i<(*net.currentContours).size();i++)
	if ((*net.currentContours)[i].size()+1>=sizeRange &&
            (*net.currentContours)[i].size()+1<lastSizeRange)
	{
	  ctr.num=i;
	  ctr.size=(*net.currentContours)[i].size();
	  enqueuePrune(ctr);
	}
      lastSizeRange=sizeRange;
      sizeRange=relprime(sizeRange);
      if (sizeRange==1) // relprime(1)==1
	sizeRange=0;
    }
    waitForThreads(TH_PRUNE);
    connect(timer,SIGNAL(timeout()),this,SLOT(prune1Contour()));
  }
  else
    connect(timer,SIGNAL(timeout()),this,SLOT(roughContours()));
}

void TinCanvas::prune1Contour()
{
  if (contourSegmentsDone<totalContourPieces)
  {
    ++progInx;
  }
  else
  {
    disconnect(timer,SIGNAL(timeout()),0,0);
    connect(timer,SIGNAL(timeout()),this,SLOT(pruneContoursFinish()));
  }
  repaintSeldom();
}

void TinCanvas::pruneContoursFinish()
{
  BoundRect br;
  int i;
  disconnect(timer,SIGNAL(timeout()),this,SLOT(pruneContoursFinish()));
  switch (goal)
  {
    case PRUNE_CONTOURS:
      goal=DONE;
      timer->stop();
      setIdle(BUSY_CTR);
      contourDrawingFinished();
      setThreadCommand(TH_WAIT);
      break;
    case SMOOTH_CONTOURS:
      connect(timer,SIGNAL(timeout()),this,SLOT(smoothContours()));
      break;
  }
  net.eraseEmptyContours();
  pruneContoursValid=true;
  repaintAllTriangles();
}

void TinCanvas::smoothContours()
{
  int i,sizeRange,lastSizeRange;
  ContourTask ctr;
  setBusy(BUSY_CTR);
  if (goal==DONE)
  {
    goal=SMOOTH_CONTOURS;
    timer->start(50);
  }
  progInx=0;
  contourSegmentsDone=totalContourPieces=0;
  ctr.tolerance=tolerance;
  disconnect(timer,SIGNAL(timeout()),0,0);
  if (pruneContoursValid && conterval==contourInterval.fineInterval())
  {
    for (i=sizeRange=0;i<net.currentContours->size();i++)
    {
      if ((*net.currentContours)[i].size()>sizeRange)
	sizeRange=(*net.currentContours)[i].size();
      totalContourPieces+=(*net.currentContours)[i].size();
    }
    sizeRange+=2;
    lastSizeRange=sizeRange;
    while (sizeRange)
    {
      for (i=0;i<(*net.currentContours).size();i++)
	if ((*net.currentContours)[i].size()+1>=sizeRange &&
            (*net.currentContours)[i].size()+1<lastSizeRange)
	{
	  ctr.num=i;
	  ctr.size=(*net.currentContours)[i].size();
	  enqueueSmooth(ctr);
	}
      lastSizeRange=sizeRange;
      sizeRange=relprime(sizeRange);
      if (sizeRange==1) // relprime(1)==1
	sizeRange=0;
    }
    waitForThreads(TH_SMOOTH);
    connect(timer,SIGNAL(timeout()),this,SLOT(smooth1Contour()));
  }
  else
    connect(timer,SIGNAL(timeout()),this,SLOT(pruneContours()));
}

void TinCanvas::smooth1Contour()
{
  if (contourSegmentsDone<totalContourPieces)
  {
    //smooth1contour(net,tolerance,progInx);
    ++progInx;
  }
  else
  {
    disconnect(timer,SIGNAL(timeout()),0,0);
    connect(timer,SIGNAL(timeout()),this,SLOT(smoothContoursFinish()));
  }
  repaintSeldom();
}

void TinCanvas::smoothContoursFinish()
{
  BoundRect br;
  int i;
  switch (goal)
  {
    case SMOOTH_CONTOURS:
      goal=DONE;
      timer->stop();
      setIdle(BUSY_CTR);
      contourDrawingFinished();
      break;
  }
  disconnect(timer,SIGNAL(timeout()),0,0);
  smoothContoursValid=true;
  repaintAllTriangles();
  setThreadCommand(TH_WAIT);
}

void TinCanvas::contoursCancel()
{
  goal=DONE;
  timer->stop();
  setIdle(BUSY_CTR);
  disconnect(timer,SIGNAL(timeout()),0,0);
  update();
}

void TinCanvas::deleteContours()
{
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  net.deleteCurrentContours();
  QGuiApplication::restoreOverrideCursor();
  repaintAllTriangles();
  contourSetsChanged();
  clearContourFlags();
}

void TinCanvas::paintEvent(QPaintEvent *event)
{
  int i;
  QPainter painter(this);
  QElapsedTimer paintTime,subTime;
  QRectF square(ballPos.getx()-10,ballPos.gety()-10,20,20);
  QRectF qind(ballPos.getx()-8,ballPos.gety()-8,16,16);
  QRectF sclera(ballPos.getx()-10,ballPos.gety()-5,20,10);
  QRectF iris(ballPos.getx()-5,ballPos.gety()-5,10,10);
  QRectF pupil(ballPos.getx()-2,ballPos.gety()-2,4,4);
  QRectF paper(ballPos.getx()-7.07,ballPos.gety()-10,14.14,20);
  QPolygonF octagon;
  QPainterPath roughCurve,pruneCurve,smoothCurve,trigon;
  double x0,x1,y;
  paintTime.start();
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()+4.14);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()+4.14);
  roughCurve.moveTo(-9.515,5.942);
  roughCurve.lineTo(-6.979,7.479);
  roughCurve.lineTo(-4.328,6.019);
  roughCurve.lineTo(-1.638,6.288);
  roughCurve.lineTo(0.129,8.862);
  roughCurve.lineTo(2.934,6.364);
  roughCurve.lineTo(6.315,7.287);
  roughCurve.lineTo(9.274,4.674);
  roughCurve.lineTo(7.122,2.637);
  roughCurve.lineTo(8.967,-0.398);
  roughCurve.lineTo(8.582,-4.087);
  roughCurve.lineTo(5.701,-4.317);
  roughCurve.lineTo(6.046,-7.199);
  roughCurve.lineTo(3.703,-8.313);
  roughCurve.lineTo(0.206,-7.814);
  roughCurve.lineTo(-0.447,-9.735);
  roughCurve.lineTo(-4.366,-9.082);
  roughCurve.lineTo(-2.945,-7.007);
  roughCurve.lineTo(-7.248,-5.47);
  roughCurve.lineTo(-8.247,-2.78);
  roughCurve.lineTo(-6.902,-0.398);
  roughCurve.lineTo(-9.246,0.832);
  roughCurve.lineTo(-7.94,2.868);
  roughCurve.closeSubpath();
  roughCurve.translate(ballPos.getx(),ballPos.gety());
  pruneCurve.moveTo(-9.515,5.942);
  pruneCurve.lineTo(6.315,7.287);
  pruneCurve.lineTo(8.967,-0.398);
  pruneCurve.lineTo(3.703,-8.313);
  pruneCurve.lineTo(-4.366,-9.082);
  pruneCurve.closeSubpath();
  pruneCurve.translate(ballPos.getx(),ballPos.gety());
  smoothCurve.moveTo(-6.642,6.426);
  smoothCurve.cubicTo(-2.808,8.64,2.808,8.64,6.642,6.426);
  smoothCurve.cubicTo(7.92,5.688,8.856,4.068,8.856,2.592);
  smoothCurve.cubicTo(8.856,-1.836,6.048,-6.696,2.214,-8.91);
  smoothCurve.cubicTo(0.936,-9.648,-0.936,-9.648,-2.214,-8.91);
  smoothCurve.cubicTo(-6.048,-6.696,-8.856,-1.836,-8.856,2.592);
  smoothCurve.cubicTo(-8.856,3.068,-7.92,5.688,-6.642,6.426);
  smoothCurve.translate(ballPos.getx(),ballPos.gety());
  trigon.moveTo(9.659,2.588);
  trigon.lineTo(-7.071,7.071);
  trigon.lineTo(-2.588,-9.659);
  trigon.translate(ballPos.getx(),ballPos.gety());
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
    case TH_ROUGH:
      painter.setPen(Qt::red);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(roughCurve);
      break;
    case TH_PRUNE:
      painter.setPen(Qt::red);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(pruneCurve);
      break;
    case TH_SMOOTH:
      painter.setPen(Qt::red);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(smoothCurve);
      break;
    case -ACT_WRITE_DXF:
    case -ACT_WRITE_TIN:
    case -ACT_WRITE_PLY:
    case -ACT_WRITE_STL:
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
    case -ACT_READ_PTIN:
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::lightGray);
      painter.drawEllipse(sclera);
      painter.setBrush(Qt::blue);
      painter.drawEllipse(iris);
      painter.setBrush(Qt::black);
      painter.drawEllipse(pupil);
      break;
    case -ACT_QINDEX:
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::blue);
      painter.drawPath(trigon);
      painter.setPen(Qt::black);
      painter.setBrush(Qt::NoBrush);
      painter.drawRect(qind);
      painter.setPen(Qt::white);
      painter.drawPoint(QPointF(ballPos.getx(),ballPos.gety()));
      break;
    case 0:
      painter.setBrush(Qt::lightGray);
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(QPointF(ballPos.getx(),ballPos.gety()),10,10);
      break;
  }
  lastPaintTime=paintTime;
  lastPaintDuration=paintTime.elapsed();
}

void TinCanvas::resizeEvent(QResizeEvent *event)
{
  setSize();
  QWidget::resizeEvent(event);
}
