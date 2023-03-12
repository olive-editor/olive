/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "markerpropertiesdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

#include "core.h"

namespace olive {

#define super QDialog

MarkerPropertiesDialog::MarkerPropertiesDialog(const std::vector<TimelineMarker *> &markers, const rational &timebase, QWidget *parent) :
  super(parent),
  markers_(markers)
{
  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  QGroupBox *time_group = new QGroupBox(tr("Time"));
  QGridLayout *time_layout = new QGridLayout(time_group);

  {
    int time_row = 0;

    time_layout->addWidget(new QLabel(tr("In:")), time_row, 0);

    in_slider_ = new RationalSlider();
    time_layout->addWidget(in_slider_, time_row, 1);

    time_row++;

    time_layout->addWidget(new QLabel(tr("Out:")), time_row, 0);

    out_slider_ = new RationalSlider();
    time_layout->addWidget(out_slider_, time_row, 1);
  }

  if (markers.size() == 1) {
    in_slider_->SetValue(markers.front()->time().in());
    in_slider_->SetDisplayType(RationalSlider::kTime);
    in_slider_->SetTimebase(timebase);
    out_slider_->SetValue(markers.front()->time().out());
    out_slider_->SetDisplayType(RationalSlider::kTime);
    out_slider_->SetTimebase(timebase);
  } else {
    // Markers cannot be on the same time, so we disable setting time if multiple markers are selected
    in_slider_->setEnabled(false);
    in_slider_->SetTristate();
    out_slider_->setEnabled(false);
    out_slider_->SetTristate();
  }

  layout->addWidget(time_group, row, 0, 1, 2);

  row++;

  layout->addWidget(new QLabel(tr("Color:")), row, 0);

  color_menu_ = new ColorCodingComboBox();
  layout->addWidget(color_menu_, row, 1);

  color_menu_->SetColor(markers.front()->color());
  for (size_t i=1; i<markers.size(); i++) {
    if (markers.at(i)->color() != color_menu_->GetSelectedColor()) {
      color_menu_->SetColor(-1);
      break;
    }
  }

  row++;

  layout->addWidget(new QLabel(tr("Name:")), row, 0);

  label_edit_ = new LineEditWithFocusSignal();
  connect(label_edit_, &LineEditWithFocusSignal::Focused, this, [this]{
    label_edit_->setPlaceholderText(QString());
  });
  layout->addWidget(label_edit_, row, 1);

  // Determine what the startup label text should be
  label_edit_->setText(markers.front()->name());
  for (size_t i=1; i<markers.size(); i++) {
    if (markers.at(i)->name() != label_edit_->text()) {
      label_edit_->clear();
      label_edit_->setPlaceholderText(tr("(multiple)"));
      break;
    }
  }

  row++;

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &MarkerPropertiesDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &MarkerPropertiesDialog::reject);
  layout->addWidget(buttons, row, 0, 1, 2);

  setWindowTitle(tr("Edit Markers"));
  label_edit_->setFocus();
}

void MarkerPropertiesDialog::accept()
{
  if (in_slider_->isEnabled() && in_slider_->GetValue() > out_slider_->GetValue()) {
    QMessageBox::critical(this, tr("Invalid Values"), tr("In point must be less than or equal to out point."));
    return;
  }

  MultiUndoCommand *command = new MultiUndoCommand();

  int color = color_menu_->GetSelectedColor();

  foreach (TimelineMarker *m, markers_) {
    if (color != -1) {
      command->add_child(new MarkerChangeColorCommand(m, color));
    }

    if (label_edit_->placeholderText().isEmpty()) {
      command->add_child(new MarkerChangeNameCommand(m, label_edit_->text()));
    }
  }

  if (markers_.size() == 1) {
    command->add_child(new MarkerChangeTimeCommand(markers_.front(), TimeRange(in_slider_->GetValue(), out_slider_->GetValue())));
  }

  Core::instance()->undo_stack()->push(command, tr("Set Marker Properties"));

  super::accept();
}

}
