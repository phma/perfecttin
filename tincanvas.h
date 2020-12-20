/******************************************************/
/*                                                    */
/* tincanvas.h - canvas for drawing TIN               */
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
#ifndef TINCANVAS_H
#define TINCANVAS_H
#include <QWidget>
#include "lissajous.h"
#include "point.h"
#include "cidialog.h"

#define SPLASH_TIME 60

class TinCanvas: public QWidget
{
  Q_OBJECT
public:
  TinCanvas(QWidget *parent=0);
  QPointF worldToWindow(xy pnt);
  xy windowToWorld(QPointF pnt);
  int state;
signals:
  void splashScreenStarted();
  void splashScreenFinished();
public slots:
  void sizeToFit();
  void setSize();
  void setLengthUnit(double unit);
  void setScalePos();
  void tick(); // 50 ms
  void startSplashScreen();
  void selectContourInterval();
protected:
  void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
  QPixmap frameBuffer;
  Lissajous lis;
  ContourIntervalDialog *ciDialog;
  double conterval;
  ContourInterval contourInterval;
  xy windowCenter,worldCenter;
  double scale;
  double lengthUnit;
  double maxScaleSize,scaleSize;
  xy ballPos;
  xy leftScaleEnd,rightScaleEnd,scaleEnd;
  int penPos;
  int triangleNum;
  int lastOpcount;
  int ballAngle,dartAngle;
  int lastntri;
  int splashScreenTime; // in ticks
};
#endif
