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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QPushButton>

#include "widget/flowlayout/flowlayout.h"
#include "widget/toolbar/toolbarbutton.h"
#include "tool/tool.h"

class Toolbar : public QWidget
{
  Q_OBJECT
public:
  Toolbar(QWidget* parent);

public slots:
  void SetTool(const olive::tool::Tool &tool);
  void SetSnapping(const bool &snapping);

protected:
  virtual void changeEvent(QEvent* e) override;

signals:
  void ToolChanged(const olive::tool::Tool&);
  void SnappingChanged(const bool&);

private:
  void Retranslate();

  ToolbarButton* CreateToolButton(const QIcon &icon, const olive::tool::Tool& tool);

  FlowLayout* layout_;

  QList<ToolbarButton*> toolbar_btns_;

  ToolbarButton* btn_pointer_tool_;
  ToolbarButton* btn_edit_tool_;
  ToolbarButton* btn_ripple_tool_;
  ToolbarButton* btn_razor_tool_;
  ToolbarButton* btn_slip_tool_;
  ToolbarButton* btn_slide_tool_;
  ToolbarButton* btn_hand_tool_;
  ToolbarButton* btn_transition_tool_;
  ToolbarButton* btn_zoom_tool_;
  ToolbarButton* btn_record_;
  ToolbarButton* btn_add_;

  ToolbarButton* btn_snapping_toggle_;

private slots:
  void ToolButtonClicked();
  void SnappingButtonClicked(bool b);
};

#endif // TOOLBAR_H
