/******************************************************/
/*                                                    */
/* boundrect.cpp - bounding rectangles                */
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
#include "boundrect.h"

BoundRect::BoundRect()
{
  int i;
  for (i=0;i<4;i++)
    bounds[i]=INFINITY;
  orientation=0;
}

BoundRect::BoundRect(int ori)
{
  int i;
  for (i=0;i<4;i++)
    bounds[i]=INFINITY;
  orientation=ori;
}

void BoundRect::clear()
{
  int i;
  for (i=0;i<4;i++)
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

void BoundRect::include(xy obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj.dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
}

void BoundRect::include(drawobj *obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj->dirbound(i*DEG90-orientation,bounds[i]);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
}

#ifdef POINTLIST
void BoundRect::include(pointlist *obj)
{
  int i;
  double newbound;
  for (i=0;i<4;i++)
  {
    newbound=obj->dirbound(i*DEG90-orientation);
    if (newbound<bounds[i])
      bounds[i]=newbound;
  }
}
#endif

