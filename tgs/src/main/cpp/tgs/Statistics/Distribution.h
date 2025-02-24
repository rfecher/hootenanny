/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. Maxar
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2015, 2021 Maxar (http://www.maxar.com/)
 */

#ifndef __TGS__DISTRIBUTIONS_H__
#define __TGS__DISTRIBUTIONS_H__

#include "../TgsExport.h"

namespace Tgs
{
  class TGS_EXPORT Distribution
  {
  public:

    static double calculationGaussian(double mean, double sigma, double x)
    {
      const double PI = 3.14159265358979323846;
      const double EULER = 2.71828182846;
      const double GAUSSIAN_COEFFICIENT = sqrt(2*PI);

      // use a gaussian curve for weighting.
      double t = (x - mean) / sigma;
      return pow(EULER, (-0.5*t*t)) / (GAUSSIAN_COEFFICIENT * sigma);
    }
  };
}

#endif
