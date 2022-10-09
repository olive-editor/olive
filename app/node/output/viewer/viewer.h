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

#ifndef VIEWER_H
#define VIEWER_H

#include "codec/encoder.h"
#include "common/rational.h"
#include "node/node.h"
#include "node/output/track/track.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "render/framehashcache.h"
#include "render/subtitleparams.h"
#include "render/videoparams.h"
#include "timeline/timelinemarker.h"
#include "timeline/timelineworkarea.h"

namespace olive {

class Footage;

/**
 * @brief A bridge between a node system and a ViewerPanel
 *
 * Receives update/time change signals from ViewerPanels and responds by sending them a texture of that frame
 */
class ViewerOutput : public Node
{
  Q_OBJECT
public:
  ViewerOutput(bool create_buffer_inputs = true, bool create_default_streams = true);

  NODE_DEFAULT_FUNCTIONS(ViewerOutput)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual QString duration() const override;
  virtual QString rate() const override;

  void set_default_parameters();

  void set_parameters_from_footage(const QVector<ViewerOutput *> footage);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options) override;

  VideoParams GetVideoParams(int index = 0) const
  {
    // This check isn't strictly necessary (GetStandardValue will return a null VideoParams anyway),
    // but it does suppress a warning message that we don't need
    if (index < InputArraySize(kVideoParamsInput)) {
      return GetStandardValue(kVideoParamsInput, index).value<VideoParams>();
    } else {
      return VideoParams();
    }
  }

  AudioParams GetAudioParams(int index = 0) const
  {
    // This check isn't strictly necessary (GetStandardValue will return a null VideoParams anyway),
    // but it does suppress a warning message that we don't need
    if (index < InputArraySize(kAudioParamsInput)) {
      return GetStandardValue(kAudioParamsInput, index).value<AudioParams>();
    } else {
      return AudioParams();
    }
  }

  SubtitleParams GetSubtitleParams(int index = 0) const
  {
    // This check isn't strictly necessary (GetStandardValue will return a null VideoParams anyway),
    // but it does suppress a warning message that we don't need
    if (index < InputArraySize(kSubtitleParamsInput)) {
      return GetStandardValue(kSubtitleParamsInput, index).value<SubtitleParams>();
    } else {
      return SubtitleParams();
    }
  }

  void SetVideoParams(const VideoParams &video, int index = 0)
  {
    SetStandardValue(kVideoParamsInput, QVariant::fromValue(video), index);
  }

  void SetAudioParams(const AudioParams &audio, int index = 0)
  {
    SetStandardValue(kAudioParamsInput, QVariant::fromValue(audio), index);
  }

  void SetSubtitleParams(const SubtitleParams &subs, int index = 0)
  {
    SetStandardValue(kSubtitleParamsInput, QVariant::fromValue(subs), index);
  }

  int GetVideoStreamCount() const
  {
    return InputArraySize(kVideoParamsInput);
  }

  int GetAudioStreamCount() const
  {
    return InputArraySize(kAudioParamsInput);
  }

  int GetSubtitleStreamCount() const
  {
    return InputArraySize(kSubtitleParamsInput);
  }

  virtual int GetTotalStreamCount() const
  {
    return GetVideoStreamCount() + GetAudioStreamCount() + GetSubtitleStreamCount();
  }

  const AudioWaveformCache *GetConnectedWaveform()
  {
    if (Node *n = GetConnectedSampleOutput()) {
      return n->waveform_cache();
    } else {
      return nullptr;
    }
  }

  bool HasEnabledVideoStreams() const;
  bool HasEnabledAudioStreams() const;
  bool HasEnabledSubtitleStreams() const;

  VideoParams GetFirstEnabledVideoStream() const;
  AudioParams GetFirstEnabledAudioStream() const;
  SubtitleParams GetFirstEnabledSubtitleStream() const;

  const rational &GetLength() const { return last_length_; }
  const rational &GetVideoLength() const { return video_length_; }
  const rational &GetAudioLength() const { return audio_length_; }

  TimelineWorkArea *GetWorkArea() const { return workarea_; }
  TimelineMarkerList *GetMarkers() const { return markers_; }

  virtual TimeRange GetVideoCacheRange() const override
  {
    return TimeRange(0, GetVideoLength());
  }

  virtual TimeRange GetAudioCacheRange() const override
  {
    return TimeRange(0, GetAudioLength());
  }

  QVector<Track::Reference> GetEnabledStreamsAsReferences() const;

  QVector<VideoParams> GetEnabledVideoStreams() const;

  QVector<AudioParams> GetEnabledAudioStreams() const;

  virtual void Retranslate() override;

  virtual Node *GetConnectedTextureOutput();

  virtual ValueHint GetConnectedTextureValueHint();

  virtual Node *GetConnectedSampleOutput();

  virtual ValueHint GetConnectedSampleValueHint();

  void SetWaveformEnabled(bool e);

  bool IsVideoAutoCacheEnabled() const { qDebug() << "sequence ac is a stub"; return false; }
  void SetVideoAutoCacheEnabled(bool e) { qDebug() << "sequence ac is a stub"; }

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  const EncodingParams &GetLastUsedEncodingParams() const { return last_used_encoding_params_; }
  void SetLastUsedEncodingParams(const EncodingParams &p) { last_used_encoding_params_ = p; }

  static const QString kVideoParamsInput;
  static const QString kAudioParamsInput;
  static const QString kSubtitleParamsInput;

  static const QString kTextureInput;
  static const QString kSamplesInput;

signals:
  void FrameRateChanged(const rational&);

  void LengthChanged(const rational& length);

  void SizeChanged(int width, int height);

  void PixelAspectChanged(const rational& pixel_aspect);

  void InterlacingChanged(VideoParams::Interlacing mode);

  void VideoParamsChanged();
  void AudioParamsChanged();

  void TextureInputChanged();

  void SampleRateChanged(int sr);

  void ConnectedWaveformChanged();

public slots:
  void VerifyLength();

protected:
  virtual void InputConnectedEvent(const QString &input, int element, Node *output) override;

  virtual void InputDisconnectedEvent(const QString &input, int element, Node *output) override;

  virtual rational VerifyLengthInternal(Track::Type type) const;

  virtual void InputValueChangedEvent(const QString& input, int element) override;

  int AddStream(Track::Type type, const QVariant &value);
  int SetStream(Track::Type type, const QVariant &value, int index);

private:
  rational last_length_;
  rational video_length_;
  rational audio_length_;

  VideoParams cached_video_params_;

  AudioParams cached_audio_params_;

  TimelineWorkArea *workarea_;
  TimelineMarkerList *markers_;

  bool autocache_input_video_;
  bool autocache_input_audio_;

  EncodingParams last_used_encoding_params_;

  bool waveform_requests_enabled_;

};

}

#endif // VIEWER_H
