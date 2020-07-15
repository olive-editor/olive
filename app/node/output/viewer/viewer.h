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
#include "timeline/trackreference.h"

OLIVE_NAMESPACE_ENTER

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

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QList<CategoryID> Category() const override;
  virtual QString Description() const override;

  void ShiftVideoCache(const rational& from, const rational& to);
  void ShiftAudioCache(const rational& from, const rational& to);
  void ShiftCache(const rational& from, const rational& to);

  NodeInput* texture_input() const {
    return texture_input_;
  }

  NodeInput* samples_input() const {
    return samples_input_;
  }

  virtual void InvalidateCache(const TimeRange &range, NodeInput *from, NodeInput* source) override;

  const VideoParams& video_params() const {
    return video_params_;
  }

  const AudioParams& audio_params() const {
    return audio_params_;
  }

  void set_video_params(const VideoParams &video);
  void set_audio_params(const AudioParams &audio);

  rational GetLength();

  const QUuid& uuid() const {
    return uuid_;
  }

  const QVector<TrackOutput *> &GetTracks() const {
    return track_cache_;
  }

  /**
   * @brief Same as GetTracks() but omits tracks that are locked.
   */
  QVector<TrackOutput *> GetUnlockedTracks() const;

  NodeInput* track_input(Timeline::TrackType type) const {
    return track_inputs_.at(type);
  }

  TrackList* track_list(Timeline::TrackType type) const {
    return track_lists_.at(type);
  }

  virtual void Retranslate() override;

  const QString& media_name() const {
    return media_name_;
  }

  void set_media_name(const QString& name);

  FrameHashCache* video_frame_cache() {
    return &video_frame_cache_;
  }

  AudioPlaybackCache* audio_playback_cache() {
    return &audio_playback_cache_;
  }

  virtual void BeginOperation() override;

  virtual void EndOperation() override;

signals:
  void TimebaseChanged(const rational&);

  void GraphChangedFrom(NodeInput* source);

  void LengthChanged(const rational& length);

  void SizeChanged(int width, int height);

  void ParamsChanged();

  void BlockAdded(Block* block, TrackReference track);
  void BlockRemoved(Block* block);

  void TrackAdded(TrackOutput* track, Timeline::TrackType type);
  void TrackRemoved(TrackOutput* track);

  void TrackHeightChanged(Timeline::TrackType type, int index, int height);

  void MediaNameChanged(const QString& name);

private:
  QUuid uuid_;

  NodeInput* texture_input_;

  NodeInput* samples_input_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QVector<NodeInput*> track_inputs_;

  QVector<TrackList*> track_lists_;

  QVector<TrackOutput*> track_cache_;

  rational last_length_;

  QString media_name_;

  FrameHashCache video_frame_cache_;

  AudioPlaybackCache audio_playback_cache_;

  int operation_stack_;

private slots:
  void UpdateTrackCache();

  void VerifyLength();

  void TrackListAddedBlock(Block* block, int index);

  void TrackListAddedTrack(TrackOutput* track);

  void TrackHeightChangedSlot(int index, int height);

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWER_H
