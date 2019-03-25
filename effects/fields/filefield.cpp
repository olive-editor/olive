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

#include "filefield.h"

#include <QDebug>

#include "ui/embeddedfilechooser.h"

FileField::FileField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FILE)
{
  // Set default value to an empty string
  SetValueAt(0, "");
}

QString FileField::GetFileAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *FileField::CreateWidget(QWidget *existing)
{
  EmbeddedFileChooser* efc = (existing != nullptr) ? static_cast<EmbeddedFileChooser*>(existing) : new EmbeddedFileChooser();

  connect(efc, SIGNAL(changed(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));
  connect(this, SIGNAL(EnabledChanged(bool)), efc, SLOT(setEnabled(bool)));

  return efc;
}

void FileField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  EmbeddedFileChooser* efc = static_cast<EmbeddedFileChooser*>(widget);

  efc->blockSignals(true);
  efc->setFilename(GetFileAt(timecode));
  efc->blockSignals(false);
}

void FileField::UpdateFromWidget(const QString &s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
