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
void aster(int n);
void ring(int n);
void regpolygon(int n);
void ellipse(int n);
void longandthin(int n);
void straightrow(int n);
void lozenge(int n);
void wheelwindow(int n);
//void rotate(int n);
//void movesideways(double sw);
//void moveup(double sw);
//void enlarge(double sc);
extern xy (*testsurfacegrad)(xy pnt);
