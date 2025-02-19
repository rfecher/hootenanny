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
 * @copyright Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */
#ifndef NODESVISITOR_H
#define NODESVISITOR_H

// hoot
#include <hoot/core/visitors/ConstElementVisitor.h>
#include <hoot/core/elements/Node.h>

namespace hoot
{

/**
 * @brief The NodesVisitor class can be used to filter nodes.
 */
class NodesVisitor : public ConstElementVisitor
{
public:

  static QString className() { return "NodesVisitor"; }

  NodesVisitor(QList<ConstNodePtr>& n);
  ~NodesVisitor() override = default;

  /**
   * @see ElementVisitor
   */
  void visit(const std::shared_ptr<const Element>& e) override;

  QString getDescription() const override { return "Collects the nodes visited"; }
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }

private:

  QList<ConstNodePtr>& _n;
};

}

#endif // NODESVISITOR_H
