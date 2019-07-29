/******************************************************/
/*                                                    */
/* tincanvas.h - canvas for drawing TIN               */
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
#ifndef TINCANVAS_H
#define TINCANVAS_H
#include <QWidget>
#include "point.h"

class TinCanvas: public QWidget
{
  Q_OBJECT
public:
  TinCanvas(QWidget *parent=0);
  void setBrush(const QBrush &qbrush);
  QPointF worldToWindow(xy pnt);
  xy windowToWorld(QPointF pnt);
signals:
public slots:
  void sizeToFit();
  //void tick();
protected:
  void setSize();
  //void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  //void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
  QBrush brush;
  xy windowCenter,worldCenter;
  double scale;
};
#endif
