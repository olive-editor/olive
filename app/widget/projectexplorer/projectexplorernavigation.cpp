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

#include "projectexplorernavigation.h"

#include <QEvent>
#include <QHBoxLayout>

#include "common/define.h"
#include "ui/icons/icons.h"

namespace olive {

ProjectExplorerNavigation::ProjectExplorerNavigation(QWidget *parent) :
  QWidget(parent)
{
  // Create widget layout
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);

  // Create "directory up" button
  dir_up_btn_ = new QPushButton(this);
  dir_up_btn_->setEnabled(false);
  dir_up_btn_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  layout->addWidget(dir_up_btn_);
  connect(dir_up_btn_, SIGNAL(clicked(bool)), this, SIGNAL(DirectoryUpClicked()));

  // Create directory tree label
  dir_lbl_ = new QLabel(this);
  dir_lbl_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  layout->addWidget(dir_lbl_);

  // Create size slider
  size_slider_ = new QSlider(this);
  size_slider_->setOrientation(Qt::Horizontal);
  size_slider_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  layout->addWidget(size_slider_);
  connect(size_slider_, SIGNAL(valueChanged(int)), this, SIGNAL(SizeChanged(int)));

  Retranslate();
  UpdateIcons();
}

void ProjectExplorerNavigation::set_text(const QString &s)
{
  dir_lbl_->setText(s);
}

void ProjectExplorerNavigation::set_dir_up_enabled(bool e)
{
  dir_up_btn_->setEnabled(e);
}

void ProjectExplorerNavigation::set_size_value(int s)
{
  size_slider_->setValue(s);
}

void ProjectExplorerNavigation::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  } else if (e->type() == QEvent::StyleChange) {
    UpdateIcons();
  }
  QWidget::changeEvent(e);
}

void ProjectExplorerNavigation::Retranslate()
{
  dir_up_btn_->setToolTip(tr("Go to parent folder"));
}

void ProjectExplorerNavigation::UpdateIcons()
{
  dir_up_btn_->setIcon(icon::DirUp);
  size_slider_->setMinimum(kProjectIconSizeMinimum);
  size_slider_->setMaximum(kProjectIconSizeMaximum);
  size_slider_->setValue(kProjectIconSizeDefault);
}

}
