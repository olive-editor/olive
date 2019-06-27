/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "footage.h"

Footage::Footage()
{

}

Footage::~Footage()
{
  ClearStreams();
}

void Footage::Clear()
{
  // Clear filename string
  filename_.clear();

  // Clear all streams
  ClearStreams();
}

const QString &Footage::filename()
{
  return filename_;
}

void Footage::set_filename(const QString &s)
{
  filename_ = s;
}

const QDateTime &Footage::timestamp()
{
  return timestamp_;
}

void Footage::set_timestamp(const QDateTime &t)
{
  timestamp_ = t;
}

void Footage::add_stream(Stream *s)
{
  // Add a copy of this stream to the list
  streams_.append(s);

  // Set its footage parent to this
  streams_.last()->set_footage(this);
}

const Stream *Footage::stream(int index)
{
  return streams_.at(index);
}

Item::Type Footage::type() const
{
  return kFootage;
}

void Footage::ClearStreams()
{
  // Delete all streams
  for (int i=0;i<streams_.size();i++) {
    delete streams_.at(i);
  }

  // Empty array
  streams_.clear();
}
