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

using namespace std;

vector<double> linearLeastSquares(matrix m,vector<double> v)
/* m should be a matrix(a,b) where a>b. If a<b, mtm is singular.
 * There are b things which can be adjusted to best fit a data.
 */
{
  matrix mtm,mt,vmat=columnvector(v),mtv;
  mt=m.transpose();
  mtm=mt*m;
  mtv=mt*vmat;
  mtm.gausselim(mtv);
  return mtv;
}
