/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef SUBTITLEPARAMS_H
#define SUBTITLEPARAMS_H

#include <QRect>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "util/timerange.h"

namespace olive {

class Subtitle
{
public:
  Subtitle() = default;

  Subtitle(const TimeRange &time, const QString &text) :
    range_(time),
    text_(text)
  {
  }

  const TimeRange &time() const { return range_; }
  void set_time(const TimeRange &t) { range_ = t; }

  const QString &text() const { return text_; }
  void set_text(const QString &t) { text_ = t; }

private:
  TimeRange range_;

  QString text_;

};

class SubtitleParams : public std::vector<Subtitle>
{
public:
  SubtitleParams()
  {
    stream_index_ = 0;
    enabled_ = true;
  }

  static QString GenerateASSHeader();

  void Load(QXmlStreamReader* reader);

  void Save(QXmlStreamWriter* writer) const;

  bool is_valid() const
  {
    return !this->empty();
  }

  rational duration() const
  {
    if (this->empty()) {
      return 0;
    } else {
      return back().time().out();
    }
  }

  int stream_index() const { return stream_index_; }
  void set_stream_index(int i) { stream_index_ = i; }

  bool enabled() const { return enabled_; }
  void set_enabled(bool e) { enabled_ = e; }

private:
  int stream_index_;

  bool enabled_;

};

}

Q_DECLARE_METATYPE(olive::Subtitle)
Q_DECLARE_METATYPE(olive::SubtitleParams)

#endif // SUBTITLEPARAMS_H
