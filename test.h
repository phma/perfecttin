/******************************************************/
/*                                                    */
/* test.h - test patterns and functions               */
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

#include "pointlist.h"

#define RUGAE 0
#define HYPAR 1
#define CIRPAR 2
#define FLATSLOPE 3
#define HASH 4
void setsurface(int surf);
void dumppoints();
void dumppointsvalence(pointlist &pl);
void aster(pointlist &pl,int n);
void ring(pointlist &pl,int n);
void regpolygon(pointlist &pl,int n);
void ellipse(pointlist &pl,int n);
void longandthin(pointlist &pl,int n);
void straightrow(pointlist &pl,int n);
void lozenge(pointlist &pl,int n);
void wheelwindow(pointlist &pl,int n);
void rotate(pointlist &pl,int n);
void movesideways(pointlist &pl,double sw);
void moveup(pointlist &pl,double sw);
void enlarge(pointlist &pl,double sc);
extern xy (*testsurfacegrad)(xy pnt);
