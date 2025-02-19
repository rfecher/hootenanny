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
 * @copyright Copyright (C) 2015, 2017, 2018, 2019, 2021 Maxar (http://www.maxar.com/)
 */
#include "MostEnglishName.h"

// Hoot
#include <hoot/core/elements/Tags.h>
#include <hoot/core/util/ConfigOptions.h>

// Standard
#include <limits>

using namespace std;

namespace hoot
{

int MostEnglishName::logWarnCount = 0;

MostEnglishNamePtr MostEnglishName::_theInstance;

MostEnglishName::MostEnglishName()
{
  _initialized = false;
  setConfiguration(conf());
}

const MostEnglishNamePtr& MostEnglishName::getInstance()
{
  if (_theInstance.get() == nullptr)
  {
    _theInstance.reset(new MostEnglishName());
  }
  return _theInstance;
}

QString MostEnglishName::getMostEnglishName(const Tags& tags)
{
  if (tags.contains("name:en") && tags.get("name:en").isEmpty() == false)
  {
    return tags.get("name:en");
  }

  QStringList names = tags.getNames();

  double bestScore = -numeric_limits<double>::max();
  QString bestName;

  for (int i = 0; i < names.size(); i++)
  {
    double score = scoreName(names[i]);

    if (score > bestScore)
    {
      bestScore = score;
      bestName = names[i];
    }
  }

  return bestName;
}

const QSet<QString>& MostEnglishName::_getWords()
{
  if (_initialized == false)
  {
    LOG_DEBUG(_wordPaths);
    for (int i = 0; i < _wordPaths.size(); i++)
    {
      if (_loadEnglishWords(_wordPaths[i]) != 0)
      {
        break;
      }
    }

    if (_englishWords.empty())
    {
      if (logWarnCount < Log::getWarnMessageLimit())
      {
        LOG_WARN("Failed to load any English dictionaries. Please modify " +
          ConfigOptions::getEnglishWordsFilesKey() + " dictionary to list an appropriate "
          "dictionary. Search path: " << _wordPaths);
      }
      else if (logWarnCount == Log::getWarnMessageLimit())
      {
        LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
      }
      logWarnCount++;
    }

    LOG_TRACE("Unique (case-insensitive) words: " + QString::number(_englishWords.size()));
    _initialized = true;
  }

  return _englishWords;
}

bool MostEnglishName::isInDictionary(const QString& text)
{
  return _getWords().contains(text.toLower());
}

bool MostEnglishName::areAllInDictionary(const QStringList& texts)
{
  for (int i = 0; i < texts.size(); i++)
  {
    if (!_getWords().contains(texts.at(i).toLower()))
    {
      return false;
    }
  }
  return true;
}

long MostEnglishName::_loadEnglishWords(const QString& path)
{
  QFile fp(path);
  const int MAX_LINE_SIZE = 2048;
  long wordCount = 0;

  if (fp.open(QFile::ReadOnly))
  {
    LOG_DEBUG("Loading English word file: " << path);
    while (!fp.atEnd())
    {
      QByteArray ba = fp.readLine(MAX_LINE_SIZE);
      if (ba.size() == MAX_LINE_SIZE && wordCount < 10)
      {
        if (logWarnCount < Log::getWarnMessageLimit())
        {
          LOG_WARN("Loaded a line of max size. Is this a proper dictionary?");
        }
        else if (logWarnCount == Log::getWarnMessageLimit())
        {
          LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
      }
      QString s = QString::fromUtf8(ba.data());
      wordCount++;

      // all words are converted to lower case. This is more liberal when matching words.
      _englishWords.insert(s.trimmed().toLower());
    }
  }

  return wordCount;
}

bool MostEnglishName::isEnglishText(const QString& text)
{
  return scoreName(text) == 1.0;
}

double MostEnglishName::scoreName(const QString& text)
{
  QStringList words = _tokenizer.tokenize(text);

  double score = 0;
  int characters = 0;

  const QSet<QString>& englishWords = _getWords();
  for (int i = 0; i < words.size(); i++)
  {
    if (englishWords.contains(words[i].toLower()))
    {
      score += words[i].size();
      characters += words[i].size();
    }
    else
    {
      QString s = words[i];

      for (int j = 0; j < s.size(); j++)
      {
        if (s[j].toLatin1() != 0)
        {
          // letters increase the score, numbers have no effect.
          if (s[j].isLetter())
          {
            score += 0.5;
            characters++;
          }
        }
        else if (s[j].isLetter())
        {
          characters++;
        }
      }
    }
  }

  if (characters == 0)
  {
    score = 0.0;
  }
  else
  {
    score = score / characters;
  }

  return score;
}

void MostEnglishName::setConfiguration(const Settings& conf)
{
  if (this == _theInstance.get())
  {
    throw HootException("Please do not set the configuration on the singleton instance.");
  }

  _wordPaths = ConfigOptions(conf).getEnglishWordsFiles();
  _tokenizer.setConfiguration(conf);
  _englishWords.clear();
  _initialized = false;
}

}
