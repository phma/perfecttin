/******************************************************/
/*                                                    */
/* mainwindow.h - main window                         */
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
#include <QMainWindow>
#include <QTimer>
#include <QtWidgets>
#include <QPixmap>
#include <array>

class MainWindow: public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(QWidget *parent=0);
  ~MainWindow();
  void makeActions();
  void makeStatusBar();
public slots:
  void tick();
private:
  int tickCount;
  QTimer *timer;
  QMenu *fileMenu,*settingsMenu,*helpMenu;
  QLabel *fileMsg,*progressMsg,*triangleMsg;
};