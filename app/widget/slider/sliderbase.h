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

#ifndef SLIDERBASE_H
#define SLIDERBASE_H

#include <QStackedWidget>

#include "sliderlabel.h"
#include "sliderlineedit.h"

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

signals:
  void ValueChanged(QVariant v);

protected:
  const QVariant& Value();

  void SetValue(const QVariant& v);

  void SetMinimumInternal(const QVariant& v);

  void SetMaximumInternal(const QVariant& v);

  void UpdateLabel(const QVariant& v);

  virtual QString ValueToString(const QVariant &v);

  virtual QVariant StringToValue(const QString& s, bool* ok);

  virtual void changeEvent(QEvent* e) override;

  int decimal_places_;

  double drag_multiplier_;

private:
  const QVariant& ClampValue(const QVariant& v);

  SliderLabel* label_;

  SliderLineEdit* editor_;

  QVariant value_;

  bool has_min_;
  QVariant min_value_;

  bool has_max_;
  QVariant max_value_;

  Mode mode_;

  bool dragged_;

  double dragged_diff_;

  QVariant temp_dragged_value_;

  bool require_valid_input_;

private slots:
  void LabelPressed();

  void LabelClicked();

  void LabelDragged(int i);

  void LineEditConfirmed();

  void LineEditCancelled();
};

#endif // SLIDERBASE_H
