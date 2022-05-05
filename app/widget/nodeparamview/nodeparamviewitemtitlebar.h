/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef NODEPARAMVIEWITEMTITLEBAR_H
#define NODEPARAMVIEWITEMTITLEBAR_H

#include <QCheckBox>
#include <QLabel>
#include <QWidget>

#include "widget/collapsebutton/collapsebutton.h"

namespace olive {

class NodeParamViewItemTitleBar : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewItemTitleBar(QWidget* parent = nullptr);

  bool IsExpanded() const
  {
    return collapse_btn_->isChecked();
  }

public slots:
  void SetExpanded(bool e);

  void SetText(const QString& s)
  {
    lbl_->setText(s);
    lbl_->setToolTip(s);
    lbl_->setMinimumWidth(1);
  }

  void SetPinButtonVisible(bool e)
  {
    pin_btn_->setVisible(e);
  }

  void SetAddEffectButtonVisible(bool e)
  {
    add_fx_btn_->setVisible(e);
  }

  void SetEnabledCheckBoxVisible(bool e)
  {
    enabled_checkbox_->setVisible(e);
  }

  void SetEnabledCheckBoxChecked(bool e)
  {
    enabled_checkbox_->setChecked(e);
  }

signals:
  void ExpandedStateChanged(bool e);

  void PinToggled(bool e);

  void AddEffectButtonClicked();

  void EnabledCheckBoxClicked(bool e);

  void Clicked();

protected:
  virtual void paintEvent(QPaintEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  bool draw_border_;

  QLabel* lbl_;

  CollapseButton* collapse_btn_;

  QPushButton *pin_btn_;

  QPushButton *add_fx_btn_;

  QCheckBox *enabled_checkbox_;

};

}

#endif // NODEPARAMVIEWITEMTITLEBAR_H
