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

#ifndef SLIDERBASE_H
#define SLIDERBASE_H

#include <QStackedWidget>

#include "node/value.h"
#include "sliderlabel.h"
#include "sliderladder.h"
#include "widget/focusablelineedit/focusablelineedit.h"

namespace olive {

class SliderBase : public QStackedWidget
{
  Q_OBJECT
public:
  using InternalType = value_t; //QVariant;

  SliderBase(QWidget* parent = nullptr);

  void SetAlignment(Qt::Alignment alignment);

  bool IsTristate() const;
  void SetTristate();

  void SetFormat(const QString& s, const bool plural = false);
  void ClearFormat();

  bool IsFormatPlural() const;

  void SetDefaultValue(const InternalType& v);

  QString GetFormattedValueToString(const InternalType& v) const;

  void InsertLabelSubstitution(const InternalType &value, const QString &label)
  {
    label_substitutions_.append({value, label});
    UpdateLabel();
  }

  void SetColor(const QColor &c)
  {
    label_->SetColor(c);
  }

  const InternalType& GetValueInternal() const;

public slots:
  void ShowEditor();

protected slots:
  void UpdateLabel();

protected:
  void SetValueInternal(const InternalType& v);

  QString GetFormat() const;

  QString GetFormattedValueToString() const;

  SliderLabel* label() { return label_; }

  virtual QString ValueToString(const InternalType &v) const = 0;

  virtual InternalType StringToValue(const QString& s, bool* ok) const = 0;

  virtual InternalType AdjustValue(const InternalType& value) const;

  virtual bool CanSetValue() const;

  virtual void ValueSignalEvent(const InternalType& value) = 0;

  virtual void changeEvent(QEvent* e) override;

  virtual bool Equals(const InternalType &a, const InternalType &b) const = 0;

private:
  bool GetLabelSubstitution(const InternalType &v, QString *out) const;

  SliderLabel* label_;

  FocusableLineEdit* editor_;

  InternalType value_;
  InternalType default_value_;

  bool tristate_;

  QString custom_format_;

  bool format_plural_;

  QVector<QPair<InternalType, QString> > label_substitutions_;

private slots:
  void LineEditConfirmed();

  void LineEditCancelled();

  void ResetValue();

};

}

#endif // SLIDERBASE_H
