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
 * @copyright Copyright (C) 2015, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */

#ifndef TO_ENGLISH_DICTIONARY_TRANSLATOR_H
#define TO_ENGLISH_DICTIONARY_TRANSLATOR_H

// Hoot
#include <hoot/core/language/ToEnglishTranslator.h>

// Qt
#include <QRegExp>

// ICU
#include <unicode/translit.h>

namespace hoot
{

class ToEnglishTranslateDictionary;

/**
 * This makes a reasonable attempt to translate or transliterate common map words in and pronouns
 * into english/latin characters. This is by no means an exhaustive translator and does not have
 * any concept of grammar. If the word is unknown it is assumed to be a pronoun and will be
 * transliterated into latin characters.
 */
class ToEnglishDictionaryTranslator : public ToEnglishTranslator
{
public:

  static QString className() { return "ToEnglishDictionaryTranslator"; }

  ToEnglishDictionaryTranslator();
  ~ToEnglishDictionaryTranslator() override = default;

  /**
   * Translates the given input string into a translation & transliteration of the input.
   */
  QString toEnglish(const QString& input, const bool tokenize = true) const;

  /**
   * @see ToEnglishTranslator; wraps call to toEnglish
   */
  QString translate(const QString& textToTranslate) override;

  /**
   * Converts the given input string into all possible known translations. E.g.
   * using the dictionary as of 5/16/2013, "rumah sakit umum" would return:
   * { "home sakit umum",
   *   "house sakit umum",
   *   "hospital umum",
   *   "general hospital" }
   */
  QStringList toEnglishAll(const QString& input);

  /**
   * Similar to above, but the input has already been tokenized.
   */
  QStringList toEnglishAll(const QStringList& l);

  QString toTitleCase(const QString& input) const;

  QString transliterateToLatin(const QString& input) const;

  QString translateStreet(const QString& input) const;

  QStringList getSourceLanguages() const override { return QStringList(); }
  void setSourceLanguages(const QStringList& /*langCodes*/) override { }
  QString getDetectedLanguage() const override { return ""; }

  void setConfiguration(const Settings& /*conf*/) override { }

  void setTokenizeInput(bool tokenize) { _tokenizeInput = tokenize; }

  void setId(const QString& /*id*/) override { }

private:

  bool _tokenizeInput;
  static QSet<QString> _streetTypes;
  QRegExp _whiteSpace;

  QString _transform(const std::shared_ptr<icu::Transliterator>& t, const QString& input) const;
};

}

#endif // TO_ENGLISH_DICTIONARY_TRANSLATOR_H
