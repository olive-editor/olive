/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include "codec/decoder.h"
#include "common/clamp.h"
#include "common/filefunctions.h"
#include "common/qtutils.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"
#include "render/job/footagejob.h"
#include "ui/icons/icons.h"

namespace olive {

const QString Footage::kFilenameInput = QStringLiteral("file_in");
const QString Footage::kLoopModeInput = QStringLiteral("loop_in");

#define super ViewerOutput

Footage::Footage(const QString &filename) :
  ViewerOutput(false, false),
  cancelled_(nullptr)
{
  SetCacheTextures(true);
  SetViewerVideoCacheEnabled(false);

  PrependInput(kLoopModeInput, NodeValue::kCombo, 0, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  IgnoreHashingFrom(kLoopModeInput);

  PrependInput(kFilenameInput, NodeValue::kFile, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  Clear();

  set_filename(filename);

  QTimer *check_timer = new QTimer(this);
  check_timer->setInterval(5000);
  connect(check_timer, &QTimer::timeout, this, &Footage::CheckFootage);
  check_timer->start();
}

void Footage::Retranslate()
{
  super::Retranslate();

  SetInputName(kFilenameInput, tr("Filename"));
  SetInputName(kLoopModeInput, tr("Loop Mode"));
  SetComboBoxStrings(kLoopModeInput, {tr("None"), tr("Loop"), tr("Clamp")});
}

void Footage::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kFilenameInput) {
    // Reset internal stream cache
    Clear();

    // Determine if file still exists
    QFileInfo info(filename());

    if (info.exists()) {
      // Grab timestamp
      set_timestamp(info.lastModified().toMSecsSinceEpoch());

      // Determine if we've already cached the metadata of this file
      QString meta_cache_file = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(FileFunctions::GetUniqueFileIdentifier(filename()));

      FootageDescription footage_info;

      if (QFileInfo::exists(meta_cache_file)) {

        // Load meta cache file
        footage_info.Load(meta_cache_file);

      } else {

        // Probe and create cache
        QVector<DecoderPtr> decoder_list = Decoder::ReceiveListOfAllDecoders();

        foreach (DecoderPtr decoder, decoder_list) {
          footage_info = decoder->Probe(filename(), cancelled_);

          if (footage_info.IsValid()) {
            break;
          }
        }

        if (!footage_info.Save(meta_cache_file)) {
          qWarning() << "Failed to save stream cache, footage will have to be re-probed";
        }

      }

      if (footage_info.IsValid()) {
        decoder_ = footage_info.decoder();

        for (int i=0; i<footage_info.GetVideoStreams().size(); i++) {
          VideoParams vp = footage_info.GetVideoStreams().at(i);

          // FIXME: Make this customizable
          vp.set_divider(VideoParams::generate_auto_divider(vp.width(), vp.height()));

          AddStream(Track::kVideo, QVariant::fromValue(vp));
        }

        for (int i=0; i<footage_info.GetAudioStreams().size(); i++) {
          AddStream(Track::kAudio, QVariant::fromValue(footage_info.GetAudioStreams().at(i)));
        }

        SetValid();
      }

    } else {
      set_timestamp(0);
    }
  } else {
    super::InputValueChangedEvent(input, element);
  }
}

rational Footage::VerifyLengthInternal(Track::Type type) const
{
  if (type == Track::kVideo) {
    VideoParams first_stream = GetFirstEnabledVideoStream();

    if (first_stream.is_valid()) {
      return Timecode::timestamp_to_time(first_stream.duration(), first_stream.time_base());
    }
  } else if (type == Track::kAudio) {
    AudioParams first_stream = GetFirstEnabledAudioStream();

    if (first_stream.is_valid()) {
      return Timecode::timestamp_to_time(first_stream.duration(), first_stream.time_base());
    }
  }

  return 0;
}

QString Footage::GetColorspaceToUse(const VideoParams &params) const
{
  if (params.colorspace().isEmpty()) {
    return project()->color_manager()->GetDefaultInputColorSpace();
  } else {
    return params.colorspace();
  }
}

void Footage::Clear()
{
  // Clear all dynamically created inputs
  InputArrayResize(kVideoParamsInput, 0);
  InputArrayResize(kAudioParamsInput, 0);

  // Clear decoder link
  decoder_.clear();

  // Reset ready state
  valid_ = false;
}

void Footage::SetValid()
{
  valid_ = true;
}

Footage::LoopMode Footage::loop_mode() const
{
  return static_cast<LoopMode>(GetStandardValue(kLoopModeInput).toInt());
}

QString Footage::filename() const
{
  return GetStandardValue(kFilenameInput).toString();
}

void Footage::set_filename(const QString &s)
{
  SetStandardValue(kFilenameInput, s);
}

const qint64 &Footage::timestamp() const
{
  return timestamp_;
}

void Footage::set_timestamp(const qint64 &t)
{
  timestamp_ = t;
}

int Footage::GetStreamIndex(Track::Type type, int index) const
{
  if (type == Track::kVideo) {
    return GetVideoParams(index).stream_index();
  } else if (type == Track::kAudio) {
    return GetAudioParams(index).stream_index();
  } else {
    return -1;
  }
}

Track::Reference Footage::GetReferenceFromRealIndex(int real_index) const
{
  // Check video streams
  for (int i=0; i<GetVideoStreamCount(); i++) {
    if (GetVideoParams(i).stream_index() == real_index) {
      return Track::Reference(Track::kVideo, i);
    }
  }

  for (int i=0; i<GetAudioStreamCount(); i++) {
    if (GetAudioParams(i).stream_index() == real_index) {
      return Track::Reference(Track::kAudio, i);
    }
  }

  return Track::Reference();
}

const QString &Footage::decoder() const
{
  return decoder_;
}

QIcon Footage::icon() const
{
  if (valid_ && GetTotalStreamCount()) {
    // Prioritize video > audio > image
    VideoParams s = GetFirstEnabledVideoStream();

    if (s.is_valid() && s.video_type() != VideoParams::kVideoTypeStill) {
      return icon::Video;
    } else if (HasEnabledAudioStreams()) {
      return icon::Audio;
    } else if (s.is_valid() && s.video_type() == VideoParams::kVideoTypeStill) {
      return icon::Image;
    }
  }

  return icon::Error;
}

QString Footage::DescribeVideoStream(const VideoParams &params)
{
  if (params.video_type() == VideoParams::kVideoTypeStill) {
    return tr("%1: Image - %2x%3").arg(QString::number(params.stream_index()),
                                       QString::number(params.width()),
                                       QString::number(params.height()));
  } else {
    return tr("%1: Video - %2x%3").arg(QString::number(params.stream_index()),
                                       QString::number(params.width()),
                                       QString::number(params.height()));
  }
}

QString Footage::DescribeAudioStream(const AudioParams &params)
{
  return tr("%1: Audio - %n Channel(s), %2Hz", nullptr, params.channel_count())
    .arg(QString::number(params.stream_index()),
         QString::number(params.sample_rate()));
}

void Footage::Hash(QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams &video_params) const
{
  super::Hash(hash, globals, video_params);

  // Footage last modified date
  hash.addData(QString::number(timestamp()).toUtf8());

  for (int i=0; i<GetVideoStreamCount(); i++) {
    VideoParams params = GetVideoParams(i);

    // Current color config and space
    hash.addData(project()->color_manager()->GetConfigFilename().toUtf8());
    hash.addData(GetColorspaceToUse(params).toUtf8());

    // Alpha associated setting
    hash.addData(QString::number(params.premultiplied_alpha()).toUtf8());

    // Pixel aspect ratio
    hash.addData(reinterpret_cast<const char*>(&params.pixel_aspect_ratio()), sizeof(params.pixel_aspect_ratio()));

    // Footage timestamp
    if (params.video_type() != VideoParams::kVideoTypeStill) {
      rational adjusted_time = AdjustTimeByLoopMode(globals.time().in(), loop_mode(), GetLength(), params.video_type(), params.frame_rate_as_time_base());

      if (!adjusted_time.isNaN()) {
        int64_t video_ts = Timecode::time_to_timestamp(adjusted_time, params.time_base());

        // Add timestamp in units of the video stream's timebase
        hash.addData(reinterpret_cast<const char*>(&video_ts), sizeof(video_ts));
      }

      // Add start time - used for both image sequences and video streams
      auto start_time = params.start_time();
      hash.addData(reinterpret_cast<const char*>(&start_time), sizeof(start_time));
    }
  }
}

void Footage::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  Q_UNUSED(globals)

  // Pop filename from table
  QString file = value[kFilenameInput].data().toString();

  LoopMode loop_mode = static_cast<LoopMode>(value[kLoopModeInput].data().toInt());

  // If the file exists and the reference is valid, push a footage job to the renderer
  if (QFileInfo(file).exists()) {
    // Push length
    table->Push(NodeValue::kRational, QVariant::fromValue(GetLength()), this, false, QStringLiteral("length"));

    // Push each stream as a footage job
    for (int i=0; i<GetTotalStreamCount(); i++) {
      Track::Reference ref = GetReferenceFromRealIndex(i);
      FootageJob job(decoder_, filename(), ref.type(), GetLength(), loop_mode);

      if (ref.type() == Track::kVideo) {
        VideoParams vp = GetVideoParams(ref.index());

        // Ensure the colorspace is valid and not empty
        vp.set_colorspace(GetColorspaceToUse(vp));

        job.set_video_params(vp);
      } else {
        AudioParams ap = GetAudioParams(ref.index());
        job.set_audio_params(ap);
        job.set_cache_path(project()->cache_path());
      }

      table->Push(NodeValue::kFootageJob, QVariant::fromValue(job), this, false, ref.ToString());
    }
  }
}

QString Footage::GetStreamTypeName(Track::Type type)
{
  switch (type) {
  case Track::kVideo:
    return tr("Video");
  case Track::kAudio:
    return tr("Audio");
  case Track::kSubtitle:
    return tr("Subtitle");
  case Track::kNone:
  case Track::kCount:
    break;
  }

  return tr("Unknown");
}

Node *Footage::GetConnectedTextureOutput()
{
  if (GetVideoStreamCount() > 0) {
    return this;
  } else {
    return nullptr;
  }
}

Node *Footage::GetConnectedSampleOutput()
{
  if (GetAudioStreamCount() > 0) {
    return this;
  } else {
    return nullptr;
  }
}

bool TimeIsOutOfBounds(const rational& time, const rational& length)
{
  return time < 0 || time >= length;
}

rational Footage::AdjustTimeByLoopMode(rational time, Footage::LoopMode loop_mode, const rational &length, VideoParams::Type type, const rational& timebase)
{
  if (type == VideoParams::kVideoTypeStill) {
    // No looping for still images
    return 0;
  }

  if (TimeIsOutOfBounds(time, length)) {
    switch (loop_mode) {
    case kLoopModeOff:
      // Return no time to indicate no frame should be shown here
      time = rational::NaN;
      break;
    case kLoopModeClamp:
      // Clamp footage time to length
      time = clamp(time, rational(0), length - timebase);
      break;
    case kLoopModeLoop:
      // Loop footage time around job length
      do {
        if (time >= length) {
          time -= length;
        } else {
          time += length;
        }
      } while (TimeIsOutOfBounds(time, length));
    }
  }

  return time;
}

qint64 Footage::creation_time() const
{
  QFileInfo info(filename());

  if (info.exists()) {
    return QtUtils::GetCreationDate(info).toSecsSinceEpoch();
  }

  return 0;
}

qint64 Footage::mod_time() const
{
  QFileInfo info(filename());

  if (info.exists()) {
    return info.lastModified().toSecsSinceEpoch();
  }

  return 0;
}

void Footage::UpdateTooltip()
{
  if (valid_) {
    QString tip = tr("Filename: %1").arg(filename());

    int vp_sz = GetVideoStreamCount();
    for (int i=0; i<vp_sz; i++) {
      VideoParams p = GetVideoParams(i);

      if (p.enabled()) {
        tip.append("\n");
        tip.append(DescribeVideoStream(p));
      }
    }

    int ap_sz = GetAudioStreamCount();
    for (int i=0; i<ap_sz; i++) {
      AudioParams p = GetAudioParams(i);

      if (p.enabled()) {
        tip.append("\n");
        tip.append(DescribeAudioStream(p));
      }
    }

    SetToolTip(tip);
  } else {
    SetToolTip(tr("Invalid"));
  }
}

void Footage::CheckFootage()
{
  // Don't check files if not the active window
  if (qApp->activeWindow()) {
    QString fn = filename();

    if (!fn.isEmpty()) {
      QFileInfo info(fn);

      qint64 current_file_timestamp = info.lastModified().toMSecsSinceEpoch();

      if (current_file_timestamp != timestamp()) {
        // File has changed!
        set_timestamp(current_file_timestamp);
        InvalidateAll(kFilenameInput);
      }
    }
  }
}

}
