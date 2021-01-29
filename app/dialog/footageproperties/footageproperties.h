/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>

#include "project/item/footage/footage.h"
#include "undo/undocommand.h"

namespace olive {

/**
 * @brief The MediaPropertiesDialog class
 *
 * A dialog for setting properties on Media. This can be loaded from any part of the application provided it's given
 * a valid Media object.
 */
class FootagePropertiesDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief MediaPropertiesDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow or Project panel.
   *
   * @param i
   *
   * Media object to set properties for.
   */
  FootagePropertiesDialog(QWidget *parent, Footage* footage);
private:
  class FootageChangeCommand : public UndoCommand {
  public:
    FootageChangeCommand(Footage* footage,
                         const QString& name);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;
    virtual void undo() override;

  private:
    Footage* footage_;

    QString new_name_;
    QString old_name_;
  };

  class StreamEnableChangeCommand : public UndoCommand {
  public:
    StreamEnableChangeCommand(Stream* stream,
                              bool enabled);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;
    virtual void undo() override;

  private:
    Stream* stream_;

    bool old_enabled_;
    bool new_enabled_;
  };

  /**
   * @brief Stack of widgets that changes based on whether the stream is a video or audio stream
   */
  QStackedWidget* stacked_widget_;

  /**
   * @brief ComboBox for interlacing setting
   */
  QComboBox* interlacing_box;

  /**
   * @brief Media name text field
   */
  QLineEdit* footage_name_field_;

  /**
   * @brief Internal pointer to Media object (set in constructor)
   */
  Footage* footage_;

  /**
   * @brief A list widget for listing the tracks in Media
   */
  QListWidget* track_list;

  /**
   * @brief Frame rate to conform to
   */
  QDoubleSpinBox* conform_fr;

private slots:
  /**
   * @brief Overridden accept function for saving the properties back to the Media class
   */
  void accept();

};

}

#endif // MEDIAPROPERTIESDIALOG_H
