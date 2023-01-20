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

#include "colorspacecombobox.h"


#include <QVBoxLayout>
#include <QMap>
#include <QMenu>

#include "ui/colorcoding.h"

namespace olive {

ColorSpaceComboBox::ColorSpaceComboBox(ColorManager* color_manager, QString categories, bool use_family, QWidget* parent) :
  QComboBox(parent),
  color_manager_(color_manager),
  categories_(categories),
  use_family_(use_family)
{
  auto space = QString(" ");
  placeholder_ = new QLabel(space + "Select Color Space");
  this->setLayout(new QVBoxLayout());
  this->layout()->setContentsMargins(0, 0, 0, 0);
  this->layout()->addWidget(placeholder_);

  setCurrentIndex(-1);
}

void ColorSpaceComboBox::showPopup()
{
  QMenu menu(this);
  menu.setMinimumWidth(width());
  
  OCIO::ConstConfigRcPtr config = color_manager_->GetConfig();

  OCIO::ColorSpaceMenuHelperRcPtr menu_helper = color_manager_->CreateMenuHelper(config, categories_);

  QMap<QString, QMenu*> submenu_map;

  for (size_t i = 0; i < menu_helper->getNumColorSpaces(); i++) {
    QString colorspace = menu_helper->getName(i);
    QString family = menu_helper->getFamily(i);

    QAction* item = new QAction(colorspace);
    item->setData(colorspace);
    item->setToolTip(menu_helper->getDescription(i));

    if (!family.isEmpty()) {
      QString submenu_title = family.split(config->getFamilySeparator()).last();
      if (!submenu_map.keys().contains(submenu_title)) {
        QMenu* submenu = new QMenu(submenu_title);
        submenu_map.insert(submenu_title, submenu);
        submenu->addAction(item);
        menu.addMenu(submenu);
      } else {
        submenu_map.value(submenu_title)->addAction(item);
      }

    } else {
      menu.addAction(item);
    } 
  }

  QAction* a = menu.exec(parentWidget()->mapToGlobal(pos()));

  if (a) {
    setColorSpacePlaceHolder(a->data().toString());
    emit currentTextChanged(a->data().toString());
  }
}

}
