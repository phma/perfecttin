/******************************************************/
/*                                                    */
/* leastsquares.cpp - least-squares adjustment        */
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

#include "leastsquares.h"

/* In a PuppetBar representing a total station setup, pole[0] is (0.0.0)
 * and the height of instrument, pole[1] is the backsight and is normally
 * (but doesn't have to be) in the yz-plane with y positive, and the rest are
 * sideshots and foresights.
 * 
 * The backsight, for proper adjustment, should be measured, but many total
 * stations do not measure it. If it is not measured, the azimuth of
 * Pole[1].displacement is kept, but the vertical angle and distance are
 * replaced with those of the target, which could be Pole[0] of another PuppetBar.
 * 
 * A PuppetBar representing a total station setup generates its own two error
 * terms, representing its tilt.
 * 
 * If you foresight a point, then occupy it, then backsight (and measure) it,
 * this generates three sets of three error terms: between the point as
 * foresighted and its actual position, between the instrument occupying it
 * and its actual position, and between the point as backsighted and its actual
 * position. The actual position and all three setups are adjusted.
 */
using namespace std;

vector<double> linearLeastSquares(matrix m,vector<double> v)
{
  matrix mtm,mt,vmat=columnvector(v),mtv;
  mt=m.transpose();
  mtm=mt*m;
  mtv=mt*vmat;
  mtm.gausselim(mtv);
  return mtv;
}
