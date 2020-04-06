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

#ifndef PROJECTEXPLORERLISTVIEWTOOLBAR_H
#define PROJECTEXPLORERLISTVIEWTOOLBAR_H

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A navigation bar widget for ProjectExplorer's Icon and List views
 *
 * Unlike the Tree view, Icon and List don't follow a hierarchical view of information. This means there is no direct
 * way of navigating in and out of folders in those view types. We solve this in two ways:
 *
 * * Double clicking a Folder in those views will enter that folder
 * * This navigation bar offers a "directory up" button for leaving a folder
 *
 * This navbar also provides an icon size slider for those views (between kProjectIconSizeMinimum and
 * kProjectIconSizeMaximum) as well as text that's intended to be set to the current Folder's name (or
 * empty for the root folder).
 *
 * This widget does not actually communicate to Project or ProjectExplorer classes. It is simply UI widgets that are
 * intended to be connected in ways that do. This is the primarily responsibility of ProjectExplorer.
 *
 * By default, the directory up button is disabled (assuming root folder), the text is empty, and the icon size slider
 * is set to kProjectIconSizeDefault.
 */
class ProjectExplorerNavigation : public QWidget
{
  Q_OBJECT
public:
  ProjectExplorerNavigation(QWidget* parent);

  /**
   * @brief Sets the text string
   *
   * This text is intended to be set to the current Folder's name
   *
   * @param s
   */
  void set_text(const QString& s);

  /**
   * @brief Set whether the "directory up" button is enabled or not
   *
   * @param e
   */
  void set_dir_up_enabled(bool e);

  /**
   * @brief Set the current value of the size slider
   *
   * NOTE: Does NOT emit SizeChanged().
   *
   * @param s
   *
   * New size value to set to
   */
  void set_size_value(int s);

signals:
  /**
   * @brief Signal emitted when the directory up button is clicked
   */
  void DirectoryUpClicked();

  /**
   * @brief Signal emitted when the icon size slider changes value
   *
   * @param size
   *
   * New size set in the slider
   */
  void SizeChanged(int size);

protected:
  virtual void changeEvent(QEvent *) override;

private:
  void Retranslate();

  void UpdateIcons();

  QPushButton* dir_up_btn_;

  QLabel* dir_lbl_;

  QSlider* size_slider_;
};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTEXPLORERLISTVIEWTOOLBAR_H
