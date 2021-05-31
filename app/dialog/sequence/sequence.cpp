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

#include "sequence.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include "config/config.h"
#include "core.h"
#include "common/channellayout.h"
#include "common/qtutils.h"
#include "common/rational.h"
#include "undo/undostack.h"

namespace olive {

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
  QPushButton *default_btn = buttons->addButton(tr("Set As Default"), QDialogButtonBox::ActionRole);
  connect(buttons, &QDialogButtonBox::accepted, this, &SequenceDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &SequenceDialog::reject);
  connect(default_btn, &QPushButton::clicked, this, &SequenceDialog::SetAsDefaultClicked);
  layout->addWidget(buttons);

  // Set window title based on type
  switch (t) {
  case kNew:
    setWindowTitle(tr("New Sequence"));
    break;
  case kExisting:
    setWindowTitle(tr("Editing \"%1\"").arg(sequence_->GetLabel()));
    break;
  }

  name_field_->setText(sequence_->GetLabel());
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
    QtUtils::MessageBox(this, QMessageBox::Critical, tr("Error editing Sequence"), tr("Please enter a name for this Sequence."));
    return;
  }

  // Generate video and audio parameter structs from data
  VideoParams video_params = VideoParams(parameter_tab_->GetSelectedVideoWidth(),
                                         parameter_tab_->GetSelectedVideoHeight(),
                                         parameter_tab_->GetSelectedVideoFrameRate().flipped(),
                                         parameter_tab_->GetSelectedPreviewFormat(),
                                         VideoParams::kInternalChannelCount,
                                         parameter_tab_->GetSelectedVideoPixelAspect(),
                                         parameter_tab_->GetSelectedVideoInterlacingMode(),
                                         parameter_tab_->GetSelectedPreviewResolution());

  AudioParams audio_params = AudioParams(parameter_tab_->GetSelectedAudioSampleRate(),
                                         parameter_tab_->GetSelectedAudioChannelLayout(),
                                         AudioParams::kInternalFormat);

  if (make_undoable_) {

    // Make undoable command to change the parameters
    SequenceParamCommand* param_command = new SequenceParamCommand(sequence_,
                                                                   video_params,
                                                                   audio_params,
                                                                   name_field_->text());

    Core::instance()->undo_stack()->push(param_command);

  } else {
    // Set sequence values directly with no undo command
    sequence_->SetVideoParams(video_params);
    sequence_->SetAudioParams(audio_params);
    sequence_->SetLabel(name_field_->text());
  }

  QDialog::accept();
}

void SequenceDialog::SetAsDefaultClicked()
{
  if (QtUtils::MessageBox(this, QMessageBox::Question, tr("Confirm Set As Default"),
                          tr("Are you sure you want to set the current parameters as defaults?"),
                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    // Maybe replace with Preset system
    Config::Current()[QStringLiteral("DefaultSequenceWidth")] = parameter_tab_->GetSelectedVideoWidth();
    Config::Current()[QStringLiteral("DefaultSequenceHeight")] = parameter_tab_->GetSelectedVideoHeight();
    Config::Current()[QStringLiteral("DefaultSequencePixelAspect")] = QVariant::fromValue(parameter_tab_->GetSelectedVideoPixelAspect());
    Config::Current()[QStringLiteral("DefaultSequenceFrameRate")] = QVariant::fromValue(parameter_tab_->GetSelectedVideoFrameRate().flipped());
    Config::Current()[QStringLiteral("DefaultSequenceInterlacing")] = parameter_tab_->GetSelectedVideoInterlacingMode();
    Config::Current()[QStringLiteral("DefaultSequenceAudioFrequency")] = parameter_tab_->GetSelectedAudioSampleRate();
    Config::Current()[QStringLiteral("DefaultSequenceAudioLayout")] = QVariant::fromValue(parameter_tab_->GetSelectedAudioChannelLayout());
  }
}

SequenceDialog::SequenceParamCommand::SequenceParamCommand(Sequence* s,
                                                           const VideoParams& video_params,
                                                           const AudioParams &audio_params,
                                                           const QString& name) :
  sequence_(s),
  new_video_params_(video_params),
  new_audio_params_(audio_params),
  new_name_(name),
  old_video_params_(s->GetVideoParams()),
  old_audio_params_(s->GetAudioParams()),
  old_name_(s->GetLabel())
{
}

Project *SequenceDialog::SequenceParamCommand::GetRelevantProject() const
{
  return sequence_->project();
}

void SequenceDialog::SequenceParamCommand::redo()
{
  sequence_->SetVideoParams(new_video_params_);
  sequence_->SetAudioParams(new_audio_params_);
  sequence_->SetLabel(new_name_);
}

void SequenceDialog::SequenceParamCommand::undo()
{
  sequence_->SetVideoParams(old_video_params_);
  sequence_->SetAudioParams(old_audio_params_);
  sequence_->SetLabel(old_name_);
}

}
