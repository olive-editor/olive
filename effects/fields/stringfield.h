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

#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "../effectfield.h"

class StringField : public EffectField
{
  Q_OBJECT
public:
  StringField(EffectRow* parent, const QString& id, bool rich_text = true);

  QString GetStringAt(double timecode);

  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;
private slots:
  void UpdateFromWidget(const QString& b);
private:
  bool rich_text_;
};

#endif // STRINGFIELD_H
