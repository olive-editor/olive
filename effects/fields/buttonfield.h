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

#ifndef BUTTONFIELD_H
#define BUTTONFIELD_H

#include "../effectfield.h"

class ButtonField : public EffectField
{
  Q_OBJECT
public:
  ButtonField(EffectRow* parent, const QString& string);

  void SetCheckable(bool c);
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;

public slots:
  void SetChecked(bool c);

signals:
  void CheckedChanged(bool);
  void Toggled(bool);

private:
  bool checkable_;
  bool checked_;

  QString button_text_;
};

#endif // BUTTONFIELD_H
