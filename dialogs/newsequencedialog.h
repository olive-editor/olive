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

#ifndef NEWSEQUENCEDIALOG_H
#define NEWSEQUENCEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>

#include "panels/project.h"
#include "project/media.h"
#include "timeline/sequence.h"

/**
 * @brief The NewSequenceDialog class
 *
 * A dialog that creates a new (or edits an existing) Sequence object.
 */
class NewSequenceDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief NewSequenceDialog constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   *
   * @param existing
   *
   * Set this to a Sequence object (wrapped in a Media object) to edit an existing Sequence,
   * or leave as nullptr to create a new one.
   */
  explicit NewSequenceDialog(QWidget *parent = nullptr, Media* existing = nullptr);

  /**
   * @brief Set the name for the new Sequence
   *
   * If creating a new Sequence, use this function before calling exec() to set what the new Sequence's
   * name will be.
   *
   * The primary use of this is to set a unique default name (i.e. one that doesn't exist
   * in the Sequence already) which is done by Project panel. This is usually "Sequence" followed by a number.
   *
   * @param s
   *
   * The name to set the new Sequence.
   */
  void set_sequence_name(const QString& s);

private slots:
  /**
   * @brief Override accept function to create/edit a Sequence
   */
  virtual void accept() override;

  /**
   * @brief Slot when the user changes the preset
   *
   * Sets all values according to the preset chosen.
   *
   * @param index
   *
   * Currently selected index of preset_combobox;
   */
  void preset_changed(int index);

private:
  /**
   * @brief Internal reference to an existing Sequence (if one was provided to the constructor)
   */
  SequencePtr existing_sequence;

  /**
   * @brief Internal reference to an existing Media wrapper (if one was provided to the constructor)
   */
  Media* existing_item;

  /**
   * @brief Internal function to create the dialog's UI
   */
  void setup_ui();

  /**
   * @brief ComboBox to set the preset
   */
  QComboBox* preset_combobox;

  /**
   * @brief SpinBox to set the Sequence height
   */
  QSpinBox* height_numeric;

  /**
   * @brief SpinBox to set the Sequence width
   */
  QSpinBox* width_numeric;

  /**
   * @brief ComboBox to set the pixel aspect ratio
   */
  QComboBox* par_combobox;

  /**
   * @brief ComboBox to set the interlacing mode
   */
  QComboBox* interlacing_combobox;

  /**
   * @brief ComboBox to set the frame rate
   */
  QComboBox* frame_rate_combobox;

  /**
   * @brief ComboBox to set the audio frequence
   */
  QComboBox* audio_frequency_combobox;

  /**
   * @brief Line edit to set the Sequence's name
   */
  QLineEdit* sequence_name_edit;
};

#endif // NEWSEQUENCEDIALOG_H
