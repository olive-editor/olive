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

#include "viewer.h"

#include "config/config.h"
#include "core.h"

namespace olive {

const QString ViewerOutput::kVideoParamsInput = QStringLiteral("video_param_in");
const QString ViewerOutput::kAudioParamsInput = QStringLiteral("audio_param_in");
const QString ViewerOutput::kSubtitleParamsInput = QStringLiteral("subtitle_param_in");
const QString ViewerOutput::kTextureInput = QStringLiteral("tex_in");
const QString ViewerOutput::kSamplesInput = QStringLiteral("samples_in");

const SampleFormat ViewerOutput::kDefaultSampleFormat = SampleFormat::F32P;

#define super Node

ViewerOutput::ViewerOutput(bool create_buffer_inputs, bool create_default_streams) :
  last_length_(0),
  video_length_(0),
  audio_length_(0),
  autocache_input_video_(false),
  autocache_input_audio_(false),
  waveform_requests_enabled_(false)
{
  AddInput(kVideoParamsInput, NodeValue::kVideoParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagArray | kInputFlagHidden));

  AddInput(kAudioParamsInput, NodeValue::kAudioParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagArray | kInputFlagHidden));

  AddInput(kSubtitleParamsInput, NodeValue::kSubtitleParams, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagArray | kInputFlagHidden));

  if (create_buffer_inputs) {
    AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
    AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));
  }

  if (create_default_streams) {
    AddStream(Track::kVideo, QVariant());
    AddStream(Track::kAudio, QVariant());
    set_default_parameters();
  }

  SetFlag(kDontShowInParamView);

  workarea_ = new TimelineWorkArea(this);
  markers_ = new TimelineMarkerList(this);
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

QVariant ViewerOutput::data(const DataType &d) const
{
  switch (d) {
  case DURATION:
  {
    rational using_timebase;
    Timecode::Display using_display = Core::instance()->GetTimecodeDisplay();

    // Get first enabled streams
    VideoParams video = GetFirstEnabledVideoStream();
    AudioParams audio = GetFirstEnabledAudioStream();
    SubtitleParams sub = GetFirstEnabledSubtitleStream();

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
    } else if (sub.is_valid()) {
      using_timebase = OLIVE_CONFIG("DefaultSequenceFrameRate").value<rational>();
    }

    if (!using_timebase.isNull()) {
      // Return time transformed to timecode
      return QString::fromStdString(Timecode::time_to_timecode(GetLength(), using_timebase, using_display));
    }
    break;
  }
  case FREQUENCY_RATE:
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
    break;
  }
  default:
    break;
  }

  return super::data(d);
}

bool ViewerOutput::HasEnabledVideoStreams() const
{
  return GetFirstEnabledVideoStream().is_valid();
}

bool ViewerOutput::HasEnabledAudioStreams() const
{
  return GetFirstEnabledAudioStream().is_valid();
}

bool ViewerOutput::HasEnabledSubtitleStreams() const
{
  return GetFirstEnabledSubtitleStream().is_valid();
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

SubtitleParams ViewerOutput::GetFirstEnabledSubtitleStream() const
{
  int sz = GetSubtitleStreamCount();

  for (int i=0; i<sz; i++) {
    SubtitleParams sp = GetSubtitleParams(i);

    if (sp.enabled()) {
      return sp;
    }
  }

  return SubtitleParams();
}

void ViewerOutput::set_default_parameters()
{
  int width = OLIVE_CONFIG("DefaultSequenceWidth").toInt();
  int height = OLIVE_CONFIG("DefaultSequenceHeight").toInt();

  SetVideoParams(VideoParams(
                   width,
                   height,
                   OLIVE_CONFIG("DefaultSequenceFrameRate").value<rational>(),
                 static_cast<PixelFormat::Format>(OLIVE_CONFIG("OfflinePixelFormat").toInt()),
      VideoParams::kInternalChannelCount,
      OLIVE_CONFIG("DefaultSequencePixelAspect").value<rational>(),
      OLIVE_CONFIG("DefaultSequenceInterlacing").value<VideoParams::Interlacing>(),
      VideoParams::generate_auto_divider(width, height)
      ));
  SetAudioParams(AudioParams(
      OLIVE_CONFIG("DefaultSequenceAudioFrequency").toInt(),
      OLIVE_CONFIG("DefaultSequenceAudioLayout").toULongLong(),
      kDefaultSampleFormat
      ));
}

void ViewerOutput::InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options)
{
  Q_UNUSED(element)

  if (Node *connected = GetConnectedOutput(from, element)) {
    if (from == kTextureInput) {
      //connected->thumbnail_cache()->Request(range.Intersected(max_range), PlaybackCache::kPreviewsOnly);
      if (autocache_input_video_) {
        TimeRange max_range = InputTimeAdjustment(from, element, TimeRange(0, GetVideoLength()), false);
        connected->video_frame_cache()->Request(this, range.Intersected(max_range));
      }
    } else if (from == kSamplesInput) {
      TimeRange max_range = InputTimeAdjustment(from, element, TimeRange(0, GetAudioLength()), false);
      if (waveform_requests_enabled_) {
        connected->waveform_cache()->Request(this, range.Intersected(max_range));
      }
      if (autocache_input_audio_) {
        connected->audio_playback_cache()->Request(this, range.Intersected(max_range));
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

  {
    int sp_sz = GetSubtitleStreamCount();

    for (int i=0; i<sp_sz; i++) {
      if (GetSubtitleParams(i).enabled()) {
        refs.append(Track::Reference(Track::kSubtitle, i));
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
  SetInputName(kSubtitleParamsInput, tr("Subtitle Parameters"));

  if (HasInputWithID(kTextureInput)) {
    SetInputName(kTextureInput, tr("Texture"));
  }

  if (HasInputWithID(kSamplesInput)) {
    SetInputName(kSamplesInput, tr("Samples"));
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

void ViewerOutput::SetPlayhead(const rational &t)
{
  playhead_ = t;
  emit PlayheadChanged(t);
}

void ViewerOutput::InputConnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else if (input == kSamplesInput) {
    connect(output->waveform_cache(), &AudioWaveformCache::Validated, this, &ViewerOutput::ConnectedWaveformChanged);
  }

  super::InputConnectedEvent(input, element, output);
}

void ViewerOutput::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kTextureInput) {
    emit TextureInputChanged();
  } else if (input == kSamplesInput) {
    disconnect(output->waveform_cache(), &AudioWaveformCache::Validated, this, &ViewerOutput::ConnectedWaveformChanged);
  }

  super::InputDisconnectedEvent(input, element, output);
}

rational ViewerOutput::VerifyLengthInternal(Track::Type type) const
{
  switch (type) {
  case Track::kVideo:
    if (ViewerOutput *v = dynamic_cast<ViewerOutput *>(GetConnectedOutput(kTextureInput))) {
      return v->GetVideoLength();
    }
    break;
  case Track::kAudio:
    if (ViewerOutput *v = dynamic_cast<ViewerOutput *>(GetConnectedOutput(kSamplesInput))) {
      return v->GetAudioLength();
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

void ViewerOutput::SetWaveformEnabled(bool e)
{
  if ((waveform_requests_enabled_ = e)) {
    if (Node *connected = this->GetConnectedSampleOutput()) {
      TimeRange max_range = InputTimeAdjustment(kSamplesInput, -1, TimeRange(0, GetAudioLength()), false);
      TimeRangeList invalid = connected->waveform_cache()->GetInvalidatedRanges(max_range);
      for (const TimeRange &r : invalid) {
        connected->waveform_cache()->Request(this, r);
      }
    }
  }
}

NodeValue ViewerOutput::Value(const ValueParams &p) const
{
  if (HasInputWithID(kTextureInput)) {
    NodeValue repush = GetInputValue(p, kTextureInput);
    repush.set_tag(Track::Reference(Track::kVideo, 0).ToString());
    return repush;
  }
  if (HasInputWithID(kSamplesInput)) {
    NodeValue repush = GetInputValue(p, kSamplesInput);
    repush.set_tag(Track::Reference(Track::kAudio, 0).ToString());
    return repush;
  }

  return NodeValue();
}

bool ViewerOutput::LoadCustom(QXmlStreamReader *reader, SerializedData *data)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("markers")) {
      if (!this->GetMarkers()->load(reader)) {
        return false;
      }
    } else if (reader->name() == QStringLiteral("workarea")) {
      if (!this->GetWorkArea()->load(reader)) {
        return false;
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void ViewerOutput::SaveCustom(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("workarea"));
  this->GetWorkArea()->save(writer);
  writer->writeEndElement(); // workarea

  writer->writeStartElement(QStringLiteral("markers"));
  this->GetMarkers()->save(writer);
  writer->writeEndElement(); // markers
}

void ViewerOutput::InputValueChangedEvent(const QString &input, int element)
{
  if (element == 0) {
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

      cached_audio_params_ = new_audio_params;

    }
  }

  super::InputValueChangedEvent(input, element);
}

void ViewerOutput::set_parameters_from_footage(const QVector<ViewerOutput *> footage)
{
  foreach (ViewerOutput* f, footage) {
    QVector<VideoParams> video_streams = f->GetEnabledVideoStreams();
    QVector<AudioParams> audio_streams = f->GetEnabledAudioStreams();

    for (int i=0; i<video_streams.size(); i++) {
      const VideoParams& s = video_streams.at(i);

      bool found_video_params = false;
      rational using_timebase;

      if (s.video_type() == VideoParams::kVideoTypeStill) {
        // If this is a still image, we'll use it's resolution but won't set
        // `found_video_params` in case something with a frame rate comes along which we'll
        // prioritize
        if (i > 0) {
          // Ignore still images past stream 0
          continue;
        }

        using_timebase = GetVideoParams().time_base();
      } else {
        using_timebase = s.frame_rate_as_time_base();
        found_video_params = true;
      }

      SetVideoParams(VideoParams(s.width(),
                                   s.height(),
                                   using_timebase,
                                   static_cast<PixelFormat::Format>(OLIVE_CONFIG("OfflinePixelFormat").toInt()),
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
      SetAudioParams(AudioParams(s.sample_rate(), s.channel_layout(), kDefaultSampleFormat));
    }
  }
}

int ViewerOutput::AddStream(Track::Type type, const QVariant& value)
{
  return SetStream(type, value, -1);
}

int ViewerOutput::SetStream(Track::Type type, const QVariant &value, int index_in)
{
  QString id;

  if (type == Track::kVideo) {
    id = kVideoParamsInput;
  } else if (type == Track::kAudio) {
    id = kAudioParamsInput;
  } else if (type == Track::kSubtitle) {
    id = kSubtitleParamsInput;
  } else {
    return -1;
  }

  // Add another video/audio param to the array for this stream
  int index = (index_in == -1) ? InputArraySize(id) : index_in;

  if (index >= InputArraySize(id)) {
    InputArrayResize(id, index+1);
  }

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
