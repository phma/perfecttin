/******************************************************/
/*                                                    */
/* mainwindow.cpp - main window                       */
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
#include "config.h"
#include "mainwindow.h"
#include "threads.h"
#include "ldecimal.h"
#include "cloud.h"
#include "adjelev.h"
#include "fileio.h"
#include "octagon.h"
using namespace std;

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "PerfectTIN"));
  fileMsg=new QLabel(this);
  dotTriangleMsg=new QLabel(this);
  toleranceMsg=new QLabel(this);
  canvas=new TinCanvas(this);
  configDialog=new ConfigurationDialog(this);
  fileDialog=new QFileDialog(this);
  connect(configDialog,SIGNAL(settingsChanged(double,double,double,int)),
	  this,SLOT(setSettings(double,double,double,int)));
  connect(this,SIGNAL(octagonReady()),canvas,SLOT(sizeToFit()));
  doneBar=new QProgressBar(this);
  busyBar=new QProgressBar(this);
  doneBar->setRange(0,16777216);
  busyBar->setRange(0,16777216);
  lpfBusyFraction=0;
  makeActions();
  makeStatusBar();
  setCentralWidget(canvas);
  readSettings();
  canvas->show();
  show();
  timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(tick()));
  connect(timer,SIGNAL(timeout()),canvas,SLOT(tick()));
  timer->start(50);
}

MainWindow::~MainWindow()
{
}

void MainWindow::tick()
{
  double toleranceRatio;
  int numDots=cloud.size();
  int tstatus=getThreadStatus();
  int numTriangles=net.triangles.size();
  ThreadAction ta;
  if (numDots!=lastNumDots || numTriangles!=lastNumTriangles)
  {
    if (lastNumTriangles<4 && numTriangles>4)
      octagonReady();
    lastNumDots=numDots;
    lastNumTriangles=numTriangles;
    if (numDots && numTriangles)
      dotTriangleMsg->setText(tr("Making octagon"));
    else if (numTriangles)
      dotTriangleMsg->setText(tr("%n triangles","",numTriangles));
    else
      dotTriangleMsg->setText(tr("%n dots","",numDots));
  }
  toleranceRatio=stageTolerance/tolerance;
  if (tolerance!=lastTolerance || stageTolerance!=lastStageTolerance)
  {
    lastTolerance=tolerance;
    lastStageTolerance=stageTolerance;
    toleranceMsg->setText(QString::fromStdString(ldecimal(tolerance,5e-4)+"×"+ldecimal(toleranceRatio,5e-4)));
  }
  lpfBusyFraction=(16*lpfBusyFraction+busyFraction())/17;
  busyBar->setValue(lrint(lpfBusyFraction*16777216));
  if ((tstatus&0x3ffbfeff)==1048577*TH_PAUSE)
  {
    areadone=areaDone(stageTolerance);
    if (actionQueueEmpty() && tstatus==1048577*TH_PAUSE+TH_ASLEEP)
      if (writtenTolerance>stageTolerance)
      {
	ta.param1=outUnit;
	ta.opcode=ACT_WRITE_DXF;
	ta.filename=saveFileName+".dxf";
	enqueueAction(ta);
	ta.opcode=ACT_WRITE_TIN;
	ta.filename=saveFileName+".tin";
	enqueueAction(ta);
	writtenTolerance=stageTolerance;
      }
      else
      {
	if (stageTolerance>tolerance)
	{
	  stageTolerance/=2;
	  setThreadCommand(TH_RUN);
	}
	else
	  setThreadCommand(TH_WAIT);
	currentAction=0;
      }
  }
  if ((tstatus&0x3ffbfeff)==1048577*TH_RUN)
  {
    areadone=areaDone(stageTolerance);
    doneBar->setValue(lrint(areadone[0]*16777216));
    rmsadj=rmsAdjustment();
    if (livelock(areadone[0],rmsadj))
    {
      randomizeSleep();
    }
    if (areadone[0]==1 && allBucketsClean())
      setThreadCommand(TH_PAUSE);
  }
  if (tstatus==1048577*TH_WAIT+TH_ASLEEP && actionQueueEmpty())
    currentAction=0;
  if (tstatus==1048577*TH_WAIT+TH_ASLEEP && actionQueueEmpty() &&
      !(toleranceRatio>0) && net.triangles.size()==6)
  { // It's finished making the octagon, and all threads are waiting and asleep.
    if (!std::isfinite(stageTolerance)) // only one point in cloud, or points are NaN
    {
      clearCloud();
      net.clear();
      // send a signal to pop up an error
    }
    else
    {
      toleranceRatio=-toleranceRatio;
      stageTolerance=tolerance;
      while (stageTolerance*2<tolerance*toleranceRatio)
	stageTolerance*=2;
      setThreadCommand(TH_RUN);
    }
  }
}

void MainWindow::loadFile()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog->setWindowTitle(tr("Load Point Cloud File"));
  fileDialog->setFileMode(QFileDialog::ExistingFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
#ifdef LibPLYXX_FOUND
  fileDialog->setNameFilter(tr("(*.las);;(*.ply);;(*)"));
#else
  fileDialog->setNameFilter(tr("(*.las);;(*)"));
#endif
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    net.clear();
    if (cloud.size()==0)
      fileNames="";
    ta.opcode=ACT_LOAD;
    ta.filename=fileName;
    ta.param1=inUnit;
    enqueueAction(ta);
    if (fileNames.length())
      fileNames+=';';
    fileNames+=baseName(fileName);
    lastFileName=fileName;
    fileMsg->setText(QString::fromStdString(fileNames));
  }
}

void MainWindow::startConversion()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog->setWindowTitle(tr("Convert to TIN"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(noExt(lastFileName)));
  fileDialog->setNameFilter(tr("*"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    saveFileName=files[0].toStdString();
    ta.opcode=ACT_OCTAGON;
    enqueueAction(ta);
    writtenTolerance=INFINITY;
    fileNames=baseName(saveFileName)+".[dxf|tin]";
    fileMsg->setText(QString::fromStdString(fileNames));
  }
}

void MainWindow::clearCloud()
{
  cloud.clear();
  fileNames=lastFileName="";
  fileMsg->setText("");
}

void MainWindow::configure()
{
  configDialog->set(inUnit,outUnit,tolerance,numberThreads);
  configDialog->open();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  writeSettings();
  event->accept();
}

void MainWindow::makeActions()
{
  fileMenu=menuBar()->addMenu(tr("&File"));
  settingsMenu=menuBar()->addMenu(tr("&Settings"));
  helpMenu=menuBar()->addMenu(tr("&Help"));
  // File menu
  loadAction=new QAction(this);
  loadAction->setIcon(QIcon::fromTheme("document-open"));
  loadAction->setText(tr("Load"));
  fileMenu->addAction(loadAction);
  connect(loadAction,SIGNAL(triggered(bool)),this,SLOT(loadFile()));
  convertAction=new QAction(this);
  convertAction->setIcon(QIcon::fromTheme("document-save-as"));
  convertAction->setText(tr("Convert"));
  fileMenu->addAction(convertAction);
  connect(convertAction,SIGNAL(triggered(bool)),this,SLOT(startConversion()));
  clearAction=new QAction(this);
  clearAction->setIcon(QIcon::fromTheme("edit-clear"));
  clearAction->setText(tr("Clear"));
  fileMenu->addAction(clearAction);
  connect(clearAction,SIGNAL(triggered(bool)),this,SLOT(clearCloud()));
  // Settings menu
  configureAction=new QAction(this);
  configureAction->setIcon(QIcon::fromTheme("configure"));
  configureAction->setText(tr("Configure"));
  settingsMenu->addAction(configureAction);
  connect(configureAction,SIGNAL(triggered(bool)),this,SLOT(configure()));
}

void MainWindow::makeStatusBar()
{
  statusBar()->addWidget(fileMsg);
  statusBar()->addWidget(dotTriangleMsg);
  statusBar()->addWidget(toleranceMsg);
  statusBar()->addWidget(doneBar);
  statusBar()->addWidget(busyBar);
  statusBar()->show();
}

void MainWindow::readSettings()
{
  QSettings settings("Bezitopo","PerfectTIN");
  resize(settings.value("size",QSize(707,500)).toSize());
  move(settings.value("pos",QPoint(0,0)).toPoint());
  numberThreads=settings.value("threads",0).toInt();
  tolerance=settings.value("tolerance",0.1).toDouble();
  inUnit=settings.value("inUnit",1).toDouble();
  outUnit=settings.value("outUnit",1).toDouble();
}

void MainWindow::writeSettings()
{
  QSettings settings("Bezitopo","PerfectTIN");
  settings.setValue("size",size());
  settings.setValue("pos",pos());
  settings.setValue("threads",numberThreads);
  settings.setValue("tolerance",tolerance);
  settings.setValue("inUnit",inUnit);
  settings.setValue("outUnit",outUnit);
}

void MainWindow::setSettings(double iu,double ou,double tol,int thr)
{
  inUnit=iu;
  outUnit=ou;
  tolerance=tol;
  numberThreads=thr;
  writeSettings();
}

