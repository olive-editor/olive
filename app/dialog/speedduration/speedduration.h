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

#ifndef SPEEDDURATIONDIALOG_H
#define SPEEDDURATIONDIALOG_H

#include <QCheckBox>
#include <QDialog>

#include "node/block/clip/clip.h"
#include "node/output/track/track.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/timeslider.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

class SpeedDurationDialog : public QDialog
{
  Q_OBJECT
public:
  SpeedDurationDialog(const rational& timebase, const QList<ClipBlock*>& clips, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

private:
  double GetUnadjustedLengthTimestamp(ClipBlock* clip) const;

  int64_t GetAdjustedDuration(ClipBlock* clip, const double& new_speed) const;

  double GetAdjustedSpeed(ClipBlock* clip, const int64_t& new_duration) const;

  QList<ClipBlock*> clips_;

  FloatSlider* speed_slider_;
  TimeSlider* duration_slider_;

  rational timebase_;

  QCheckBox* link_speed_and_duration_;
  QCheckBox* reverse_speed_checkbox_;
  QCheckBox* maintain_audio_pitch_checkbox_;
  QCheckBox* ripple_clips_checkbox_;

private slots:
  void SpeedChanged();

  void DurationChanged();
};

class BlockReverseCommand : public UndoCommand {
public:
  BlockReverseCommand(Block* block, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;

};

OLIVE_NAMESPACE_EXIT

#endif // SPEEDDURATIONDIALOG_H
