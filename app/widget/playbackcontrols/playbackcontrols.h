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

#ifndef PLAYBACKCONTROLS_H
#define PLAYBACKCONTROLS_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

#include "common/rational.h"
#include "dragbutton.h"
#include "widget/slider/timeslider.h"

namespace olive {

/**
 * @brief A playback controls widget providing buttons for navigating media
 *
 * This widget optionally features timecode displays for the current timecode and end timecode.
 */
class PlaybackControls : public QWidget
{
  Q_OBJECT
public:
  PlaybackControls(QWidget* parent = nullptr);

  /**
   * @brief Set whether the timecodes should be shown or not
   */
  void SetTimecodeEnabled(bool enabled);

  void SetTimebase(const rational& r);

  void SetAudioVideoDragButtonsVisible(bool e);

public slots:
  void SetTime(const int64_t &r);

  void SetEndTime(const int64_t &r);

  void ShowPauseButton();

  void ShowPlayButton();

signals:
  /**
   * @brief Signal emitted when "Go to Start" is clicked
   */
  void BeginClicked();

  /**
   * @brief Signal emitted when "Previous Frame" is clicked
   */
  void PrevFrameClicked();

  /**
   * @brief Signal emitted when "Play" is clicked
   */
  void PlayClicked();

  /**
   * @brief Signal emitted when "Pause" is clicked
   */
  void PauseClicked();

  /**
   * @brief Signal emitted when "Next Frame" is clicked
   */
  void NextFrameClicked();

  /**
   * @brief Signal emitted when "Go to End" is clicked
   */
  void EndClicked();

  void AudioClicked();

  void VideoClicked();

  void AudioPressed();

  void VideoPressed();

  void TimeChanged(const int64_t& t);

protected:
  virtual void changeEvent(QEvent *) override;

private:
  void UpdateIcons();

  QWidget* lower_left_container_;
  QWidget* lower_right_container_;

  TimeSlider* cur_tc_lbl_;
  QLabel* end_tc_lbl_;

  int64_t end_time_;

  rational time_base_;

  QPushButton* go_to_start_btn_;
  QPushButton* prev_frame_btn_;
  QPushButton* play_btn_;
  QPushButton* pause_btn_;
  QPushButton* next_frame_btn_;
  QPushButton* go_to_end_btn_;
  DragButton* video_drag_btn_;
  DragButton* audio_drag_btn_;

  QStackedWidget* playpause_stack_;

private slots:
  void TimecodeChanged();

};

}

#endif // PLAYBACKCONTROLS_H
