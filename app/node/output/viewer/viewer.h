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

#ifndef VIEWER_H
#define VIEWER_H

#include "common/rational.h"
#include "node/node.h"
#include "node/output/track/track.h"
#include "project/item/footage/footage.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "render/framehashcache.h"
#include "render/videoparams.h"
#include "render/videoparams.h"

namespace olive {

/**
 * @brief A bridge between a node system and a ViewerPanel
 *
 * Receives update/time change signals from ViewerPanels and responds by sending them a texture of that frame
 */
class ViewerOutput : public Node
{
  Q_OBJECT
public:
  ViewerOutput();
  virtual ~ViewerOutput() override;

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual QString duration() const override;
  virtual QString rate() const override;

  void set_default_parameters();

  void set_parameters_from_footage(const QVector<Footage *> footage);

  void ShiftVideoCache(const rational& from, const rational& to);
  void ShiftAudioCache(const rational& from, const rational& to);
  void ShiftCache(const rational& from, const rational& to);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, qint64 job_time) override;

  VideoParams video_params() const
  {
    return GetStandardValue(kVideoParamsInput).value<VideoParams>();
  }

  AudioParams audio_params() const
  {
    return GetStandardValue(kAudioParamsInput).value<AudioParams>();
  }

  void set_video_params(const VideoParams &video)
  {
    SetStandardValue(kVideoParamsInput, QVariant::fromValue(video));
  }

  void set_audio_params(const AudioParams &audio)
  {
    SetStandardValue(kAudioParamsInput, QVariant::fromValue(audio));
  }

  const rational &GetLength() const;

  FrameHashCache* video_frame_cache()
  {
    return &video_frame_cache_;
  }

  AudioPlaybackCache* audio_playback_cache()
  {
    return &audio_playback_cache_;
  }

  virtual void Retranslate() override;

  virtual void BeginOperation() override;

  virtual void EndOperation() override;

  static const QString kVideoParamsInput;
  static const QString kAudioParamsInput;

  static const QString kTextureInput;
  static const QString kSamplesInput;

  static const uint64_t kVideoParamEditMask;

signals:
  void TimebaseChanged(const rational&);

  void LengthChanged(const rational& length);

  void SizeChanged(int width, int height);

  void PixelAspectChanged(const rational& pixel_aspect);

  void InterlacingChanged(VideoParams::Interlacing mode);

  void VideoParamsChanged();
  void AudioParamsChanged();

  void TextureInputChanged();

public slots:
  void VerifyLength();

protected:
  virtual void InputConnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual void InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual rational GetCustomLength(Track::Type type) const;

  virtual void ShiftVideoEvent(const rational &from, const rational &to);

  virtual void ShiftAudioEvent(const rational &from, const rational &to);

  virtual void InputValueChangedEvent(const QString& input, int element) override;

private:
  rational last_length_;

  FrameHashCache video_frame_cache_;

  AudioPlaybackCache audio_playback_cache_;

  int operation_stack_;

  VideoParams cached_video_params_;

};

}

#endif // VIEWER_H
