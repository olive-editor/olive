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

#ifndef TIMELINE_PANEL_H
#define TIMELINE_PANEL_H

#include "panel/timebased/timebased.h"
#include "widget/timelinewidget/timelinewidget.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief Panel container for a TimelineWidget
 */
class TimelinePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  TimelinePanel(QWidget* parent);

  void Clear();

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

  virtual void Insert() override;

  virtual void Overwrite() override;

  virtual void ToggleLinks() override;

  virtual void CutSelected() override;

  virtual void CopySelected() override;

  virtual void Paste() override;

  virtual void PasteInsert() override;

  virtual void DeleteInToOut() override;

  virtual void RippleDeleteInToOut() override;

  virtual void ToggleSelectedEnabled() override;

  void InsertFootageAtPlayhead(const QList<Footage *> &footage);

  void OverwriteFootageAtPlayhead(const QList<Footage *> &footage);

protected:
  virtual void Retranslate() override;

signals:
  void SelectionChanged(const QList<Block*>& selected_blocks);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINE_PANEL_H
