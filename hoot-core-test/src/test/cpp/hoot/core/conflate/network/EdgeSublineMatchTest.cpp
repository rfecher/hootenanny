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
 * @copyright Copyright (C) 2016, 2017, 2018, 2019, 2021 Maxar (http://www.maxar.com/)
 */

// Hoot
#include <hoot/core/TestUtils.h>
#include <hoot/core/conflate/network/EdgeSublineMatch.h>

namespace hoot
{

class EdgeSublineMatchTest : public HootTestFixture
{
  CPPUNIT_TEST_SUITE(EdgeSublineMatchTest);
  CPPUNIT_TEST(basicTest);
  CPPUNIT_TEST_SUITE_END();

public:

  EdgeSublineMatchTest()
  {
    setResetType(ResetEnvironment);
  }

  void basicTest()
  {
    OsmMapPtr map = std::make_shared<OsmMap>();
    ConstNetworkVertexPtr vertex1 =
      std::make_shared<NetworkVertex>(TestUtils::createNode(map, "", Status::Unknown1, 0, 0));
    ConstNetworkVertexPtr vertex2 =
      std::make_shared<NetworkVertex>(TestUtils::createNode(map, "", Status::Unknown1, 10, 0));
    ConstNetworkEdgePtr edge = std::make_shared<NetworkEdge>(vertex1, vertex2, true);
    ConstEdgeLocationPtr edgeLocStart = std::make_shared<EdgeLocation>(edge, 0.0);
    ConstEdgeLocationPtr edgeLocEnd = std::make_shared<EdgeLocation>(edge, 0.7);
    ConstEdgeSublinePtr edgeSubline1 = std::make_shared<EdgeSubline>(edgeLocStart, edgeLocEnd);
    ConstEdgeSublinePtr edgeSubline2 = std::make_shared<EdgeSubline>(edgeLocEnd, edgeLocStart);

    EdgeSublineMatch edgeSublineMatch(edgeSubline1, edgeSubline2);
    CPPUNIT_ASSERT(edgeSublineMatch.getSubline1() == edgeSubline1);
    CPPUNIT_ASSERT(edgeSublineMatch.getSubline2() == edgeSubline2);
    HOOT_STR_EQUALS(
      "{subline1: { _start: { _e: (0) Node(-1) --  --> (1) Node(-2), _portion: 0 }, _end: { _e: (0) Node(-1) --  --> (1) Node(-2), _portion: 0.7 } }, subline2: { _start: { _e: (0) Node(-1) --  --> (1) Node(-2), _portion: 0.7 }, _end: { _e: (0) Node(-1) --  --> (1) Node(-2), _portion: 0 } }}",
      edgeSublineMatch.toString());
  }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(EdgeSublineMatchTest, "quick");

}
