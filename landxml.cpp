/******************************************************/
/*                                                    */
/* landxml.cpp - output TIN in LandXML                */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
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
#include <ctime>
#include <iomanip>
#include "config.h"
#include "octagon.h"
#include "ldecimal.h"
using namespace std;

void writeLandXml(string outputFile,double outUnit,int flags)
{
  int i;
  ofstream xmlFile(outputFile,ofstream::trunc);
  tm *convtm;
  if (outUnit==0.3047996)
  {
    cerr<<"Indian survey foot is not supported in LandXML. Writing in meters.\n";
    outUnit=1;
  }
  xmlFile<<"<?xml version=\"1.0\" ?>\n";
  convtm=gmtime(&net.conversionTime);
  /* The LandXML header has no field for time zone, so the time is output in UTC.
   * It is the time at which conversion to TIN started.
   */
  xmlFile<<"<LandXML xmlns=\"http://www.landxml.org/schema/LandXML-1.2\" ";
  xmlFile<<"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
  xmlFile<<"xsi:schemaLocation=\"http://www.landxml.org/schema/LandXML-1.2 http://www.landxml.org/schema/LandXML-1.2/LandXML-1.2.xsd\" ";
  xmlFile<<"version=\"1.2\" date=\""<<put_time(convtm,"%F")<<'"';
  xmlFile<<" time=\""<<put_time(convtm,"%T")<<"\">\n";
  xmlFile<<"<Application name=\"PerfectTIN\" desc=\"LandXML export\" ";
  xmlFile<<"version=\""<<VERSION<<"\">\n";
  xmlFile<<"<Author createdBy=\"Pierre Abbat\" createdByEmail=\"phma@bezitopo.org\" /></Application>\n";
  if (outUnit==1)
  {
    xmlFile<<"<Units><Metric areaUnit=\"squareMeter\" linearUnit=\"meter\" ";
    xmlFile<<"volumeUnit=\"cubicMeter\" temperatureUnit=\"kelvin\" ";
    xmlFile<<"pressureUnit=\"HPA\" /></Units>\n";
  }
  else
  {
    xmlFile<<"<Units><Imperial areaUnit=\"squareFoot\" linearUnit=\"";
    if (outUnit==0.3048)
      xmlFile<<"foot\" ";
    else
      xmlFile<<"USSurveyFoot\" ";
    xmlFile<<"volumeUnit=\"cubicFeet\" temperatureUnit=\"kelvin\" ";
    xmlFile<<"pressureUnit=\"inHG\" /></Units>\n";
  }
  xmlFile<<"<Surfaces><Surface name=\""<<noExt(baseName(outputFile))<<"\">\n";
  xmlFile<<"<Definition surfType=\"TIN\"><Pnts>\n";
  for (i=1;i<=net.points.size();i++)
  {
    xmlFile<<"<P id=\""<<i<<"\">";
    xmlFile<<ldecimal(net.points[i].getx()/outUnit)<<' ';
    xmlFile<<ldecimal(net.points[i].gety()/outUnit)<<' ';
    xmlFile<<ldecimal(net.points[i].getz()/outUnit)<<"</P>\n";
  }
  xmlFile<<"</Pnts><Faces>\n";
  for (i=0;i<net.triangles.size();i++)
    if ((flags&1) || net.triangles[i].dots.size())
    {
      xmlFile<<"<F>";
      xmlFile<<net.revpoints[net.triangles[i].a]<<' ';
      xmlFile<<net.revpoints[net.triangles[i].b]<<' ';
      xmlFile<<net.revpoints[net.triangles[i].c]<<"</F>\n";
    }
  xmlFile<<"</Faces></Definition></Surface></Surfaces>\n";
  xmlFile<<"</LandXML>\n";
}
