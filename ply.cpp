/******************************************************/
/*                                                    */
/* ply.cpp - polygon files                            */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of Decisite.
 * 
 * Decisite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Decisite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decisite. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libplyxx.h>
#include "ply.h"
#include "cloud.h"

using namespace std;
using namespace libply;

void receivePoint(ElementBuffer &buf)
{
  xyz pnt(buf[0],buf[1],buf[2]);
  cloud.push_back(pnt);
}

void readPly(string fileName)
{
  try
  {
    File plyfile(fileName);
    ElementReadCallback pointCallback=receivePoint;
    plyfile.setElementReadCallback("vertex",pointCallback);
    plyfile.read();
  }
  catch (...)
  {
  }
}

