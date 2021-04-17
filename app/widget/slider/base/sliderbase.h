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

#ifndef SLIDERBASE_H
#define SLIDERBASE_H

#include <QStackedWidget>

#include "sliderlabel.h"
#include "sliderladder.h"
#include "widget/focusablelineedit/focusablelineedit.h"

namespace olive {

class SliderBase : public QStackedWidget
{
  Q_OBJECT
public:
  SliderBase(QWidget* parent = nullptr);

  void SetAlignment(Qt::Alignment alignment);

  bool IsTristate() const;
  void SetTristate();

  void SetFormat(const QString& s, const bool plural = false);
  void ClearFormat();

  bool IsFormatPlural() const;

  void SetDefaultValue(const QVariant& v);

  QString GetFormattedValueToString(const QVariant& v) const;

public slots:
  void ShowEditor();

protected slots:
  void UpdateLabel();

protected:
  const QVariant& GetValueInternal() const;

  void SetValueInternal(const QVariant& v);

  QString GetFormat() const;

  QString GetFormattedValueToString() const;

  SliderLabel* label() { return label_; }

  virtual QString ValueToString(const QVariant &v) const = 0;

  virtual QVariant StringToValue(const QString& s, bool* ok) const = 0;

  virtual QVariant AdjustValue(const QVariant& value) const;

  virtual bool CanSetValue() const;

  virtual void ValueSignalEvent(const QVariant& value) = 0;

  virtual void changeEvent(QEvent* e) override;

private:
  SliderLabel* label_;

  FocusableLineEdit* editor_;

  QVariant value_;
  QVariant default_value_;

  bool tristate_;

  QString custom_format_;

  bool format_plural_;

private slots:
  void LineEditConfirmed();

  void LineEditCancelled();

  void ResetValue();

};

}

#endif // SLIDERBASE_H
