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

#include "exportformatcombobox.h"

#include <QHBoxLayout>
#include <QLabel>

#include "ui/icons/icons.h"

namespace olive {

ExportFormatComboBox::ExportFormatComboBox(Mode mode, QWidget *parent) :
  QComboBox(parent)
{
  custom_menu_ = new Menu(this);

  // Populate combobox formats
  switch (mode) {
  case kShowAllFormats:
    custom_menu_->addAction(CreateHeader(icon::Video, tr("Video")));
    PopulateType(Track::kVideo);
    custom_menu_->addSeparator();

    custom_menu_->addAction(CreateHeader(icon::Audio, tr("Audio")));
    PopulateType(Track::kAudio);
    custom_menu_->addSeparator();

    custom_menu_->addAction(CreateHeader(icon::Subtitles, tr("Subtitle")));
    PopulateType(Track::kSubtitle);
    break;
  case kShowAudioOnly:
    PopulateType(Track::kAudio);
    break;
  case kShowVideoOnly:
    PopulateType(Track::kVideo);
    break;
  case kShowSubtitlesOnly:
    PopulateType(Track::kSubtitle);
    break;
  }

  connect(custom_menu_, &Menu::triggered, this, &ExportFormatComboBox::HandleIndexChange);
}

void ExportFormatComboBox::showPopup()
{
  custom_menu_->setMinimumWidth(this->width());
  custom_menu_->exec(mapToGlobal(QPoint(0, 0)));
}

void ExportFormatComboBox::SetFormat(ExportFormat::Format fmt)
{
  current_ = fmt;
  clear();
  addItem(ExportFormat::GetName(current_));
}

void ExportFormatComboBox::HandleIndexChange(QAction *a)
{
  ExportFormat::Format f = static_cast<ExportFormat::Format>(a->data().toInt());
  SetFormat(f);
  emit FormatChanged(f);
}

void ExportFormatComboBox::PopulateType(Track::Type type)
{
  for (int i=0; i<ExportFormat::kFormatCount; i++) {
    ExportFormat::Format f = static_cast<ExportFormat::Format>(i);

    if (type == Track::kVideo
        && !ExportFormat::GetVideoCodecs(f).isEmpty()) {
      // Do nothing
    } else if (type == Track::kAudio
               && ExportFormat::GetVideoCodecs(f).isEmpty()
               && !ExportFormat::GetAudioCodecs(f).isEmpty()) {
      // Do nothing
    } else if (type == Track::kSubtitle
               && ExportFormat::GetVideoCodecs(f).isEmpty()
               && ExportFormat::GetAudioCodecs(f).isEmpty()
               && !ExportFormat::GetSubtitleCodecs(f).isEmpty()) {
      // Do nothing
    } else {
      continue;
    }

    QString format_name = ExportFormat::GetName(f);

    QAction *a = custom_menu_->addAction(format_name);
    a->setData(i);
    a->setIconVisibleInMenu(false);
  }
}

QWidgetAction *ExportFormatComboBox::CreateHeader(const QIcon &icon, const QString &title)
{
  QWidgetAction *a = new QWidgetAction(this);

  QWidget *w = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(w);

  QLabel *icon_lbl = new QLabel();

  QLabel *text_lbl = new QLabel(title);
  text_lbl->setAlignment(Qt::AlignCenter);
  QFont f = text_lbl->font();
  f.setWeight(QFont::Bold);
  text_lbl->setFont(f);

  icon_lbl->setPixmap(icon.pixmap(text_lbl->sizeHint()));

  layout->addStretch();
  layout->addWidget(icon_lbl);
  layout->addWidget(text_lbl);
  layout->addStretch();

  a->setDefaultWidget(w);
  a->setEnabled(false);
  return a;
}

}
