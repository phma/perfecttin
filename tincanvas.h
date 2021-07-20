/******************************************************/
/*                                                    */
/* tincanvas.h - canvas for drawing TIN               */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#ifndef TINCANVAS_H
#define TINCANVAS_H
#include <QWidget>
#include <QProgressDialog>
#include <QTimer>
#include <QTime>
#include "lissajous.h"
#include "point.h"
#include "cidialog.h"

// goals
#define DONE 0
#define MAKE_TIN 1
#define ROUGH_CONTOURS 2
#define PRUNE_CONTOURS 3
#define SMOOTH_CONTOURS 4

#define SPLASH_TIME 60

class TinCanvas: public QWidget
{
  Q_OBJECT
public:
  TinCanvas(QWidget *parent=0);
  QPointF worldToWindow(xy pnt);
  xy windowToWorld(QPointF pnt);
  ContourInterval contourInterval;
  int state;
  int totalContourSegments;
  void repaintSeldom();
signals:
  void splashScreenStarted();
  void splashScreenFinished();
  void contourDrawingFinished();
public slots:
  void sizeToFit();
  void setSize();
  void setLengthUnit(double unit);
  void repaintAllTriangles();
  void setScalePos();
  void tick(); // 50 ms
  void startSplashScreen();
  void selectContourInterval();
  void clearContourFlags();
  void roughContours();
  void rough1Contour();
  void roughContoursFinish();
  void pruneContours();
  void prune1Contour();
  void pruneContoursFinish();
  void smoothContours();
  void smooth1Contour();
  void smoothContoursFinish();
  void contoursCancel();
protected:
  void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
  QPixmap frameBuffer;
  Lissajous lis;
  QPen contourPen[3][20];
  unsigned short contourColor[20];
  short contourLineType[3];
  unsigned short contourThickness[3];
  QTimer *timer;
  ContourIntervalDialog *ciDialog;
  double conterval,tolerance;
  xy windowCenter,worldCenter;
  double scale;
  double lengthUnit;
  double maxScaleSize,scaleSize;
  xy ballPos;
  xy leftScaleEnd,rightScaleEnd,scaleEnd;
  int goal;
  int progInx; // used in progress bar loops
  int elevHi,elevLo; // in contour interval unit
  std::array<double,2> tinlohi;
  bool roughContoursValid; // If false, to do smooth contours, must first do rough contours.
  bool pruneContoursValid;
  bool smoothContoursValid;
  int penPos;
  int triangleNum;
  int lastOpcount;
  int ballAngle,dartAngle;
  int lastntri;
  int splashScreenTime; // in ticks
  QTime lastPaintTime;
  int lastPaintDuration;
};
#endif
