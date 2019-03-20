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

#ifndef FILEFIELD_H
#define FILEFIELD_H

#include "../effectfield.h"

class FileField : public EffectField
{
  Q_OBJECT
public:
  FileField(EffectRow* parent, const QString& id);

  QString GetFileAt(double timecode);

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
private slots:
  void UpdateFromWidget(const QString &s);
};

#endif // FILEFIELD_H
