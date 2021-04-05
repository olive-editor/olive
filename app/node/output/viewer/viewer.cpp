/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "viewer.h"

#include "config/config.h"
#include "core.h"
#include "node/traverser.h"
#include "widget/videoparamedit/videoparamedit.h"

namespace olive {

const QString ViewerOutput::kVideoParamsInput = QStringLiteral("video_param_in");
const QString ViewerOutput::kAudioParamsInput = QStringLiteral("audio_param_in");
const QString ViewerOutput::kTextureInput = QStringLiteral("tex_in");
const QString ViewerOutput::kSamplesInput = QStringLiteral("samples_in");

const uint64_t ViewerOutput::kVideoParamEditMask = VideoParamEdit::kWidthHeight | VideoParamEdit::kInterlacing | VideoParamEdit::kFrameRate | VideoParamEdit::kPixelAspect;

#define super Node

ViewerOutput::ViewerOutput() :
  video_frame_cache_(this),
  audio_playback_cache_(this),
  operation_stack_(0)
{
  AddInput(kVideoParamsInput, NodeValue::kVideoParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetInputProperty(kVideoParamsInput, QStringLiteral("mask"), QVariant::fromValue(kVideoParamEditMask));

  AddInput(kAudioParamsInput, NodeValue::kAudioParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));
}

ViewerOutput::~ViewerOutput()
{
  // Should prevent traversing graph unnecessarily
  BeginOperation();
  DisconnectAll();
  EndOperation();
}

Node *ViewerOutput::copy() const
{
  return new ViewerOutput();
}

QString ViewerOutput::Name() const
{
  return tr("Viewer");
}

QString ViewerOutput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.vieweroutput");
}

QVector<Node::CategoryID> ViewerOutput::Category() const
{
  return {kCategoryOutput};
}

QString ViewerOutput::Description() const
{
  return tr("Interface between a Viewer panel and the node system.");
}

QString ViewerOutput::duration() const
{
  rational timeline_length = GetLength();

  int64_t timestamp = Timecode::time_to_timestamp(timeline_length, video_params().time_base());

  return Timecode::timestamp_to_timecode(timestamp, video_params().time_base(), Core::instance()->GetTimecodeDisplay());
}

QString ViewerOutput::rate() const
{
  return tr("%1 FPS").arg(video_params().time_base().flipped().toDouble());
}

void ViewerOutput::set_default_parameters()
{
  int width = Config::Current()["DefaultSequenceWidth"].toInt();
  int height = Config::Current()["DefaultSequenceHeight"].toInt();

  set_video_params(VideoParams(width,
                               height,
                               Config::Current()["DefaultSequenceFrameRate"].value<rational>(),
                               static_cast<VideoParams::Format>(Config::Current()["OfflinePixelFormat"].toInt()),
                               VideoParams::kInternalChannelCount,
                               Config::Current()["DefaultSequencePixelAspect"].value<rational>(),
                               Config::Current()["DefaultSequenceInterlacing"].value<VideoParams::Interlacing>(),
                               VideoParams::generate_auto_divider(width, height)));
  set_audio_params(AudioParams(Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                   Config::Current()["DefaultSequenceAudioLayout"].toULongLong(),
                   AudioParams::kInternalFormat));
}

void ViewerOutput::ShiftVideoCache(const rational &from, const rational &to)
{
  video_frame_cache_.Shift(from, to);

  ShiftVideoEvent(from, to);
}

void ViewerOutput::ShiftAudioCache(const rational &from, const rational &to)
{
  audio_playback_cache_.Shift(from, to);

  ShiftAudioEvent(from, to);
}

void ViewerOutput::ShiftCache(const rational &from, const rational &to)
{
  ShiftVideoCache(from, to);
  ShiftAudioCache(from, to);
}

void ViewerOutput::InvalidateCache(const TimeRange& range, const QString& from, int element, qint64 job_time)
{
  Q_UNUSED(element)

  if (operation_stack_ == 0) {
    if (from == kTextureInput || from == kSamplesInput
        || from == kVideoParamsInput || from == kAudioParamsInput) {
      TimeRange invalidated_range(qMax(rational(), range.in()),
                                  qMin(GetLength(), range.out()));

      if (invalidated_range.in() != invalidated_range.out()) {
        if (from == kTextureInput || from == kVideoParamsInput) {
          video_frame_cache_.Invalidate(invalidated_range, job_time);
        } else {
          audio_playback_cache_.Invalidate(invalidated_range, job_time);
        }
      }
    }

    VerifyLength();
  }

  super::InvalidateCache(range, from, element, job_time);
}

const rational& ViewerOutput::GetLength() const
{
  return last_length_;
}

void ViewerOutput::Retranslate()
{
  super::Retranslate();

  SetInputName(kVideoParamsInput, tr("Video Parameters"));
  SetInputName(kAudioParamsInput, tr("Audio Parameters"));

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kSamplesInput, tr("Samples"));
}

void ViewerOutput::VerifyLength()
{
  if (operation_stack_ != 0) {
    return;
  }

  NodeTraverser traverser;

  rational video_length, audio_length, subtitle_length;

  {
    video_length = GetCustomLength(Track::kVideo);

    if (video_length.isNull() && IsInputConnected(kTextureInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kTextureInput), TimeRange(0, 0));
      video_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    video_frame_cache_.SetLength(video_length);
  }

  {
    audio_length = GetCustomLength(Track::kAudio);

    if (audio_length.isNull() && IsInputConnected(kSamplesInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kSamplesInput), TimeRange(0, 0));
      audio_length = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
    }

    audio_playback_cache_.SetLength(audio_length);
  }

  {
    subtitle_length = GetCustomLength(Track::kSubtitle);
  }

  rational real_length = qMax(subtitle_length, qMax(video_length, audio_length));

  if (real_length != last_length_) {
    last_length_ = real_length;
    emit LengthChanged(last_length_);
  }
}

void ViewerOutput::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  }

  super::InputConnectedEvent(input, element, output);
}

void ViewerOutput::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  }

  super::InputDisconnectedEvent(input, element, output);
}

rational ViewerOutput::GetCustomLength(Track::Type type) const
{
  Q_UNUSED(type)
  return rational();
}

void ViewerOutput::BeginOperation()
{
  operation_stack_++;

  super::BeginOperation();
}

void ViewerOutput::EndOperation()
{
  operation_stack_--;

  super::EndOperation();
}

void ViewerOutput::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kVideoParamsInput) {

    VideoParams new_video_params = video_params();

    bool size_changed = cached_video_params_.width() != new_video_params.width() || cached_video_params_.height() != new_video_params.height();
    bool timebase_changed = cached_video_params_.time_base() != new_video_params.time_base();
    bool pixel_aspect_changed = cached_video_params_.pixel_aspect_ratio() != new_video_params.pixel_aspect_ratio();
    bool interlacing_changed = cached_video_params_.interlacing() != new_video_params.interlacing();

    if (size_changed) {
      emit SizeChanged(new_video_params.width(), new_video_params.height());
    }

    if (pixel_aspect_changed) {
      emit PixelAspectChanged(new_video_params.pixel_aspect_ratio());
    }

    if (interlacing_changed) {
      emit InterlacingChanged(new_video_params.interlacing());
    }

    if (timebase_changed) {
      video_frame_cache_.SetTimebase(new_video_params.time_base());
      emit TimebaseChanged(new_video_params.time_base());
    }

    emit VideoParamsChanged();

    cached_video_params_ = video_params();

  } else if (input == kAudioParamsInput) {

    emit AudioParamsChanged();

    audio_playback_cache_.SetParameters(audio_params());

  }

  super::InputValueChangedEvent(input, element);
}

void ViewerOutput::ShiftVideoEvent(const rational &from, const rational &to)
{
  Q_UNUSED(from)
  Q_UNUSED(to)
}

void ViewerOutput::ShiftAudioEvent(const rational &from, const rational &to)
{
  Q_UNUSED(from)
  Q_UNUSED(to)
}

void ViewerOutput::set_parameters_from_footage(const QVector<Footage *> footage)
{
  foreach (Footage* f, footage) {
    QVector<VideoParams> video_streams = f->GetEnabledVideoStreams();
    QVector<AudioParams> audio_streams = f->GetEnabledAudioStreams();

    foreach (const VideoParams& s, video_streams) {
      bool found_video_params = false;
      rational using_timebase;

      if (s.video_type() == VideoParams::kVideoTypeStill) {
        // If this is a still image, we'll use it's resolution but won't set
        // `found_video_params` in case something with a frame rate comes along which we'll
        // prioritize
        using_timebase = video_params().time_base();
      } else {
        using_timebase = s.frame_rate().flipped();
        found_video_params = true;
      }

      set_video_params(VideoParams(s.width(),
                                   s.height(),
                                   using_timebase,
                                   static_cast<VideoParams::Format>(Config::Current()[QStringLiteral("OfflinePixelFormat")].toInt()),
                                   VideoParams::kInternalChannelCount,
                                   s.pixel_aspect_ratio(),
                                   s.interlacing(),
                                   VideoParams::generate_auto_divider(s.width(), s.height())));

      if (found_video_params) {
        break;
      }
    }

    if (!audio_streams.isEmpty()) {
      const AudioParams& s = audio_streams.first();
      set_audio_params(AudioParams(s.sample_rate(), s.channel_layout(), AudioParams::kInternalFormat));
    }
  }
}

bool ViewerOutput::LoadCustom(QXmlStreamReader *reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled)
{
  if (reader->name() == QStringLiteral("points")) {
    timeline_points_.Load(reader);
    return true;
  } else {
    return LoadCustom(reader, xml_node_data, version, cancelled);
  }
}

void ViewerOutput::SaveCustom(QXmlStreamWriter *writer) const
{
  // Write TimelinePoints
  writer->writeStartElement(QStringLiteral("points"));
  timeline_points_.Save(writer);
  writer->writeEndElement(); // points
}

}
