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
#ifndef RELATION_TO_MULTIPOLYGON_CONVERTER_H
#define RELATION_TO_MULTIPOLYGON_CONVERTER_H

// hoot
#include <hoot/core/elements/ElementProvider.h>

// geos
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/LinearRing.h>

namespace hoot
{

/**
 * This will happily try to convert any relation into a multipolygon regardless of tags. It is the
 * caller's job to determine if this is appropriate or not.
 *
 * A best effort is made to detect bad polygons and throw an appropriate error, but there are
 * guaranteed to be outliers that will not be caught. You may receive an invalid geometry as a
 * result. If there are no outer members, then a valid empty geometry will be returned.
 *
 * Interpretation of some ambiguous situations:
 * 1. If three or more ways of a single type meet at a end/start point, then it is an error.
 * 2. A hole can only be a hole in one outer ring. If the hole is inside multiple outer rings,
 *    it will placed in the smallest outer ring that contains the hole.
 * 3. If multiple outer ways overlap, it is an error.
 *
 * See the OSM wiki [1] for a detailed description of what constitutes a valid multipolygon and
 * how to compose complex objects.
 *
 * This is not very efficient for multipolygons with a large number of components (e.g. dozens
 * of inner and/or outer rings).
 *
 * 1. http://wiki.openstreetmap.org/wiki/Relation:multipolygon
 *
 * See Redmine #2115 for additional details
 */
class RelationToMultiPolygonConverter
{
public:

  static QString className() { return "RelationToMultiPolygonConverter"; }

  static int logWarnCount;

  RelationToMultiPolygonConverter(
    const ConstElementProviderPtr& provider, const ConstRelationPtr& r);

  /**
   * Create and return a multipolygon. If the multipolygon contains gross errors then an empty
   * geometry will be returned. Otherwise, a best effort will be made to return a reasonable
   * polygon.
   */
  std::shared_ptr<geos::geom::Geometry> createMultipolygon() const;

private:

  const ConstElementProviderPtr& _provider;
  const ConstRelationPtr& _r;

  geos::geom::Geometry* _addHoles(std::vector<geos::geom::LinearRing*>& outers,
                                  std::vector<geos::geom::LinearRing*>& inners) const;

  void _addWayToSequence(ConstWayPtr w, geos::geom::CoordinateSequence& cs, bool reversed) const;

  void _createRings(const QString &role, std::vector<geos::geom::LinearRing *>& rings) const;

  void _createRingsFromPartials(
    const std::vector<ConstWayPtr>& partials, std::vector<geos::geom::LinearRing *>& rings) const;

  /*
   * Given a vector of unsorted partial ways, create a ring. If the partials do not create a
   * complete ring, log a warning and just connect the ends.
   */
  void _createSingleRing(const std::vector<ConstWayPtr>& partials,
                         std::vector<geos::geom::LinearRing *>& rings) const;

  /*
   * Given a set of rings that are not either an inner or an outer, figure out what they are and
   * add them to the appropriate ring set.
   */
  void _classifyRings(std::vector<geos::geom::LinearRing *>& noRole,
                      std::vector<geos::geom::LinearRing *>& inners,
                      std::vector<geos::geom::LinearRing *>& outers) const;

  /*
   * Given two Linear Rings, determine the realtionship between the two.
   * Inner, Outer or "" for neither
   */
  QString _findRelationship(
    const geos::geom::LinearRing* ring1, const geos::geom::LinearRing* ring2) const;

  bool _isValidInner(const geos::geom::LinearRing* innerRing) const;

  /*
   * Put the ways into the right order so they can just be added one after another into a ring.
   */
  std::deque<ConstWayPtr> _orderWaysForRing(const std::vector<ConstWayPtr>& partials) const;

  geos::geom::LinearRing* _toLinearRing(const ConstWayPtr& w) const;
};

}

#endif // RELATION_TO_MULTIPOLYGON_CONVERTER_H
