/******************************************************/
/*                                                    */
/* cidialog.cpp - contour interval dialog             */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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
#include <iostream>
#include "cidialog.h"
using namespace std;

const double tol[]=
{
  0.1,0.2,1/3.,0.5
};
const char tolStr[4][5]=
{
  "1/10","1/5","1/3","1/2"
};

ContourIntervalDialog::ContourIntervalDialog(QWidget *parent):QDialog(parent)
{
  currentInterval=new QLabel(tr("None"),this);
  intervalBox=new QComboBox(this);
  toleranceBox=new QComboBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  gridLayout=new QGridLayout(this);
  setLayout(gridLayout);
  gridLayout->addWidget(currentInterval,0,0);
  gridLayout->addWidget(intervalBox,0,1);
  gridLayout->addWidget(toleranceBox,1,1);
  gridLayout->addWidget(okButton,2,0);
  gridLayout->addWidget(cancelButton,2,1);
  contourInterval=nullptr;
  okButton->setEnabled(false);
  okButton->setDefault(true);
  connect(intervalBox,SIGNAL(currentIndexChanged(int)),this,SLOT(selectInterval(int)));
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void ContourIntervalDialog::set(ContourInterval *ci,double unit)
{
  bool precise;
  int i,closeInx=-1;
  ContourInterval temp;
  double mantissa,closeDiff=INFINITY;
  contourInterval=ci;
  intervalBox->clear();
  toleranceBox->clear();
  ciList.clear();
  if (ci)
  {
    /* The Indian and US feet are 1.3 ppm less and 2 ppm more than the
     * international foot. If the current contour interval differs by more
     * than 1.15 ppm from 1, 2, or 5 times a power of 10 in the current unit,
     * display it with six more digits of precision to show that it's in
     * a different unit.
     */
    mantissa=frac(log10(ci->mediumInterval()/unit));
    precise=fabs(mantissa)>0.0000005 && fabs(mantissa-0.30103)>0.0000005 &&
            fabs(mantissa-0.69897)>0.0000005 && fabs(mantissa-1)>0.0000005;
    currentInterval->setText(QString::fromStdString(ci->valueString(unit,precise)));
    okButton->setEnabled(true);
    for (i=rint(3*log10(MININTERVAL/3/unit));
         i<=rint(3*log10(MAXINTERVAL*3/unit));i++)
    {
      temp=ContourInterval(unit,i,false);
      if (fabs(log(ci->mediumInterval()/temp.mediumInterval()))<closeDiff)
      {
        closeDiff=fabs(log(ci->mediumInterval()/temp.mediumInterval()));
        closeInx=intervalBox->count();
      }
      if (temp.mediumInterval()>=MININTERVAL && temp.mediumInterval()<=MAXINTERVAL)
      {
        ciList.push_back(temp);
        intervalBox->addItem(QString::fromStdString(temp.valueString(unit,false)));
      }
    }
    intervalBox->setCurrentIndex(closeInx);
    for (i=0;i<sizeof(tol)/sizeof(tol[0]);i++)
    {
      toleranceBox->addItem(QString::fromStdString(tolStr[i]));
      if (relTol==tol[i])
	toleranceBox->setCurrentIndex(i);
    }
  }
  else
  {
    currentInterval->setText(tr("None"));
    okButton->setEnabled(false);
  }
}

void ContourIntervalDialog::selectInterval(int n)
{
  if (n>=0 && n<ciList.size())
  {
    //cout<<n<<" selected"<<endl;
    selectedInterval=ciList[n];
  }
}

void ContourIntervalDialog::accept()
{
  if (contourInterval)
    *contourInterval=selectedInterval;
  QDialog::accept();
}
