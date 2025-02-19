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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */
#include "OsmNetworkExtractor.h"

#include <hoot/core/elements/Element.h>
#include <hoot/core/visitors/ConstElementVisitor.h>
#include <hoot/core/elements/Relation.h>

#include <hoot/core/criterion/LinearCriterion.h>
#include <hoot/core/criterion/OneWayCriterion.h>
#include <hoot/core/criterion/ReversedRoadCriterion.h>

using namespace std;

namespace hoot
{

int OsmNetworkExtractor::logWarnCount = 0;

class OsmNetworkExtractorVisitor : public ConstElementVisitor
{
public:

  OsmNetworkExtractorVisitor(OsmNetworkExtractor& parent) : _parent(parent) { }
  ~OsmNetworkExtractorVisitor() = default;

  void visit(const ConstElementPtr& e) override
  {
    _parent._visit(e);
  }

  QString getDescription() const override { return ""; }
  QString getName() const override { return ""; }
  QString getClassName() const override { return ""; }

private:

  OsmNetworkExtractor& _parent;
};

void OsmNetworkExtractor::_addEdge(ConstElementPtr from, ConstElementPtr to,
  QList<ConstElementPtr> members, bool directed) const
{
  ConstNetworkVertexPtr v1 = _network->getSingleVertex(from->getElementId());
  if (!v1.get())
  {
    v1 = std::make_shared<NetworkVertex>(from);
    _network->addVertex(v1);
  }
  ConstNetworkVertexPtr v2 = _network->getSingleVertex(to->getElementId());
  if (!v2.get())
  {
    v2 = std::make_shared<NetworkVertex>(to);
    _network->addVertex(v2);
  }

  NetworkEdgePtr edge = std::make_shared<NetworkEdge>(v1, v2, directed);
  edge->setMembers(members);

  _network->addEdge(edge);
}

OsmNetworkPtr OsmNetworkExtractor::extractNetwork(ConstOsmMapPtr map)
{
  _network = std::make_shared<OsmNetwork>();

  _map = map;
  // go through all the elements.
  OsmNetworkExtractorVisitor v(*this);
  map->visitRo(v);

  return _network;
}

bool OsmNetworkExtractor::_isContiguous(const ConstRelationPtr& r)
{
  long lastNode = 0;
  return _isContiguous(r, lastNode);
}

bool OsmNetworkExtractor::_isContiguous(const ConstRelationPtr& r, long& lastNode)
{
  assert(LinearCriterion().isSatisfied(r));

  const vector<RelationData::Entry>& members = r->getMembers();

  if (members.size() < 1)
    return false;
  for (size_t i = 0; i < members.size(); ++i)
  {
    ElementId eid = members[i].getElementId();

    if (eid.getType() == ElementType::Way)
    {
      ConstWayPtr w = _map->getWay(eid);
      if (i > 0 && w->getFirstNodeId() != lastNode)
      {
        return false;
      }
      lastNode = w->getLastNodeId();
    }
    else if (eid.getType() == ElementType::Relation)
    {
      ConstRelationPtr sub = _map->getRelation(eid);
      if (!_isContiguous(sub, lastNode))
        return false;
    }
  }
  return true;
}

void OsmNetworkExtractor::_getFirstLastNodes(const ConstRelationPtr& r, ElementId& first, ElementId& last)
{
  ElementId temp;
  ConstRelationPtr sub;
  const vector<RelationData::Entry>& members = r->getMembers();
  //  Get the first and last members
  ElementId from = members[0].getElementId();
  ElementId to = members[members.size() - 1].getElementId();
  //  Find the first node of the first member
  switch(from.getType().getEnum())
  {
  case ElementType::Node:
    first = from;
    break;
  case ElementType::Way:
    first = ElementId::node(_map->getWay(from)->getFirstNodeId());
    break;
  case ElementType::Relation:
    sub = _map->getRelation(from);
    _getFirstLastNodes(sub, first, temp);
    break;
  default:
    LOG_ERROR("Unknown relation member type");
    break;
  }
  //  Find the last node of the last member
  switch(to.getType().getEnum())
  {
  case ElementType::Node:
    last = to;
    break;
  case ElementType::Way:
    last = ElementId::node(_map->getWay(to)->getLastNodeId());
    break;
  case ElementType::Relation:
    sub = _map->getRelation(to);
    _getFirstLastNodes(sub, temp, last);
    break;
  default:
    LOG_ERROR("Unknown relation member type");
    break;
  }
}

bool OsmNetworkExtractor::_isValidElement(const ConstElementPtr& e) const
{
  bool result = true;
  if (e->getElementType() == ElementType::Node)
  {
    result = false;
  }
  else if (e->getElementType() == ElementType::Relation)
  {
    ConstRelationPtr r = std::dynamic_pointer_cast<const Relation>(e);
    if (LinearCriterion().isSatisfied(e) == false)
    {
      if (logWarnCount < Log::getWarnMessageLimit())
      {
        LOG_WARN("Received a non-linear relation as a valid network element. Ignoring relation...");
        LOG_VART(e);
      }
      else if (logWarnCount == Log::getWarnMessageLimit())
      {
        LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
      }
      logWarnCount++;
      result = false;
    }
    else
    {
      const vector<RelationData::Entry>& members = r->getMembers();
      for (size_t i = 0; i < members.size(); ++i)
      {
        if (members[i].getElementId().getType() != ElementType::Way)
        {
          if (logWarnCount < Log::getWarnMessageLimit())
          {
            LOG_WARN("Received a linear relation that contains a non-linear element.");
            LOG_VART(e);
          }
          else if (logWarnCount == Log::getWarnMessageLimit())
          {
            LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
          }
          logWarnCount++;
        }
      }
    }
  }

  return result;
}

void OsmNetworkExtractor::_visit(const ConstElementPtr& e)
{
  if (_criterion->isSatisfied(e) && _isValidElement(e))
  {
    QList<ConstElementPtr> members;
    ElementId from, to;

    if (e->getElementType() == ElementType::Way)
    {
      members.append(e);
      ConstWayPtr w = std::dynamic_pointer_cast<const Way>(e);
      from = ElementId::node(w->getFirstNodeId());
      to = ElementId::node(w->getLastNodeId());
    }
    else if (e->getElementType() == ElementType::Relation)
    {
      members.append(e);
      ConstRelationPtr r = std::dynamic_pointer_cast<const Relation>(e);

      if (_isContiguous(r))
      {
        //  Getting the first and last nodes of a relation is non-trivial
        _getFirstLastNodes(r, from, to);
      }
      else
      {
        // If this is a bad multi-linestring, then don't include it in the network.
        if (logWarnCount < Log::getWarnMessageLimit())
        {
          // Moving this to trace for now, since it happens a fair amount and as been going on
          // awhile.
          LOG_TRACE(
            "Found a non-contiguous relation when extracting a network. Ignoring: " <<
            e->getElementId());
          LOG_TRACE("Non-contiguous relation: " << e);
        }
        else if (logWarnCount == Log::getWarnMessageLimit())
        {
          LOG_TRACE(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
        return;
      }
    }

    bool directed = false;
    if (OneWayCriterion().isSatisfied(e))
    {
      directed = true;
      if (ReversedRoadCriterion().isSatisfied(e))
      {
        swap(from, to);
      }
    }

    _addEdge(_map->getNode(from), _map->getNode(to), members, directed);
  }
}

}
