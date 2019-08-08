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

#include <QCoreApplication>

#include "ui/icons/icons.h"

Footage::Footage()
{
  Clear();
}

Footage::~Footage()
{
  ClearStreams();
}

const Footage::Status& Footage::status()
{
  return status_;
}

void Footage::set_status(const Footage::Status &status)
{
  status_ = status;

  UpdateIcon();

  UpdateTooltip();
}

void Footage::Clear()
{
  // Clear all streams
  ClearStreams();

  // Reset ready state
  set_status(kUnprobed);
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

int Footage::stream_count()
{
  return streams_.size();
}

Item::Type Footage::type() const
{
  return kFootage;
}

const QString &Footage::decoder()
{
  return decoder_;
}

void Footage::set_decoder(const QString &id)
{
  decoder_ = id;
}

void Footage::ClearStreams()
{
  if (streams_.empty()) {
    return;
  }

  // Delete all streams
  for (int i=0;i<streams_.size();i++) {
    delete streams_.at(i);
  }

  // Empty array
  streams_.clear();
}

bool Footage::HasStreamsOfType(const Stream::Type type)
{
  // Return true if any streams are video streams
  for (int i=0;i<streams_.size();i++) {
    if (streams_.at(i)->type() == type) {
      return true;
    }
  }

  return false;
}

void Footage::UpdateIcon()
{
  switch (status_) {
  case kUnprobed:
  case kUnindexed:
    // FIXME Set a waiting icon
    set_icon(QIcon());
    break;
  case kReady:
    if (HasStreamsOfType(Stream::kVideo)) {

      // Prioritize the video icon
      set_icon(olive::icon::Video);

      // FIXME: When image sources can be reliably picked up, use image icon instead
      //        Perhaps all image sources can be left to OpenImageIO meaning only video sources need to be here

    } else if (HasStreamsOfType(Stream::kAudio)) {

      // Otherwise assume it's audio only
      set_icon(olive::icon::Audio);

    } else {

      // FIXME Icon/indicator for a media file with no video or audio streams?
      //       The footage should probably be deemed kInvalid in this state

    }
    break;
  case kInvalid:
    set_icon(olive::icon::Error);
    break;
  }
}

void Footage::UpdateTooltip()
{
  switch (status_) {
  case kUnprobed:
    set_tooltip(QCoreApplication::translate("Footage", "Waiting for probe"));
    break;
  case kUnindexed:
    set_tooltip(QCoreApplication::translate("Footage", "Waiting for index"));
    break;
  case kReady:
  {
    QString tip = QCoreApplication::translate("Footage", "Filename: %1").arg(filename());

    if (!streams_.isEmpty()) {
      tip.append("\n");

      for (int i=0;i<streams_.size();i++) {

        Stream* s = streams_.at(i);

        if (s->type() == Stream::kVideo) {
          VideoStream* vs = static_cast<VideoStream*>(s);

          tip.append(
                QCoreApplication::translate("Footage",
                                            "\nVideo %1: %2x%3").arg(QString::number(i),
                                                                     QString::number(vs->width()),
                                                                     QString::number(vs->height()))
                );
        } else if (streams_.at(i)->type() == Stream::kAudio) {
          AudioStream* as = static_cast<AudioStream*>(s);

          tip.append(
                QCoreApplication::translate("Footage",
                                            "\nAudio %1: %2 channels %3 Hz").arg(QString::number(i),
                                                                                 QString::number(as->channels()),
                                                                                 QString::number(as->sample_rate()))
                );
        }
      }
    }

    set_tooltip(tip);
  }
    break;
  case kInvalid:
    set_tooltip(QCoreApplication::translate("Footage", "An error occurred probing this footage"));
    break;
  }
}
