/******************************************************/
/*                                                    */
/* edgeop.h - edge operation                          */
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
#include "tin.h"

void initTempPointlist(int nthreads);
void flip(edge *e);
point *bend(edge *e);
void dealDots(triangle *tri0,triangle *tri1,triangle *tri2=nullptr,triangle *tri3=nullptr);
int edgeop(edge *e,double tolerance,double minArea,int thread);
