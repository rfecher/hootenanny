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
 * @copyright Copyright (C) 2015, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */

#include "WayHeading.h"

// Hoot
#include <hoot/core/algorithms/linearreference/WayLocation.h>

// Standard
#include <math.h>

using namespace geos::geom;

namespace hoot
{

Radians WayHeading::calculateHeading(const WayLocation& loc, Meters delta)
{
  Coordinate v = calculateVector(loc, delta);
  return atan2(v.y, v.x);
}

Radians WayHeading::calculateHeading(const Coordinate& c1, const Coordinate& c2)
{
  return atan2(c2.y - c1.y, c2.x - c1.x);
}

Coordinate WayHeading::calculateVector(const WayLocation& loc, Meters delta)
{
  Coordinate sc = loc.move(-delta).getCoordinate();
  Coordinate ec = loc.move(delta).getCoordinate();

  Coordinate result;
  result.x = ec.x - sc.x;
  result.y = ec.y - sc.y;

  double mag = result.distance(Coordinate(0, 0));
  result.x /= mag;
  result.y /= mag;

  return result;
}

Coordinate WayHeading::calculateVector(const Coordinate& c1, const Coordinate& c2)
{
  Coordinate result;
  result.x = c2.x - c1.x;
  result.y = c2.y - c1.y;

  double mag = result.distance(Coordinate(0, 0));
  result.x /= mag;
  result.y /= mag;

  return result;
}

Radians WayHeading::deltaMagnitude(Radians r1, Radians r2)
{
  Radians delta = fabs(r1 - r2);
  if (delta > M_PI)
  {
    delta = fabs(delta - M_PI * 2);
  }
  return delta;
}

}
