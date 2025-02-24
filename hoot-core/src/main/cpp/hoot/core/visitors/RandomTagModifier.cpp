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
#include "RandomTagModifier.h"

// boost
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>

// hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/ops/RecursiveElementRemover.h>
#include <hoot/core/util/Settings.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/RandomNumberUtils.h>
#include <hoot/core/schema/MetadataTags.h>
#include <hoot/core/schema/OsmSchema.h>

// Standard
#include <algorithm>

namespace hoot
{

HOOT_FACTORY_REGISTER(ElementVisitor, RandomTagModifier)

RandomTagModifier::RandomTagModifier() :
_localRng(std::make_shared<boost::minstd_rand>())
{
  _rng = _localRng.get();
}

void RandomTagModifier::setConfiguration(const Settings& conf)
{
  ConfigOptions configOptions(conf);
  setProbability(configOptions.getRandomTagModifierProbability());
  const int seed = configOptions.getRandomSeed();
  LOG_VARD(seed);
  if (seed == -1)
  {
    _rng->seed(RandomNumberUtils::generateSeed());
  }
  else
  {
    _rng->seed(seed);
  }

  _exemptTagKeys = configOptions.getRandomTagModifierExemptTagKeys();
  // hardcode adding REF1/REF2 here since they aren't metadata tags; should they be?
  _exemptTagKeys.append(MetadataTags::Ref1());
  _exemptTagKeys.append(MetadataTags::Ref2());

  _replacementTagKeys = configOptions.getRandomTagModifierSubstitutionKeys();
  _replacementTagValues = configOptions.getRandomTagModifierSubstitutionValues();
}

void RandomTagModifier::visit(const std::shared_ptr<Element>& e)
{
  boost::uniform_real<> uni(0.0, 1.0);

  OsmSchema& schema = OsmSchema::getInstance();
  Tags t = e->getTags();
  for (Tags::const_iterator it = t.constBegin(); it != t.constEnd(); ++it)
  {
    const QString tagKey = it.key();
    if (uni(*_rng) <= _p && !_exemptTagKeys.contains(tagKey) && !schema.isMetaData(tagKey, ""))
    {
      if (!_replacementTagKeys.contains(tagKey))
      {
        LOG_TRACE("Removing tag with key: " << tagKey << " ...");
        e->getTags().remove(tagKey);
      }
      else
      {
        const int tagIndex = _replacementTagKeys.indexOf(tagKey);
        const QString tagValue = _replacementTagValues.at(tagIndex);
        LOG_TRACE("Substituting value: " << tagValue << " for tag with key: " << tagKey);
        e->getTags().set(tagKey, tagValue);
      }
    }
  }
}

}
