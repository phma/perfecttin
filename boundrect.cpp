/******************************************************/
/*                                                    */
/* boundrect.cpp - bounding rectangles                */
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
#include <cmath>
#include "angle.h"
#include "boundrect.h"

BoundRect::BoundRect()
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
  orientation=0;
}

BoundRect::BoundRect(int ori)
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
  orientation=ori;
}

void BoundRect::clear()
{
  int i;
  for (i=0;i<6;i++)
    bounds[i]=INFINITY;
}

void BoundRect::setOrientation(int ori)
{
  orientation=ori;
}

int BoundRect::getOrientation()
{
  return orientation;
}

void BoundRect::include(xyz obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=xy(obj).dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
  if (obj.elev()<bounds[4])
    bounds[4]=obj.elev();
  if (-obj.elev()<bounds[5])
    bounds[5]=-obj.elev();
}

void BoundRect::include(pointlist *obj)
{
  int i;
  double newbound,elev;
  for (i=0;i<4;i++)
  {
    newbound=obj->dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
  for (i=1;i<=obj->points.size();i++)
  {
    elev=obj->points[i].elev();
    if (elev<bounds[4])
      bounds[4]=elev;
    if (-elev<bounds[5])
      bounds[5]=-elev;
  }
}

