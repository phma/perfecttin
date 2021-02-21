/******************************************************/
/*                                                    */
/* xyzfile.cpp - XYZ point cloud files                */
/*                                                    */
/******************************************************/
/* Copyright 2021 Pierre Abbat.
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
#include <fstream>
#include "xyzfile.h"
#include "cloud.h"
using namespace std;

/* There is no specification for XYZ point cloud files. They are plain text,
 * with the first three fields being x, y, and z, but the fields after that
 * could be anything, and the separator could be spaces, comma, or tab.
 * There is also a chemical XYZ file format, in which the first field is
 * a chemical element symbol (alphabetic, e.g. F or Ne).
 */

void readXyzText(string fname)
{
  ifstream xyzfile(fname);
}
