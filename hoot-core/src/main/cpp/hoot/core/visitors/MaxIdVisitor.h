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

#ifndef MAXIDVISITOR_H
#define MAXIDVISITOR_H

// hoot
#include <hoot/core/visitors/ConstElementVisitor.h>
#include <hoot/core/info/SingleStatistic.h>

// Standard
#include <limits>

namespace hoot
{

/**
 * @brief The MaxIdVisitor class finds the largest element ID value in a map (least negative).
 */
class MaxIdVisitor : public ConstElementVisitor, public SingleStatistic
{
public:

  static QString className() { return "MaxIdVisitor"; }

  MaxIdVisitor();
  ~MaxIdVisitor() override = default;

  double getStat() const override { return _maxId; }

  void visit(const ConstElementPtr& e) override;

  QString getDescription() const override
  { return "Returns the largest element ID value found"; }
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }

  long getMinId() const { return _maxId; }

private:

  long _maxId;
};

}
#endif // MAXIDVISITOR_H
