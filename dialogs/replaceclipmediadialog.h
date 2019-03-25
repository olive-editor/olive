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

#ifndef REPLACECLIPMEDIADIALOG_H
#define REPLACECLIPMEDIADIALOG_H

#include <QDialog>
#include <QTreeView>
#include <QCheckBox>

#include "ui/sourcetable.h"
#include "project/projectelements.h"

/**
 * @brief The ReplaceClipMediaDialog class
 *
 * A dialog to replace all Clips using a certain Media with a different Media. This dialog can be run from anywhere
 * provided it's given a valid Media object.
 */
class ReplaceClipMediaDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief ReplaceClipMediaDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow or Project panel.
   *
   * @param old_media
   *
   * A valid Media object which will be used to scan the currently active Sequence for Clips using it.
   */
  ReplaceClipMediaDialog(QWidget* parent, Media* old_media);
private slots:
  /**
   * @brief Overrided accept for when the user clicks "Replace"
   *
   * Checks whether the requested replace is valid using the following criteria:
   * * Any Media is selected
   * * The selected Media is not the same Media that the user is trying to replace
   * * The Media is not a folder
   * * The Media is not the currently active Sequence
   */
  virtual void accept() override;
private:
  /**
   * @brief Internal pointer to the Media we're replacing
   */
  Media* media;

  /**
   * @brief Tree widget to show Project's media
   */
  QTreeView* tree;

  /**
   * @brief CheckBox for using the same media in points
   *
   * When the starting point of a Clip is trimmed (i.e. the Clip no longer starts at 0),
   */
  QCheckBox* use_same_media_in_points;
};

#endif // REPLACECLIPMEDIADIALOG_H
