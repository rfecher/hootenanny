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
#include "RecursiveElementRemover.h"

// hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/elements/ParentMembershipRemover.h>
#include <hoot/core/index/OsmMapIndex.h>
#include <hoot/core/ops/RemoveWayByEid.h>
#include <hoot/core/ops/RemoveNodeByEid.h>
#include <hoot/core/ops/RemoveRelationByEid.h>
#include <hoot/core/visitors/UniqueElementIdVisitor.h>

using namespace std;

namespace hoot
{

HOOT_FACTORY_REGISTER(OsmMapOperation, RecursiveElementRemover)

RecursiveElementRemover::RecursiveElementRemover() :
_criterion(nullptr),
_removeRefsFromParents(false)
{
}

RecursiveElementRemover::RecursiveElementRemover(
  ElementId eid, const bool removeRefsFromParents, const ElementCriterionPtr& criterion) :
_eid(eid),
_criterion(criterion),
_removeRefsFromParents(removeRefsFromParents)
{
}

void RecursiveElementRemover::apply(const std::shared_ptr<OsmMap>& map)
{
  _numAffected = 0;

  assert(_eid.isNull() == false);
  LOG_VART(map->containsElement(_eid));
  if (!map->containsElement(_eid))
  {
    return;
  }
  const ConstElementPtr& e = map->getElement(_eid);
  if (!e)
  {
    return;
  }

  if (_removeRefsFromParents)
  {
    const int numRemoved = ParentMembershipRemover::removeMemberships(_eid, map);
    LOG_VART(numRemoved);
  }

  UniqueElementIdVisitor sv;
  e->visitRo(*map, sv);

  // Find all potential candidates for erasure. We'll whittle away any invalid candidates.
  set<ElementId> toErase = sv.getElementSet();

  bool foundOne = true;
  // Keep looping through until we stop removing children. There may be times when the ordering of
  // the removal matters.
  while (foundOne)
  {
    foundOne = false;
    // go through all
    for (set<ElementId>::iterator it = toErase.begin(); it != toErase.end();)
    {
      bool erased = false;
      set<ElementId> parents = map->getIndex().getParents(*it);

      // Go through each of the child's direct parents.
      for (set<ElementId>::const_iterator jt = parents.begin(); jt != parents.end(); ++jt)
      {
        LOG_TRACE("Checking parent: " << *jt << " of child: " << *it << "...");
        if (toErase.find(*jt) == toErase.end())
        {
          // Remove the child b/c it is owned by an element outside _eid.
          LOG_TRACE("Removing child: " << *it);
          toErase.erase(it++);
          erased = true;
          foundOne = true;
          break;
        }
        else
        {
          LOG_TRACE("Not removing child: " << *it);
        }
      }

      // If we didn't erase the element, then move the iterator forward.
      if (!erased)
      {
        ++it;
      }
    }
  }

  if (_criterion)
  {
    // Go through all remaining delete candidates.
    for (set<ElementId>::iterator it = toErase.begin(); it != toErase.end();)
    {
      ConstElementPtr child = map->getElement(*it);
      if (_criterion->isSatisfied(child))
      {
        // Remove the child.
        toErase.erase(it++);
      }
      else
      {
        ++it;
      }
    }
  }

  // remove the relations from most general to most specific
  _remove(map, _eid, toErase);
}

void RecursiveElementRemover::_remove(
  const std::shared_ptr<OsmMap>& map, ElementId eid, const set<ElementId>& removeSet)
{
  // if this element isn't being removed
  if (removeSet.find(eid) == removeSet.end() || map->containsElement(eid) == false)
  {
    LOG_TRACE("Not removing " << eid);
    LOG_VART(removeSet);
    LOG_VART(map->containsElement(eid));
    return;
  }

  LOG_TRACE("Removing: " << eid << "...");
  if (eid.getType() == ElementType::Relation)
  {
    const RelationPtr& r = map->getRelation(eid.getId());

    // Make a copy so we can traverse it after this element is cleared.
    vector<RelationData::Entry> e = r->getMembers();
    r->clear();
    for (size_t i = 0; i < e.size(); i++)
    {
      _remove(map, e[i].getElementId(), removeSet);
    }


    RemoveRelationByEid::removeRelation(map, eid.getId());
    LOG_VART(map->getRelation(eid.getId()));
    _numAffected++;
  }
  else if (eid.getType() == ElementType::Way)
  {
    const WayPtr& w = map->getWay(eid.getId());

    std::vector<long> nodes = w->getNodeIds();
    LOG_VART(nodes);
    w->clear();
    for (size_t i = 0; i < nodes.size(); i++)
    {
      _remove(map, ElementId::node(nodes[i]), removeSet);
    }

    RemoveWayByEid::removeWay(map, w->getId());
    LOG_VART(map->getWay(w->getId()));
    _numAffected++;
  }
  else if (eid.getType() == ElementType::Node)
  {
    RemoveNodeByEid::removeNodeNoCheck(map, eid.getId());
    LOG_VART(map->getNode(eid.getId()));
    _numAffected++;
  }
  else
  {
    throw HootException("Unexpected element type.");
  }
}

}
