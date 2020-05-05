/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef PREFERENCESAPPEARANCETAB_H
#define PREFERENCESAPPEARANCETAB_H

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

#include "preferencestab.h"
#include "ui/style/style.h"

OLIVE_NAMESPACE_ENTER

class PreferencesAppearanceTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesAppearanceTab();

  virtual void Accept() override;

private:
  /**
   * @brief Show a file dialog to browse for an external CSS file to load for styling the application.
   */
  void BrowseForCSS();

  void UpdateButtonColor(int index);

  /**
   * @brief UI widget for selecting the current UI style
   */
  QComboBox* style_;

  /**
   * @brief List of internal styles
   */
  QList<StyleDescriptor> style_list_;

  QString custom_style_path_;

  QList<QColor> colors_;

  QList<QPushButton*> color_btns_;

private slots:
  void ColorButtonClicked();

};

OLIVE_NAMESPACE_EXIT

#endif // PREFERENCESAPPEARANCETAB_H
