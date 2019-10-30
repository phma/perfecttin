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
#include <cmath>
#include "configdialog.h"
#include "ldecimal.h"
#include "threads.h"

using namespace std;

const double conversionFactors[4]={1,0.3048,12e2/3937,0.3047996};
const char unitNames[4][12]=
{
  QT_TRANSLATE_NOOP("ConfigurationDialog","Meter"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","Int'l foot"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","US foot"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","Indian foot")
};
const double tolerances[10]={1e-2,25e-3,5e-2,1e-1,15e-2,2e-1,1/3.,2/3.,1.,10/3.};
const char toleranceStr[10][7]=
{
  "10 mm","25 mm","50 mm","100 mm","150 mm","200 mm","1/3 m","2/3 m","1 m","10/3 m"
};

ConfigurationDialog::ConfigurationDialog(QWidget *parent):QDialog(parent)
{
  int i;
  lengthUnitLabel=new QLabel(this);
  outUnitLabel=new QLabel(this);
  toleranceLabel=new QLabel(this);
  toleranceInUnit=new QLabel(this);
  toleranceOutUnit=new QLabel(this);
  threadLabel=new QLabel(this);
  threadDefault=new QLabel(this);
  lengthUnitBox=new QComboBox(this);
  outUnitBox=new QComboBox(this);
  toleranceBox=new QComboBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  threadInput=new QLineEdit(this);
  dxfCheck=new QCheckBox(this);
  sameCheck=new QCheckBox(this);
  gridLayout=new QGridLayout(this);
  setLayout(gridLayout);
  gridLayout->addWidget(lengthUnitLabel,1,0);
  gridLayout->addWidget(lengthUnitBox,1,1);
  gridLayout->addWidget(toleranceLabel,2,0);
  gridLayout->addWidget(toleranceBox,2,1);
  gridLayout->addWidget(toleranceInUnit,2,2);
  gridLayout->addWidget(threadLabel,3,0);
  gridLayout->addWidget(threadInput,3,1);
  gridLayout->addWidget(threadDefault,3,2);
  gridLayout->addWidget(okButton,4,0);
  gridLayout->addWidget(cancelButton,4,1);
  toleranceLabel->setAlignment(Qt::AlignHCenter);
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
  connect(lengthUnitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSameUnits(int)));
  connect(outUnitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSameUnits(int)));
  connect(lengthUnitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateToleranceConversion()));
  connect(outUnitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateToleranceConversion()));
  connect(toleranceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateToleranceConversion()));
}

void ConfigurationDialog::set(double lengthUnit,double tolerance,int threads)
{
  int i;
  lengthUnitBox->clear();
  outUnitBox->clear();
  toleranceBox->clear();
  lengthUnitLabel->setText(tr("Input unit"));
  sameCheck->setText(tr("Same units"));
  outUnitLabel->setText(tr("Output unit"));
  toleranceLabel->setText(tr("Tolerance"));
  threadLabel->setText(tr("Threads:"));
  threadDefault->setText(tr("default is %n","",thread::hardware_concurrency()));
  dxfCheck->setText(tr("Text mode DXF"));
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    lengthUnitBox->addItem(tr(unitNames[i]));
    outUnitBox->addItem(tr(unitNames[i]));
  }
  sameCheck->setCheckState(Qt::Unchecked);
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    if (lengthUnit==conversionFactors[i])
      lengthUnitBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    toleranceBox->addItem(tr(toleranceStr[i]));
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    if (tolerance==tolerances[i])
      toleranceBox->setCurrentIndex(i);
  threadInput->setText(QString::number(threads));
}

void ConfigurationDialog::updateToleranceConversion()
{
  double tolIn,tolOut;
  tolIn=tolerances[toleranceBox->currentIndex()]/conversionFactors[lengthUnitBox->currentIndex()];
  tolOut=tolerances[toleranceBox->currentIndex()]/conversionFactors[outUnitBox->currentIndex()];
  if (std::isfinite(tolIn) && std::isfinite(tolOut))
  {
    toleranceInUnit->setText(QString::fromStdString(ldecimal(tolIn,tolIn/1000)));
    toleranceOutUnit->setText(QString::fromStdString(ldecimal(tolOut,tolOut/1000)));
  }
}

void ConfigurationDialog::updateSameUnits(int index)
{
  if (sameCheck->checkState())
  {
    lengthUnitBox->setCurrentIndex(index);
    outUnitBox->setCurrentIndex(index);
  }
}

void ConfigurationDialog::accept()
{
  settingsChanged(conversionFactors[lengthUnitBox->currentIndex()],
		  tolerances[toleranceBox->currentIndex()],
		  threadInput->text().toInt());
  QDialog::accept();
}
