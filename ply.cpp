/******************************************************/
/*                                                    */
/* ply.cpp - polygon files                            */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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

#include "config.h"
#ifdef Plytapus_FOUND
#include <plytapus.h>
#endif
#include "ply.h"
#include "cloud.h"

using namespace std;
#ifdef Plytapus_FOUND
using namespace plytapus;

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

void writePly(string filename)
{
  vector<Property> vertexProperties,faceProperties;
  vertexProperties.push_back(Property("x",Type::DOUBLE,false));
  vertexProperties.push_back(Property("y",Type::DOUBLE,false));
  vertexProperties.push_back(Property("z",Type::DOUBLE,false));
  faceProperties.push_back(Property("vertex_index",Type::INT,true));
  
}
#else
void readPly(string fileName)
{
}
void writePly(string fileName)
{
}
#endif
