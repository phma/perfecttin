/******************************************************/
/*                                                    */
/* gui.cpp - GUI main program                         */
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
#include <QtGui>
#include <QtWidgets>
#include <QTranslator>
#include <iostream>
#include "point.h"
#include "octagon.h"
#include "config.h"
#include "mainwindow.h"
#include "threads.h"

using namespace std;

int main(int argc, char *argv[])
{
  int exitStatus;
  int i,sz;
  QApplication app(argc, argv);
  QTranslator translator,qtTranslator;
  int nthreads;
  if (qtTranslator.load(QLocale(),QLatin1String("qt"),QLatin1String("_"),
                        QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    app.installTranslator(&qtTranslator);
  if (translator.load(QLocale(),QLatin1String("perfecttin"),
                      QLatin1String("_"),QLatin1String(".")))
  {
    //cout<<"Translations found in current directory"<<endl;
    app.installTranslator(&translator);
  }
  else if (translator.load(QLocale(),QLatin1String("perfecttin"),
                      QLatin1String("_"),QLatin1String(SHARE_DIR)))
  {
    //cout<<"Translations found in share directory"<<endl;
    app.installTranslator(&translator);
  }
  MainWindow window;
  nthreads=window.getNumberThreads();
  if (nthreads<1)
    nthreads=boost::thread::hardware_concurrency();
  if (nthreads<1)
    nthreads=2;
  startThreads(nthreads);
  window.show();
  exitStatus=app.exec();
  waitForThreads(TH_STOP);
  joinThreads();
  sz=net.convexHull.size();
  for (i=0;i<sz;i++)
  {
    cout<<dist((xy)*net.convexHull[i],(xy)*net.convexHull[(i+1)%sz])<<' '<<hex;
    cout<<dir((xy)*net.convexHull[i],(xy)*net.convexHull[(i+1)%sz])<<'\n'<<dec;
  }
  return exitStatus;
}
