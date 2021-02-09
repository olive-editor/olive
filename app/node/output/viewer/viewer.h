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

#include <QUuid>

#include "node/block/block.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/node.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "render/framehashcache.h"
#include "render/videoparams.h"
#include "timeline/timelinecommon.h"

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

  void ShiftVideoCache(const rational& from, const rational& to);
  void ShiftAudioCache(const rational& from, const rational& to);
  void ShiftCache(const rational& from, const rational& to);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1) override;

  const VideoParams& video_params() const
  {
    return video_params_;
  }

  const AudioParams& audio_params() const
  {
    return audio_params_;
  }

  void set_video_params(const VideoParams &video);
  void set_audio_params(const AudioParams &audio);

  rational GetLength();

  const QUuid& uuid() const
  {
    return uuid_;
  }

  const QVector<Track *> &GetTracks() const
  {
    return track_cache_;
  }

  Track* GetTrackFromReference(const Track::Reference& track_ref) const
  {
    return track_lists_.at(track_ref.type())->GetTrackAt(track_ref.index());
  }

  /**
   * @brief Same as GetTracks() but omits tracks that are locked.
   */
  QVector<Track *> GetUnlockedTracks() const;

  TrackList* track_list(Track::Type type) const
  {
    return track_lists_.at(type);
  }

  virtual void Retranslate() override;

  FrameHashCache* video_frame_cache()
  {
    return &video_frame_cache_;
  }

  AudioPlaybackCache* audio_playback_cache()
  {
    return &audio_playback_cache_;
  }

  virtual void BeginOperation() override;

  virtual void EndOperation() override;

  static const QString kTextureInput;
  static const QString kSamplesInput;
  static const QString kTrackInputFormat;

signals:
  void TimebaseChanged(const rational&);

  void LengthChanged(const rational& length);

  void SizeChanged(int width, int height);

  void PixelAspectChanged(const rational& pixel_aspect);

  void InterlacingChanged(VideoParams::Interlacing mode);

  void VideoParamsChanged();
  void AudioParamsChanged();

  void TrackAdded(Track* track);
  void TrackRemoved(Track* track);

  void TextureInputChanged();

protected:
  void InputConnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  void InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output) override;

private:
  QUuid uuid_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QVector<TrackList*> track_lists_;

  QVector<Track*> track_cache_;

  rational last_length_;

  FrameHashCache video_frame_cache_;

  AudioPlaybackCache audio_playback_cache_;

  int operation_stack_;

private slots:
  void UpdateTrackCache();

  void VerifyLength();

};

}

#endif // VIEWER_H
