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
 * @copyright Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */

#include "ParallelWayCriterion.h"

// GEOS
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/LineString.h>
#include <geos/geom/Point.h>
#include <geos/operation/distance/DistanceOp.h>

// Hoot
#include <hoot/core/algorithms/WayDiscretizer.h>
#include <hoot/core/algorithms/WayHeading.h>
#include <hoot/core/algorithms/linearreference/LocationOfPoint.h>
#include <hoot/core/elements/Element.h>
#include <hoot/core/elements/ElementGeometryUtils.h>
#include <hoot/core/geometry/ElementToGeometryConverter.h>
#include <hoot/core/util/Factory.h>

using namespace geos::geom;
using namespace geos::operation::distance;

namespace hoot
{

HOOT_FACTORY_REGISTER(ElementCriterion, ParallelWayCriterion)

ParallelWayCriterion::ParallelWayCriterion(
  const ConstOsmMapPtr& map, ConstWayPtr baseWay, bool isParallel) :
  _map(map),
  _baseWay(baseWay),
  _isParallel(isParallel)
{
  // Default threshold
  _threshold = 10.0;

  // space 4m or get at least 5 samples
  std::vector<Coordinate> coords;
  Meters spacing = std::min(ElementGeometryUtils::calculateLength(baseWay, map) / 5, 4.0);
  if (spacing <= 0.0)
  {
    spacing = 4.0;
  }
  WayDiscretizer wd1(_map, baseWay);
  wd1.discretize(spacing, coords);

  LocationOfPoint lop(map, baseWay);
  _headings.resize(coords.size());
  _points.resize(coords.size());
  for (size_t i = 0; i < coords.size(); i++)
  {
    WayLocation loc = lop.locate(coords[i]);
    _points[i] = GeometryFactory::getDefaultInstance()->createPoint(coords[i]);
    _headings[i] = WayHeading::calculateHeading(loc);
  }
}

ParallelWayCriterion::~ParallelWayCriterion()
{
  for (size_t i = 0; i < _points.size(); i++)
  {
    delete _points[i];
  }
}

Radians ParallelWayCriterion::calculateDifference(const ConstWayPtr& w) const
{
  std::shared_ptr<LineString> ls = ElementToGeometryConverter(_map).convertToLineString(w);

  Radians deltaSum = 0.0;
  int count = 0;

  for (size_t i = 0; i < _points.size(); i++)
  {
    // calculate the heading from point to the nearest point on the candidate way.
    std::shared_ptr<CoordinateSequence> seq(DistanceOp::nearestPoints(_points[i], ls.get()));
    double d = seq->getAt(0).distance(seq->getAt(1));
    if (d > 0.5)
    {
      Radians shortestHeading = WayHeading::calculateHeading(seq->getAt(0), seq->getAt(1));

      Radians delta = WayHeading::deltaMagnitude(_headings[i], shortestHeading);

      deltaSum += delta;
      count++;
    }
    else
    {
      deltaSum += toRadians(90.0);
      count++;
    }
  }

  if (count == 0)
  {
    return 0.0;
  }
  else
  {
    Radians mean = deltaSum / (double)count;

    return fabs(mean - toRadians(90));
  }
}

bool ParallelWayCriterion::isSatisfied(const ConstElementPtr& e) const
{
  if (e->getElementType() == ElementType::Way)
  {
    ConstWayPtr way = std::dynamic_pointer_cast<const Way>(e);
    double difference = calculateDifference(way);

    // If the mean "normals" are within 10 degrees of perpendicular.
    bool parallel = difference < toRadians(_threshold);
    return parallel == _isParallel;
  }
  return false;
}

bool ParallelWayCriterion::isParallel(const ConstOsmMapPtr& map, const ConstElementPtr& e1, const ConstElementPtr& e2)
{
  if (e1->getElementType() != ElementType::Way)
    return false;
  ConstWayPtr way = std::dynamic_pointer_cast<const Way>(e1);
  return ParallelWayCriterion(map, way, true).isSatisfied(e2);
}

bool ParallelWayCriterion::notParallel(const ConstOsmMapPtr& map, const ConstElementPtr& e1, const ConstElementPtr& e2)
{
  if (e1->getElementType() != ElementType::Way)
    return false;
  ConstWayPtr way = std::dynamic_pointer_cast<const Way>(e1);
  return ParallelWayCriterion(map, way, false).isSatisfied(e2);
}

}
