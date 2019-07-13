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

#ifndef PLAYBACKCONTROLS_H
#define PLAYBACKCONTROLS_H

#include <QWidget>
#include <QLabel>

/**
 * @brief A playback controls widget providing buttons for navigating media
 *
 * This widget optionally features timecode displays for the current timecode and end timecode.
 */
class PlaybackControls : public QWidget
{
  Q_OBJECT
public:
  PlaybackControls(QWidget* parent);

  /**
   * @brief Set whether the timecodes should be shown or not
   */
  void SetTimecodeEnabled(bool enabled);

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
   * @brief Signal emitted when "Play/Pause" is clicked
   */
  void PlayClicked();

  /**
   * @brief Signal emitted when "Next Frame" is clicked
   */
  void NextFrameClicked();

  /**
   * @brief Signal emitted when "Go to End" is clicked
   */
  void EndClicked();

private:
  QWidget* lower_left_container_;
  QWidget* lower_right_container_;

  QLabel* cur_tc_lbl_;
  QLabel* end_tc_lbl_;
};

#endif // PLAYBACKCONTROLS_H
