/******************************************************/
/*                                                    */
/* csaction.cpp - color scheme menu items             */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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
#include "csaction.h"

using namespace std;

ColorSchemeAction::ColorSchemeAction(QObject *parent,int sch):QAction(parent)
{
  scheme=sch;
  setCheckable(true);
}

void ColorSchemeAction::setScheme(int sch)
{
  setChecked(sch==scheme);
}

void ColorSchemeAction::selfTriggered(bool dummy)
{
  schemeChanged(scheme);
}
