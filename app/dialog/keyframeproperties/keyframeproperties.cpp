#include "keyframeproperties.h"

#include <QDialogButtonBox>
#include <QGridLayout>

#include "core.h"
#include "common/timecodefunctions.h"
#include "widget/keyframeview/keyframeviewundo.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

KeyframePropertiesDialog::KeyframePropertiesDialog(const QList<NodeKeyframePtr> &keys, const rational &timebase, QWidget *parent) :
  QDialog(parent),
  keys_(keys),
  timebase_(timebase)
{
  setWindowTitle(tr("Keyframe Properties"));

  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel("Time:"), row, 0);

  time_slider_ = new TimeSlider();
  time_slider_->SetTimebase(timebase_);
  layout->addWidget(time_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel("Type:"), row, 0);

  type_select_ = new QComboBox();
  connect(type_select_, SIGNAL(currentIndexChanged(int)), this, SLOT(KeyTypeChanged(int)));
  layout->addWidget(type_select_, row, 1);

  row++;

  // Bezier handles
  bezier_group_ = new QGroupBox();

  QGridLayout* bezier_group_layout = new QGridLayout(bezier_group_);

  bezier_group_layout->addWidget(new QLabel(tr("In:")), 0, 0);

  bezier_in_x_slider_ = new FloatSlider();
  bezier_group_layout->addWidget(bezier_in_x_slider_, 0, 1);

  bezier_in_y_slider_ = new FloatSlider();
  bezier_group_layout->addWidget(bezier_in_y_slider_, 0, 2);

  bezier_group_layout->addWidget(new QLabel(tr("Out:")), 1, 0);

  bezier_out_x_slider_ = new FloatSlider();
  bezier_group_layout->addWidget(bezier_out_x_slider_, 1, 1);

  bezier_out_y_slider_ = new FloatSlider();
  bezier_group_layout->addWidget(bezier_out_y_slider_, 1, 2);

  layout->addWidget(bezier_group_, row, 0, 1, 2);

  bool all_same_time = true;
  bool can_set_time = true;

  bool all_same_type = true;

  bool all_same_bezier_in_x = true;
  bool all_same_bezier_in_y = true;
  bool all_same_bezier_out_x = true;
  bool all_same_bezier_out_y = true;

  for (int i=0;i<keys_.size();i++) {
    if (i > 0) {
      NodeKeyframePtr prev_key = keys_.at(i-1);
      NodeKeyframePtr this_key = keys_.at(i);

      // Determine if the keyframes are all the same time or not
      if (all_same_time) {
        all_same_time = (prev_key->time() == this_key->time());
      }

      // Determine if the keyframes are all the same type
      if (all_same_type) {
        all_same_type = (prev_key->type() == this_key->type());
      }

      // Check all four bezier control points
      if (all_same_bezier_in_x) {
        all_same_bezier_in_x = (prev_key->bezier_control_in().x() == this_key->bezier_control_in().x());
      }

      if (all_same_bezier_in_y) {
        all_same_bezier_in_y = (prev_key->bezier_control_in().y() == this_key->bezier_control_in().y());
      }

      if (all_same_bezier_out_x) {
        all_same_bezier_out_x = (prev_key->bezier_control_out().x() == this_key->bezier_control_out().x());
      }

      if (all_same_bezier_out_y) {
        all_same_bezier_out_y = (prev_key->bezier_control_out().y() == this_key->bezier_control_out().y());
      }
    }

    // Determine if any keyframes are on the same track (in which case we can't set the time)
    if (can_set_time) {
      for (int j=0;j<keys_.size();j++) {
        if (i != j) {
          can_set_time = (keys_.at(j)->track() == keys_.at(i)->track());
        }

        if (!can_set_time) {
          break;
        }
      }
    }

    if (!all_same_time
        && !all_same_type
        && !can_set_time
        && !all_same_bezier_in_x
        && !all_same_bezier_in_y
        && !all_same_bezier_out_x
        && !all_same_bezier_out_y) {
      break;
    }
  }

  if (all_same_time) {
    time_slider_->SetValue(Timecode::time_to_timestamp(keys_.first()->time(), timebase_));
  } else {
    time_slider_->SetTristate();
  }

  if (!can_set_time) {
    time_slider_->setEnabled(false);
  }

  if (!all_same_type) {
    // If all keyframes aren't the same type, add an empty item
    type_select_->addItem(QStringLiteral("--"), -1);

    // Ensure UI updates for the index being 0
    KeyTypeChanged(0);
  }

  type_select_->addItem(tr("Linear"), NodeKeyframe::kLinear);
  type_select_->addItem(tr("Hold"), NodeKeyframe::kHold);
  type_select_->addItem(tr("Bezier"), NodeKeyframe::kBezier);

  if (all_same_type) {
    // If all keyframes are the same type, set it here
    for (int i=0;i<type_select_->count();i++) {
      if (type_select_->itemData(i).toInt() == keys_.first()->type()) {
        type_select_->setCurrentIndex(i);

        // Ensure UI updates for this index
        KeyTypeChanged(i);
        break;
      }
    }
  }

  SetUpBezierSlider(bezier_in_x_slider_, all_same_bezier_in_x, keys_.first()->bezier_control_in().x());
  SetUpBezierSlider(bezier_in_y_slider_, all_same_bezier_in_y, keys_.first()->bezier_control_in().y());
  SetUpBezierSlider(bezier_out_x_slider_, all_same_bezier_out_x, keys_.first()->bezier_control_out().x());
  SetUpBezierSlider(bezier_out_y_slider_, all_same_bezier_out_y, keys_.first()->bezier_control_out().y());

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void KeyframePropertiesDialog::accept()
{
  QUndoCommand* command = new QUndoCommand();

  rational new_time = Timecode::timestamp_to_time(time_slider_->GetValue(), timebase_);
  int new_type = type_select_->currentData().toInt();

  foreach (NodeKeyframePtr key, keys_) {
    if (time_slider_->isEnabled() && !time_slider_->IsTristate()) {
      new NodeParamSetKeyframeTimeCommand(key, new_time, command);
    }

    if (new_type > -1) {
      new KeyframeSetTypeCommand(key, static_cast<NodeKeyframe::Type>(new_type), command);
    }

    if (bezier_group_->isEnabled()) {
      new KeyframeSetBezierControlPoint(key,
                                        NodeKeyframe::kInHandle,
                                        QPointF(bezier_in_x_slider_->GetValue(), bezier_in_y_slider_->GetValue()),
                                        command);

      new KeyframeSetBezierControlPoint(key,
                                        NodeKeyframe::kOutHandle,
                                        QPointF(bezier_out_x_slider_->GetValue(), bezier_out_y_slider_->GetValue()),
                                        command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  QDialog::accept();
}

void KeyframePropertiesDialog::SetUpBezierSlider(FloatSlider *slider, bool all_same, double value)
{
  if (all_same) {
    slider->SetValue(value);
  } else {
    slider->SetTristate();
  }
}

void KeyframePropertiesDialog::KeyTypeChanged(int index)
{
  bezier_group_->setEnabled(type_select_->itemData(index) == NodeKeyframe::kBezier);
}
