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
 * @copyright Copyright (C) 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */

#ifndef WAYJOINER_H
#define WAYJOINER_H

// Hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/elements/Way.h>
#include <hoot/core/info/ApiEntityInfo.h>

namespace hoot
{

/**
 * @brief Interface for way joiners.
 *
 * During conflation, linear features may need to be split in order for matching and/or merging to
 * perform correctly. The job of the way joiners is to restore such split features by rejoining
 * them.
 */
class WayJoiner : public ApiEntityInfo
{
public:

  static QString className() { return "WayJoiner"; }

  WayJoiner();
  virtual ~WayJoiner() = default;

  /**
   * @brief join Runs all joining algorithms
   */
  virtual void join(const OsmMapPtr& map);

  /**
   * @brief get/set parent ID reset flag
   */
  virtual void setLeavePid(bool leavePid) { _leavePid = leavePid; }
  virtual bool getLeavePid() const { return _leavePid; }
  virtual void setWritePidToChildId(bool write) { _writePidToChildId = write; }
  virtual bool getWritePidToChildId() const { return _writePidToChildId; }

  virtual int getNumJoined() const { return _numJoined; }

  QHash<long, long> getJoinedWayIdMappings() const { return _joinedWayIdMappings; }

  QString toString() const override { return ""; }

  /**
   * @brief joinWay Explicitly join two ways together
   */
  virtual void joinWay(const OsmMapPtr& map, const WayPtr& parent, const WayPtr& child);

protected:

  /** Debugging flag to leave parent IDs intact for output */
  bool _leavePid;
  // If enabled, the ID of any element with a parent ID will be updated with the parent ID's value.
  // at the end of joining. In the case of multiple elements with the same parent ID, only the first
  // element's ID will be updated.
  bool _writePidToChildId;
  /** Pointer to the map to work on */
  OsmMapPtr _map;

  // mapping of joined ways ids; ID before joining mapped to ID after joining
  QHash<long, long> _joinedWayIdMappings;

  int _numJoined;
  int _numProcessed;
  int _totalWays;

  int _taskStatusUpdateInterval;

  /**
   * @brief joinParentChild Simplest joining algorithm that joins a way with a parent id to that
   * parent
   */
  virtual void _joinParentChild();

  /**
   * @brief joinAtNode Joining algorithm that searches all ways that have a parent id and tries
   *    to join them with adjacent ways that have the same tags
   */
  virtual void _joinAtNode();

  /**
   * @brief areJoinable Check the status of the ways to see if they are compatible when joining at
   * a node; essentially UNKNOWN1 and UNKNOWN2 ways aren't joinable together
   */
  virtual bool _areJoinable(const WayPtr& w1, const WayPtr& w2) const;

  /**
   * @brief rejoinSiblings Function that rejoins ways that all have the same parent id
   *    but that parent way doesn't exist
   * @param way_ids Deque of sorted ways to join
   */
  virtual void _rejoinSiblings(std::deque<long>& way_ids);

  /**
   * @brief joinSiblings Joining algorithm that searches for all ways that have the same parent id,
   *    attempts to order them into adjoining way order, then joins them
   */
  virtual void _joinSiblings();

  /**
   * @brief joinWays Function to rejoin two ways
   * @param parent Way that is modified to include the child way
   * @param child Way that will be merged into the parent and then deleted
   * @return true if the two ways were joined; false otherwise
   */
  virtual bool _joinWays(const WayPtr& parent, const WayPtr& child);

private:

  /**
   * @brief resetParents Resets parent id for all ways after joining operation has completed
   *    does nothing if _leavePid is true
   */
  void _resetParents() const;

  /*
   * @see _writePidToChildId
   */
  void _writeParentIdsToChildIds() const;
};

}

#endif  //  WAYJOINER_H
