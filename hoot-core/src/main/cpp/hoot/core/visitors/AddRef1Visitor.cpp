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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2021 Maxar (http://www.maxar.com/)
 */
#include "AddRef1Visitor.h"

// hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/schema/MetadataTags.h>

namespace hoot
{

HOOT_FACTORY_REGISTER(ElementVisitor, AddRef1Visitor)

AddRef1Visitor::AddRef1Visitor()
{
  _count = 0;
  setConfiguration(conf());
}

void AddRef1Visitor::visit(const ElementPtr& e)
{
  if (_informationOnly == false || e->getTags().getNonDebugCount() > 0)
  {
    e->getTags()[MetadataTags::Ref1()] = _prefix + QString("%1").arg(_count++, 6, 16, QChar('0'));
  }
}

void AddRef1Visitor::setConfiguration(const Settings& conf)
{
  _prefix = ConfigOptions(conf).getAddRef1VisitorPrefix();
  _informationOnly = ConfigOptions(conf).getAddRefVisitorInformationOnly();
}

}
