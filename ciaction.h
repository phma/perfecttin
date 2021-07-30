/******************************************************/
/*                                                    */
/* ciaction.h - contour interval action               */
/*                                                    */
/******************************************************/
/* Copyright 2021 Pierre Abbat.
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
#ifndef CIACTION_H
#define CIACTION_H
class ContourInterval;
#include <QtWidgets>
#include "contour.h"

class ContourIntervalAction: public QAction
{
  Q_OBJECT
public:
  ContourIntervalAction(QObject *parent=nullptr,ContourInterval ci=ContourInterval());
public slots:
  void setInterval(ContourInterval ci);
  void selfTriggered(bool dummy);
signals:
  void intervalChanged(ContourInterval ci);
protected:
private:
  ContourInterval cInt;
};
#endif
