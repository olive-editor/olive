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

#include "sequence.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "core.h"
#include "common/channellayout.h"
#include "common/rational.h"
#include "undo/undostack.h"

OLIVE_NAMESPACE_ENTER

SequenceDialog::SequenceDialog(Sequence* s, Type t, QWidget* parent) :
  QDialog(parent),
  sequence_(s),
  make_undoable_(true)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QSplitter* splitter = new QSplitter();
  layout->addWidget(splitter);

  preset_tab_ = new SequenceDialogPresetTab();
  splitter->addWidget(preset_tab_);

  parameter_tab_ = new SequenceDialogParameterTab(sequence_);
  splitter->addWidget(parameter_tab_);

  connect(preset_tab_, &SequenceDialogPresetTab::PresetChanged,
          parameter_tab_, &SequenceDialogParameterTab::PresetChanged);
  connect(preset_tab_, &SequenceDialogPresetTab::PresetAccepted,
          this, &SequenceDialog::accept);
  connect(parameter_tab_, &SequenceDialogParameterTab::SaveParametersAsPreset,
          preset_tab_, &SequenceDialogPresetTab::SaveParametersAsPreset);

  // Set up name section
  QHBoxLayout* name_layout = new QHBoxLayout();
  name_layout->addWidget(new QLabel(tr("Name:")));
  name_field_ = new QLineEdit();
  name_layout->addWidget(name_field_);
  layout->addLayout(name_layout);

  // Set up dialog buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  connect(buttons, &QDialogButtonBox::accepted, this, &SequenceDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &SequenceDialog::reject);
  layout->addWidget(buttons);

  // Set window title based on type
  switch (t) {
  case kNew:
    setWindowTitle(tr("New Sequence"));
    break;
  case kExisting:
    setWindowTitle(tr("Editing \"%1\"").arg(sequence_->name()));
    break;
  }

  name_field_->setText(sequence_->name());
}

void SequenceDialog::SetUndoable(bool u)
{
  make_undoable_ = u;
}

void SequenceDialog::SetNameIsEditable(bool e)
{
  name_field_->setEnabled(e);
}

void SequenceDialog::accept()
{
  if (name_field_->isEnabled() && name_field_->text().isEmpty()) {
    QMessageBox::critical(this, tr("Error editing Sequence"), tr("Please enter a name for this Sequence."));
    return;
  }

  // Generate video and audio parameter structs from data
  VideoParams video_params = VideoParams(parameter_tab_->GetSelectedVideoWidth(),
                                         parameter_tab_->GetSelectedVideoHeight(),
                                         parameter_tab_->GetSelectedVideoFrameRate().flipped(),
                                         parameter_tab_->GetSelectedPreviewFormat(),
                                         parameter_tab_->GetSelectedVideoPixelAspect(),
                                         parameter_tab_->GetSelectedVideoInterlacingMode(),
                                         parameter_tab_->GetSelectedPreviewResolution());

  AudioParams audio_params = AudioParams(parameter_tab_->GetSelectedAudioSampleRate(),
                                         parameter_tab_->GetSelectedAudioChannelLayout(),
                                         SampleFormat::kInternalFormat);

  if (make_undoable_) {

    // Make undoable command to change the parameters
    SequenceParamCommand* param_command = new SequenceParamCommand(sequence_,
                                                                   video_params,
                                                                   audio_params,
                                                                   name_field_->text());

    Core::instance()->undo_stack()->push(param_command);

  } else {
    // Set sequence values directly with no undo command
    sequence_->set_video_params(video_params);
    sequence_->set_audio_params(audio_params);
    sequence_->set_name(name_field_->text());
  }

  QDialog::accept();
}

SequenceDialog::SequenceParamCommand::SequenceParamCommand(Sequence* s,
                                                           const VideoParams& video_params,
                                                           const AudioParams &audio_params,
                                                           const QString& name,
                                                           QUndoCommand* parent) :
  UndoCommand(parent),
  sequence_(s),
  new_video_params_(video_params),
  new_audio_params_(audio_params),
  new_name_(name),
  old_video_params_(s->video_params()),
  old_audio_params_(s->audio_params()),
  old_name_(s->name())
{
}

Project *SequenceDialog::SequenceParamCommand::GetRelevantProject() const
{
  return sequence_->project();
}

void SequenceDialog::SequenceParamCommand::redo_internal()
{
  sequence_->set_video_params(new_video_params_);
  sequence_->set_audio_params(new_audio_params_);
  sequence_->set_name(new_name_);
}

void SequenceDialog::SequenceParamCommand::undo_internal()
{
  sequence_->set_video_params(old_video_params_);
  sequence_->set_audio_params(old_audio_params_);
  sequence_->set_name(old_name_);
}

OLIVE_NAMESPACE_EXIT
