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
#ifndef OSMMAPOPERATIONJS_H
#define OSMMAPOPERATIONJS_H

// Hoot
#include <hoot/js/HootBaseJs.h>

namespace hoot
{

class OsmMapOperation;

class OsmMapOperationJs : public HootBaseJs
{
public:

  ~OsmMapOperationJs() override = default;

  static void Init(v8::Local<v8::Object> target);

  OsmMapOperation* getMapOp() { return _op.get(); }

private:

  OsmMapOperationJs(std::shared_ptr<OsmMapOperation> op);

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void apply(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void applyAndGetResult(const v8::FunctionCallbackInfo<v8::Value>& args);

  QString _className;
  std::shared_ptr<OsmMapOperation> _op;
};

}

#endif // OSMMAPOPJS_H
