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
#include "project/item/item.h"

/**
 * @brief The main timeline object, an graph of edited clips that forms a complete edit
 */
class Sequence : public Item, public NodeGraph
{
public:
  Sequence();

  /**
   * @brief Item::Type() override
   */
  virtual Type type() const override;

  /* VIDEO GETTER/SETTER FUNCTIONS */

  const int& video_width();
  void set_video_width(const int& width);

  const int& video_height() const;
  void set_video_height(const int& video_height);

  const rational& video_time_base();
  void set_video_time_base(const rational& time_base);

  /* AUDIO GETTER/SETTER FUNCTIONS */

  const int& audio_sampling_rate();
  void set_audio_sampling_rate(const int& sample_rate);

  const rational& audio_time_base();
  void set_audio_time_base(const rational& time_base);

private:
  int video_width_;
  int video_height_;
  rational video_time_base_;

  int audio_sampling_rate_;
  rational audio_time_base_;
};

#endif // SEQUENCE_H
