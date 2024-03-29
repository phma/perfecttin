/******************************************************/
/*                                                    */
/* configdialog.cpp - configuration dialog            */
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
#include <cmath>
#include "configdialog.h"
#include "ldecimal.h"
#include "threads.h"
#include "units.h"

using namespace std;

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
const char shapeNames[2][12]=
{
  QT_TRANSLATE_NOOP("Printer3dTab","Absolute"),
  QT_TRANSLATE_NOOP("Printer3dTab","Rectangular")
};

GeneralTab::GeneralTab(QWidget *parent):QWidget(parent)
{
  lengthUnitLabel=new QLabel(this);
  toleranceLabel=new QLabel(this);
  toleranceInUnit=new QLabel(this);
  threadLabel=new QLabel(this);
  threadDefault=new QLabel(this);
  lengthUnitBox=new QComboBox(this);
  toleranceBox=new QComboBox(this);
  threadInput=new QLineEdit(this);
  exportEmptyCheck=new QCheckBox(tr("Export empty"),this);
  onlyContourCheck=new QCheckBox(tr("Only contours"),this);
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
  gridLayout->addWidget(exportEmptyCheck,4,1);
  gridLayout->addWidget(onlyContourCheck,5,1);
}

Printer3dTab::Printer3dTab(QWidget *parent):QWidget(parent)
{
  int i;
  shapeLabel=new QLabel(this);
  shapeLabel->setText(tr("Shape"));
  lengthLabel=new QLabel(this);
  lengthLabel->setText(tr("Length"));
  widthLabel=new QLabel(this);
  widthLabel->setText(tr("Width"));
  heightLabel=new QLabel(this);
  heightLabel->setText(tr("Height"));
  baseLabel=new QLabel(this);
  baseLabel->setText(tr("Base"));
  scaleLabel=new QLabel(this);
  scaleLabel->setText(tr("Scale"));
  colonLabel=new QLabel(this);
  colonLabel->setText(tr(":"));
  for (i=0;i<sizeof(mmLabel)/sizeof(mmLabel[0]);i++)
  {
    mmLabel[i]=new QLabel(this);
    mmLabel[i]->setText(tr("mm"));
  }
  gridLayout=new QGridLayout(this);
  shapeBox=new QComboBox(this);
  for (i=0;i<sizeof(shapeNames)/sizeof(shapeNames[0]);i++)
  {
    shapeBox->addItem(tr(shapeNames[i]));
  }
  lengthInput=new QLineEdit(this);
  widthInput=new QLineEdit(this);
  heightInput=new QLineEdit(this);
  baseInput=new QLineEdit(this);
  scaleNumInput=new QLineEdit(this);
  scaleDenomInput=new QLineEdit(this);
  gridLayout->addWidget(shapeLabel,0,0,1,1);
  gridLayout->addWidget(shapeBox,0,1,1,3);
  gridLayout->addWidget(lengthLabel,1,0,1,1);
  gridLayout->addWidget(lengthInput,1,1,1,3);
  gridLayout->addWidget(mmLabel[0],1,4,1,1);
  gridLayout->addWidget(widthLabel,2,0,1,1);
  gridLayout->addWidget(widthInput,2,1,1,3);
  gridLayout->addWidget(mmLabel[1],2,4,1,1);
  gridLayout->addWidget(heightLabel,3,0,1,1);
  gridLayout->addWidget(heightInput,3,1,1,3);
  gridLayout->addWidget(mmLabel[2],3,4,1,1);
  gridLayout->addWidget(baseLabel,4,0,1,1);
  gridLayout->addWidget(baseInput,4,1,1,3);
  gridLayout->addWidget(mmLabel[3],4,4,1,1);
  gridLayout->addWidget(scaleLabel,5,0,1,1);
  gridLayout->addWidget(scaleNumInput,5,1,1,1);
  gridLayout->addWidget(colonLabel,5,2,1,1);
  gridLayout->addWidget(scaleDenomInput,5,3,1,1);
  connect(shapeBox,SIGNAL(currentIndexChanged(int)),this,SLOT(disableSome()));
  connect(lengthInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(widthInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(heightInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(baseInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(scaleNumInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(scaleDenomInput,SIGNAL(textChanged(QString)),this,SIGNAL(contentChanged()));
  connect(shapeBox,SIGNAL(currentIndexChanged(int)),this,SIGNAL(contentChanged()));
}

bool Printer3dTab::isValid()
{
  switch (shapeBox->currentIndex())
  {
    case P3S_ABSOLUTE:
      return scaleNumInput->text().toUInt()>0 &&
	     scaleDenomInput->text().toUInt()>0 &&
	     baseInput->text().toDouble()>0;
    case P3S_RECTANGULAR:
      return lengthInput->text().toDouble()>0 &&
	     widthInput->text().toDouble()>0 &&
	     baseInput->text().toDouble()>0 &&
	     heightInput->text().toDouble()>baseInput->text().toDouble();
    default:
      return false;
  }
}

void Printer3dTab::disableSome()
{
  switch (shapeBox->currentIndex())
  {
    case P3S_ABSOLUTE:
      lengthInput->setEnabled(false);
      widthInput->setEnabled(false);
      heightInput->setEnabled(false);
      baseInput->setEnabled(true);
      scaleNumInput->setEnabled(true);
      scaleDenomInput->setEnabled(true);
      break;
    case P3S_RECTANGULAR:
      lengthInput->setEnabled(true);
      widthInput->setEnabled(true);
      heightInput->setEnabled(true);
      baseInput->setEnabled(true);
      scaleNumInput->setEnabled(false);
      scaleDenomInput->setEnabled(false);
      break;
  }
}

ConfigurationDialog::ConfigurationDialog(QWidget *parent):QDialog(parent)
{
  boxLayout=new QVBoxLayout(this);
  tabWidget=new QTabWidget(this);
  general=new GeneralTab(this);
  printTab=new Printer3dTab(this);
  buttonBox=new QDialogButtonBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  buttonBox->addButton(okButton,QDialogButtonBox::AcceptRole);
  buttonBox->addButton(cancelButton,QDialogButtonBox::RejectRole);
  boxLayout->addWidget(tabWidget);
  boxLayout->addWidget(buttonBox);
  tabWidget->addTab(general,tr("General"));
  tabWidget->addTab(printTab,tr("3D Printer"));
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
  connect(general->lengthUnitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateToleranceConversion()));
  connect(general->toleranceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateToleranceConversion()));
  connect(printTab,SIGNAL(contentChanged()),this,SLOT(checkValid()));
}

void ConfigurationDialog::set(double lengthUnit,double tolerance,int threads,
			      bool exportEmpty,bool onlyContours,Printer3dSize printer)
{
  int i;
  general->lengthUnitBox->clear();
  general->toleranceBox->clear();
  general->lengthUnitLabel->setText(tr("Length unit"));
  general->toleranceLabel->setText(tr("Tolerance"));
  general->threadLabel->setText(tr("Threads:"));
  general->threadDefault->setText(tr("default is %n","",thread::hardware_concurrency()));
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    general->lengthUnitBox->addItem(tr(unitNames[i]));
  }
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    if (lengthUnit==conversionFactors[i])
      general->lengthUnitBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    general->toleranceBox->addItem(tr(toleranceStr[i]));
  for (i=0;i<sizeof(tolerances)/sizeof(tolerances[1]);i++)
    if (tolerance==tolerances[i])
      general->toleranceBox->setCurrentIndex(i);
  general->threadInput->setText(QString::number(threads));
  general->exportEmptyCheck->setCheckState(exportEmpty?Qt::Checked:Qt::Unchecked);
  general->onlyContourCheck->setCheckState(onlyContours?Qt::Checked:Qt::Unchecked);
  for (i=0;i<sizeof(shapeNames)/sizeof(shapeNames[1]);i++)
    if (printer.shape==i)
      printTab->shapeBox->setCurrentIndex(i);
  printTab->lengthInput->setText(QString::number(printer.x));
  printTab->widthInput->setText(QString::number(printer.y));
  printTab->heightInput->setText(QString::number(printer.z));
  printTab->baseInput->setText(QString::number(printer.minBase));
  printTab->scaleNumInput->setText(QString::number(printer.scaleNum));
  printTab->scaleDenomInput->setText(QString::number(printer.scaleDenom));
}

void ConfigurationDialog::updateToleranceConversion()
{
  double tolIn;
  tolIn=tolerances[general->toleranceBox->currentIndex()]/conversionFactors[general->lengthUnitBox->currentIndex()];
  if (std::isfinite(tolIn))
  {
    general->toleranceInUnit->setText(QString::fromStdString(ldecimal(tolIn,tolIn/1000)));
  }
}

void ConfigurationDialog::checkValid()
{
  okButton->setEnabled(printTab->isValid());
}

void ConfigurationDialog::accept()
{
  Printer3dSize pri;
  pri.shape=printTab->shapeBox->currentIndex();
  pri.x=printTab->lengthInput->text().toDouble();
  pri.y=printTab->widthInput->text().toDouble();
  pri.z=printTab->heightInput->text().toDouble();
  pri.minBase=printTab->baseInput->text().toDouble();
  pri.scaleNum=printTab->scaleNumInput->text().toUInt();
  pri.scaleDenom=printTab->scaleDenomInput->text().toUInt();
  settingsChanged(conversionFactors[general->lengthUnitBox->currentIndex()],
		  tolerances[general->toleranceBox->currentIndex()],
		  general->threadInput->text().toInt(),
		  general->exportEmptyCheck->checkState()>0,
		  general->onlyContourCheck->checkState()>0,
		  pri);
  QDialog::accept();
}
