/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#ifndef COLORFIELD_H
#define COLORFIELD_H

#include "../effectfield.h"

class ColorField : public EffectField
{
  Q_OBJECT
public:
  ColorField(EffectRow* parent, const QString& id);

  QColor GetColorAt(double timecode);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

  virtual QVariant ConvertStringToValue(const QString& s) override;
  virtual QString ConvertValueToString(const QVariant& v) override;
private slots:
  void UpdateFromWidget(const QColor &c);
};

#endif // COLORFIELD_H
