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
#ifndef __TAG_DIFFERENCER_JS_H__
#define __TAG_DIFFERENCER_JS_H__

// hoot
#include <hoot/js/HootBaseJs.h>

namespace hoot
{

class TagDifferencer;

class TagDifferencerJs : public HootBaseJs
{
public:

  ~TagDifferencerJs() override = default;

  static void Init(v8::Local<v8::Object> target);

  TagDifferencer* getDifferencer() { return _td.get(); }

private:

  TagDifferencerJs(std::shared_ptr<TagDifferencer> op);

  static void diff(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  QString _className;
  std::shared_ptr<TagDifferencer> _td;
};

}

#endif // __TAG_DIFFERENCER_JS_H__
