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
#include "threads.h"

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
  bool conversionBusy();
signals:
  void tinSizeChanged();
  void noCloudArea();
  void gotResult(ThreadAction ta);
public slots:
  void tick();
  void setSettings(double lu,double tol,int thr);
  void openFile();
  void loadFile();
  void exportDxfTxt();
  void exportDxfBin();
  void exportTinTxt();
  void exportCarlsonTin();
  void exportLandXml();
  void startConversion();
  void stopConversion();
  void resumeConversion();
  void clearCloud();
  void configure();
  void msgNoCloudArea();
  void handleResult(ThreadAction ta);
  void aboutProgram();
  void aboutQt();
protected:
  void closeEvent(QCloseEvent *event) override;
private:
  int lastNumDots,lastNumTriangles,lastNumEdges;
  double lastTolerance,lastStageTolerance,writtenTolerance,rmsadj;
  int numberThreads;
  int lastState; // state is in TinCanvas
  bool dxfText,sameUnits,conversionStopped,showingResult;
  double tolerance,lengthUnit,outUnit;
  double lpfBusyFraction;
  std::string fileNames,saveFileName,lastFileName;
  QTimer *timer;
  QFileDialog *fileDialog;
  QMessageBox *msgBox;
  ConfigurationDialog *configDialog;
  QMenu *fileMenu,*settingsMenu,*helpMenu,*exportMenu;
  QLabel *fileMsg,*dotTriangleMsg,*toleranceMsg;
  QProgressBar *doneBar,*busyBar;
  QAction *openAction,*loadAction,*convertAction,*clearAction;
  QAction *exportAction,*stopAction,*resumeAction,*exitAction;
  QAction *exportDxfTxtAction,*exportDxfBinAction,*exportTinTxtAction;
  QAction *exportCarlsonTinAction,*exportLandXmlAction;
  QAction *configureAction;
  QAction *aboutProgramAction,*aboutQtAction;
  TinCanvas *canvas;
};
