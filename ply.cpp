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
#include "octagon.h"

using namespace std;
#ifdef Plytapus_FOUND
using namespace plytapus;

void receivePoint(ElementBuffer &buf)
{
  xyz pnt(buf[0],buf[1],buf[2]);
  cloud.push_back(pnt);
}

void transmitPoint(ElementBuffer &buf,size_t i)
{
  buf[0]=net.points[i].getx();
  buf[1]=net.points[i].gety();
  buf[2]=net.points[i].getz();
}

void transmitTriangle(ElementBuffer &buf,size_t i)
{
  triangle *tri=&net.triangles[i];
  buf.reset(3);
  buf[0]=net.revpoints[tri->a];
  buf[1]=net.revpoints[tri->b];
  buf[2]=net.revpoints[tri->c];
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

void writePly(string filename,bool asc,double outUnit,int flags)
{
  vector<Property> vertexProperties,faceProperties;
  vertexProperties.push_back(Property("x",Type::DOUBLE,false));
  vertexProperties.push_back(Property("y",Type::DOUBLE,false));
  vertexProperties.push_back(Property("z",Type::DOUBLE,false));
  faceProperties.push_back(Property("vertex_index",Type::INT,true));
  Element vertexElement("vertex",net.points.size(),vertexProperties);
  Element faceElement("face",net.triangles.size(),faceProperties);
  FileOut plyfile(filename,File::Format::BINARY_LITTLE_ENDIAN);
  ElementWriteCallback pointCallback=transmitPoint;
  ElementWriteCallback triangleCallback=transmitTriangle;
  ElementsDefinition elements;
  elements.push_back(vertexElement);
  elements.push_back(faceElement);
  plyfile.setElementsDefinition(elements);
  plyfile.setElementWriteCallback("vertex",pointCallback);
  plyfile.setElementWriteCallback("face",triangleCallback);
  plyfile.write();
}
#else
void readPly(string fileName)
{
}
void writePly(string fileName)
{
}
#endif
