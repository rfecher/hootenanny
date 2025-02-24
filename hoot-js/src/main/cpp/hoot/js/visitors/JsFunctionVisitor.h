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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2020, 2021 Maxar (http://www.maxar.com/)
 */
#ifndef JSFUNCTIONVISITOR_H
#define JSFUNCTIONVISITOR_H

// hoot
#include <hoot/core/elements/ConstOsmMapConsumer.h>
#include <hoot/core/visitors/ConstElementVisitor.h>
#include <hoot/js/util/JsFunctionConsumer.h>
#include <hoot/js/SystemNodeJs.h>

namespace hoot
{

/**
 * A criterion that will either keep or remove matches.
 */
class JsFunctionVisitor : public ConstElementVisitor, public ConstOsmMapConsumer,
  public JsFunctionConsumer
{
public:

  static QString className() { return "JsFunctionVisitor"; }

  JsFunctionVisitor();
  ~JsFunctionVisitor() override = default;

  void addFunction(v8::Isolate* isolate, v8::Local<v8::Function>& func) override
  { _func.Reset(isolate, func); }

  void setOsmMap(OsmMap* map) override { _map = map; }

  void setOsmMap(const OsmMap*) override { }

  void visit(const ConstElementPtr& e) override;

  QString getDescription() const override { return ""; }
  QString getName() const override { return ""; }
  QString getClassName() const override { return ""; }

private:

  v8::Persistent<v8::Function> _func;
  OsmMap* _map;
};

}

#endif // JSFUNCTIONVISITOR_H
