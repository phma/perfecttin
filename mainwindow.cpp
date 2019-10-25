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
#include "ldecimal.h"
#include "cloud.h"
#include "adjelev.h"
#include "fileio.h"
#include "octagon.h"
using namespace std;

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "PerfectTIN"));
  setWindowIcon(QIcon(":/perfecttin.png"));
  fileMsg=new QLabel(this);
  dotTriangleMsg=new QLabel(this);
  toleranceMsg=new QLabel(this);
  canvas=new TinCanvas(this);
  configDialog=new ConfigurationDialog(this);
  fileDialog=new QFileDialog(this);
  msgBox=new QMessageBox(this);
  connect(configDialog,SIGNAL(settingsChanged(double,double,bool,double,int,bool)),
	  this,SLOT(setSettings(double,double,bool,double,int,bool)));
  connect(this,SIGNAL(tinSizeChanged()),canvas,SLOT(setSize()));
  connect(this,SIGNAL(noCloudArea()),this,SLOT(msgNoCloudArea()));
  connect(this,SIGNAL(gotResult(ThreadAction)),this,SLOT(handleResult(ThreadAction)));
  doneBar=new QProgressBar(this);
  busyBar=new QProgressBar(this);
  doneBar->setRange(0,16777216);
  busyBar->setRange(0,16777216);
  lpfBusyFraction=0;
  conversionStopped=false;
  showingResult=false;
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

/* Rules for enabling actions:
 * You cannot load a file while a conversion is in progress.
 * You can load a file while loading another file (it will be queued).
 * You can load a file while a conversion is stopped, but then you cannot resume.
 * You cannot clear while loading a file or converting.
 * You cannot start a conversion while loading a file or converting.
 * You cannot stop a conversion unless one is in progress and has
 * passed the octagon stage.
 * You cannot resume a conversion unless one has been stopped
 * and no file has been loaded and the TIN has not been cleared.
 */

void MainWindow::tick()
{
  double toleranceRatio;
  int numDots=cloud.size();
  int tstatus=getThreadStatus();
  int numTriangles=net.triangles.size();
  int numEdges=net.edges.size();
  ThreadAction ta;
  if (!showingResult && !resultQueueEmpty())
  {
    ta=dequeueResult();
    gotResult(ta);
  }
  if (lastState!=canvas->state)
  {
    //cout<<"Last state "<<lastState<<" Current state "<<canvas->state<<endl;
    if (lastState==-ACT_LOAD && canvas->state==TH_WAIT)
    { // finished loading file
      clearAction->setEnabled(true);
      convertAction->setEnabled(true);
    }
    if (canvas->state==-ACT_LOAD)
    { // started loading file
      convertAction->setEnabled(false);
      resumeAction->setEnabled(false);
      clearAction->setEnabled(false);
      conversionStopped=false;
    }
    if (canvas->state==TH_WAIT && conversionStopped)
    {
      loadAction->setEnabled(true);
      stopAction->setEnabled(false);
      resumeAction->setEnabled(true);
      clearAction->setEnabled(true);
    }
    if (canvas->state==TH_RUN)
    {
      loadAction->setEnabled(false);
      stopAction->setEnabled(true);
      resumeAction->setEnabled(false);
      clearAction->setEnabled(false);
    }
    lastState=canvas->state;
  }
  if (numDots!=lastNumDots || numTriangles!=lastNumTriangles || numEdges!=lastNumEdges)
  { // Number of dots or triangles has changed: update status bar
    if (lastNumTriangles<4 && numTriangles>4)
      tinSizeChanged();
    if (numTriangles<lastNumTriangles || ((numTriangles^lastNumTriangles)&1))
      canvas->update();
    lastNumDots=numDots;
    lastNumTriangles=numTriangles;
    if (numDots && numTriangles)
    {
      dotTriangleMsg->setText(tr("Making octagon"));
      loadAction->setEnabled(false);
      convertAction->setEnabled(false);
      clearAction->setEnabled(false);
    }
    else if (numEdges>0 && numEdges<numTriangles*3/2)
    { // When it's finished making edges, numEdges=(numTriangles*3+numConvexHull)/2.
      dotTriangleMsg->setText(tr("Making edges"));
      loadAction->setEnabled(false);
      convertAction->setEnabled(false);
      clearAction->setEnabled(false);
    }
    else if (numTriangles)
      dotTriangleMsg->setText(tr("%n triangles","",numTriangles));
    else
      dotTriangleMsg->setText(tr("%n dots","",numDots));
  }
  toleranceRatio=stageTolerance/tolerance;
  if (tolerance!=lastTolerance || stageTolerance!=lastStageTolerance)
  { // Tolerance has changed: update status bar
    lastTolerance=tolerance;
    lastStageTolerance=stageTolerance;
    toleranceMsg->setText(QString::fromStdString(ldecimal(tolerance,5e-4)+"×"+ldecimal(toleranceRatio,5e-4)));
  }
  lpfBusyFraction=(16*lpfBusyFraction+busyFraction())/17;
  busyBar->setValue(lrint(lpfBusyFraction*16777216));
  if ((tstatus&0x3ffbfeff)==1048577*TH_PAUSE)
  { // Stage has completed: write files and go to next stage
    areadone=areaDone(stageTolerance);
    if (actionQueueEmpty() && tstatus==1048577*TH_PAUSE+TH_ASLEEP)
      if (writtenTolerance>stageTolerance)
      {
	ta.param1=outUnit;
	ta.param0=dxfText;
	ta.opcode=ACT_WRITE_DXF;
	ta.filename=saveFileName+".dxf";
	//enqueueAction(ta);
	ta.opcode=ACT_WRITE_TIN;
	ta.filename=saveFileName+".tin";
	//enqueueAction(ta);
	ta.param1=tolerance;
	ta.param0=lrint(toleranceRatio);
	ta.opcode=ACT_WRITE_PTIN;
	if (ta.param0==1)
	  ta.filename=saveFileName+".ptin";
	else
	  ta.filename=saveFileName+"."+to_string(ta.param0)+".ptin";
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
	else // conversion is finished
	{
	  setThreadCommand(TH_WAIT);
	  loadAction->setEnabled(true);
	  convertAction->setEnabled(true);
	  clearAction->setEnabled(true);
	  stopAction->setEnabled(false);
	}
	currentAction=0;
	canvas->update();
      }
  }
  if ((tstatus&0x3ffbfeff)==1048577*TH_RUN)
  { // Conversion is running: check whether stage is complete
    areadone=areaDone(stageTolerance);
    doneBar->setValue(lrint(areadone[0]*16777216));
    rmsadj=rmsAdjustment();
    if (livelock(areadone[0],rmsadj))
    {
      randomizeSleep();
    }
    if ((areadone[0]==1 && allBucketsClean()) || (areadone[1]==1 && stageTolerance>tolerance))
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
      noCloudArea();
    }
    else
    {
      toleranceRatio=-toleranceRatio;
      stageTolerance=tolerance;
      while (stageTolerance*2<tolerance*toleranceRatio)
	stageTolerance*=2;
      setThreadCommand(TH_RUN);
      stopAction->setEnabled(true);
    }
  }
}

void MainWindow::openFile()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog->setWindowTitle(tr("Open PerfectTIN File"));
  fileDialog->setFileMode(QFileDialog::ExistingFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
  fileDialog->setNameFilter(tr("(*.ptin);;(*)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.opcode=ACT_READ_PTIN;
    ta.filename=fileName;
    enqueueAction(ta);
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
    tinSizeChanged();
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
    clearLog();
    writtenTolerance=INFINITY;
    fileNames=baseName(saveFileName)+".[dxf|tin]";
    fileMsg->setText(QString::fromStdString(fileNames));
  }
}

void MainWindow::stopConversion()
{
  setThreadCommand(TH_WAIT);
  conversionStopped=true;
}

void MainWindow::resumeConversion()
{
  if (conversionStopped)
  {
    setThreadCommand(TH_RUN);
    conversionStopped=false;
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
  configDialog->set(inUnit,outUnit,sameUnits,tolerance,numberThreads,dxfText);
  configDialog->open();
}

void MainWindow::msgNoCloudArea()
{
  msgBox->warning(this,tr("PerfectTIN"),tr("Point cloud no area"));
}

void MainWindow::handleResult(ThreadAction ta)
/* Receives the result of reading a file. If an error happened, pops up a message.
 * The file was read by a worker thread, which put the result in a queue.
 */
{
  QString message;
  showingResult=true;
  tinSizeChanged();
  switch (ta.opcode)
  {
    case ACT_READ_PTIN:
      if (ta.ptinResult.tolRatio>0 && ta.ptinResult.tolerance>0)
      {
	tolerance=ta.ptinResult.tolerance;
	stageTolerance=writtenTolerance=tolerance*ta.ptinResult.tolRatio;
	saveFileName=noExt(ta.filename);
	if (ta.ptinResult.tolRatio>1)
	{
	  conversionStopped=true;
	  resizeBuckets(1);
	  if (extension(saveFileName)=="."+to_string(ta.ptinResult.tolRatio))
	    saveFileName=noExt(saveFileName);
	}
	fileNames=baseName(saveFileName)+".ptin";
	net.conversionTime=ta.ptinResult.conversionTime;
      }
      else if (ta.ptinResult.tolRatio>0 && std::isnan(ta.ptinResult.tolerance))
	message=tr("File incomplete");
      else
      {
	saveFileName="";
	fileNames="";
	switch (ta.ptinResult.tolRatio)
	{
	  case PT_UNKNOWN_HEADER_FORMAT:
	    message=tr("Newer version");
	    break;
	  case PT_NOT_PTIN_FILE:
	  case PT_COUNT_MISMATCH:
	    message=tr("Not ptin file");
	    break;
	  default:
	    message=tr("File corrupt");
	}
      }
      if (message.length())
	msgBox->warning(this,tr("PerfectTIN"),message);
      break;
  }
  fileMsg->setText(QString::fromStdString(fileNames));
  showingResult=false;
}

void MainWindow::aboutProgram()
{
  QString progName=tr("PerfectTIN");
#ifdef LibPLYXX_FOUND
  QString rajotte=tr("\nPLY file code © Simon Rajotte, MIT license");
#else
  QString rajotte("");
#endif
  QMessageBox::about(this,tr("PerfectTIN"),
		     tr("%1\nVersion %2\nCopyright %3 Pierre Abbat\nLicense GPL 3 or later%4")
		     .arg(progName).arg(QString(VERSION)).arg(COPY_YEAR).arg(rajotte));
}

void MainWindow::aboutQt()
{
  QMessageBox::aboutQt(this,tr("PerfectTIN"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  int result;
  writeSettings();
  if (conversionBusy())
  {
    msgBox->setWindowTitle(tr("PerfectTIN"));
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setText(tr("A conversion is in progress."));
    msgBox->setInformativeText(tr("Do you want to abort it and exit?"));
    msgBox->setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    result=msgBox->exec();
    if (result==QMessageBox::Yes)
      event->accept();
    else
      event->ignore();
  }
  else
    event->accept();
}

void MainWindow::makeActions()
{
  fileMenu=menuBar()->addMenu(tr("&File"));
  settingsMenu=menuBar()->addMenu(tr("&Settings"));
  helpMenu=menuBar()->addMenu(tr("&Help"));
  // File menu
  openAction=new QAction(this);
  openAction->setIcon(QIcon::fromTheme("document-open"));
  openAction->setText(tr("Open"));
  fileMenu->addAction(openAction);
  connect(openAction,SIGNAL(triggered(bool)),this,SLOT(openFile()));
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
  stopAction=new QAction(this);
  stopAction->setIcon(QIcon::fromTheme("process-stop"));
  stopAction->setText(tr("Stop"));
  stopAction->setEnabled(false);
  fileMenu->addAction(stopAction);
  connect(stopAction,SIGNAL(triggered(bool)),this,SLOT(stopConversion()));
  resumeAction=new QAction(this);
  //resumeAction->setIcon(QIcon::fromTheme("edit-clear"));
  resumeAction->setText(tr("Resume"));
  resumeAction->setEnabled(false);
  fileMenu->addAction(resumeAction);
  connect(resumeAction,SIGNAL(triggered(bool)),this,SLOT(resumeConversion()));
  exitAction=new QAction(this);
  exitAction->setIcon(QIcon::fromTheme("application-exit"));
  exitAction->setText(tr("Exit"));
  fileMenu->addAction(exitAction);
  connect(exitAction,SIGNAL(triggered(bool)),this,SLOT(close()));
  // Settings menu
  configureAction=new QAction(this);
  configureAction->setIcon(QIcon::fromTheme("configure"));
  configureAction->setText(tr("Configure"));
  settingsMenu->addAction(configureAction);
  connect(configureAction,SIGNAL(triggered(bool)),this,SLOT(configure()));
  // Help menu
  aboutProgramAction=new QAction(this);
  aboutProgramAction->setText(tr("About PerfectTIN"));
  helpMenu->addAction(aboutProgramAction);
  connect(aboutProgramAction,SIGNAL(triggered(bool)),this,SLOT(aboutProgram()));
  aboutQtAction=new QAction(this);
  aboutQtAction->setText(tr("About Qt"));
  helpMenu->addAction(aboutQtAction);
  connect(aboutQtAction,SIGNAL(triggered(bool)),this,SLOT(aboutQt()));
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
  sameUnits=settings.value("sameUnits",false).toBool();
  dxfText=settings.value("dxfText",false).toBool();
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
  settings.setValue("sameUnits",sameUnits);
  settings.setValue("dxfText",dxfText);
}

void MainWindow::setSettings(double iu,double ou,bool ieqo,double tol,int thr,bool dxf)
{
  inUnit=iu;
  outUnit=ou;
  sameUnits=ieqo;
  tolerance=tol;
  numberThreads=thr;
  dxfText=dxf;
  writeSettings();
}

bool MainWindow::conversionBusy()
{
  //cout<<"state "<<canvas->state<<endl;
  return canvas->state==TH_RUN ||
	 canvas->state==TH_PAUSE ||
	 canvas->state==-ACT_WRITE_DXF ||
	 canvas->state==-ACT_WRITE_TIN ||
	 canvas->state==-ACT_WRITE_PTIN ||
	 canvas->state==-ACT_OCTAGON ||
	 canvas->state==0;
}
