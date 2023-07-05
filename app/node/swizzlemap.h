/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef SWIZZLE_H
#define SWIZZLE_H

#include <cstddef>
#include <map>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/xmlutils.h"

namespace olive {

class SwizzleMap
{
public:
  class From
  {
  public:
    From() = default;

    From(size_t output, size_t element)
    {
      output_ = output;
      element_ = element;
    }

    const size_t &output() const { return output_; }
    const size_t &element() const { return element_; }

    bool operator==(const From &f) const { return output_ == f.output_ && element_ == f.element_; }

  private:
    size_t output_;
    size_t element_;

  };

  class To
  {
  public:
    To() = default;

    To(size_t index, type_t tag)
    {
      index_ = index;
      tag_ = tag;
    }

    const size_t &index() const { return index_; }
    const type_t &tag() const { return tag_; }

    bool operator==(const To &to) const { return index_ == to.index_ && tag_ == to.tag_; }

  private:
    size_t index_;
    type_t tag_;
  };

  SwizzleMap() = default;

  bool empty() const { return map_.empty(); }

  void clear() { map_.clear(); }

  void insert(size_t to, const From &from)
  {
    map_[to] = from;
  }

  void remove(size_t to)
  {
    map_.erase(to);
  }

  void load(QXmlStreamReader *reader)
  {
    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("entry")) {
        bool got_output = false, got_from = false, got_to = false;
        size_t from, to, output;
        while (XMLReadNextStartElement(reader)) {
          if (reader->name() == QStringLiteral("from")) {
            from = reader->readElementText().toULongLong();
            got_from = true;
          } else if (reader->name() == QStringLiteral("output")) {
            output = reader->readElementText().toULongLong();
            got_output = true;
          } else if (reader->name() == QStringLiteral("to")) {
            to = reader->readElementText().toULongLong();
            got_to = true;
          } else {
            reader->skipCurrentElement();
          }
        }

        if (got_output && got_from && got_to) {
          insert(to, From(output, from));
        }
      } else {
        reader->skipCurrentElement();
      }
    }
  }

  void save(QXmlStreamWriter *writer) const
  {
    for (auto it = map_.cbegin(); it != map_.cend(); it++) {
      writer->writeStartElement(QStringLiteral("entry"));
      writer->writeTextElement(QStringLiteral("to"), QString::number(it->first));
      writer->writeTextElement(QStringLiteral("output"), QString::number(it->second.output()));
      writer->writeTextElement(QStringLiteral("from"), QString::number(it->second.element()));
      writer->writeEndElement(); // entry
    }
  }

  bool operator==(const SwizzleMap &m) const { return map_ == m.map_; }
  bool operator!=(const SwizzleMap &m) const { return map_ != m.map_; }

  std::map<size_t, From>::const_iterator cbegin() const { return map_.cbegin(); }
  std::map<size_t, From>::const_iterator cend() const { return map_.cend(); }

  size_t size() const { return map_.size(); }

private:

  std::map<size_t, From> map_;

};

}

#endif // SWIZZLE_H
