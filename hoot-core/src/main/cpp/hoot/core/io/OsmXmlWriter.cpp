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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */
#include "OsmXmlWriter.h"

// Hoot
#include <hoot/core/elements/Node.h>
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/elements/Relation.h>
#include <hoot/core/elements/Tags.h>
#include <hoot/core/elements/Way.h>
#include <hoot/core/index/OsmMapIndex.h>
#include <hoot/core/schema/MetadataTags.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/DateTimeUtils.h>
#include <hoot/core/util/Factory.h>
#include <hoot/core/util/StringUtils.h>
#include <hoot/core/visitors/CalculateMapBoundsVisitor.h>

// Qt
#include <QBuffer>
#include <QDateTime>
#include <QXmlStreamWriter>

using namespace geos::geom;
using namespace std;

namespace hoot
{

int OsmXmlWriter::logWarnCount = 0;

HOOT_FACTORY_REGISTER(OsmMapWriter, OsmXmlWriter)

OsmXmlWriter::OsmXmlWriter()
  : _formatXml(ConfigOptions().getWriterXmlFormat()),
    _includePointInWays(false),
    _includeCompatibilityTags(true),
    _sortTags(ConfigOptions().getWriterSortTagsByKey()),
    _osmSchema(ConfigOptions().getMapWriterSchema()),
    _precision(ConfigOptions().getWriterPrecision()),
    _encodingErrorCount(0),
    _numWritten(0),
    _statusUpdateInterval(ConfigOptions().getTaskStatusUpdateInterval() * 10)
{
}

OsmXmlWriter::~OsmXmlWriter()
{
  close();
}

QString OsmXmlWriter::removeInvalidCharacters(const QString& s)
{
  // See #3553 for an explanation.

  QString result;
  result.reserve(s.size());

  bool foundError = false;
  for (int i = 0; i < s.size(); i++)
  {
    QChar c = s[i];
    // See http://stackoverflow.com/questions/730133/invalid-characters-in-xml
    if (c == 0x1f)
      result.append('|');
    else if (c < 0x20 && c != 0x9 && c != 0xA && c != 0xD)
      foundError = true;
    else
      result.append(c);
  }

  if (foundError)
  {
    _encodingErrorCount++;
    if (logWarnCount < Log::getWarnMessageLimit())
    {
      LOG_WARN("Found an invalid character in string: '" << s << "'");
      LOG_WARN("  UCS-4 version of the string: " << s.toUcs4());
    }
    else if (logWarnCount == Log::getWarnMessageLimit())
    {
      LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
    }
    logWarnCount++;
  }

  return result;
}

void OsmXmlWriter::open(const QString& url)
{
  std::shared_ptr<QFile> f = std::make_shared<QFile>();
  f->setFileName(url);
  _fp = f;
  if (!_fp->open(QIODevice::WriteOnly | QIODevice::Text))
    throw HootException(QObject::tr("Error opening %1 for writing").arg(url));

  _initWriter();

  _bounds.init();

  _numWritten = 0;
}

void OsmXmlWriter::close()
{
  if (_fp.get() && _fp->isOpen())
  {
    if (_writer.get())
    {
      _writer->writeEndElement();
      _writer->writeEndDocument();
    }

    _fp->close();
  }
}

QString OsmXmlWriter::toString(const ConstOsmMapPtr& map, const bool formatXml)
{
  LOG_DEBUG("Writing map to xml string...");

  OsmXmlWriter writer;
  writer.setFormatXml(formatXml);
  // This will be deleted by the _fp std::shared_ptr.
  std::shared_ptr<QBuffer> buf = std::make_shared<QBuffer>();
  writer._fp = buf;
  if (!writer._fp->open(QIODevice::WriteOnly | QIODevice::Text))
    throw InternalErrorException(QObject::tr("Error opening QBuffer for writing. Odd."));

  writer.write(map);
  return QString::fromUtf8(buf->data(), buf->size());
}

QString OsmXmlWriter::_typeName(ElementType e)
{
  switch(e.getEnum())
  {
  case ElementType::Node:
    return "node";
  case ElementType::Way:
    return "way";
  case ElementType::Relation:
    return "relation";
  default:
    throw HootException("Unexpected element type.");
  }
}

void OsmXmlWriter::_initWriter()
{
  _writer = std::make_shared<QXmlStreamWriter>(_fp.get());
  _writer->setCodec("UTF-8");

  if (_formatXml)
    _writer->setAutoFormatting(true);

  _writer->writeStartDocument();

  _writer->writeStartElement("osm");
  _writer->writeAttribute("version", "0.6");
  _writer->writeAttribute("generator", HOOT_PACKAGE_NAME);
}

void OsmXmlWriter::write(const ConstOsmMapPtr& map, const QString& path)
{
  open(path);
  write(map);
}

void OsmXmlWriter::write(const ConstOsmMapPtr& map)
{
  if (!_fp.get() || _fp->isWritable() == false)
    throw HootException("Please open the file before attempting to write.");

  //Some code paths don't call the open method before invoking this write method, so make sure the
  //writer has been initialized.
  if (!_writer.get())
    _initWriter();

  //  Debug maps get a bunch of debug settings setup here
  LOG_VART(getIsDebugMap());
  if (getIsDebugMap())
    _overrideDebugSettings();

  // The coord sys and schema entries don't get written to streamed output b/c we don't have
  // the map object to read the coord sys from.

  int epsg = map->getProjection()->GetEPSGGeogCS();
  if (epsg > -1)
    _writer->writeAttribute("srs", QString("+epsg:%1").arg(epsg));
  else
  {
    char *wkt;
    map->getProjection()->exportToWkt(&wkt);
    _writer->writeAttribute("srs", wkt);
    free(wkt);
  }

  if (_osmSchema != "")
    _writer->writeAttribute("schema", _osmSchema);

  //  Osmosis chokes on the bounds being written at the end of the file, so write it first
  const geos::geom::Envelope bounds = CalculateMapBoundsVisitor::getGeosBounds(map);
  _writeBounds(bounds);

  _writeNodes(map);
  _writeWays(map);
  _writeRelations(map);

  close();
}

void OsmXmlWriter::_writeMetadata(const Element* e) const
{
  if (_includeCompatibilityTags)
  {
    _writer->writeAttribute("timestamp", DateTimeUtils::toTimeString(e->getTimestamp()));
    long version = e->getVersion();
    if (version == ElementData::VERSION_EMPTY)
      version = 1;

    _writer->writeAttribute("version", QString::number(version));
  }
  else
  {
    // This comparison seems to be still unequal when I set an element's timestamp to
    // ElementData::TIMESTAMP_EMPTY.  See RemoveAttributesVisitor
    if (e->getTimestamp() != ElementData::TIMESTAMP_EMPTY)
      _writer->writeAttribute("timestamp", DateTimeUtils::toTimeString(e->getTimestamp()));
    if (e->getVersion() != ElementData::VERSION_EMPTY)
      _writer->writeAttribute("version", QString::number(e->getVersion()));
  }
  //  Negative IDs are considered "new" elements and shouldn't have a changeset.
  if (e->getChangeset() != ElementData::CHANGESET_EMPTY && e->getId() > 0)
    _writer->writeAttribute("changeset", QString::number(e->getChangeset()));
  if (e->getUser() != ElementData::USER_EMPTY)
    _writer->writeAttribute("user", e->getUser());
  if (e->getUid() != ElementData::UID_EMPTY)
    _writer->writeAttribute("uid", QString::number(e->getUid()));
}

void OsmXmlWriter::_writeTags(const ConstElementPtr& element)
{
  //  Get the tags to write
  Tags tags = _getElementTags(element);

  //  Sort the keys for output
  QList<QString> keys = tags.keys();
  if (_sortTags)
    keys.sort();

  //  Write out the tags with their key/value pairs
  for (const auto& key : keys)
  {
    QString val = tags.get(key).trimmed();
    if (!val.isEmpty())
    {
      _writer->writeStartElement("tag");
      LOG_VART(key);
      _writer->writeAttribute("k", removeInvalidCharacters(key));
      LOG_VART(val);
      _writer->writeAttribute("v", removeInvalidCharacters(val));
      _writer->writeEndElement();
    }
  }
}

void OsmXmlWriter::_writeNodes(ConstOsmMapPtr map)
{
  QList<long> nids;
  const NodeMap& nodes = map->getNodes();
  for (auto it = nodes.begin(); it != nodes.end(); ++it)
    nids.append(it->first);

  // Sort the values to give consistent results.
  if (nids.size() > 100000)
  {
    LOG_INFO("Sorting nodes...");
  }
  qSort(nids.begin(), nids.end(), qLess<long>());
  for (auto nid : nids)
    writePartial(map->getNode(nid));
}

void OsmXmlWriter::_writeWays(ConstOsmMapPtr map)
{
  QList<long> wids;
  const WayMap& ways = map->getWays();
  for (auto it = ways.begin(); it != ways.end(); ++it)
    wids.append(it->first);

  // sort the values to give consistent results.
  if (wids.size() > 100000)
  {
    LOG_INFO("Sorting ways...");
  }
  qSort(wids.begin(), wids.end(), qLess<long>());
  for (auto wid : wids)
  {
    //I'm not really sure how to reconcile the duplicated code between these two versions of
    //partial way writing.
    if (_includePointInWays)
      _writePartialIncludePoints(map->getWay(wid), map);
    else
      writePartial(map->getWay(wid));
  }
}

void OsmXmlWriter::_writeRelations(ConstOsmMapPtr map)
{
  QList<long> rids;
  const RelationMap& relations = map->getRelations();
  for (auto it = relations.begin(); it != relations.end(); ++it)
    rids.append(it->first);

  // sort the values to give consistent results.
  if (rids.size() > 100000)
  {
    LOG_INFO("Sorting relations...");
  }
  qSort(rids.begin(), rids.end(), qLess<long>());
  for (auto rid : rids)
    writePartial(map->getRelation(rid));
}

void OsmXmlWriter::_writeBounds(const Envelope& bounds) const
{
  _writer->writeStartElement("bounds");
  _writer->writeAttribute("minlat", QString::number(bounds.getMinY(), 'g', _precision));
  _writer->writeAttribute("minlon", QString::number(bounds.getMinX(), 'g', _precision));
  _writer->writeAttribute("maxlat", QString::number(bounds.getMaxY(), 'g', _precision));
  _writer->writeAttribute("maxlon", QString::number(bounds.getMaxX(), 'g', _precision));
  _writer->writeEndElement();
}

void OsmXmlWriter::writePartial(const ConstNodePtr& n)
{
  LOG_TRACE("Writing " << n->getElementId() << "...");

  _writer->writeStartElement("node");

  _writer->writeAttribute("visible", "true");
  _writer->writeAttribute("id", QString::number(n->getId()));

  _writeMetadata(n.get());

  _writer->writeAttribute("lat", QString::number(n->getY(), 'f', _precision));
  _writer->writeAttribute("lon", QString::number(n->getX(), 'f', _precision));

  _writeTags(n);

  _writer->writeEndElement();

  _bounds.expandToInclude(n->getX(), n->getY());

  _numWritten++;
  if (_numWritten % _statusUpdateInterval == 0)
  {
    PROGRESS_STATUS(
      "Wrote " << StringUtils::formatLargeNumber(_numWritten) << " elements to output.");
  }
}

void OsmXmlWriter::_writePartialIncludePoints(const ConstWayPtr& w, ConstOsmMapPtr map)
{
  //  Ignore NULL elements
  if (!w) return;

  LOG_TRACE("Writing " << w->getElementId() << "...");

  _writer->writeStartElement("way");
  _writer->writeAttribute("visible", "true");
  _writer->writeAttribute("id", QString::number(w->getId()));

  _writeMetadata(w.get());

  for (auto nid : w->getNodeIds())
  {
    _writer->writeStartElement("nd");
    _writer->writeAttribute("ref", QString::number(nid));
    if (_includePointInWays)
    {
      ConstNodePtr n = map->getNode(nid);
      _writer->writeAttribute("x", QString::number(n->getX(), 'g', _precision));
      _writer->writeAttribute("y", QString::number(n->getY(), 'g', _precision));
    }
    _writer->writeEndElement();
  }

  const Tags& tags = w->getTags();

  for (auto it = tags.constBegin(); it != tags.constEnd(); ++it)
  {
    QString key = it.key();
    QString val = it.value().trimmed();
    if (val.isEmpty() == false)
    {
      _writer->writeStartElement("tag");
      _writer->writeAttribute("k", removeInvalidCharacters(key));
      _writer->writeAttribute("v", removeInvalidCharacters(val));
      _writer->writeEndElement();
    }
  }

  _writer->writeEndElement();
}

void OsmXmlWriter::writePartial(const ConstWayPtr& w)
{
  LOG_TRACE("Writing " << w->getElementId() << "...");

  if (_includePointInWays)
    throw HootException("Adding points to way output is not supported in streaming output.");

  _writer->writeStartElement("way");
  _writer->writeAttribute("visible", "true");
  _writer->writeAttribute("id", QString::number(w->getId()));

  _writeMetadata(w.get());

  for (auto nid : w->getNodeIds())
  {
    _writer->writeStartElement("nd");
    _writer->writeAttribute("ref", QString::number(nid));
    _writer->writeEndElement();
  }

  _writeTags(w);

  _writer->writeEndElement();

  _numWritten++;
  if (_numWritten % _statusUpdateInterval == 0)
  {
    PROGRESS_STATUS(
      "Wrote " << StringUtils::formatLargeNumber(_numWritten) << " elements to output.");
  }
}

void OsmXmlWriter::writePartial(const ConstRelationPtr& r)
{
  LOG_TRACE("Writing " << r->getElementId() << "...");

  _writer->writeStartElement("relation");
  _writer->writeAttribute("visible", "true");
  _writer->writeAttribute("id", QString::number(r->getId()));

  _writeMetadata(r.get());

  const vector<RelationData::Entry>& members = r->getMembers();
  for (const auto& member : members)
  {
    _writer->writeStartElement("member");
    _writer->writeAttribute("type", _typeName(member.getElementId().getType()));
    _writer->writeAttribute("ref", QString::number(member.getElementId().getId()));
    _writer->writeAttribute("role", removeInvalidCharacters(member.getRole()));
    _writer->writeEndElement();
  }

  _writeTags(r);

  _writer->writeEndElement();

  _numWritten++;
  if (_numWritten % _statusUpdateInterval == 0)
  {
    PROGRESS_STATUS(
      "Wrote " << StringUtils::formatLargeNumber(_numWritten) << " elements to output.");
  }
}

void OsmXmlWriter::finalizePartial()
{
  close();
}

void OsmXmlWriter::_overrideDebugSettings()
{
  // include circular error, text status and debug
  _addExportTagsVisitor.overrideDebugSettings();
  //  Include parent ID tag
  setIncludePid(true);
  //  Sort the tags by key
  _sortTags = true;
}

}
