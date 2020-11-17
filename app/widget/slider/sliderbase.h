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

#ifndef SLIDERBASE_H
#define SLIDERBASE_H

#include <QStackedWidget>

#include "sliderlabel.h"
#include "sliderladder.h"
#include "widget/focusablelineedit/focusablelineedit.h"

OLIVE_NAMESPACE_ENTER
class SliderBase : public QStackedWidget
{
  Q_OBJECT
public:
  enum Mode {
    kString,
    kInteger,
    kFloat
  };

  SliderBase(Mode mode, QWidget* parent = nullptr);

  void SetDragMultiplier(const double& d);

  void SetRequireValidInput(bool e);

  void SetAlignment(Qt::Alignment alignment);

  void SetDefaultValue(const QVariant& v);

  bool IsTristate() const;
  void SetTristate();

  bool IsDragging() const;

  void SetFormat(const QString& s);
  void ClearFormat();

  void SetLadderElementCount(int b)
  {
    ladder_element_count_ = b;
  }

signals:
  void ValueChanged(QVariant v);

protected:
  const QVariant& Value() const;

  void SetValue(const QVariant& v);

  void SetMinimumInternal(const QVariant& v);

  void SetMaximumInternal(const QVariant& v);

  void UpdateLabel(const QVariant& v);

  virtual double AdjustDragDistanceInternal(const double& start, const double& drag);

  virtual QString ValueToString(const QVariant &v);

  virtual QVariant StringToValue(const QString& s, bool* ok);

  virtual void changeEvent(QEvent* e) override;

  void ForceLabelUpdate();

  double drag_multiplier_;

private:
  const QVariant& ClampValue(const QVariant& v);

  QString GetFormat() const;

  void RepositionLadder();

  SliderLabel* label_;

  FocusableLineEdit* editor_;

  QVariant value_;
  QVariant default_value_;

  bool has_min_;
  QVariant min_value_;

  bool has_max_;
  QVariant max_value_;

  Mode mode_;

  double dragged_diff_;

  QVariant temp_dragged_value_;

  bool require_valid_input_;

  bool tristate_;

  QString custom_format_;

  SliderLadder* drag_ladder_;

  int ladder_element_count_;

  bool dragged_;

private slots:
  void ShowEditor();

  void LabelPressed();

  void LadderDragged(int value, double multiplier);

  void LadderReleased();

  void LineEditConfirmed();

  void LineEditCancelled();

  void ResetValue();
};

OLIVE_NAMESPACE_EXIT

#endif // SLIDERBASE_H
