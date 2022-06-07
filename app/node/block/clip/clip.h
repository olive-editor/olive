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

#ifndef CLIPBLOCK_H
#define CLIPBLOCK_H

#include "audio/audiovisualwaveform.h"
#include "node/block/block.h"
#include "node/output/track/track.h"

namespace olive {

class ViewerOutput;

/**
 * @brief Node that represents a block of Media
 */
class ClipBlock : public Block
{
  Q_OBJECT
public:
  ClipBlock();

  NODE_DEFAULT_FUNCTIONS(ClipBlock)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual void set_length_and_media_out(const rational &length) override;
  virtual void set_length_and_media_in(const rational &length) override;

  Track::Type GetTrackType() const
  {
    if (track()) {
      return track()->type();
    } else {
      return Track::kNone;
    }
  }

  rational media_in() const;
  void set_media_in(const rational& media_in);

  bool IsAutocaching() const { return GetStandardValue(kAutoCacheInput).toBool(); }
  void SetAutocache(bool e);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options) override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void Retranslate() override;

  double speed() const
  {
    return GetStandardValue(kSpeedInput).toDouble();
  }

  bool reverse() const
  {
    return GetStandardValue(kReverseInput).toBool();
  }

  void set_reverse(bool e)
  {
    SetStandardValue(kReverseInput, e);
  }

  bool maintain_audio_pitch() const
  {
    return GetStandardValue(kMaintainAudioPitchInput).toBool();
  }

  void set_maintain_audio_pitch(bool e)
  {
    SetStandardValue(kMaintainAudioPitchInput, e);
  }

  TransitionBlock* in_transition()
  {
    return in_transition_;
  }

  void set_in_transition(TransitionBlock* t)
  {
    in_transition_ = t;
  }

  TransitionBlock* out_transition()
  {
    return out_transition_;
  }

  void set_out_transition(TransitionBlock* t)
  {
    out_transition_ = t;
  }

  const QVector<Block*>& block_links() const
  {
    return block_links_;
  }

  FrameHashCache *connected_video_cache() const
  {
    if (Node *n = GetConnectedOutput(kBufferIn)) {
      return n->video_frame_cache();
    } else {
      return nullptr;
    }
  }

  AudioPlaybackCache *connected_audio_cache() const
  {
    if (Node *n = GetConnectedOutput(kBufferIn)) {
      return n->audio_playback_cache();
    } else {
      return nullptr;
    }
  }

  FrameHashCache *thumbnails()
  {
    if (Node *n = GetConnectedOutput(kBufferIn)) {
      return n->thumbnail_cache();
    } else {
      return nullptr;
    }
  }

  AudioWaveformCache *waveform()
  {
    if (Node *n = GetConnectedOutput(kBufferIn)) {
      return n->waveform_cache();
    } else {
      return nullptr;
    }
  }

  void AddCachePassthroughFrom(ClipBlock *other);

  ViewerOutput *connected_viewer() const
  {
    return connected_viewer_;
  }

  virtual TimeRange GetVideoCacheRange() const override
  {
    return TimeRange(0, length());
  }

  virtual TimeRange GetAudioCacheRange() const override
  {
    return TimeRange(0, length());
  }

  virtual void ConnectedToPreviewEvent() override;

  static const QString kBufferIn;
  static const QString kMediaInInput;
  static const QString kSpeedInput;
  static const QString kReverseInput;
  static const QString kMaintainAudioPitchInput;

  static const QString kAutoCacheInput;

protected:
  virtual void LinkChangeEvent() override;

  virtual void InputConnectedEvent(const QString& input, int element, Node *output) override;

  virtual void InputDisconnectedEvent(const QString& input, int element, Node *output) override;

  virtual void InputValueChangedEvent(const QString& input, int element) override;

private:
  rational SequenceToMediaTime(const rational& sequence_time, bool ignore_reverse = false, bool ignore_speed = false) const;

  rational MediaToSequenceTime(const rational& media_time) const;

  void RequestRangeFromConnected(const TimeRange &range);
  void RequestInvalidatedFromConnected();

  void RequestRangeForCache(PlaybackCache *cache, const TimeRange &max_range, const TimeRange &range, bool invalidate, bool request);
  void RequestInvalidatedForCache(PlaybackCache *cache, const TimeRange &max_range);

  QVector<Block*> block_links_;

  TransitionBlock* in_transition_;
  TransitionBlock* out_transition_;

  ViewerOutput *connected_viewer_;

private:
  rational last_media_in_;

};

}

#endif // TIMELINEBLOCK_H
