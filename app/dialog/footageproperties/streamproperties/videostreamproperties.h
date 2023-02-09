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

#ifndef VIDEOSTREAMPROPERTIES_H
#define VIDEOSTREAMPROPERTIES_H

#include <QCheckBox>
#include <QComboBox>

#include "node/project/footage/footage.h"
#include "streamproperties.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/standardcombos.h"

namespace olive {

class VideoStreamProperties : public StreamProperties
{
  Q_OBJECT
public:
  VideoStreamProperties(Footage *footage, int video_index);

  virtual void Accept(MultiUndoCommand *parent) override;

  virtual bool SanityCheck() override;

private:
  Footage *footage_;

  int video_index_;

  /**
   * @brief Setting for associated/premultiplied alpha
   */
  QCheckBox* video_premultiply_alpha_;

  /**
   * @brief Setting for this media's color space
   */
  QComboBox* video_color_space_;

  /**
   * @brief Setting for this streams's color range
   */
  QComboBox *color_range_combo_;

  /**
   * @brief Setting for video interlacing
   */
  InterlacedComboBox* video_interlace_combo_;

  /**
   * @brief Sets the start index for image sequences
   */
  IntegerSlider* imgseq_start_time_;

  /**
   * @brief Sets the end index for image sequences
   */
  IntegerSlider* imgseq_end_time_;

  /**
   * @brief Sets the frame rate for image sequences
   */
  FrameRateComboBox* imgseq_frame_rate_;

  /**
   * @brief Sets the pixel aspect ratio of the stream
   */
  PixelAspectRatioComboBox* pixel_aspect_combo_;

  class VideoStreamChangeCommand : public UndoCommand {
  public:
    VideoStreamChangeCommand(Footage *footage,
                             int video_index,
                             bool premultiplied,
                             QString colorspace,
                             VideoParams::Interlacing interlacing,
                             const rational& pixel_ar,
                             VideoParams::ColorRange range);

    virtual Project* GetRelevantProject() const override;

  protected:
    virtual void redo() override;
    virtual void undo() override;

  private:
    Footage *footage_;
    int video_index_;

    bool new_premultiplied_;
    QString new_colorspace_;
    VideoParams::Interlacing new_interlacing_;
    rational new_pixel_ar_;
    VideoParams::ColorRange new_range_;

    bool old_premultiplied_;
    QString old_colorspace_;
    VideoParams::Interlacing old_interlacing_;
    rational old_pixel_ar_;
    VideoParams::ColorRange old_range_;

  };

  class ImageSequenceChangeCommand : public UndoCommand {
  public:
    ImageSequenceChangeCommand(Footage *footage,
                               int video_index,
                               int64_t start_index,
                               int64_t duration,
                               const rational& frame_rate);

    virtual Project* GetRelevantProject() const override;

  protected:
    virtual void redo() override;
    virtual void undo() override;

  private:
    Footage *footage_;
    int video_index_;

    int64_t new_start_index_;
    int64_t old_start_index_;

    int64_t new_duration_;
    int64_t old_duration_;

    rational new_frame_rate_;
    rational old_frame_rate_;

  };

};

}

#endif // VIDEOSTREAMPROPERTIES_H
