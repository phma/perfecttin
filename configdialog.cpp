/******************************************************/
/*                                                    */
/* configdialog.cpp - configuration dialog            */
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
#include "configdialog.h"

const double conversionFactors[4]={1,0.3048,12e2/3937,0.3047996};
const char unitNames[4][12]={"Meter","Int'l foot","US foot","Indian foot"};
const double tolerances[10]={1e-2,25e-3,5e-2,1e-1,15e-2,2e-1,1/3.,2/3.,1.,10/3.};
const char toleranceStr[10][7]=
{
  "10 mm","25 mm","50 mm","100 mm","150 mm","200 mm","1/3 m","2/3 m","1 m","10/3 m"
};

ConfigurationDialog::ConfigurationDialog(QWidget *parent):QDialog(parent)
{
  int i;
  inUnitBox=new QComboBox(this);
  outUnitBox=new QComboBox(this);
  toleranceBox=new QComboBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  threadInput=new QLineEdit(this);
  gridLayout=new QGridLayout(this);
  setLayout(gridLayout);
  gridLayout->addWidget(inUnitBox,1,0);
  gridLayout->addWidget(outUnitBox,1,2);
  gridLayout->addWidget(toleranceBox,2,1);
  gridLayout->addWidget(threadInput,3,1);
  gridLayout->addWidget(okButton,4,0);
  gridLayout->addWidget(cancelButton,4,2);
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void ConfigurationDialog::set(double inUnit,double outUnit,double tolerance,int threads)
{
  int i;
  inUnitBox->clear();
  outUnitBox->clear();
  toleranceBox->clear();
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    inUnitBox->addItem(tr(unitNames[i]));
    outUnitBox->addItem(tr(unitNames[i]));
  }
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    if (inUnit==conversionFactors[i])
      inUnitBox->setCurrentIndex(i);
    if (outUnit==conversionFactors[i])
      outUnitBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    toleranceBox->addItem(tr(toleranceStr[i]));
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    if (tolerance==tolerances[i])
      toleranceBox->setCurrentIndex(i);
  threadInput->setText(QString::number(threads));
}

void ConfigurationDialog::accept()
{
  QDialog::accept();
}
