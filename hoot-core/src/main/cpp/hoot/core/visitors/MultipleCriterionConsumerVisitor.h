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
 * @copyright Copyright (C) 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */
#ifndef MULTIPLE_CRITERION_CONSUMER_VISITOR_H
#define MULTIPLE_CRITERION_CONSUMER_VISITOR_H

// hoot
#include <hoot/core/visitors/ElementVisitor.h>
#include <hoot/core/criterion/ElementCriterionConsumer.h>

namespace hoot
{

/**
 * Base class for a visitor that consumes multiple element criteria
 *
 * Should we make this inherit ConstElementVisitor instead, if that's possible?
 */
class MultipleCriterionConsumerVisitor : public ElementVisitor, public ElementCriterionConsumer
{
public:

  static QString className() { return "MultipleCriterionConsumerVisitor"; }

  MultipleCriterionConsumerVisitor();
  ~MultipleCriterionConsumerVisitor() override = default;

  void addCriterion(const ElementCriterionPtr& crit) override;

  QString toString() const override;
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }

  void setChainCriteria(bool chain) { _chainCriteria = chain; }
  void setNegateCriteria(bool negate) { _negateCriteria = negate; }
  void setConfigureChildren(bool configure) { _configureChildren = configure; }

protected:

  std::vector<ElementCriterionPtr> _criteria;
  //This allows for negating the criterion as an option sent in from the command line.
  bool _negateCriteria;
  bool _chainCriteria;
  bool _configureChildren;

  bool _criteriaSatisfied(const ConstElementPtr& e) const;
  void _addCriteria(const QStringList& criteriaClassNames);
};


}

#endif // MULTIPLE_CRITERION_CONSUMER_VISITOR_H
