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
#include "BuildingMerger.h"

// hoot
#include <hoot/core/algorithms/extractors/IntersectionOverUnionExtractor.h>
#include <hoot/core/conflate/IdSwap.h>
#include <hoot/core/conflate/merging/LinearTagOnlyMerger.h>
#include <hoot/core/conflate/polygon/BuildingMatch.h>
#include <hoot/core/conflate/review/ReviewMarker.h>
#include <hoot/core/criterion/BuildingCriterion.h>
#include <hoot/core/criterion/BuildingPartCriterion.h>
#include <hoot/core/criterion/ElementCriterion.h>
#include <hoot/core/criterion/NodeCriterion.h>
#include <hoot/core/elements/ElementIdUtils.h>
#include <hoot/core/elements/InMemoryElementSorter.h>
#include <hoot/core/elements/OsmUtils.h>
#include <hoot/core/elements/TagUtils.h>
#include <hoot/core/elements/Relation.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/ops/IdSwapOp.h>
#include <hoot/core/ops/RecursiveElementRemover.h>
#include <hoot/core/ops/RemoveRelationByEid.h>
#include <hoot/core/ops/ReplaceElementOp.h>
#include <hoot/core/ops/ReuseNodeIdsOnWayOp.h>
#include <hoot/core/schema/BuildingRelationMemberTagMerger.h>
#include <hoot/core/schema/MetadataTags.h>
#include <hoot/core/schema/OverwriteTagMerger.h>
#include <hoot/core/schema/PreserveTypesTagMerger.h>
#include <hoot/core/schema/TagMergerFactory.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/Factory.h>
#include <hoot/core/visitors/ElementCountVisitor.h>
#include <hoot/core/visitors/FilteredVisitor.h>
#include <hoot/core/visitors/UniqueElementIdVisitor.h>
#include <hoot/core/visitors/WorstCircularErrorVisitor.h>

using namespace std;

namespace hoot
{

HOOT_FACTORY_REGISTER(Merger, BuildingMerger)

class DeletableBuildingCriterion : public ElementCriterion
{

public:

  DeletableBuildingCriterion() = default;
  ~DeletableBuildingCriterion() override = default;

  bool isSatisfied(const ConstElementPtr& e) const override
  {
    bool result = false;
    if (e->getElementType() == ElementType::Node && e->getTags().getInformationCount() == 0)
    {
      result = true;
    }
    else if (e->getElementType() != ElementType::Node &&
             (_buildingCrit.isSatisfied(e) || _buildingPartCrit.isSatisfied(e)))
    {
      result = true;
    }
    return !result;
  }

  QString getDescription() const override { return ""; }
  QString getName() const override { return ""; }
  QString getClassName() const override { return ""; }

  ElementCriterionPtr clone() override { return std::make_shared<DeletableBuildingCriterion>(); }

  QString toString() const override { return ""; }

private:

  BuildingCriterion _buildingCrit;
  BuildingPartCriterion _buildingPartCrit;
};

int BuildingMerger::logWarnCount = 0;

BuildingMerger::BuildingMerger(const set<pair<ElementId, ElementId>>& pairs) :
MergerBase(pairs),
_keepMoreComplexGeometryWhenAutoMerging(
  ConfigOptions().getBuildingKeepMoreComplexGeometryWhenAutoMerging()),
_mergeManyToManyMatches(ConfigOptions().getBuildingMergeManyToManyMatches()),
_manyToManyMatch(false),
_useChangedReview(ConfigOptions().getBuildingChangedReview()),
_changedReviewIouThreshold(ConfigOptions().getBuildingChangedReviewIouThreshold())
{
  LOG_VART(_pairs);
}

void BuildingMerger::apply(const OsmMapPtr& map, vector<pair<ElementId, ElementId>>& replaced)
{
  set<ElementId> firstPairs;
  set<ElementId> secondPairs;
  set<ElementId> combined;
  LOG_VART(_pairs);
  _markedReviewText = "";

  // check to see if it is many to many
  for (set<pair<ElementId, ElementId>>::const_iterator sit = _pairs.begin(); sit != _pairs.end();
       ++sit)
  {
    firstPairs.emplace(sit->first);
    secondPairs.emplace(sit->second);
    combined.emplace(sit->first);
    combined.emplace(sit->second);
  }
  _manyToManyMatch = firstPairs.size() > 1 && secondPairs.size() > 1;

  LOG_VART(_manyToManyMatch);
  LOG_VART(_mergeManyToManyMatches);

  ReviewMarker reviewMarker;

  if (_manyToManyMatch && !_mergeManyToManyMatches)
  {
    // If the corresponding auto merge config option is not enabled, then auto review this many to
    // many match.
    _markedReviewText =
      "Merging multiple buildings from each data source is error prone and requires a human eye.";
    reviewMarker.mark(map, combined, _markedReviewText, BuildingMatch::MATCH_NAME, 1.0);
    return;
  }

  ElementPtr e1 = _buildBuilding(map, true);
  if (!e1)
  {
    LOG_TRACE("Built building 1 null. Skipping merge.");
    return;
  }
  ElementPtr e2 = _buildBuilding(map, false);
  if (!e2)
  {
    LOG_TRACE("Built building 2 null. Skipping merge.");
    return;
  }

  LOG_VART(_keepMoreComplexGeometryWhenAutoMerging);
  LOG_VART(_useChangedReview);

  double iou = 1.0;
  if (_useChangedReview)
  {
    iou = IntersectionOverUnionExtractor().extract(*map, e1, e2);
  }

  ElementPtr keeper;
  ElementPtr scrap;

  if (_useChangedReview &&
      // doubt iou would ever be <= 0 if these buildings matched in the first place but adding the
      // check anyway
      iou > 0.0 && iou < _changedReviewIouThreshold)
  {
    // If the corresponding config option is enabled, auto review this "changed" building when the
    // IoU score falls below the specified threshold.
    _markedReviewText =
      "Identified as changed with an IoU score of: " + QString::number(iou) +
      ", which is less than the specified threshold of: " +
      QString::number(_changedReviewIouThreshold);
    reviewMarker.mark(map, combined, _markedReviewText, BuildingMatch::MATCH_NAME, 1.0);
    return;
  }

  bool preserveBuildingId = false;

  if (_keepMoreComplexGeometryWhenAutoMerging)
  {
    // keep the most complex building

    const ElementId moreComplexBuildingId = _getIdOfMoreComplexBuilding(e1, e2, map);
    if (moreComplexBuildingId.isNull())
    {
      // couldn't determine which one is more complex (one or both buildings were missing nodes)
      return;
    }
    else if (e1->getElementId() == moreComplexBuildingId)
    {
      // ref is more complex
      keeper = e1;
      scrap = e2;
    }
    else
    {
      // sec is more complex
      keeper = e2;
      scrap = e1;
      //  Keep e2's geometry but keep e1's ID
      preserveBuildingId = true;
    }
  }
  else
  {
    // default merging strategy: keep the reference building
    keeper = e1;
    scrap = e2;
    LOG_TRACE("Keeping the reference building geometry...");
  }

  keeper->setTags(_getMergedTags(e1, e2));
  keeper->setStatus(Status::Conflated);
  ConfigOptions conf;
  if (conf.getWriterIncludeDebugTags() && conf.getWriterIncludeMatchedByTag())
  {
    keeper->setTag(MetadataTags::HootMatchedBy(), BuildingMatch::MATCH_NAME);
  }

  LOG_TRACE("BuildingMerger: keeper\n" << OsmUtils::getElementDetailString(keeper, map));
  LOG_TRACE("BuildingMerger: scrap\n" << OsmUtils::getElementDetailString(scrap, map));

  // Check to see if we are removing a multipoly building relation.  If so, its multipolygon
  // relation members, need to be removed as well.
  const QSet<ElementId> multiPolyMemberIds = _getMultiPolyMemberIds(scrap);

  // Here is where we are able to reuse node IDs between buildings
  ReuseNodeIdsOnWayOp(scrap->getElementId(), keeper->getElementId()).apply(map);
  // Replace the scrap with the keeper in any parents
  ReplaceElementOp(scrap->getElementId(), keeper->getElementId()).apply(map);
  // Favor the positive ID over the negative ID
  if (keeper->getId() < 0 && scrap->getId() > 0)
    preserveBuildingId = true;
  // Swap the IDs of the two elements if keeper isn't UNKNOWN1
  if (preserveBuildingId)
  {
    ElementId oldKeeperId = keeper->getElementId();
    ElementId oldScrapId = scrap->getElementId();
    IdSwapOp swapOp(oldScrapId, oldKeeperId);
    swapOp.apply(map);
    // Now swap the pointers so that the wrong one isn't deleted
    if (swapOp.getNumFeaturesAffected() > 0)
    {
      scrap = map->getElement(oldKeeperId);
      keeper = map->getElement(oldScrapId);
    }
  }

  // remove the scrap element from the map
  std::shared_ptr<DeletableBuildingCriterion> crit = std::make_shared<DeletableBuildingCriterion>();
  RecursiveElementRemover(scrap->getElementId(), false, crit).apply(map);
  scrap->getTags().clear();

  // delete any pre-existing multipoly members
  LOG_TRACE("Removing multi-poly members: " << multiPolyMemberIds);
  for (QSet<ElementId>::const_iterator it = multiPolyMemberIds.begin();
       it != multiPolyMemberIds.end(); ++it)
  {
    RecursiveElementRemover(*it).apply(map);
  }

  set<pair<ElementId, ElementId>> replacedSet;
  for (set<pair<ElementId, ElementId>>::const_iterator it = _pairs.begin(); it != _pairs.end();
       ++it)
  {
    // if we replaced the second group of buildings
    if (it->second != keeper->getElementId())
      replacedSet.emplace(it->second, keeper->getElementId());

    if (it->first != keeper->getElementId())
      replacedSet.emplace(it->first, keeper->getElementId());
  }
  replaced.insert(replaced.end(), replacedSet.begin(), replacedSet.end());
}

Tags BuildingMerger::_getMergedTags(const ElementPtr& e1, const ElementPtr& e2)
{
  Tags mergedTags;
  LOG_TRACE("e1 tags before merging and after built building tag merge: " << e1->getTags());
  LOG_TRACE("e2 tags before merging and after built building tag merge: " << e2->getTags());
  if (_manyToManyMatch && _mergeManyToManyMatches)
  {
    // preserve type tags
    mergedTags = PreserveTypesTagMerger().mergeTags(e1->getTags(), e2->getTags(), ElementType::Way);
  }
  else
  {
    // use the default tag merging mechanism
    mergedTags = TagMergerFactory::mergeTags(e1->getTags(), e2->getTags(), ElementType::Way);
  }
  LOG_TRACE("tags after merging: " << mergedTags);

  QStringList ref1;
  e1->getTags().readValues(MetadataTags::Ref1(), ref1);
  QStringList ref2;
  e2->getTags().readValues(MetadataTags::Ref2(), ref2);

  ref1.sort();
  ref2.sort();

  if (!ref1.empty() || !ref2.empty())
  {
    if (ref1 == ref2)
    {
      mergedTags[MetadataTags::HootBuildingMatch()] = "true";
    }
    else
    {
      mergedTags[MetadataTags::HootBuildingMatch()] = "false";
    }
  }

  LOG_VART(mergedTags);
  return mergedTags;
}

ElementId BuildingMerger::_getIdOfMoreComplexBuilding(
  const ElementPtr& building1, const ElementPtr& building2, const OsmMapPtr& map) const
{
  // Use node count as a surrogate for complexity of the geometry.
  int nodeCount1 = 0;
  if (building1.get())
  {
    LOG_VART(building1);
    nodeCount1 =
      (int)FilteredVisitor::getStat(
        std::make_shared<NodeCriterion>(), std::make_shared<ElementCountVisitor>(), map, building1);
  }
  LOG_VART(nodeCount1);

  int nodeCount2 = 0;
  if (building2.get())
  {
    LOG_VART(building2);
    nodeCount2 =
      (int)FilteredVisitor::getStat(
        std::make_shared<NodeCriterion>(), std::make_shared<ElementCountVisitor>(), map, building2);
  }
  LOG_VART(nodeCount2);

  // This will happen if a way/relation building is passed in with missing nodes.
  if (nodeCount1 == 0 || nodeCount2 == 0)
  {
    if (logWarnCount < Log::getWarnMessageLimit())
    {
      LOG_WARN("One or more of the buildings to merge are empty.  Skipping merge...");
      if (building1.get())
      {
        LOG_VART(building1->getElementId());
      }
      else
      {
        LOG_TRACE("Building one null.");
      }
      if (building2.get())
      {
        LOG_VART(building2->getElementId());
      }
      else
      {
        LOG_TRACE("Building two null.");
      }
    }
    else if (logWarnCount == Log::getWarnMessageLimit())
    {
      LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
    }
    logWarnCount++;

    return ElementId();
  }

  if (nodeCount1 == nodeCount2)
  {
    LOG_TRACE("Buildings have equally complex geometries.  Keeping the first building geometry...");
    return building1->getElementId();
  }
  else
  {
    if (nodeCount1 > nodeCount2)
    {
      LOG_TRACE("The first building is more complex.");
      return building1->getElementId();
    }
    else
    {
      LOG_TRACE("The second building is more complex.");
      return building2->getElementId();
    }
  }
}

QSet<ElementId> BuildingMerger::_getMultiPolyMemberIds(const ConstElementPtr& element) const
{
  QSet<ElementId> relationMemberIdsToRemove;
  if (element->getElementType() == ElementType::Relation)
  {
    ConstRelationPtr relation = std::dynamic_pointer_cast<const Relation>(element);
    if (relation->getType() == MetadataTags::RelationMultiPolygon())
    {
      const vector<RelationData::Entry>& entries = relation->getMembers();
      for (size_t i = 0; i < entries.size(); i++)
      {
        const RelationData::Entry entry = entries[i];
        const QString role = entry.getRole();
        if (entry.getElementId().getType() == ElementType::Way &&
            (role == MetadataTags::RoleInner() || role == MetadataTags::RoleOuter()))
        {
          relationMemberIdsToRemove.insert(entry.getElementId());
        }
      }
    }
  }
  return relationMemberIdsToRemove;
}

std::shared_ptr<Element> BuildingMerger::buildBuilding(
  const OsmMapPtr& map, const set<ElementId>& eid, const bool preserveTypes)
{
  if (!eid.empty())
  {
    LOG_TRACE("Creating building for eid's: " << eid << "...");
  }

  if (eid.empty())
  {
    throw IllegalArgumentException("No element ID passed to building builder.");
  }
  else if (eid.size() == 1)
  {
    return map->getElement(*eid.begin());
  }
  else
  {
    LOG_VART(preserveTypes);

    vector<std::shared_ptr<Element>> constituentBuildings;
    vector<ElementId> toRemove;
    constituentBuildings.reserve(eid.size());
    OverwriteTagMerger tagMerger;
    for (set<ElementId>::const_iterator it = eid.begin(); it != eid.end(); ++it)
    {
      std::shared_ptr<Element> e = map->getElement(*it);
      LOG_VART(e);
      bool isBuilding = false;
      if (e && e->getElementType() == ElementType::Relation)
      {
        RelationPtr r = std::dynamic_pointer_cast<Relation>(e);

        // If its a building relation or a building represented by a multipoly relation...
        if (r->getType() == MetadataTags::RelationBuilding() ||
            (r->getType() == MetadataTags::RelationMultiPolygon() &&
             BuildingCriterion().isSatisfied(r)))
        {
          LOG_VART(r);
          isBuilding = true;

          // Go through each of the members.
          // This is odd. Originally I had a const reference to the result, but that was causing
          // an obscure segfault. I changed it to a copy and everything is happy. I don't know
          // when/where the reference would be changing, but I also don't think this will be
          // a significant optimization issue.
          vector<RelationData::Entry> m = r->getMembers();
          for (size_t i = 0; i < m.size(); ++i)
          {
            RelationData::Entry constituentBuildingMember = m[i];

            // If its a building relation and the member is a building part,
            if ((r->getType() == MetadataTags::RelationBuilding() &&
                constituentBuildingMember.getRole() == MetadataTags::RolePart()) ||
            // or its a multipoly relation and the member is an inner/outer part...
                (r->getType() == MetadataTags::RelationMultiPolygon() &&
                 (constituentBuildingMember.getRole() == MetadataTags::RoleOuter() ||
                  constituentBuildingMember.getRole() == MetadataTags::RoleInner())))
            {
              std::shared_ptr<Element> constituentBuilding = map->getElement(m[i].getElementId());

              // Push any non-conflicting tags in the parent relation down into the constituent
              // building.
              LOG_TRACE("Constituent building before tag update: " << constituentBuilding);
              constituentBuilding->setTags(
                tagMerger.mergeTags(
                  constituentBuilding->getTags(), r->getTags(),
                  constituentBuilding->getElementType()));

              if (r->getType() == MetadataTags::RelationMultiPolygon())
              {
                // Need to preserve this for later...not sure of a better way to do it. It will
                // get removed during the creation of the relation.
                constituentBuilding->getTags()[MetadataTags::HootMultiPolyRole()] =
                  constituentBuildingMember.getRole();
              }

              // Add the building to the list to be merged into a relation.
              LOG_TRACE("Constituent building after tag update: " << constituentBuilding);
              constituentBuildings.push_back(constituentBuilding);
            }
          }

          // Remove the parent relation, as we'll be creating a new one to contain all the
          // constituent buildings being merged in the next step.
          toRemove.push_back(r->getElementId());
        }
      }

      if (e && !isBuilding)
      {
        // If the building wasn't a relation, then just add the way building on the list of
        // buildings to be merged into a relation.
        LOG_TRACE(
          "BuildingMerger: non-relation building\n" << OsmUtils::getElementDetailString(e, map));
        constituentBuildings.push_back(e);
      }
    }
    LOG_VART(constituentBuildings.size());
    LOG_VART(constituentBuildings);
    LOG_VART(toRemove.size());
    LOG_VART(toRemove);

    // add the constituent buildings to a new relation
    std::shared_ptr<Element> result =
      combineConstituentBuildingsIntoRelation(map, constituentBuildings, preserveTypes);
    LOG_TRACE("Combined constituent buildings into: " << result);

    // remove the relation we previously marked for removal
    std::shared_ptr<DeletableBuildingCriterion> crit =
      std::make_shared<DeletableBuildingCriterion>();
    for (size_t i = 0; i < toRemove.size(); i++)
    {
      if (map->containsElement(toRemove[i]))
      {
        ElementPtr willRemove = map->getElement(toRemove[i]);
        ReplaceElementOp(toRemove[i], result->getElementId()).apply(map);
        RecursiveElementRemover(toRemove[i], false, crit).apply(map);
        // just in case it wasn't removed (e.g. part of another relation)
        willRemove->getTags().clear();
      }
    }

    return result;
  }
}

RelationPtr BuildingMerger::combineConstituentBuildingsIntoRelation(
  const OsmMapPtr& map, std::vector<ElementPtr>& constituentBuildings, const bool preserveTypes)
{
  if (constituentBuildings.empty())
  {
    throw IllegalArgumentException("No constituent buildings passed to building merger.");
  }
  LOG_TRACE("Combining constituent buildings into a relation...");

  // This is primarily put here to support testable output.
  InMemoryElementSorter::sort(constituentBuildings);

  // Just looking for any key that denotes multi-level buildings. Not handling the situation where
  // a non-3D building is merging with a 3D building...not exactly sure what to do there...create
  // both a multipoly and building relation (even though it wouldn't be valid)? No point in worrying
  // about it until its seen in the wild.
  QStringList threeDBuildingKeys;
  threeDBuildingKeys.append(MetadataTags::BuildingLevels());
  threeDBuildingKeys.append(MetadataTags::BuildingHeight());
  const bool allAreBuildingParts =
    TagUtils::allElementsHaveAnyTagKey(threeDBuildingKeys, constituentBuildings);
  LOG_VART(allAreBuildingParts);
  // Here, we're skipping a building relation and doing a multipoly if only some of the buildings
  // have height tags. This behavior is debatable...
  if (!allAreBuildingParts &&
      TagUtils::anyElementsHaveAnyTagKey(threeDBuildingKeys, constituentBuildings))
  {
    if (logWarnCount < Log::getWarnMessageLimit())
    {
      // used to actually log a warning for this but seem excessive...still going to limit it like
      // a warning, though.
      LOG_TRACE(
        "Merging building group where some buildings have 3D tags and others do not. A " <<
        "multipolygon relation will be created instead of a building relation. Buildings: " <<
        ElementIdUtils::elementsToElementIds(constituentBuildings));
    }
    else if (logWarnCount == Log::getWarnMessageLimit())
    {
      LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
    }
    logWarnCount++;
    LOG_VART(OsmUtils::getElementsDetailString(constituentBuildings, map));
  }

  // put the building parts into a relation
  QString relationType = MetadataTags::RelationMultiPolygon();
  if (allAreBuildingParts)
  {
    relationType = MetadataTags::RelationBuilding();
  }
  RelationPtr parentRelation =
    std::make_shared<Relation>(
      constituentBuildings[0]->getStatus(), map->createNextRelationId(),
      WorstCircularErrorVisitor::getWorstCircularError(constituentBuildings), relationType);
  LOG_VART(parentRelation->getElementId());

  TagMergerPtr tagMerger;
  LOG_VART(preserveTypes);
  QSet<QString> overwriteExcludeTags;
  if (allAreBuildingParts)
  {
    // exclude building part type tags from the type tag preservation by passing them in to be
    // skipped
    overwriteExcludeTags = BuildingRelationMemberTagMerger::getBuildingPartTagNames();
  }
  LOG_VART(overwriteExcludeTags);
  if (!preserveTypes)
  {
    tagMerger = std::make_shared<BuildingRelationMemberTagMerger>(overwriteExcludeTags);
  }
  else
  {
    tagMerger = std::make_shared<PreserveTypesTagMerger>(overwriteExcludeTags);
  }

  Tags& relationTags = parentRelation->getTags();
  LOG_TRACE("Parent relation starting tags:" << relationTags);
  for (size_t i = 0; i < constituentBuildings.size(); i++)
  {
    ElementPtr constituentBuilding = constituentBuildings[i];
    if (allAreBuildingParts)
    {
      parentRelation->addElement(MetadataTags::RolePart(), constituentBuilding);
    }
    else
    {
      // If the building was originally pulled out of a relation, remove the temp role tag.
      if (constituentBuilding->getTags().contains(MetadataTags::HootMultiPolyRole()))
      {
        parentRelation->addElement(
          constituentBuilding->getTags()[MetadataTags::HootMultiPolyRole()], constituentBuilding);
        constituentBuilding->getTags().remove(MetadataTags::HootMultiPolyRole());
      }
      // Otherwise, it was a matched building to be grouped together with an outer role (think this
      // will always be true...).
      else
      {
        parentRelation->addElement(MetadataTags::RoleOuter(), constituentBuilding);
      }
    }
    relationTags =
      tagMerger->mergeTags(
        parentRelation->getTags(), constituentBuilding->getTags(), ElementType::Relation);
    parentRelation->setTags(relationTags);
  }
  if (!parentRelation->getTags().contains("building"))
  {
    parentRelation->getTags()["building"] = "yes";
  }
  relationTags = parentRelation->getTags();
  LOG_VART(relationTags);

  const bool isAttributeConflate =
    ConfigOptions().getGeometryLinearMergerDefault() == LinearTagOnlyMerger::className();
  // Doing this for multipoly relations in Attribute Conflation only for the time being.
  const bool suppressBuildingTagOnConstituents =
    // relatively loose way to identify AC; also used in ConflateExecutor
    isAttributeConflate &&
    // allAreBuildingParts = building relation
    !allAreBuildingParts &&
    ConfigOptions().getAttributeConflationSuppressBuildingTagOnMultipolyRelationConstituents();
  LOG_VART(suppressBuildingTagOnConstituents);
  for (Tags::const_iterator it = relationTags.begin(); it != relationTags.end(); ++it)
  {
    // Remove any tags in the parent relation from each of the constituent buildings.
    for (size_t i = 0; i < constituentBuildings.size(); i++)
    {
      ElementPtr constituentBuilding = constituentBuildings[i];
      // leave building=* on the relation member; remove status here since it will be on the parent
      // relation as conflated
      const bool isBuildingTag = it.key() == "building";
      if ((!isBuildingTag || (isBuildingTag && suppressBuildingTagOnConstituents)) &&
          (constituentBuilding->getTags().contains(it.key()) ||
           it.key() == MetadataTags::HootStatus()))
      {
        constituentBuilding->getTags().remove(it.key());
      }
    }
  }

  // If we're dealing with a building relation, replace the building tag on the constituents with a
  // building:part tag.
  if (allAreBuildingParts)
  {
    for (size_t i = 0; i < constituentBuildings.size(); i++)
    {
      ElementPtr constituentBuilding = constituentBuildings[i];
      constituentBuilding->getTags().remove("building");
      constituentBuilding->getTags()[MetadataTags::BuildingPart()] = "yes";
    }
  }

  LOG_VART(parentRelation);
  for (size_t i = 0; i < constituentBuildings.size(); i++)
  {
    LOG_VART(constituentBuildings[i]);
  }

  map->addRelation(parentRelation);
  return parentRelation;
}

std::shared_ptr<Element> BuildingMerger::_buildBuilding(
  const OsmMapPtr& map, const bool unknown1) const
{
  set<ElementId> eids;
  for (set<pair<ElementId, ElementId>>::const_iterator it = _pairs.begin(); it != _pairs.end();
       ++it)
  {
    if (unknown1)
    {
      eids.insert(it->first);
    }
    else
    {
      eids.insert(it->second);
    }
  }
  return buildBuilding(map, eids, _manyToManyMatch && _mergeManyToManyMatches);
}

void BuildingMerger::_fixStatuses(OsmMapPtr map)
{
  UniqueElementIdVisitor idVis;
  map->visitRo(idVis);
  const QList<ElementId> idVisList =
    QList<ElementId>::fromStdList(
      std::list<ElementId>(idVis.getElementSet().begin(), idVis.getElementSet().end()));
  ElementPtr firstElement = map->getElement(idVisList.at(0));
  ElementPtr secondElement = map->getElement(idVisList.at(1));
  // not handling invalid statuses here like is done in PoiPolygonMerger::mergePoiAndPolygon b/c
  // not sure how to do it yet
  if (firstElement->getStatus() == Status::Conflated &&
      secondElement->getStatus() != Status::Conflated)
  {
    if (secondElement->getStatus() == Status::Unknown1)
    {
      firstElement->setStatus(Status::Unknown2);
    }
    else if (secondElement->getStatus() == Status::Unknown2)
    {
      firstElement->setStatus(Status::Unknown1);
    }
  }
  else if (secondElement->getStatus() == Status::Conflated &&
           firstElement->getStatus() != Status::Conflated)
  {
    if (firstElement->getStatus() == Status::Unknown1)
    {
      secondElement->setStatus(Status::Unknown2);
    }
    else if (firstElement->getStatus() == Status::Unknown2)
    {
      secondElement->setStatus(Status::Unknown1);
    }
  }
}

void BuildingMerger::merge(OsmMapPtr map, const ElementId& mergeTargetId)
{
  LOG_INFO("Merging buildings...");

  // The building merger by default uses geometric complexity (node count) to determine which
  // building geometry to keep.  Since the UI at this point will never pass in buildings with their
  // child nodes, we want to override the default behavior and make sure the building merger always
  // arbitrarily keeps the geometry of the first building passed in.  This is ok, b/c the UI
  // workflow lets the user select which building to keep and using complexity wouldn't make sense.
  LOG_VART(ConfigOptions::getBuildingKeepMoreComplexGeometryWhenAutoMergingKey());
  conf().set(
    ConfigOptions::getBuildingKeepMoreComplexGeometryWhenAutoMergingKey(), "false");
  LOG_VART(ConfigOptions().getBuildingKeepMoreComplexGeometryWhenAutoMerging());

  // See related note about statuses in PoiPolygonMerger::mergePoiAndPolygon. Don't know how to
  // handle this situation for more than two buildings yet. The logic below will fail in situations
  // where we have more than one conflated building as input...haven't seen that in the wild yet
  // though.
  if (map->getElementCount() == 2)
  {
    _fixStatuses(map);
  }

  // Formerly, we required that the buildings have a status other than conflated in order to be
  // merged...can't remember exactly why...but removed that stipulation and test output still looks
  // good.

  int buildingsMerged = 0;
  BuildingCriterion buildingCrit;

  const WayMap ways = map->getWays();
  for (WayMap::const_iterator wayItr = ways.begin(); wayItr != ways.end(); ++wayItr)
  {
    const ConstWayPtr& way = wayItr->second;
    if (way->getElementId() != mergeTargetId && buildingCrit.isSatisfied(way))
    {
      LOG_VART(way->getElementId());
      std::set<std::pair<ElementId, ElementId>> pairs;
      pairs.emplace(mergeTargetId, way->getElementId());
      BuildingMerger merger(pairs);
      LOG_VART(pairs.size());
      std::vector<std::pair<ElementId, ElementId>> replacedElements;
      merger.apply(map, replacedElements);
      buildingsMerged++;
    }
  }

  const RelationMap relations = map->getRelations();
  for (RelationMap::const_iterator relItr = relations.begin(); relItr != relations.end(); ++relItr)
  {
    const ConstRelationPtr& relation = relItr->second;
    if (relation->getElementId() != mergeTargetId && buildingCrit.isSatisfied(relation))
    {
      LOG_VART(relation->getElementId());
      std::set<std::pair<ElementId, ElementId>> pairs;
      pairs.emplace(mergeTargetId, relation->getElementId());
      BuildingMerger merger(pairs);
      LOG_VART(pairs.size());
      std::vector<std::pair<ElementId, ElementId>> replacedElements;
      merger.apply(map, replacedElements);
      buildingsMerged++;
    }
  }

  LOG_INFO("Merged " << buildingsMerged << " buildings.");
}

QString BuildingMerger::toString() const
{
  return QString("BuildingMerger %1").arg(hoot::toString(_pairs));
}

}
