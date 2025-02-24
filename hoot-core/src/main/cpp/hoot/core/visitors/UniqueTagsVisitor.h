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
 * @copyright Copyright (C) 2021 Maxar (http://www.maxar.com/)
 */
#ifndef UNIQUE_TAGS_VISITOR_H
#define UNIQUE_TAGS_VISITOR_H

// hoot
#include <hoot/core/visitors/ConstElementVisitor.h>

namespace hoot
{

/**
 * Collects unique tags
 */
class UniqueTagsVisitor : public ConstElementVisitor
{
public:

  static QString className() { return "UniqueTagsVisitor"; }

  UniqueTagsVisitor() = default;
  ~UniqueTagsVisitor() override = default;

  /**
   * @see ElementVisitor
   */
  void visit(const ConstElementPtr& e) override;

  QString getDescription() const override { return "Collects unique tags"; }
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }

  std::set<QString> getUniqueKvps() const { return _uniqueKvps; }

private:

  std::set<QString> _uniqueKvps;
};

}

#endif // UNIQUE_TAG_VALUES_VISITOR_H
