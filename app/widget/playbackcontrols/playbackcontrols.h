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

class PlaybackControls : public QWidget
{
public:
  PlaybackControls(QWidget* parent);

  void SetTimecodeEnabled(bool enabled);

signals:
  void BeginClicked();
  void PrevFrameClicked();
  void PlayClicked();
  void NextFrameClicked();
  void EndClicked();

private:
  QWidget* lower_left_container_;
  QWidget* lower_right_container_;

  QLabel* cur_tc_lbl_;
  QLabel* end_tc_lbl_;
};

#endif // PLAYBACKCONTROLS_H
