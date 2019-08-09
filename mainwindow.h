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
#include <string>
#include <array>
#include "configdialog.h"
#include "tincanvas.h"

class MainWindow: public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(QWidget *parent=0);
  ~MainWindow();
  void makeActions();
  void makeStatusBar();
  void readSettings();
  void writeSettings();
  int getNumberThreads()
  {
    return numberThreads;
  }
signals:
  void octagonReady();
public slots:
  void tick();
  void setSettings(double iu,double ou,double tol,int thr);
  void loadFile();
  void startConversion();
  void clearCloud();
  void configure();
protected:
  void closeEvent(QCloseEvent *event) override;
private:
  int lastNumDots,lastNumTriangles;
  double lastTolerance,lastStageTolerance,rmsadj;
  int numberThreads;
  double tolerance,inUnit,outUnit;
  std::string fileNames,saveFileName,lastFileName;
  QTimer *timer;
  QFileDialog *fileDialog;
  ConfigurationDialog *configDialog;
  QMenu *fileMenu,*settingsMenu,*helpMenu;
  QLabel *fileMsg,*dotTriangleMsg,*toleranceMsg;
  QAction *loadAction,*convertAction,*clearAction;
  QAction *configureAction;
  TinCanvas *canvas;
};
