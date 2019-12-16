#include "speedduration.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

#include "common/timecodefunctions.h"
#include "undo/undostack.h"
#include "widget/nodeview/nodeviewundo.h"
#include "widget/timelinewidget/undo/undo.h"

SpeedDurationDialog::SpeedDurationDialog(const rational& timebase, const QList<ClipBlock *> &clips, QWidget *parent) :
  QDialog(parent),
  clips_(clips),
  timebase_(timebase)
{
  setWindowTitle(tr("Speed/Duration"));

  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  original_speed_ = clips.first()->speed();
  original_length_ = olive::time_to_timestamp(clips.first()->length(), timebase_);

  // Check all clips after the first to see if they all share the same speed or not
  for (int i=1;i<clips.size();i++) {
    // If all the clips aren't the same speed, we'll set to NaN
    if (!qFuzzyCompare(original_speed_, clips.at(i)->speed())) {
      original_speed_ = qSNaN();
    }

    if (original_length_ != olive::time_to_timestamp(clips.at(i)->length(), timebase_)) {
      original_length_ = -1;
    }

    if (original_length_ == -1 && qIsNaN(original_speed_)) {
      break;
    }
  }

  layout->addWidget(new QLabel(tr("Speed:")), row, 0);

  speed_slider_ = new FloatSlider();

  // Convert speed to percentage value
  speed_slider_->SetValue(original_speed_ * 100.0);
  connect(speed_slider_, SIGNAL(ValueChanged(double)), this, SLOT(SpeedChanged()));
  layout->addWidget(speed_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Duration:")), row, 0);

  duration_slider_ = new TimeSlider();
  duration_slider_->SetTimebase(timebase_);
  duration_slider_->SetValue(original_length_);
  layout->addWidget(duration_slider_, row, 1);

  if (!qIsNaN(original_speed_)) {
    // Convert internally stored original length value to 1x speed for later calculations
    original_length_ = qRound(static_cast<double>(original_length_) * original_speed_);
  }

  row++;

  reverse_speed_checkbox_ = new QCheckBox(tr("Reverse Speed"));
  layout->addWidget(reverse_speed_checkbox_, row, 0, 1, 2);

  row++;

  maintain_audio_pitch_checkbox_ = new QCheckBox(tr("Maintain Audio Pitch"));
  layout->addWidget(maintain_audio_pitch_checkbox_, row, 0, 1, 2);

  row++;

  ripple_clips_checkbox_ = new QCheckBox(tr("Ripple Clips"));
  layout->addWidget(ripple_clips_checkbox_, row, 0, 1, 2);

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons, row, 0, 1, 2);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void SpeedDurationDialog::accept()
{
  // FIXME: Support multiple incompatible speeds/lengths

  QUndoCommand* command = new QUndoCommand();

  rational new_clip_length = olive::timestamp_to_time(duration_slider_->GetValue(), timebase_);
  double new_speed = speed_slider_->GetValue() * 0.01;

  foreach (ClipBlock* clip, clips_) {
    rational this_clip_new_length = new_clip_length;

    Block* next_block = clip->next();

    if (this_clip_new_length != clip->length()
        && !ripple_clips_checkbox_->isChecked()
        && next_block) {

      if (this_clip_new_length > clip->length()) {

        // Check if next clip is a gap, and if so we can take it all up
        if (next_block->type() == Block::kGap) {
          this_clip_new_length = qMin(next_block->out(), clip->in() + this_clip_new_length);

          // If we're taking up the entire clip, we'll just remove it
          if (this_clip_new_length == next_block->out()) {
            new TrackRippleRemoveBlockCommand(TrackOutput::TrackFromBlock(next_block), next_block, command);

            // Delete node and its exclusive deps
            QList<Node*> gap_and_its_deps;
            gap_and_its_deps.append(next_block);
            gap_and_its_deps.append(next_block->GetExclusiveDependencies());
            new NodeRemoveCommand(static_cast<NodeGraph*>(next_block->parent()), gap_and_its_deps, command);
          } else {
            // Otherwise we can just resize it
            new BlockResizeCommand(next_block, next_block->out() - this_clip_new_length, command);
          }

        } else {
          // Otherwise we can't extend any further
          this_clip_new_length = clip->length();
        }

      } else if (this_clip_new_length < clip->length()) {

        // If we're not rippling these clips, we'll need to insert a gap (unless the clip is already at the end)
        rational gap_length = clip->length() - this_clip_new_length;

        if (next_block->type() == Block::kGap) {
          // If we've already got a gap here, we can just resize it
          new BlockResizeCommand(next_block, next_block->length() + gap_length, command);
        } else {
          // Otherwise we have to create a new gap
          GapBlock* gap = new GapBlock();
          gap->set_length(gap_length);
          new NodeAddCommand(static_cast<NodeGraph*>(clip->parent()), gap, command);
          new TrackInsertBlockBetweenBlocksCommand(TrackOutput::TrackFromBlock(clip), gap, clip, next_block, command);
        }
      }
    }

    if (this_clip_new_length != clip->length()) {
      new BlockResizeWithoutMediaOutCommand(clip, this_clip_new_length, command);
    }

    int64_t media_length = qRound(static_cast<double>(olive::time_to_timestamp(this_clip_new_length, timebase_)) * new_speed);
    new BlockSetMediaOutCommand(clip,
                                clip->media_in() + olive::timestamp_to_time(media_length, timebase_),
                                command);
  }

  olive::undo_stack.pushIfHasChildren(command);

  QDialog::accept();
}

void SpeedDurationDialog::SpeedChanged()
{
  if (original_length_ > -1) {
    double new_speed = speed_slider_->GetValue()*0.01;

    if (qIsNull(new_speed)) {
      // 0 speed is considered a still image which could be any length and since we can't divide by zero anyway, we
      // assume the length will stay the same
      duration_slider_->SetValue(original_length_);
    } else {
      // Otherwise, calculate a length that will show the same amount of "content" with a new length
      int64_t new_length = qRound(static_cast<double>(original_length_) / new_speed);
      duration_slider_->SetValue(new_length);
    }
  }
}
