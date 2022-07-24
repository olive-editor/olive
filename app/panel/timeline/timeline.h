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

#ifndef TIMELINE_PANEL_H
#define TIMELINE_PANEL_H

#include "panel/timebased/timebased.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

/**
 * @brief Panel container for a TimelineWidget
 */
class TimelinePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  TimelinePanel(QWidget* parent);

  inline TimelineWidget *timeline_widget() const
  {
    return static_cast<TimelineWidget*>(GetTimeBasedWidget());
  }

  void SplitAtPlayhead();

  QByteArray SaveSplitterState() const;

  void RestoreSplitterState(const QByteArray& state);

  virtual void SelectAll() override;

  virtual void DeselectAll() override;

  virtual void RippleToIn() override;

  virtual void RippleToOut() override;

  virtual void EditToIn() override;

  virtual void EditToOut() override;

  virtual void DeleteSelected() override;

  virtual void RippleDelete() override;

  virtual void IncreaseTrackHeight() override;

  virtual void DecreaseTrackHeight() override;

  virtual void ToggleLinks() override;

  virtual void PasteInsert() override;

  virtual void DeleteInToOut() override;

  virtual void RippleDeleteInToOut() override;

  virtual void ToggleSelectedEnabled() override;

  virtual void SetColorLabel(int index) override;

  virtual void NudgeLeft() override;

  virtual void NudgeRight() override;

  virtual void MoveInToPlayhead() override;

  virtual void MoveOutToPlayhead() override;

  virtual void RenameSelected() override;

  void AddDefaultTransitionsToSelected()
  {
    timeline_widget()->AddDefaultTransitionsToSelected();
  }

  void ShowSpeedDurationDialogForSelectedClips()
  {
    timeline_widget()->ShowSpeedDurationDialogForSelectedClips();
  }

  void InsertFootageAtPlayhead(const QVector<ViewerOutput *> &footage);

  void OverwriteFootageAtPlayhead(const QVector<ViewerOutput *> &footage);

  const QVector<Block*>& GetSelectedBlocks() const
  {
    return timeline_widget()->GetSelectedBlocks();
  }

protected:
  virtual void Retranslate() override;

signals:
  void BlockSelectionChanged(const QVector<Block*>& selected_blocks);

  void RequestCaptureStart(const TimeRange &time, const Track::Reference &track);

  void RevealViewerInProject(ViewerOutput *r);
  void RevealViewerInFootageViewer(ViewerOutput *r, const TimeRange &range);

};

}

#endif // TIMELINE_PANEL_H
