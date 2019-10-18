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

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "common/rational.h"
#include "node/graph.h"
#include "node/output/timeline/timeline.h"
#include "node/output/viewer/viewer.h"
#include "node/processor/videorenderer/videorenderer.h"
#include "project/item/item.h"

class Sequence;
using SequencePtr = std::shared_ptr<Sequence>;

/**
 * @brief The main timeline object, an graph of edited clips that forms a complete edit
 */
class Sequence : public Item, public NodeGraph
{
public:
  Sequence();

  static void Open(SequencePtr sequence);

  void add_default_nodes();

  /**
   * @brief Item::Type() override
   */
  virtual Type type() const override;

  virtual QIcon icon() override;

  virtual QString duration() override;
  virtual QString rate() override;

  /* VIDEO GETTER/SETTER FUNCTIONS */

  const int& video_width();
  void set_video_width(const int& width);

  const int& video_height() const;
  void set_video_height(const int& height);

  const rational& video_time_base();
  void set_video_time_base(const rational& time_base);

  /* AUDIO GETTER/SETTER FUNCTIONS */

  const rational& audio_time_base();
  void set_audio_time_base(const rational& time_base);

  const uint64_t& audio_channel_layout();
  void set_audio_channel_layout(const uint64_t& channel_layout);

  void SetDefaultParameters();

private:
  void update_video_parameters();

  TimelineOutput* timeline_output_;
  VideoRendererProcessor* video_renderer_processor_;
  ViewerOutput* viewer_output_;
  TrackOutput* video_track_output_;
  TrackOutput* audio_track_output_;

  int video_width_;
  int video_height_;
  rational video_time_base_;

  rational audio_time_base_;
  uint64_t audio_channel_layout_;
};

#endif // SEQUENCE_H
