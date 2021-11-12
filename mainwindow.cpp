/******************************************************/
/*                                                    */
/* mainwindow.cpp - main window                       */
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
#include "config.h"
#include "mainwindow.h"
#include "ldecimal.h"
#include "cloud.h"
#include "ply.h"
#include "adjelev.h"
#include "neighbor.h"
#include "fileio.h"
#include "angle.h"
#include "units.h"
#include "octagon.h"
#include "brevno.h"
using namespace std;

const char unitIconNames[4][28]=
{
  ":/meter.png",":/international-foot.png",":/us-foot.png",":/indian-foot.png"
};

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "PerfectTIN"));
  setWindowIcon(QIcon(":/perfecttin.png"));
  fileMsg=new QLabel(this);
  dotTriangleMsg=new QLabel(this);
  toleranceMsg=new QLabel(this);
  densityMsg=new QLabel(this);
  canvas=new TinCanvas(this);
  configDialog=new ConfigurationDialog(this);
  msgBox=new QMessageBox(this);
  connect(configDialog,SIGNAL(settingsChanged(double,double,int,bool,bool,Printer3dSize)),
	  this,SLOT(setSettings(double,double,int,bool,bool,Printer3dSize)));
  connect(this,SIGNAL(tinSizeChanged()),canvas,SLOT(setSize()));
  connect(this,SIGNAL(lengthUnitChanged(double)),canvas,SLOT(setLengthUnit(double)));
  connect(this,SIGNAL(noCloudArea()),this,SLOT(msgNoCloudArea()));
  connect(this,SIGNAL(verticalOutlier()),this,SLOT(msgVerticalOutlier()));
  connect(this,SIGNAL(gotResult(ThreadAction)),this,SLOT(handleResult(ThreadAction)));
  connect(canvas,SIGNAL(setBusy(int)),this,SLOT(setBusy(int)));
  connect(canvas,SIGNAL(setIdle(int)),this,SLOT(setIdle(int)));
  connect(canvas,SIGNAL(contourDrawingFinished()),this,SLOT(refreshNumTriangles()));
  connect(canvas,SIGNAL(contourSetsChanged()),this,SLOT(updateContourIntervalActions()));
  doneBar=new QProgressBar(this);
  busyBar=new QProgressBar(this);
  doneBar->setRange(0,16777216);
  busyBar->setRange(0,16777216);
  lpfBusyFraction=0;
  density=tolerance=0;
  busy=0;
  lastTolerance=lastStageTolerance=writtenTolerance=0;
  lastState=TH_WAIT;
  lastNumDots=lastNumTriangles=lastNumEdges=0;
  density=lastDensity=0;
  conversionStopped=false;
  showingResult=false;
  toolbar=new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,toolbar);
  toolbar->setIconSize(QSize(32,32));
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
  canvas->startSplashScreen();
  traceHiLo=0;
}

void dumpSteepestTriangle()
{
  int i,iSteep;
  double gSteep=0;
  vector<point *> cornersToDump;
  xy grad;
  for (i=0;i<net.triangles.size();i++)
  {
    grad=net.triangles[i].gradient(net.triangles[i].centroid());
    if (grad.length()>gSteep)
    {
      gSteep=grad.length();
      iSteep=i;
    }
  }
  cornersToDump.push_back(net.triangles[iSteep].a);
  cornersToDump.push_back(net.triangles[iSteep].b);
  cornersToDump.push_back(net.triangles[iSteep].c);
  dumpTriangles("steepdump",triangleNeighbors(cornersToDump));
  cout<<"Steepest triangle has slope "<<gSteep<<endl;
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
  int numContours=net.contours.size();
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
    if (canvas->state==-ACT_LOAD || canvas->state==-ACT_READ_PTIN)
    { // started loading file
      conversionStopped=false;
    }
    lastState=canvas->state;
  }
  if (numDots!=lastNumDots || numTriangles!=lastNumTriangles ||
      numEdges!=lastNumEdges || numContours!=lastNumContours)
  { // Number of dots or triangles has changed: update status bar
    if (lastNumDots==0 && numDots>0)
      setBusy(BUSY_CLD);
    if (lastNumDots>0 && numDots==0)
      setIdle(BUSY_CLD);
    if (lastNumTriangles==0 && numTriangles>0)
      setBusy(BUSY_TIN);
    if (lastNumTriangles>0 && numTriangles==0)
      setIdle(BUSY_TIN);
    if ((lastNumTriangles<4 && numTriangles>4) || lastNumDots>numDots)
      tinSizeChanged();
    if (numTriangles<lastNumTriangles || ((numTriangles^lastNumTriangles)&1))
    {
      canvas->update();
      canvas->setScalePos();
    }
    lastNumDots=numDots;
    lastNumTriangles=numTriangles;
    lastNumEdges=numEdges;
    lastNumContours=numContours;
    if (numDots && numTriangles && numTriangles<8)
    /* Reading a .ptin file can make numDots small (like 3) as it puts dots that
     * were pushed over an edge by roundoff error into cloud, while numTriangles
     * is in the thousands. This is not making octagon. While making octagon,
     * the number of triangles is 6, and the number of dots is large (millions).
     */
    {
      dotTriangleMsg->setText(tr("Making octagon"));
    }
    else if (numContours && numEdges==0)
    {
      dotTriangleMsg->setText(tr("Reading contours"));
    }
    else if (numEdges>0 && numEdges<numTriangles*3/2)
    { // When it's finished making edges, numEdges=(numTriangles*3+numConvexHull)/2.
      dotTriangleMsg->setText(tr("Making edges"));
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
  if (density!=lastDensity)
  { // Density has changed: update status bar
    lastDensity=density;
    if (density>0)
      densityMsg->setText(QString::fromStdString(ldecimal(density,density/256)+"/m²"));
    else
      densityMsg->clear();
  }
  lpfBusyFraction=(16*lpfBusyFraction+busyFraction())/17;
  busyBar->setValue(lrint(lpfBusyFraction*16777216));
  if ((tstatus&0x3ffbfeff)==1048577*TH_PAUSE)
  { // Stage has completed: write files and go to next stage
    areadone=areaDone(stageTolerance,sqr(stageTolerance/tolerance)/density);
    canvas->clearContourFlags();
    if (actionQueueEmpty() && tstatus==1048577*TH_PAUSE+TH_ASLEEP)
      if (writtenTolerance>stageTolerance)
      {
	ta.opcode=ACT_QINDEX;
	enqueueAction(ta);
	ta.param0=lrint(toleranceRatio*4);
	ta.opcode=ACT_DELETE_FILE;
	ta.filename=saveFileName+"."+to_string(ta.param0)+".ptin";
	enqueueAction(ta);
	ta.param1=tolerance;
	ta.param0=lrint(toleranceRatio);
	ta.param2=density;
	ta.opcode=ACT_WRITE_PTIN;
	if (ta.param0==1)
	{
	  ta.filename=saveFileName+".ptin";
	  setIdle(BUSY_OCT|BUSY_CVT|BUSY_UNFIN);
	}
	else
	{
	  ta.filename=saveFileName+"."+to_string(ta.param0)+".ptin";
	  setIdle(BUSY_OCT);
	  setBusy(BUSY_UNFIN);
	}
	setBusy(BUSY_SAVE);
	enqueueAction(ta);
	writtenTolerance=stageTolerance;
      }
      else
      {
	if (stageTolerance>tolerance)
	{
	  stageTolerance/=2;
	  minArea/=4;
	  net.swishFactor=traceHiLo*stageTolerance;
	  setThreadCommand(TH_RUN);
	}
	else // conversion is finished
	{
	  ta.param0=lrint(stageTolerance*2/tolerance);
	  ta.opcode=ACT_DELETE_FILE;
	  ta.filename=saveFileName+"."+to_string(ta.param0)+".ptin";
	  enqueueAction(ta);
	  setThreadCommand(TH_WAIT);
	}
	currentAction=0;
	canvas->update();
      }
    else;
  }
  if ((tstatus&0x3ffbfeff)==1048577*TH_ROUGH ||
      (tstatus&0x3ffbfeff)==1048577*TH_PRUNE ||
      (tstatus&0x3ffbfeff)==1048577*TH_SMOOTH)
  {
    doneBar->setValue(lrint((double)contourSegmentsDone/canvas->totalContourPieces*16777216));
    switch ((tstatus&0x3ffbfeff)/1048577)
    {
      case TH_ROUGH:
	dotTriangleMsg->setText(tr("Drawing rough contours"));
	break;
      case TH_PRUNE:
	dotTriangleMsg->setText(tr("Pruning contours"));
	break;
      case TH_SMOOTH:
	dotTriangleMsg->setText(tr("Smoothing contours"));
	break;
    }
  }
  if ((tstatus&0x3ffbfeff)==1048577*TH_RUN)
  { // Conversion is running: check whether stage is complete
    areadone=areaDone(stageTolerance,sqr(stageTolerance/tolerance)/density);
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
      canvas->clearContourFlags();
      noCloudArea();
    }
    else
    {
      toleranceRatio=-toleranceRatio;
      density=estimatedDensity();
      stageTolerance=tolerance;
      while (stageTolerance*2<tolerance*toleranceRatio)
	stageTolerance*=2;
      minArea=sqr(stageTolerance/tolerance)/densify/density;
      net.swishFactor=traceHiLo*stageTolerance;
      setThreadCommand(TH_RUN);
    }
  }
  if (largeVertical)
  {
    if (!lastLargeVertical)
      verticalOutlier();
    lastLargeVertical=true;
  }
  else
    lastLargeVertical=false;
  writeBufLog();
}

void MainWindow::refreshNumTriangles()
{
  lastNumDots=lastNumEdges=lastNumTriangles=-1;
}

void MainWindow::openFile()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
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
    setBusy(BUSY_OPEN);
    canvas->clearContourFlags();
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::loadFile()
{
  int dialogResult;
  QStringList files;
  int i;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Load Point Cloud File"));
  fileDialog->setFileMode(QFileDialog::ExistingFiles);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
#ifdef Plytapus_FOUND
  fileDialog->setNameFilter(tr("(*.las);;(*.ply);;(*.xyz);;(*)"));
#else
  fileDialog->setNameFilter(tr("(*.las);;(*.xyz);;(*)"));
#endif
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    tinSizeChanged();
    if (cloud.size()==0)
      fileNames="";
    density=0;
    for (i=0;i<files.size();i++)
    {
      fileName=files[i].toStdString();
      ta.opcode=ACT_LOAD;
      ta.filename=fileName;
      ta.flags=0; // ground point flag
      ta.param1=lengthUnit;
      enqueueAction(ta);
      if (fileNames.length())
	fileNames+=';';
      fileNames+=baseName(fileName);
      lastFileName=fileName;
    }
    fileMsg->setText(QString::fromStdString(fileNames));
    updateContourIntervalActions();
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::loadBoundary()
{
  int dialogResult;
  QStringList files;
  int i;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Load Clipping Boundary"));
  fileDialog->setFileMode(QFileDialog::ExistingFiles);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
#ifdef Plytapus_FOUND
  fileDialog->setNameFilter(tr("(*.las);;(*.ply);;(*.xyz);;(*)"));
#else
  fileDialog->setNameFilter(tr("(*.las);;(*.xyz);;(*)"));
#endif
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    for (i=0;i<files.size();i++)
    {
      fileName=files[i].toStdString();
      ta.opcode=ACT_LOADBDY;
      ta.filename=fileName;
      ta.flags=0;
      ta.param1=lengthUnit;
      enqueueAction(ta);
    }
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::disableMenuSplash()
/* Disable menu during splash screen, so that the user can't mess up
 * the TIN that the splash screen shows.
 */
{
}

void MainWindow::enableMenuSplash()
{
}

/* Rules for enabling actions:
 * You cannot load or open a file while a conversion is in progress.
 * You can open a file while opening another file (it's pointless, as the
 * first TIN will be cleared on opening the second file).
 * You can load a file while loading another file (it will be queued).
 * You can load a file while a conversion is stopped, but then you cannot resume.
 * You can open a file while a conversion is stopped. If it is a final file,
 * you cannot resume; if it is a checkpoint file, you can resume.
 * You cannot load a boundary while loading or opening a file, saving a file,
 * or exporting a TIN. You can load a boundary while converting a point cloud
 * or drawing contours.
 * You cannot clear or export while loading a file or converting.
 * You cannot start a conversion while loading a file or converting.
 * You can start a conversion after loading a file, but not after opening one.
 * You cannot stop a conversion unless one is in progress and has
 * passed the octagon stage.
 * You cannot resume a conversion unless one has been stopped or you have
 * opened a checkpoint file and no file has been loaded and the TIN
 * has not been cleared.
 */

void MainWindow::endisableMenu()
{
  int i;
  openAction->setEnabled(bfl(0,(BUSY_DO-BUSY_OPEN)|BUSY_SPL));
  loadAction->setEnabled(bfl(0,(BUSY_DO-BUSY_LOAD)|BUSY_SPL));
  loadBoundaryAction->setEnabled(bfl(0,BUSY_SPL|BUSY_OPEN|BUSY_LOAD|BUSY_EXP|BUSY_SAVE));
  convertAction->setEnabled(bfl(BUSY_CLD,BUSY_DO));
  saveFileAction->setEnabled(bfl(BUSY_TIN,BUSY_DO|BUSY_SPL));
  exportMenu->setEnabled(bfl(BUSY_TIN,BUSY_DO|BUSY_SPL));
  clearAction->setEnabled(bfl(BUSY_CLD,BUSY_DO));
  stopAction->setEnabled(bfl(BUSY_CVT,0));
  resumeAction->setEnabled(bfl(BUSY_UNFIN,BUSY_DO));
  selectContourIntervalAction->setEnabled(bfl(0,BUSY_CTR));
  roughContoursAction->setEnabled(bfl(BUSY_TIN,BUSY_DO|BUSY_SPL));
  smoothContoursAction->setEnabled(bfl(BUSY_TIN,BUSY_DO|BUSY_SPL));
  deleteContoursAction->setEnabled(bfl(0,BUSY_DO|BUSY_SPL));
  for (i=0;i<ciActions.size();i++)
    ciActions[i].setEnabled(bfl(0,BUSY_CTR));
}

void MainWindow::setBusy(int activity)
{
  busy|=activity;
  endisableMenu();
}

void MainWindow::setIdle(int activity)
{
  busy&=~activity;
  endisableMenu();
}

void MainWindow::updateContourIntervalActions()
{
  int i;
  vector<ContourInterval> cis=net.contourIntervals();
  for (i=0;i<ciActions.size();i++)
    contourMenu->removeAction(&ciActions[i]);
  ciActions.clear();
  for (i=0;i<cis.size();i++)
    ciActions.emplace_back(this,cis[i]);
  for (i=0;i<ciActions.size();i++)
  {
    contourMenu->addAction(&ciActions[i]);
    ciActions[i].setText((QString::fromStdString(cis[i].valueToleranceString())));
    connect(canvas,SIGNAL(contourIntervalChanged(ContourInterval)),&ciActions[i],SLOT(setInterval(ContourInterval)));
    connect(&ciActions[i],SIGNAL(triggered(bool)),&ciActions[i],SLOT(selfTriggered(bool)));
    connect(&ciActions[i],SIGNAL(intervalChanged(ContourInterval)),canvas,SLOT(setContourInterval(ContourInterval)));
  }
}

void MainWindow::exportDxfTxt()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as DXF Text"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".dxf"));
  fileDialog->setNameFilter(tr("(*.dxf)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=true;
    ta.flags=exportEmpty+2*onlyInBoundary+4*onlyContours;
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_DXF;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportDxfBin()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as DXF Binary"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".dxf"));
  fileDialog->setNameFilter(tr("(*.dxf)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=false;
    ta.flags=exportEmpty+2*onlyInBoundary+4*onlyContours;
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_DXF;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportPlyTxt()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as PLY Text"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".ply"));
  fileDialog->setNameFilter(tr("(*.ply)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=true;
    ta.flags=exportEmpty+2*onlyInBoundary;
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_PLY;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportPlyBin()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as PLY Binary"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".ply"));
  fileDialog->setNameFilter(tr("(*.ply)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=false;
    ta.flags=exportEmpty+2*onlyInBoundary;
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_PLY;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportStlTxt()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as STL Text"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".stl"));
  fileDialog->setNameFilter(tr("(*.stl)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=true;
    ta.flags=true; // TODO choose whether to round down to a round scale
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_STL;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportStlBin()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as STL Binary"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".stl"));
  fileDialog->setNameFilter(tr("(*.stl)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.param0=false;
    ta.flags=true; // TODO choose whether to round down to a round scale
    ta.filename=fileName;
    ta.opcode=ACT_WRITE_STL;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportTinTxt()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as Text (AquaVeo)"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".tin"));
  fileDialog->setNameFilter(tr("(*.tin)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.filename=fileName;
    ta.flags=exportEmpty+2*onlyInBoundary;
    ta.opcode=ACT_WRITE_TIN;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportCarlsonTin()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as Carlson"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".tin"));
  fileDialog->setNameFilter(tr("(*.tin)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.filename=fileName;
    ta.flags=exportEmpty+2*onlyInBoundary;
    ta.opcode=ACT_WRITE_CARLSON_TIN;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::exportLandXml()
{
  int dialogResult;
  QStringList files;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Export TIN as LandXML"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(saveFileName+".xml"));
  fileDialog->setNameFilter(tr("(*.xml)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    fileName=files[0].toStdString();
    ta.param1=lengthUnit;
    ta.filename=fileName;
    ta.flags=exportEmpty+2*onlyInBoundary;
    ta.opcode=ACT_WRITE_LANDXML;
    enqueueAction(ta);
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::startConversion()
{
  int dialogResult;
  QStringList files;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Convert to TIN"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(noExt(lastFileName)+".ptin"));
  fileDialog->setNameFilter(tr("(*.ptin)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    saveFileName=files[0].toStdString();
    if (extension(saveFileName)==".ptin")
      saveFileName=noExt(saveFileName);
    net.swishFactor=0;
    ta.opcode=ACT_OCTAGON;
    enqueueAction(ta);
    clearLog();
    setBusy(BUSY_OCT|BUSY_CVT);
    updateContourIntervalActions();
    writtenTolerance=INFINITY;
    fileNames=baseName(saveFileName)+".ptin";
    fileMsg->setText(QString::fromStdString(fileNames));
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::stopConversion()
{
  setThreadCommand(TH_WAIT);
  conversionStopped=true;
  setIdle(BUSY_CVT);
}

void MainWindow::resumeConversion()
{
  if (conversionStopped)
  {
    setThreadCommand(TH_RUN);
    conversionStopped=false;
    setBusy(BUSY_CVT);
  }
}

void MainWindow::saveFile()
{
  ThreadAction ta;
  double toleranceRatio=stageTolerance/tolerance;
  ta.param1=tolerance;
  ta.param0=lrint(toleranceRatio);
  ta.param2=density;
  ta.opcode=ACT_WRITE_PTIN;
  ta.filename=saveFileName+".ptin";
  if (ta.param0==1 && ta.param1>0 && ta.param2>0 && saveFileName.length())
  {
    enqueueAction(ta);
    setBusy(BUSY_SAVE);
  }
}

void MainWindow::setColorScheme(int scheme)
{
  colorize.setScheme(scheme);
  tinSizeChanged();
  colorSchemeChanged(scheme);
}

void MainWindow::clearCloud()
{
  cloud.clear();
  canvas->clearContourFlags();
  fileNames=lastFileName="";
  fileMsg->setText("");
  setIdle(BUSY_CLD);
  density=0;
}

void MainWindow::deleteContours()
{
  int result=QMessageBox::Yes;
  if (net.currentContours && net.currentContours->size())
  {
    msgBox->setWindowTitle(tr("PerfectTIN"));
    msgBox->setIcon(QMessageBox::Question);
    msgBox->setText(tr("This contour interval has contours."));
    msgBox->setInformativeText(tr("Do you want to delete them?"));
    msgBox->setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    result=msgBox->exec();
  }
  if (result==QMessageBox::Yes)
    canvas->deleteContours();
}

void MainWindow::configure()
{
  configDialog->set(lengthUnit,tolerance,numberThreads,
		    exportEmpty,onlyContours,printer3d);
  configDialog->open();
}

void MainWindow::msgNoCloudArea()
{
  msgBox->warning(this,tr("PerfectTIN"),tr("Point cloud no area"));
}

void MainWindow::msgVerticalOutlier()
{
  msgBox->warning(this,tr("PerfectTIN"),tr("Vertical outlier"));
}

void MainWindow::handleResult(ThreadAction ta)
/* Receives the result of reading a file. If an error happened, pops up a message.
 * The file was read by a worker thread, which put the result in a queue.
 */
{
  QString message;
  int i;
  showingResult=true;
  tinSizeChanged();
  switch (ta.opcode)
  {
    case ACT_LOAD_START:
      setBusy(BUSY_LOAD);
      break;
    case ACT_LOAD:
      updateContourIntervalActions();
      setIdle(BUSY_LOAD);
      break;
    case ACT_WRITE_TIN_START:
      setBusy(BUSY_EXP);
      break;
    case ACT_WRITE_TIN:
      setIdle(BUSY_EXP);
      break;
    case ACT_WRITE_PTIN:
      setIdle(BUSY_SAVE);
      break;
    case ACT_READ_PTIN:
      if (ta.ptinResult.tolRatio>0 && ta.ptinResult.tolerance>0)
      {
	//cout<<"Finished reading ptin\n";
	tolerance=ta.ptinResult.tolerance;
	density=ta.ptinResult.density;
	stageTolerance=writtenTolerance=tolerance*ta.ptinResult.tolRatio;
	minArea=sqr(stageTolerance/tolerance)/densify/density;
	saveFileName=noExt(ta.filename);
	if (ta.ptinResult.tolRatio>1)
	{
	  conversionStopped=true;
	  resizeBuckets(1);
	  setBusy(BUSY_UNFIN);
	  if (extension(saveFileName)=="."+to_string(ta.ptinResult.tolRatio))
	    saveFileName=noExt(saveFileName);
	}
	else
	  setIdle(BUSY_UNFIN);
	fileNames=baseName(saveFileName)+".ptin";
	net.conversionTime=ta.ptinResult.conversionTime;
	ta.opcode=ACT_QINDEX;
	enqueueAction(ta);
      }
      else if (ta.ptinResult.tolRatio>0 && std::isnan(ta.ptinResult.tolerance))
	message=tr("File incomplete %1").arg(QString::fromStdString(ta.filename));
      else
      {
	saveFileName="";
	fileNames="";
	switch (ta.ptinResult.tolRatio)
	{
	  case PT_UNKNOWN_HEADER_FORMAT:
	    message=tr("Newer version %1").arg(QString::fromStdString(ta.filename));
	    break;
	  case PT_NOT_PTIN_FILE:
	  case PT_COUNT_MISMATCH:
	    message=tr("Not ptin file %1").arg(QString::fromStdString(ta.filename));
	    break;
	  default:
	    message=tr("File corrupt %1").arg(QString::fromStdString(ta.filename));
	}
      }
      if (message.length())
      {
	setIdle(BUSY_OPEN);
	msgBox->warning(this,tr("PerfectTIN"),message);
      }
      updateContourIntervalActions();
      break;
    case ACT_QINDEX:
      /* This results in a signal back to the MainWindow to set the checkmark
       * on the contour interval action with this contour interval.
       */
      canvas->setContourInterval(canvas->contourInterval);
      setIdle(BUSY_OPEN);
      break;
  }
  fileMsg->setText(QString::fromStdString(fileNames));
  showingResult=false;
}

void MainWindow::aboutProgram()
{
  QString progName=tr("PerfectTIN");
#ifdef Plytapus_FOUND
  QString rajotte=tr("\nPlytapus library version %1\nCopyright %2\nSimon Rajotte and Pierre Abbat\nMIT license")
  .arg(QString::fromStdString(plytapusVersion())).arg(plytapusYear());
#else
  QString rajotte("");
#endif
  QMessageBox::about(this,tr("PerfectTIN"),
		     tr("%1\nVersion %2\nCopyright %3 Pierre Abbat\nLicense LGPL 3 or later%4")
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
  int i;
  fileMenu=menuBar()->addMenu(tr("&File"));
  viewMenu=menuBar()->addMenu(tr("&View"));
  contourMenu=menuBar()->addMenu(tr("&Contour"));
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
  loadAction->setText(tr("Load cloud"));
  fileMenu->addAction(loadAction);
  connect(loadAction,SIGNAL(triggered(bool)),this,SLOT(loadFile()));
  loadBoundaryAction=new QAction(this);
  loadBoundaryAction->setIcon(QIcon::fromTheme("document-open"));
  loadBoundaryAction->setText(tr("Load boundary"));
  fileMenu->addAction(loadBoundaryAction);
  connect(loadBoundaryAction,SIGNAL(triggered(bool)),this,SLOT(loadBoundary()));
  convertAction=new QAction(this);
  convertAction->setIcon(QIcon::fromTheme("document-save-as"));
  convertAction->setText(tr("Convert"));
  fileMenu->addAction(convertAction);
  connect(convertAction,SIGNAL(triggered(bool)),this,SLOT(startConversion()));
  saveFileAction=new QAction(this);
  saveFileAction->setIcon(QIcon::fromTheme("document-save"));
  saveFileAction->setText(tr("Save"));
  fileMenu->addAction(saveFileAction);
  connect(saveFileAction,SIGNAL(triggered(bool)),this,SLOT(saveFile()));
  exportMenu=fileMenu->addMenu(tr("Export"));
  clearAction=new QAction(this);
  clearAction->setIcon(QIcon::fromTheme("edit-clear"));
  clearAction->setText(tr("Clear"));
  fileMenu->addAction(clearAction);
  connect(clearAction,SIGNAL(triggered(bool)),this,SLOT(clearCloud()));
  stopAction=new QAction(this);
  stopAction->setIcon(QIcon::fromTheme("process-stop"));
  stopAction->setText(tr("Stop"));
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
  // Export menu
  exportDxfTxtAction=new QAction(this);
  exportDxfTxtAction->setText(tr("DXF Text"));
  exportMenu->addAction(exportDxfTxtAction);
  connect(exportDxfTxtAction,SIGNAL(triggered(bool)),this,SLOT(exportDxfTxt()));
  exportDxfBinAction=new QAction(this);
  exportDxfBinAction->setText(tr("DXF Binary"));
  exportMenu->addAction(exportDxfBinAction);
  connect(exportDxfBinAction,SIGNAL(triggered(bool)),this,SLOT(exportDxfBin()));
#ifdef Plytapus_FOUND
  exportPlyTxtAction=new QAction(this);
  exportPlyTxtAction->setText(tr("PLY Text"));
  exportMenu->addAction(exportPlyTxtAction);
  connect(exportPlyTxtAction,SIGNAL(triggered(bool)),this,SLOT(exportPlyTxt()));
  exportPlyBinAction=new QAction(this);
  exportPlyBinAction->setText(tr("PLY Binary"));
  exportMenu->addAction(exportPlyBinAction);
  connect(exportPlyBinAction,SIGNAL(triggered(bool)),this,SLOT(exportPlyBin()));
#endif
  exportStlTxtAction=new QAction(this);
  exportStlTxtAction->setText(tr("STL Text"));
  exportMenu->addAction(exportStlTxtAction);
  connect(exportStlTxtAction,SIGNAL(triggered(bool)),this,SLOT(exportStlTxt()));
  exportStlBinAction=new QAction(this);
  exportStlBinAction->setText(tr("STL Binary"));
  exportMenu->addAction(exportStlBinAction);
  connect(exportStlBinAction,SIGNAL(triggered(bool)),this,SLOT(exportStlBin()));
  exportTinTxtAction=new QAction(this);
  exportTinTxtAction->setText(tr("TIN Text"));
  exportMenu->addAction(exportTinTxtAction);
  connect(exportTinTxtAction,SIGNAL(triggered(bool)),this,SLOT(exportTinTxt()));
  exportCarlsonTinAction=new QAction(this);
  exportCarlsonTinAction->setText(tr("Carlson TIN"));
  exportMenu->addAction(exportCarlsonTinAction);
  connect(exportCarlsonTinAction,SIGNAL(triggered(bool)),this,SLOT(exportCarlsonTin()));
  exportLandXmlAction=new QAction(this);
  exportLandXmlAction->setText(tr("LandXML"));
  exportMenu->addAction(exportLandXmlAction);
  connect(exportLandXmlAction,SIGNAL(triggered(bool)),this,SLOT(exportLandXml()));
  // View menu
  colorMenu=viewMenu->addMenu(tr("Color by"));
  // Color menu
  colorGradientAction=new ColorSchemeAction(this,CS_GRADIENT);
  colorGradientAction->setText(tr("Gradient"));
  colorMenu->addAction(colorGradientAction);
  connect(this,SIGNAL(colorSchemeChanged(int)),colorGradientAction,SLOT(setScheme(int)));
  connect(colorGradientAction,SIGNAL(triggered(bool)),colorGradientAction,SLOT(selfTriggered(bool)));
  connect(colorGradientAction,SIGNAL(schemeChanged(int)),this,SLOT(setColorScheme(int)));
  colorElevationAction=new ColorSchemeAction(this,CS_ELEVATION);
  colorElevationAction->setText(tr("Elevation"));
  colorMenu->addAction(colorElevationAction);
  connect(this,SIGNAL(colorSchemeChanged(int)),colorElevationAction,SLOT(setScheme(int)));
  connect(colorElevationAction,SIGNAL(triggered(bool)),colorElevationAction,SLOT(selfTriggered(bool)));
  connect(colorElevationAction,SIGNAL(schemeChanged(int)),this,SLOT(setColorScheme(int)));
  // Contour menu
  selectContourIntervalAction=new QAction(this);
  selectContourIntervalAction->setText(tr("Select contour interval"));
  contourMenu->addAction(selectContourIntervalAction);
  connect(selectContourIntervalAction,SIGNAL(triggered(bool)),canvas,SLOT(selectContourInterval()));
  roughContoursAction=new QAction(this);
  //roughContoursAction->setIcon(QIcon(":/roughcon.png"));
  roughContoursAction->setText(tr("Draw rough contours"));
  contourMenu->addAction(roughContoursAction);
  connect(roughContoursAction,SIGNAL(triggered(bool)),canvas,SLOT(roughContours()));
  smoothContoursAction=new QAction(this);
  //smoothContoursAction->setIcon(QIcon(":/smoothcon.png"));
  smoothContoursAction->setText(tr("Draw smooth contours"));
  contourMenu->addAction(smoothContoursAction);
  connect(smoothContoursAction,SIGNAL(triggered(bool)),canvas,SLOT(smoothContours()));
  deleteContoursAction=new QAction(this);
  //smoothContoursAction->setIcon(QIcon(":/deletecon.png"));
  deleteContoursAction->setText(tr("Delete contours"));
  contourMenu->addAction(deleteContoursAction);
  connect(deleteContoursAction,SIGNAL(triggered(bool)),this,SLOT(deleteContours()));
  contourMenu->addSeparator();
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
  // Toolbar
  for (i=0;i<4;i++)
  {
    unitButtons[i]=new UnitButton(this,conversionFactors[i]);
    unitButtons[i]->setText(configDialog->tr(unitNames[i])); // lupdate warns but it works
    unitButtons[i]->setIcon(QIcon(unitIconNames[i]));
    connect(this,SIGNAL(lengthUnitChanged(double)),unitButtons[i],SLOT(setUnit(double)));
    connect(unitButtons[i],SIGNAL(triggered(bool)),unitButtons[i],SLOT(selfTriggered(bool)));
    connect(unitButtons[i],SIGNAL(unitChanged(double)),this,SLOT(setUnit(double)));
    toolbar->addAction(unitButtons[i]);
  }
}

void MainWindow::makeStatusBar()
{
  statusBar()->addWidget(fileMsg);
  statusBar()->addWidget(dotTriangleMsg);
  statusBar()->addWidget(toleranceMsg);
  statusBar()->addWidget(densityMsg);
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
  lengthUnit=settings.value("lengthUnit",1).toDouble();
  exportEmpty=settings.value("exportEmpty",false).toBool();
  onlyContours=settings.value("onlyContours",false).toBool();
  onlyInBoundary=true; //TODO add to config
  colorize.setScheme(settings.value("colorScheme",CS_GRADIENT).toInt());
  printer3d.shape=settings.value("3dprinter/shape",1).toUInt();
  printer3d.x=settings.value("3dprinter/length",300).toDouble();
  printer3d.y=settings.value("3dprinter/width",300).toDouble();
  printer3d.z=settings.value("3dprinter/height",300).toDouble();
  printer3d.minBase=settings.value("3dprinter/base",10).toDouble();
  printer3d.scaleNum=settings.value("3dprinter/scaleNum",1).toUInt();
  printer3d.scaleDenom=settings.value("3dprinter/scaleDenom",1000).toUInt();
  canvas->contourInterval.setIntervalRatios(settings.value("contourInterval",1).toDouble(),
					    1,settings.value("contourRatio",5).toInt());
  canvas->contourInterval.setRelativeTolerance(settings.value("relativeTolerance",0.5).toDouble());
  lengthUnitChanged(lengthUnit);
  colorSchemeChanged(colorize.getScheme());
}

void MainWindow::writeSettings()
{
  QSettings settings("Bezitopo","PerfectTIN");
  settings.setValue("size",size());
  settings.setValue("pos",pos());
  settings.setValue("threads",numberThreads);
  settings.setValue("tolerance",tolerance);
  settings.setValue("lengthUnit",lengthUnit);
  settings.setValue("exportEmpty",exportEmpty);
  settings.setValue("onlyContours",onlyContours);
  settings.setValue("colorScheme",colorize.getScheme());
  settings.setValue("3dprinter/shape",printer3d.shape);
  settings.setValue("3dprinter/length",printer3d.x);
  settings.setValue("3dprinter/width",printer3d.y);
  settings.setValue("3dprinter/height",printer3d.z);
  settings.setValue("3dprinter/base",printer3d.minBase);
  settings.setValue("3dprinter/scaleNum",printer3d.scaleNum);
  settings.setValue("3dprinter/scaleDenom",printer3d.scaleDenom);
  settings.setValue("contourInterval",canvas->contourInterval.mediumInterval());
  settings.setValue("relativeTolerance",canvas->contourInterval.getRelativeTolerance());
  settings.setValue("contourRatio",(int)lrint(canvas->contourInterval.coarseInterval()/
					      canvas->contourInterval.mediumInterval()));
}

void MainWindow::setSettings(double lu,double tol,int thr,
			     bool ee,bool oc,Printer3dSize pri)
{
  lengthUnit=lu;
  tolerance=tol;
  numberThreads=thr;
  exportEmpty=ee;
  onlyContours=oc;
  printer3d=pri;
  writeSettings();
  lengthUnitChanged(lengthUnit);
}

void MainWindow::setUnit(double lu)
{
  lengthUnit=lu;
  lengthUnitChanged(lengthUnit);
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

bool MainWindow::bfl(int set,int clear)
/* Returns true if all bits in set are set and all bits in clear are clear
 * in the busy flag, and false otherwise.
 */
{
  assert((set&clear)==0);
  return ((busy^set)&(set|clear))==0;
}
