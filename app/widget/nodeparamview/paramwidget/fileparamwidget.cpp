/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "fileparamwidget.h"

#include "widget/filefield/filefield.h"

namespace olive {

#define super AbstractParamWidget

FileParamWidget::FileParamWidget(QObject *parent)
  : super{parent}
{

}

void FileParamWidget::Initialize(QWidget *parent, size_t channels)
{
  for (size_t i = 0; i < channels; i++) {
    FileField* file_field = new FileField(parent);
    AddWidget(file_field);
    connect(file_field, &FileField::FilenameChanged, this, &FileParamWidget::Arbitrate);
  }
}

void FileParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    FileField* ff = static_cast<FileField*>(GetWidgets().at(i));
    ff->SetFilename(val.value<QString>(i));
  }
}

void FileParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("placeholder")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<FileField*>(GetWidgets().at(i))->SetPlaceholder(val.value<QString>(i));
    }
  } else if (key == QStringLiteral("directory")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<FileField*>(GetWidgets().at(i))->SetDirectoryMode(val.value<int64_t>(i));
    }
  } else {
    super::SetProperty(key, val);
  }
}

void FileParamWidget::Arbitrate()
{
  FileField* ff = static_cast<FileField*>(sender());

  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == ff) {
      emit ChannelValueChanged(i, ff->GetFilename());
      break;
    }
  }
}

}
