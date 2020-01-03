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

#include "common/timelinecommon.h"
#include "node/block/block.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/node.h"
#include "render/videoparams.h"
#include "render/audioparams.h"
#include "timeline/trackreference.h"
#include "timeline/tracktypes.h"

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
  virtual QString Category() const override;
  virtual QString Description() const override;

  NodeInput* texture_input();
  NodeInput* samples_input();
  NodeInput* length_input();

  virtual void InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from = nullptr) override;

  const VideoParams& video_params() const;
  const AudioParams& audio_params() const;

  void set_video_params(const VideoParams& video);
  void set_audio_params(const AudioParams& audio);

  rational Length();

  const QUuid& uuid() const;

  const QVector<TrackOutput *> &Tracks() const;

  NodeInput* track_input(TrackType type) const;

  TrackList* track_list(TrackType type) const;

  virtual void Retranslate() override;

protected:
  virtual void DependentEdgeChanged(NodeInput* from) override;

signals:
  void TimebaseChanged(const rational&);

  void VideoChangedBetween(const rational&, const rational&);

  void AudioChangedBetween(const rational&, const rational&);

  void VideoGraphChanged();

  void AudioGraphChanged();

  void LengthChanged(const rational& length);

  void SizeChanged(int width, int height);

  void BlockAdded(Block* block, TrackReference track);
  void BlockRemoved(Block* block);

  void TrackAdded(TrackOutput* track, TrackType type);
  void TrackRemoved(TrackOutput* track);

  void TrackHeightChanged(TrackType type, int index, int height);

private:
  QUuid uuid_;

  NodeInput* texture_input_;

  NodeInput* samples_input_;

  NodeInput* length_input_;

  VideoParams video_params_;

  AudioParams audio_params_;

  QVector<NodeInput*> track_inputs_;

  QVector<TrackList*> track_lists_;

  QVector<TrackOutput*> track_cache_;

  rational timeline_length_;

private slots:
  void UpdateTrackCache();

  void UpdateLength(const rational &length);

  void TrackListAddedBlock(Block* block, int index);

  void TrackListAddedTrack(TrackOutput* track);

  void TrackHeightChangedSlot(int index, int height);

};

#endif // VIEWER_H
