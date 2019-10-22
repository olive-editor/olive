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

#ifndef SEQUENCEDIALOG_H
#define SEQUENCEDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QSpinBox>
#include <QUndoCommand>

#include "project/item/sequence/sequence.h"

/**
 * @brief A dialog for editing Sequence parameters
 *
 * This dialog exposes all the parameters of a Sequence to users allowing them to set up a Sequence however they wish.
 * A Sequence can be sent to this dialog through the constructor. All fields will be filled using that Sequence's
 * parameters, allowing the user to view and edit them. Accepting the dialog will apply them back to that Sequence,
 * either directly or using a QUndoCommand (see SetUndoable()).
 *
 * If creating a new Sequence, the Sequence must still be constructed first before sending it to SequenceDialog.
 * SequenceDialog does not create any new objects. In most cases when creating a new Sequence, editing its parameters
 * with SequenceDialog will be paired with the action of adding the Sequence to a project. In this situation, since the
 * latter will be the main undoable action, the parameter editing doesn't have to be undoable since to the user they'll
 * be viewed as one single action (see SetUndoable()).
 */
class SequenceDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief Used to set the dialog mode of operation (see SequenceDialog())
   */
  enum Type {
    kNew,
    kExisting
  };

  /**
   * @brief SequenceDialog Constructor
   *
   * @param s
   * Sequence to edit
   *
   * @param t
   * Mode of operation (changes some UI like the window title to best represent the action being performed)
   *
   * @param parent
   * QWidget parent
   */
  SequenceDialog(Sequence* s, Type t = kExisting, QWidget* parent = nullptr);

  /**
   * @brief Set whether the parameter changes should be made into an undo command or not
   *
   * @param u
   */
  void SetUndoable(bool u);

public slots:
  /**
   * @brief Function called when the user presses OK
   */
  virtual void accept() override;

private:
  /**
   * @brief Internal function for adding a selectable frame rate
   */
  void AddFrameRate(const rational& r);

  /**
   * @brief Internal function for adding a selectable sample rate
   */
  void AddSampleRate(const int &rate);

  /**
   * @brief Internal function for adding a selectable channel layout
   */
  void AddChannelLayout(int layout);

  Sequence* sequence_;

  bool make_undoable_;

  QSpinBox* video_width_field_;

  QSpinBox* video_height_field_;

  QComboBox* video_frame_rate_field_;

  QComboBox* audio_sample_rate_field_;

  QComboBox* audio_channels_field_;

  QLineEdit* name_field_;

  QVector<rational> frame_rate_list_;

  QVector<int> sample_rate_list_;

  /**
   * @brief A QUndoCommand for setting the parameters on a sequence
   */
  class SequenceParamCommand : public QUndoCommand {
  public:
    SequenceParamCommand(Sequence* s,
                         const VideoParams& video_params,
                         const AudioParams& audio_params,
                         const QString& name,
                         QUndoCommand* parent = nullptr);

    virtual void redo() override;
    virtual void undo() override;
  private:
    Sequence* sequence_;

    VideoParams new_video_params_;
    AudioParams new_audio_params_;
    QString new_name_;

    VideoParams old_video_params_;
    AudioParams old_audio_params_;
    QString old_name_;
  };
};

#endif // SEQUENCEDIALOG_H
