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
#ifndef SPLITLONGLINEARWAYSVISITOR_H
#define SPLITLONGLINEARWAYSVISITOR_H

// Hoot
#include <hoot/core/visitors/ElementOsmMapVisitor.h>

// Std
#include <cassert>

namespace hoot
{

class OsmMap;
class Element;

/**
 * @brief The SplitLongLinearWaysVisitor class splits ways that are too long to be written to
 * OpenStreetMap into smaller ones.
 */
class SplitLongLinearWaysVisitor : public ElementOsmMapVisitor
{
public:

  static QString className() { return "SplitLongLinearWaysVisitor"; }

  static int logWarnCount;

  SplitLongLinearWaysVisitor();
  ~SplitLongLinearWaysVisitor() override = default;

  void setOsmMap(OsmMap* map) override { _map = map; }
  void setOsmMap(const OsmMap*) override { assert(false); }

  /**
   * @see ElementVisitor
   */
  void visit(const std::shared_ptr<Element>& e) override;

  QString getInitStatusMessage() const override { return "Splitting ways..."; }
  QString getCompletedStatusMessage() const override
  { return "Split " + QString::number(_numAffected) + " ways"; }

  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }
  QString getDescription() const override
  { return "Splits ways containing a number of nodes above a specified threshold"; }

  unsigned int getMaxNumberOfNodes() const { return _maxNodesPerWay; }
  void setMaxNumberOfNodes(unsigned int maxNodesPerWay);

private:

  unsigned int _maxNodesPerWay;

  // Actual max is 2000, but in order to allow editors to insert nodes without issues,
  // leaving some breathing room.
  static const unsigned int _defaultMaxNodesPerWay = 1900;
};

}

#endif // SPLITLONGLINEARWAYSVISITOR_H
