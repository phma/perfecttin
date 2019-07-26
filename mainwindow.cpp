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
  setWindowTitle(QApplication::translate("main", "ViewTIN"));
  fileMsg=new QLabel(this);
  progressMsg=new QLabel(this);
  triangleMsg=new QLabel(this);
  makeActions();
  makeStatusBar();
  tickCount=0;
  readSettings();
  show();
  timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(tick()));
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
