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

#ifndef COMBOFIELD_H
#define COMBOFIELD_H

#include "../effectfield.h"

struct ComboFieldItem {
  QString name;
  QVariant data;
};

class ComboField : public EffectField
{
  Q_OBJECT
public:
  ComboField(EffectRow* parent, const QString& id);

  void AddItem(const QString& text, const QVariant& data);

  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

signals:
  void DataChanged(const QVariant&);

private:
  QVector<ComboFieldItem> items_;

private slots:
  void UpdateFromWidget(int index);
};

#endif // COMBOFIELD_H
