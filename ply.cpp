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

vector<triangle *> trianglesToWrite;
double plyUnit;
xyz plyOffset;
bool centerPlyOut=false;

string plytapusVersion()
{ // plytapus.h includes its own config.h which redefines VERSION
  return VERSION;
}

void receivePoint(ElementBuffer &buf)
{
  if (buf.size()==3)
  {
    xyz pnt(buf[0],buf[1],buf[2]);
    cloud.push_back(pnt);
  }
}

void transmitPoint(ElementBuffer &buf,size_t i)
{
  xyz pnt=net.points[i+1]-plyOffset;
  buf[0]=pnt.getx()/plyUnit;
  buf[1]=pnt.gety()/plyUnit;
  buf[2]=pnt.getz()/plyUnit;
}

void transmitTriangle(ElementBuffer &buf,size_t i)
{
  triangle *tri=trianglesToWrite[i];
  buf.reset(3);
  buf[0]=net.revpoints[tri->a]-1;
  buf[1]=net.revpoints[tri->b]-1;
  buf[2]=net.revpoints[tri->c]-1;
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
  int i;
  plyOffset=xyz(0,0,0);
  if (centerPlyOut)	// This is to get around a bug in MeshLab
    for (i=1;i<=8;i++)	// where files are read in single precision.
      plyOffset+=net.points[i];
  plyOffset/=8;
  trianglesToWrite.clear();
  plyUnit=outUnit;
  for (i=0;i<net.triangles.size();i++)
    if (net.triangles[i].ptValid())
      if (net.triangles[i].dots.size() || (flags&1))
	trianglesToWrite.push_back(&net.triangles[i]);
      else;
    else
      cerr<<"Invalid triangle "<<i<<endl;
  Element vertexElement("vertex",net.points.size(),vertexProperties);
  Element faceElement("face",trianglesToWrite.size(),faceProperties);
  FileOut plyfile(filename,asc?File::Format::ASCII:File::Format::BINARY_LITTLE_ENDIAN);
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

string plytapusVersion()
{
  return "";
}

void readPly(string fileName)
{
}

void writePly(string fileName,bool asc,double outUnit,int flags)
{
}
#endif
