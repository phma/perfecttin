/******************************************************/
/*                                                    */
/* angle.h - angles as binary fractions of rotation   */
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

// Angles are represented as integers, with 2147483648 representing 360°.
// This file overloads the functions sin and cos.

#ifndef ANGLE_H
#define ANGLE_H
#include <cmath>
#include <string>

class xy;

#include "point.h"

#ifndef M_PIl
#define M_PIl M_PI
#endif

double sqr(double x);
double cub(double x);
double frac(double x);

double sin(int angle);
double cos(int angle);
double tan(int angle);
double cot(int angle);
double sinhalf(int angle);
double coshalf(int angle);
double tanhalf(int angle);
double cosquarter(int angle);
double tanquarter(int angle);
int atan2i(double y,double x);
int atan2i(xy vect); // range is [-536870912,536870912]
int twiceasini(double x);
xy cossin(double angle);
xy cossin(int angle);
xy cossinhalf(int angle);

int foldangle(int angle); // if angle is outside [-180°,180°), adds 360°
bool isinsector(int angle,int sectors);

double bintorot(int angle);
double bintogon(int angle);
double bintodeg(int angle);
double bintomin(int angle);
double bintosec(int angle);
double bintorad(int angle);
int rottobin(double angle);
int degtobin(double angle);
int mintobin(double angle);
int sectobin(double angle);
int gontobin(double angle);
int radtobin(double angle);
double radtodeg(double angle);
double degtorad(double angle);
double radtomin(double angle);
double mintorad(double angle);
double radtosec(double angle);
double sectorad(double angle);
double radtogon(double angle);
double gontorad(double angle);

/* Angles, azimuths, and bearings are expressed in text as follows:
 * Hex integer  Angle, deg  Angle, gon  Azimuth, deg  Azimuth, gon  Bearing, deg  Bearing, gon
 * 0x00000000   0°00′00″    0.0000      90°00′00″     100.0000      N90°00′00″E   N100.0000E
 * 0x0aaaaaab   30°00′00″   33.3333     60°00′00″     66.6667       N60°00′00″E   N066.6667E
 * 0x15555555   60°00′00″   66.6667     30°00′00″     33.3333       N30°00′00″E   N033.3333E
 * 0x168dfd71   63°26′06″   70.4833	27°33′54″     29.5167       N27°33′54″E   N029.5167E
 * 0x80000000   -360°00′00″ -400.0000   90°00′00″     100.0000      N90°00′00″E   N100.0000E
 * Internally, angles are measured counterclockwise, and azimuths/bearings are
 * counterclockwise from east. For I/O, angles can be measured either way and
 * azimuths/bearings are measured from north.
 * As azimuths or bearings, 0x80000000 and 0x00000000 are equivalent; as deltas they are not.
 */

#define SEC1 1657
#define FURMAN1 32768
#define MIN1 99421
#define DEG1 5965232
#define AT0512 0x80ae90e
// AT0512 is arctangent of 5/12, 22.619865°
#define DEG30 0x0aaaaaab
#define AT34 0x0d1bfae2
// AT34 is arctangent of 3/4, 36.8698976°
#define DEG40 0x0e38e38e
#define DEG45 0x10000000
#define DEG50 0x11c71c72
#define DEG60 0x15555555
#define DEG72 0x1999999a
#define DEG90 0x20000000
#define DEG120 0x2aaaaaab
#define DEG144 0x33333333
#define DEG150 0x35555555
#define DEG180 0x40000000
#define DEG270 0x60000000
#define DEG360 0x80000000
#define PHITURN 0xcf1bbcdd

#endif
