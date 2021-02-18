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

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QUuid>

#include "common/rational.h"
#include "node/block/block.h"
#include "node/graph.h"
#include "node/node.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/traverser.h"
#include "project/item/footage/footage.h"
#include "project/item/item.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "render/framehashcache.h"
#include "render/videoparams.h"
#include "render/videoparams.h"
#include "timeline/timelinecommon.h"
#include "timeline/timelinepoints.h"
#include "timeline/timelinepoints.h"

namespace olive {

/**
 * @brief The main timeline object, an graph of edited clips that forms a complete edit
 */
class Sequence : public Item, public TimelinePoints
{
  Q_OBJECT
public:
  Sequence(bool viewer_only_mode = false);

  virtual ~Sequence() override;

  virtual Node* copy() const override
  {
    return new Sequence();
  }

  virtual QString Name() const override
  {
    return tr("Sequence");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.sequence");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryProject};
  }

  virtual QString Description() const override
  {
    return tr("A series of cuts that result in an edited video. Also called a timeline.");
  }

  void add_default_nodes(MultiUndoCommand *command);

  virtual QIcon icon() const override;

  virtual QString duration() override;
  virtual QString rate() override;

  void set_default_parameters();

  void set_parameters_from_footage(const QVector<Footage *> footage);

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

  void ShiftVideoCache(const rational& from, const rational& to);
  void ShiftAudioCache(const rational& from, const rational& to);
  void ShiftCache(const rational& from, const rational& to);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1) override;

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

  rational GetLength();

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

  static const QString kVideoParamsInput;
  static const QString kAudioParamsInput;

  static const QString kTextureInput;
  static const QString kSamplesInput;
  static const QString kTrackInputFormat;

  TimelinePoints* timeline_points()
  {
    return &timeline_points_;
  }

  const QUuid& uuid() const
  {
    return uuid_;
  }

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

public slots:
  void VerifyLength();

protected:
  virtual void InputConnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual void InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual rational GetCustomLength(Track::Type type) const;

  virtual void ShiftVideoEvent(const rational &from, const rational &to);

  virtual void ShiftAudioEvent(const rational &from, const rational &to);

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt* cancelled) override;

  virtual void SaveInternal(QXmlStreamWriter *writer) const override;

  virtual void InputValueChangedEvent(const QString& input, int element) override;

private:
  QVector<TrackList*> track_lists_;

  QVector<Track*> track_cache_;

  QUuid uuid_;

  rational last_length_;

  FrameHashCache video_frame_cache_;

  AudioPlaybackCache audio_playback_cache_;

  int operation_stack_;

  VideoParams cached_video_params_;

  TimelinePoints timeline_points_;

private slots:
  void UpdateTrackCache();

};

}

#endif // SEQUENCE_H
