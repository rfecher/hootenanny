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
#ifndef EDGELOCATION_H
#define EDGELOCATION_H

// hoot
#include <hoot/core/conflate/network/NetworkEdge.h>

// Tgs
#include <tgs/HashMap.h>

namespace hoot
{

/**
 * Represents a location on an edge. This assumes an edge can be represented as a line and addressed
 * as a portion [0, 1] along that line.
 *
 * This class parallels WayLocation.
 */
class EdgeLocation
{
public:

  // the suggested sloppy segment fraction distance from a vertex that is still considered on a
  // vertex.
  static const double SLOPPY_EPSILON;

  EdgeLocation(ConstNetworkEdgePtr e, double portion) : _e(e), _portion(portion) {}

  const ConstNetworkEdgePtr& getEdge() const { return _e; }
  Meters getOffset(const ConstElementProviderPtr& provider) const;
  double getPortion() const { return _portion; }
  /**
   * Returns the vertex at this location if isExtreme(epsilon) == true. Otherwise an exception is
   * thrown.
   */
  ConstNetworkVertexPtr getVertex(double epsilon = 0.0) const;

  /**
   * Returns true if the location is either at the start or end of an edge.
   */
  bool isExtreme(double epsilon = 0.0) const { return isFirst(epsilon) || isLast(epsilon); }
  bool isFirst(double epsilon = 0.0) const { return _portion <= epsilon; }
  bool isLast(double epsilon = 0.0) const { return _portion >= 1 - epsilon; }
  bool isValid() const { return _portion >= 0.0 && _portion <= 1.0; }

  std::shared_ptr<EdgeLocation> move(const ConstElementProviderPtr& provider, Meters distance) const;

  QString toString() const;

private:

  ConstNetworkEdgePtr _e;
  double _portion;

};

using EdgeLocationPtr = std::shared_ptr<EdgeLocation>;
using ConstEdgeLocationPtr = std::shared_ptr<const EdgeLocation>;

inline bool operator==(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  return a->getEdge() == b->getEdge() && a->getPortion() == b->getPortion();
}

inline bool operator!=(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  return !(a == b);
}

inline bool operator<(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  assert(a->getEdge() == b->getEdge());
  return a->getPortion() < b->getPortion();
}

inline bool operator>(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  assert(a->getEdge() == b->getEdge());
  return a->getPortion() > b->getPortion();
}

inline bool operator<=(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  assert(a->getEdge() == b->getEdge());
  return a->getPortion() <= b->getPortion();
}

inline bool operator>=(const ConstEdgeLocationPtr& a, const ConstEdgeLocationPtr& b)
{
  assert(a->getEdge() == b->getEdge());
  return a->getPortion() >= b->getPortion();
}

inline uint qHash(const ConstEdgeLocationPtr& el)
{
  uint result = ::qHash(Tgs::cantorPairing(qHash(el->getEdge()),
    ::qHash((quint64)__gnu_cxx::fastHashDouble()(el->getPortion()))));
  return result;
}

}

#endif // EDGELOCATION_H
