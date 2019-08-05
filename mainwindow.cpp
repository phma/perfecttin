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
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "PerfectTIN"));
  fileMsg=new QLabel(this);
  progressMsg=new QLabel(this);
  triangleMsg=new QLabel(this);
  canvas=new TinCanvas(this);
  configDialog=new ConfigurationDialog(this);
  connect(configDialog,SIGNAL(settingsChanged(double,double,double,int)),
	  this,SLOT(setSettings(double,double,double,int)));
  makeActions();
  makeStatusBar();
  setCentralWidget(canvas);
  tickCount=0;
  readSettings();
  canvas->show();
  show();
  timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(tick()));
  connect(timer,SIGNAL(timeout()),canvas,SLOT(tick()));
  timer->start(50);
  fileMsg->setText(QString("File loaded"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::tick()
{
  triangleMsg->setNum(++tickCount);
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
  //connect(loadAction,SIGNAL(triggered(bool)),this,SLOT(loadFile()));
  convertAction=new QAction(this);
  convertAction->setIcon(QIcon::fromTheme("document-save-as"));
  convertAction->setText(tr("Convert"));
  fileMenu->addAction(convertAction);
  //connect(convertAction,SIGNAL(triggered(bool)),this,SLOT(startConversion()));
  clearAction=new QAction(this);
  clearAction->setIcon(QIcon::fromTheme("edit-clear"));
  clearAction->setText(tr("Clear"));
  fileMenu->addAction(clearAction);
  //connect(clearAction,SIGNAL(triggered(bool)),this,SLOT(clearCloud()));
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
  statusBar()->addWidget(progressMsg);
  statusBar()->addWidget(triangleMsg);
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

