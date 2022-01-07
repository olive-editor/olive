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

#include "viewer.h"

#include "config/config.h"
#include "core.h"
#include "node/traverser.h"

namespace olive {

const QString ViewerOutput::kVideoParamsInput = QStringLiteral("video_param_in");
const QString ViewerOutput::kAudioParamsInput = QStringLiteral("audio_param_in");
const QString ViewerOutput::kTextureInput = QStringLiteral("tex_in");
const QString ViewerOutput::kSamplesInput = QStringLiteral("samples_in");
const QString ViewerOutput::kVideoAutoCacheInput = QStringLiteral("video_autocache_in");
const QString ViewerOutput::kAudioAutoCacheInput = QStringLiteral("audio_autocache_in");

#define super Node

ViewerOutput::ViewerOutput(bool create_buffer_inputs, bool create_default_streams) :
  last_length_(0),
  video_length_(0),
  audio_length_(0),
  video_frame_cache_(this),
  audio_playback_cache_(this),
  video_cache_enabled_(true),
  audio_cache_enabled_(true)
{
  AddInput(kVideoParamsInput, NodeValue::kVideoParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagArray | kInputFlagHidden));

  AddInput(kAudioParamsInput, NodeValue::kAudioParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagArray | kInputFlagHidden));

  if (create_buffer_inputs) {
    AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
    AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

    AddInput(kVideoAutoCacheInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
    IgnoreHashingFrom(kVideoAutoCacheInput);
    IgnoreInvalidationsFrom(kVideoAutoCacheInput);

    AddInput(kAudioAutoCacheInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
    IgnoreHashingFrom(kAudioAutoCacheInput);
    IgnoreInvalidationsFrom(kAudioAutoCacheInput);
  }

  if (create_default_streams) {
    AddStream(Track::kVideo, QVariant());
    AddStream(Track::kAudio, QVariant());
    set_default_parameters();
  }

  SetFlags(kDontShowInParamView);
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
  rational using_timebase;
  Timecode::Display using_display = Core::instance()->GetTimecodeDisplay();

  // Get first enabled streams
  VideoParams video = GetFirstEnabledVideoStream();
  AudioParams audio = GetFirstEnabledAudioStream();

  if (video.is_valid() && video.video_type() != VideoParams::kVideoTypeStill) {
    // Prioritize video
    using_timebase = video.frame_rate_as_time_base();
  } else if (audio.is_valid()) {
    // Use audio as a backup
    // If we're showing in a timecode, we prefer showing audio in seconds instead
    if (using_display == Timecode::kTimecodeDropFrame
        || using_display == Timecode::kTimecodeNonDropFrame) {
      using_display = Timecode::kTimecodeSeconds;
    }

    using_timebase = audio.sample_rate_as_time_base();
  }

  if (using_timebase.isNull()) {
    // No timebase, return null
    return QString();
  } else {
    // Return time transformed to timecode
    return Timecode::time_to_timecode(GetLength(), using_timebase, using_display);
  }
}

QString ViewerOutput::rate() const
{
  VideoParams video_stream;

  if (HasEnabledVideoStreams()
      && (video_stream = GetFirstEnabledVideoStream()).video_type() != VideoParams::kVideoTypeStill) {
    // This is a video editor, prioritize video streams
    return tr("%1 FPS").arg(video_stream.frame_rate().toDouble());
  } else if (HasEnabledAudioStreams()) {
    // No video streams, return audio
    AudioParams audio_stream = GetFirstEnabledAudioStream();
    return tr("%1 Hz").arg(audio_stream.sample_rate());
  }

  return QString();
}

bool ViewerOutput::HasEnabledVideoStreams() const
{
  return GetFirstEnabledVideoStream().is_valid();
}

bool ViewerOutput::HasEnabledAudioStreams() const
{
  return GetFirstEnabledAudioStream().is_valid();
}

VideoParams ViewerOutput::GetFirstEnabledVideoStream() const
{
  int sz = GetVideoStreamCount();

  for (int i=0; i<sz; i++) {
    VideoParams vp = GetVideoParams(i);

    if (vp.enabled()) {
      return vp;
    }
  }

  return VideoParams();
}

AudioParams ViewerOutput::GetFirstEnabledAudioStream() const
{
  int sz = GetAudioStreamCount();

  for (int i=0; i<sz; i++) {
    AudioParams ap = GetAudioParams(i);

    if (ap.enabled()) {
      return ap;
    }
  }

  return AudioParams();
}

void ViewerOutput::set_default_parameters()
{
  int width = Config::Current()["DefaultSequenceWidth"].toInt();
  int height = Config::Current()["DefaultSequenceHeight"].toInt();

  SetVideoParams(VideoParams(
                   width,
                   height,
                   Config::Current()["DefaultSequenceFrameRate"].value<rational>(),
                 static_cast<VideoParams::Format>(Config::Current()["OfflinePixelFormat"].toInt()),
      VideoParams::kInternalChannelCount,
      Config::Current()["DefaultSequencePixelAspect"].value<rational>(),
      Config::Current()["DefaultSequenceInterlacing"].value<VideoParams::Interlacing>(),
      VideoParams::generate_auto_divider(width, height)
      ));
  SetAudioParams(AudioParams(
                   Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                 Config::Current()["DefaultSequenceAudioLayout"].toULongLong(),
      AudioParams::kInternalFormat
      ));

  SetVideoAutoCacheEnabled(Config::Current()["DefaultSequenceAutoCache"].toBool());
}

void ViewerOutput::ShiftVideoCache(const rational &from, const rational &to)
{
  if (video_cache_enabled_) {
    video_frame_cache_.Shift(from, to);
  }

  ShiftVideoEvent(from, to);
}

void ViewerOutput::ShiftAudioCache(const rational &from, const rational &to)
{
  if (audio_cache_enabled_) {
    audio_playback_cache_.Shift(from, to);
  }

  ShiftAudioEvent(from, to);
}

void ViewerOutput::ShiftCache(const rational &from, const rational &to)
{
  ShiftVideoCache(from, to);
  ShiftAudioCache(from, to);
}

void ViewerOutput::InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options)
{
  Q_UNUSED(element)

  if ((video_cache_enabled_ && (from == kTextureInput || from == kVideoParamsInput))
      || (audio_cache_enabled_ && (from == kSamplesInput || from == kAudioParamsInput))) {
    TimeRange invalidated_range(qMax(rational(0), range.in()),
                                qMin(GetLength(), range.out()));

    if (invalidated_range.in() != invalidated_range.out()) {
      if (from == kTextureInput || from == kVideoParamsInput) {
        video_frame_cache_.Invalidate(invalidated_range);
      } else {
        audio_playback_cache_.Invalidate(invalidated_range);
      }
    }
  }

  VerifyLength();

  super::InvalidateCache(range, from, element, options);
}

QVector<Track::Reference> ViewerOutput::GetEnabledStreamsAsReferences() const
{
  QVector<Track::Reference> refs;

  {
    int vp_sz = GetVideoStreamCount();

    for (int i=0; i<vp_sz; i++) {
      if (GetVideoParams(i).enabled()) {
        refs.append(Track::Reference(Track::kVideo, i));
      }
    }
  }

  {
    int ap_sz = GetAudioStreamCount();

    for (int i=0; i<ap_sz; i++) {
      if (GetAudioParams(i).enabled()) {
        refs.append(Track::Reference(Track::kAudio, i));
      }
    }
  }

  return refs;
}

void ViewerOutput::Retranslate()
{
  super::Retranslate();

  SetInputName(kVideoParamsInput, tr("Video Parameters"));
  SetInputName(kAudioParamsInput, tr("Audio Parameters"));

  if (HasInputWithID(kTextureInput)) {
    SetInputName(kTextureInput, tr("Texture"));
  }

  if (HasInputWithID(kSamplesInput)) {
    SetInputName(kSamplesInput, tr("Samples"));
  }

  if (HasInputWithID(kVideoAutoCacheInput)) {
    SetInputName(kVideoAutoCacheInput, tr("Auto-Cache Video"));
  }

  if (HasInputWithID(kAudioAutoCacheInput)) {
    SetInputName(kAudioAutoCacheInput, tr("Auto-Cache Audio"));
  }
}

void ViewerOutput::VerifyLength()
{
  video_length_ = VerifyLengthInternal(Track::kVideo);

  audio_length_ = VerifyLengthInternal(Track::kAudio);

  rational subtitle_length = VerifyLengthInternal(Track::kSubtitle);

  rational real_length = qMax(subtitle_length, qMax(video_length_, audio_length_));

  if (real_length != last_length_) {
    last_length_ = real_length;
    emit LengthChanged(last_length_);
  }
}

void ViewerOutput::InputConnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  }

  super::InputConnectedEvent(input, element, output);
}

void ViewerOutput::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  }

  super::InputDisconnectedEvent(input, element, output);
}

rational ViewerOutput::VerifyLengthInternal(Track::Type type) const
{
  NodeTraverser traverser;

  switch (type) {
  case Track::kVideo:
    if (IsInputConnected(kTextureInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kTextureInput), GetValueHintForInput(kTextureInput), TimeRange(0, 0));
      rational r = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();
      if (!r.isNaN()) {
        return r;
      }
    }
    break;
  case Track::kAudio:
    if (IsInputConnected(kSamplesInput)) {
      NodeValueTable t = traverser.GenerateTable(GetConnectedOutput(kSamplesInput), GetValueHintForInput(kSamplesInput), TimeRange(0, 0));
      rational r = t.Get(NodeValue::kRational, QStringLiteral("length")).value<rational>();;
      if (!r.isNaN()) {
        return r;
      }
    }
    break;
  case Track::kNone:
  case Track::kSubtitle:
  case Track::kCount:
    break;
  }

  return 0;
}

Node *ViewerOutput::GetConnectedTextureOutput()
{
  return GetConnectedOutput(kTextureInput);
}

Node::ValueHint ViewerOutput::GetConnectedTextureValueHint()
{
  return GetValueHintForInput(kTextureInput);
}

Node *ViewerOutput::GetConnectedSampleOutput()
{
  return GetConnectedOutput(kSamplesInput);
}

Node::ValueHint ViewerOutput::GetConnectedSampleValueHint()
{
  return GetValueHintForInput(kSamplesInput);
}

void ViewerOutput::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kVideoAutoCacheInput) {
    emit VideoAutoCacheChanged(GetVideoAutoCacheEnabled());
  } else if (input == kAudioAutoCacheInput) {
    emit AudioAutoCacheChanged(GetAudioAutoCacheEnabled());
  } else if (element == 0) {
    if (input == kVideoParamsInput) {

      VideoParams new_video_params = GetVideoParams();

      bool size_changed = cached_video_params_.width() != new_video_params.width() || cached_video_params_.height() != new_video_params.height();
      bool frame_rate_changed = cached_video_params_.frame_rate() != new_video_params.frame_rate();
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

      if (frame_rate_changed) {
        if (video_cache_enabled_) {
          video_frame_cache_.SetTimebase(new_video_params.frame_rate_as_time_base());
        }
        emit FrameRateChanged(new_video_params.frame_rate());
      }

      emit VideoParamsChanged();

      cached_video_params_ = new_video_params;

    } else if (input == kAudioParamsInput) {

      AudioParams new_audio_params = GetAudioParams();

      bool sample_rate_changed = new_audio_params.sample_rate() != cached_audio_params_.sample_rate();

      if (sample_rate_changed) {
        emit SampleRateChanged(new_audio_params.sample_rate());
      }

      emit AudioParamsChanged();

      if (audio_cache_enabled_) {
        audio_playback_cache_.SetParameters(GetAudioParams());
      }

      cached_audio_params_ = new_audio_params;

    }
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

void ViewerOutput::set_parameters_from_footage(const QVector<ViewerOutput *> footage)
{
  foreach (ViewerOutput* f, footage) {
    QVector<VideoParams> video_streams = f->GetEnabledVideoStreams();
    QVector<AudioParams> audio_streams = f->GetEnabledAudioStreams();

    foreach (const VideoParams& s, video_streams) {
      bool found_video_params = false;
      rational using_timebase;

      if (s.video_type() == VideoParams::kVideoTypeStill) {
        // If this is a still image, we'll use it's resolution but won't set
        // `found_video_params` in case something with a frame rate comes along which we'll
        // prioritize
        using_timebase = GetVideoParams().time_base();
      } else {
        using_timebase = s.frame_rate_as_time_base();
        found_video_params = true;
      }

      SetVideoParams(VideoParams(s.width(),
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
      SetAudioParams(AudioParams(s.sample_rate(), s.channel_layout(), AudioParams::kInternalFormat));
    }
  }
}

bool ViewerOutput::LoadCustom(QXmlStreamReader *reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled)
{
  if (reader->name() == QStringLiteral("points")) {
    timeline_points_.Load(reader);
    return true;
  } else {
    return super::LoadCustom(reader, xml_node_data, version, cancelled);
  }
}

void ViewerOutput::SaveCustom(QXmlStreamWriter *writer) const
{
  super::SaveCustom(writer);

  // Write TimelinePoints
  writer->writeStartElement(QStringLiteral("points"));
  timeline_points_.Save(writer);
  writer->writeEndElement(); // points
}

int ViewerOutput::AddStream(Track::Type type, const QVariant& value)
{
  QString id;

  if (type == Track::kVideo) {
    id = kVideoParamsInput;
  } else if (type == Track::kAudio) {
    id = kAudioParamsInput;
  } else {
    return -1;
  }

  // Add another video/audio param to the array for this stream
  int index = InputArraySize(id);
  InputArrayAppend(id);

  SetStandardValue(id, value, index);

  return index;
}

QVector<VideoParams> ViewerOutput::GetEnabledVideoStreams() const
{
  QVector<VideoParams> streams;

  int vp_sz = GetVideoStreamCount();

  for (int i=0; i<vp_sz; i++) {
    VideoParams vp = GetVideoParams(i);

    if (vp.enabled()) {
      streams.append(vp);
    }
  }

  return streams;
}

QVector<AudioParams> ViewerOutput::GetEnabledAudioStreams() const
{
  QVector<AudioParams> streams;

  int ap_sz = GetAudioStreamCount();

  for (int i=0; i<ap_sz; i++) {
    AudioParams ap = GetAudioParams(i);

    if (ap.enabled()) {
      streams.append(ap);
    }
  }

  return streams;
}

}
