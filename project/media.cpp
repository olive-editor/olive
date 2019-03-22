/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "media.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QCoreApplication>
#include <QtMath>

#include "footage.h"
#include "timeline/sequence.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "global/config.h"
#include "panels/viewer.h"
#include "panels/project.h"
#include "ui/icons.h"
#include "projectmodel.h"
#include "global/debug.h"

QString get_interlacing_name(int interlacing) {
  switch (interlacing) {
  case VIDEO_PROGRESSIVE: return QCoreApplication::translate("InterlacingName", "None (Progressive)");
  case VIDEO_TOP_FIELD_FIRST: return QCoreApplication::translate("InterlacingName", "Top Field First");
  case VIDEO_BOTTOM_FIELD_FIRST: return QCoreApplication::translate("InterlacingName", "Bottom Field First");
  default: return QCoreApplication::translate("InterlacingName", "Invalid");
  }
}

QString get_channel_layout_name(int channels, uint64_t layout) {
  switch (channels) {
  case 0: return QCoreApplication::translate("ChannelLayoutName", "Invalid");
  case 1: return QCoreApplication::translate("ChannelLayoutName", "Mono");
  case 2: return QCoreApplication::translate("ChannelLayoutName", "Stereo");
  default: {
    char buf[50];
    av_get_channel_layout_string(buf, sizeof(buf), channels, layout);
    return QString(buf);
  }
  }
}

Media::Media() :
  root(false),
  type(-1)
{
}

Footage* Media::to_footage() {
  return static_cast<Footage*>(object.get());
}

SequencePtr Media::to_sequence() {
  return std::static_pointer_cast<Sequence>(object);
}

void Media::set_footage(FootagePtr f) {
  type = MEDIA_TYPE_FOOTAGE;
  object = VoidPtr(f);
}

void Media::set_sequence(SequencePtr s) {
  set_icon(olive::icon::MediaSequence);
  type = MEDIA_TYPE_SEQUENCE;
  object = VoidPtr(s);
  if (s != nullptr) {
    update_tooltip();
  }
}

void Media::set_folder() {
  if (folder_name.isEmpty()) {
    folder_name = QCoreApplication::translate("Media", "New Folder");
  }
  set_icon(olive::icon::MediaFolder);
  type = MEDIA_TYPE_FOLDER;
  object = nullptr;
}

void Media::set_icon(const QString &str)
{
  set_icon(QIcon(str));
}

void Media::set_icon(const QIcon &ico) {
  icon = ico;
}

void Media::set_parent(Media *p) {
  parent = p;
}

void Media::update_tooltip(const QString& error) {
  switch (type) {
  case MEDIA_TYPE_FOOTAGE:
  {
    Footage* f = to_footage();
    tooltip = QCoreApplication::translate("Media", "Name:") + " " + f->name + "\n" + QCoreApplication::translate("Media", "Filename:") + " " + f->url + "\n";

    if (error.isEmpty()) {
      if (f->video_tracks.size() > 0) {
        tooltip += QCoreApplication::translate("Media", "Video Dimensions:") + " ";
        for (int i=0;i<f->video_tracks.size();i++) {
          if (i > 0) {
            tooltip += ", ";
          }
          tooltip += QString::number(f->video_tracks.at(i).video_width) + "x" + QString::number(f->video_tracks.at(i).video_height);
        }
        tooltip += "\n";

        if (!f->video_tracks.at(0).infinite_length) {
          tooltip += QCoreApplication::translate("Media", "Frame Rate:") + " ";
          for (int i=0;i<f->video_tracks.size();i++) {
            if (i > 0) {
              tooltip += ", ";
            }
            if (f->video_tracks.at(i).video_interlacing == VIDEO_PROGRESSIVE) {
              tooltip += QString::number(f->video_tracks.at(i).video_frame_rate * f->speed);
            } else {
              double adjusted_rate = f->video_tracks.at(i).video_frame_rate * f->speed;

              tooltip += QCoreApplication::translate("Media", "%1 field(s) (%2 frame(s))").arg(
                    QString::number(adjusted_rate*2.0),
                    QString::number(adjusted_rate)
                    );
            }
          }
          tooltip += "\n";
        }

        tooltip += QCoreApplication::translate("Media", "Interlacing:") + " ";
        for (int i=0;i<f->video_tracks.size();i++) {
          if (i > 0) {
            tooltip += ", ";
          }
          tooltip += get_interlacing_name(f->video_tracks.at(i).video_interlacing);
        }
      }

      if (f->audio_tracks.size() > 0) {
        tooltip += "\n";

        tooltip += QCoreApplication::translate("Media", "Audio Frequency:") + " ";
        for (int i=0;i<f->audio_tracks.size();i++) {
          if (i > 0) {
            tooltip += ", ";
          }
          tooltip += QString::number(f->audio_tracks.at(i).audio_frequency * f->speed);
        }
        tooltip += "\n";

        tooltip += QCoreApplication::translate("Media", "Audio Channels:") + " ";
        for (int i=0;i<f->audio_tracks.size();i++) {
          if (i > 0) {
            tooltip += ", ";
          }
          tooltip += get_channel_layout_name(f->audio_tracks.at(i).audio_channels, f->audio_tracks.at(i).audio_layout);
        }
        // tooltip += "\n";
      }
      tooltip += "\n";
      for (int i=0;i<f->video_tracks.size();i++) {
        tooltip += QString("Timecode Start: " + f->video_tracks.at(i).timecode_source_start);
      }
    } else {
      tooltip += error;
    }
  }
    break;
  case MEDIA_TYPE_SEQUENCE:
  {
    Sequence* s = to_sequence().get();

    tooltip = QCoreApplication::translate("Media", "Name: %1"
                                                   "\nVideo Dimensions: %2x%3"
                                                   "\nFrame Rate: %4"
                                                   "\nAudio Frequency: %5"
                                                   "\nAudio Layout: %6").arg(
          s->name,
          QString::number(s->width),
          QString::number(s->height),
          QString::number(s->frame_rate),
          QString::number(s->audio_frequency),
          get_channel_layout_name(av_get_channel_layout_nb_channels(s->audio_layout), s->audio_layout)
          );
  }
    break;
  }

}

VoidPtr Media::to_object() {
  return object;
}

int Media::get_type() {
  return type;
}

const QString &Media::get_name() {
  switch (type) {
  case MEDIA_TYPE_FOOTAGE: return to_footage()->name;
  case MEDIA_TYPE_SEQUENCE: return to_sequence()->name;
  default: return folder_name;
  }
}

void Media::set_name(const QString &n) {
  switch (type) {
  case MEDIA_TYPE_FOOTAGE: to_footage()->name = n; break;
  case MEDIA_TYPE_SEQUENCE: to_sequence()->name = n; break;
  case MEDIA_TYPE_FOLDER: folder_name = n; break;
  }
}

double Media::get_frame_rate(int stream) {
  switch (get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    Footage* f = to_footage();
    if (stream < 0) return f->video_tracks.at(0).video_frame_rate * f->speed;
    return f->get_stream_from_file_index(true, stream)->video_frame_rate * f->speed;
  }
  case MEDIA_TYPE_SEQUENCE: return to_sequence()->frame_rate;
  }
  return 0;
}

int Media::get_sampling_rate(int stream) {
  switch (get_type()) {
  case MEDIA_TYPE_FOOTAGE:
  {
    Footage* f = to_footage();
    if (stream < 0) return f->audio_tracks.at(0).audio_frequency * f->speed;
    return to_footage()->get_stream_from_file_index(false, stream)->audio_frequency * f->speed;
  }
  case MEDIA_TYPE_SEQUENCE: return to_sequence()->audio_frequency;
  }
  return 0;
}

void Media::appendChild(MediaPtr child) {
  child->set_parent(this);
  children.append(child);
}

bool Media::setData(int col, const QVariant &value) {
  if (col == 0) {
    QString n = value.toString();
    if (!n.isEmpty() && get_name() != n) {
      olive::UndoStack.push(new MediaRename(this, value.toString()));
      return true;
    }
  }
  return false;
}

Media *Media::child(int row) {
  return children.value(row).get();
}

int Media::childCount() const {
  return children.count();
}

int Media::columnCount() const {
  return 3;
}

QVariant Media::data(int column, int role) {
  switch (role) {
  case Qt::DecorationRole:
    if (column == 0) {
      if (get_type() == MEDIA_TYPE_FOOTAGE) {
        Footage* f = to_footage();
        if (f->video_tracks.size() > 0
            && f->video_tracks.at(0).preview_done) {
          return f->video_tracks.at(0).video_preview_square;
        }
      }

      return icon;
    }
    break;
  case Qt::DisplayRole:
    switch (column) {
    case 0: return (root) ? QCoreApplication::translate("Media", "Name") : get_name();
    case 1:
      if (root) return QCoreApplication::translate("Media", "Duration");
      if (get_type() == MEDIA_TYPE_SEQUENCE) {
        Sequence* s = to_sequence().get();
        return frame_to_timecode(s->getEndFrame(), olive::CurrentConfig.timecode_view, s->frame_rate);
      }
      if (get_type() == MEDIA_TYPE_FOOTAGE) {
        Footage* f = to_footage();
        double r = 30;

        if (f->video_tracks.size() > 0 && !qIsNull(f->video_tracks.at(0).video_frame_rate))
          r = f->video_tracks.at(0).video_frame_rate * f->speed;

        long len = f->get_length_in_frames(r);
        if (len > 0) return frame_to_timecode(len, olive::CurrentConfig.timecode_view, r);
      }
      break;
    case 2:
      if (root) return QCoreApplication::translate("Media", "Rate");
      if (get_type() == MEDIA_TYPE_SEQUENCE) return QString::number(get_frame_rate()) + " FPS";
      if (get_type() == MEDIA_TYPE_FOOTAGE) {
        Footage* f = to_footage();
        double r;
        if (f->video_tracks.size() > 0 && !qIsNull(r = get_frame_rate())) {
          return QString::number(get_frame_rate()) + " FPS";
        } else if (f->audio_tracks.size() > 0) {
          return QString::number(get_sampling_rate()) + " Hz";
        }
      }
      break;
    }
    break;
  case Qt::ToolTipRole:
    return tooltip;
  }
  return QVariant();
}

int Media::row() const {
  if (parent != nullptr) {
    for (int i=0;i<parent->children.size();i++) {
      if (parent->children.at(i).get() == this) {
        return i;
      }
    }
  }
  return 0;
}

Media *Media::parentItem() {
  return parent;
}

void Media::removeChild(int i) {
  children.removeAt(i);
}

MediaPtr Media::get_shared_ptr(Media *m)
{
  for (int i=0;i<children.size();i++) {
    if (children.at(i).get() == m) {
      return children.at(i);
    }
  }
  return MediaPtr();
}

QVector<Marker> &Media::get_markers() {
  // returns the marker array from the internal object
  //
  // NOTE: if this media object is not footage or a sequence, the result is
  // undefined - most likely a crash

  if (get_type() == MEDIA_TYPE_FOOTAGE) {
    return to_footage()->markers;
  } else {
    return to_sequence()->markers;
  }
}
