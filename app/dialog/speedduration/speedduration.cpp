#include "speedduration.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

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

  QVBoxLayout* layout = new QVBoxLayout(this);

  {
    // Create groupbox for the speed/duration
    QGroupBox* speed_groupbox = new QGroupBox(tr("Speed/Duration"));
    layout->addWidget(speed_groupbox);
    QGridLayout* speed_layout = new QGridLayout(speed_groupbox);

    int row = 0;

    // For any other clips that are selected, determine if they share speeds and lengths. If they don't, the UI can't
    // show them all as having the same parameters
    bool same_speed = true;
    bool same_duration = true;

    for (int i=1;i<clips.size();i++) {
      ClipBlock* prev_clip = clips_.at(i-1);
      ClipBlock* this_clip = clips_.at(i);

      // Check if the speeds are different
      if (same_speed
          && !qFuzzyCompare(prev_clip->speed(), this_clip->speed())) {
        same_speed = false;
      }

      // Check if the durations are different
      if (same_duration
          && prev_clip->length() != this_clip->length()) {
        same_duration = false;
      }

      // If we've already determined both are different, no need to continue
      if (!same_speed && !same_duration) {
        break;
      }
    }

    speed_layout->addWidget(new QLabel(tr("Speed:")), row, 0);

    // Create "Speed" slider
    speed_slider_ = new FloatSlider();
    speed_slider_->SetMinimum(0);
    speed_slider_->SetDisplayType(FloatSlider::kPercentage);
    speed_layout->addWidget(speed_slider_, row, 1);

    if (same_speed) {
      // All clips share the same speed so we can show the value
      speed_slider_->SetValue(clips_.first()->speed());
    } else {
      // Else, we show an invalid initial state
      speed_slider_->SetTristate();
    }

    row++;

    speed_layout->addWidget(new QLabel(tr("Duration:")), row, 0);

    // Create "Duration" slider
    duration_slider_ = new TimeSlider();
    duration_slider_->SetTimebase(timebase_);
    duration_slider_->SetMinimum(1);
    speed_layout->addWidget(duration_slider_, row, 1);

    if (same_duration) {
      duration_slider_->SetValue(olive::time_to_timestamp(clips_.first()->length(), timebase_));
    } else {
      duration_slider_->SetTristate();
    }

    row++;

    link_speed_and_duration_ = new QCheckBox(tr("Link Speed and Duration"));
    link_speed_and_duration_->setChecked(true);
    speed_layout->addWidget(link_speed_and_duration_, row, 0, 1, 2);

    // Pick up when the speed or duration slider changes so we can programmatically link them
    connect(speed_slider_, SIGNAL(ValueChanged(double)), this, SLOT(SpeedChanged()));
    connect(duration_slider_, SIGNAL(ValueChanged(int64_t)), this, SLOT(DurationChanged()));
  }

  reverse_speed_checkbox_ = new QCheckBox(tr("Reverse Speed"));
  layout->addWidget(reverse_speed_checkbox_);

  maintain_audio_pitch_checkbox_ = new QCheckBox(tr("Maintain Audio Pitch"));
  layout->addWidget(maintain_audio_pitch_checkbox_);

  ripple_clips_checkbox_ = new QCheckBox(tr("Ripple Clips"));
  layout->addWidget(ripple_clips_checkbox_);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void SpeedDurationDialog::accept()
{
  if (duration_slider_->IsTristate() && speed_slider_->IsTristate()) {
    // Nothing to be done
    QDialog::accept();
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  foreach (ClipBlock* clip, clips_) {
    bool change_duration = !duration_slider_->IsTristate() || link_speed_and_duration_->isChecked();
    bool change_speed = !speed_slider_->IsTristate() || link_speed_and_duration_->isChecked();

    double new_speed = speed_slider_->GetValue();

    rational new_clip_length = clip->length();

    if (change_duration) {
      // Change the duration
      int64_t current_duration = olive::time_to_timestamp(clip->length(), timebase_);
      int64_t new_duration = current_duration;

      // Check if we're getting the duration value directly from the slider or calculating it from the speed
      if (duration_slider_->IsTristate()) {
        // Calculate duration from speed
        new_duration = GetAdjustedDuration(clip, new_speed);
      } else {
        // Get duration directly from slider
        new_duration = duration_slider_->GetValue();

        // Check if we're calculating the speed from this duration
        if (speed_slider_->IsTristate() && change_speed) {
          // If we're here, the duration is going to override the speed
          new_speed = GetAdjustedSpeed(clip, new_duration);
        }
      }

      if (new_duration != current_duration) {
        new_clip_length = olive::timestamp_to_time(new_duration, timebase_);
        Block* next_block = clip->next();

        // If "ripple clips" isn't checked, we need to calculate around the timeline as-is
        if (!ripple_clips_checkbox_->isChecked()
            && next_block) {

          if (new_clip_length > clip->length()) {

            // Check if next clip is a gap, and if so we can take it all up
            if (next_block->type() == Block::kGap) {
              new_clip_length = qMin(next_block->out(), clip->in() + new_clip_length);

              // If we're taking up the entire clip, we'll just remove it
              if (new_clip_length == next_block->out()) {
                new TrackRippleRemoveBlockCommand(TrackOutput::TrackFromBlock(next_block), next_block, command);

                // Delete node and its exclusive deps
                QList<Node*> gap_and_its_deps;
                gap_and_its_deps.append(next_block);
                gap_and_its_deps.append(next_block->GetExclusiveDependencies());
                new NodeRemoveCommand(static_cast<NodeGraph*>(next_block->parent()), gap_and_its_deps, command);
              } else {
                // Otherwise we can just resize it
                new BlockResizeCommand(next_block, next_block->out() - new_clip_length, command);
              }

            } else {
              // Otherwise we can't extend any further
              new_clip_length = clip->length();
            }

          } else if (new_clip_length < clip->length()) {

            // If we're not rippling these clips, we'll need to insert a gap (unless the clip is already at the end)
            rational gap_length = clip->length() - new_clip_length;

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

        if (new_clip_length != clip->length()) {
          new BlockResizeCommand(clip, new_clip_length, command);
        }
      }
    }

    if (change_speed) {
      int64_t new_clip_duration = olive::time_to_timestamp(new_clip_length, timebase_);
      int64_t new_media_duration = qRound(static_cast<double>(new_clip_duration) * new_speed);
      rational new_media_length = olive::timestamp_to_time(new_media_duration, timebase_);
      rational new_media_out = clip->media_in() + new_media_length;

      // Change the speed by calculating the appropriate media out point for this clip
      new BlockSetMediaOutCommand(clip, new_media_out, command);
    }
  }

  olive::undo_stack.pushIfHasChildren(command);

  QDialog::accept();
}

double SpeedDurationDialog::GetUnadjustedLengthTimestamp(ClipBlock *clip) const
{
  double duration = static_cast<double>(olive::time_to_timestamp(clip->length(), timebase_));

  // Convert duration to non-speed adjusted duration
  duration *= clip->speed();

  return duration;
}

int64_t SpeedDurationDialog::GetAdjustedDuration(ClipBlock *clip, const double &new_speed) const
{
  double duration = GetUnadjustedLengthTimestamp(clip);

  // Re-adjust by new speed
  duration /= new_speed;

  // Return rounded time
  return qRound64(duration);
}

double SpeedDurationDialog::GetAdjustedSpeed(ClipBlock *clip, const int64_t &new_duration) const
{
  double duration = GetUnadjustedLengthTimestamp(clip);

  // Create a fraction of the original duration over the new duration
  duration /= static_cast<double>(new_duration);

  return duration;
}

void SpeedDurationDialog::SpeedChanged()
{
  if (link_speed_and_duration_->isChecked()) {
    double new_speed = speed_slider_->GetValue();

    if (qIsNull(new_speed)) {
      // A speed of 0 is considered a still frame. Since we can't divide by zero and a still frame could be any length,
      // we don't bother updating the
      return;
    }

    bool same_durations = true;
    int64_t new_duration = GetAdjustedDuration(clips_.first(), new_speed);

    for (int i=1;i<clips_.size();i++) {
      // Calculate what this clip's duration will be
      int64_t this_clip_duration = GetAdjustedDuration(clips_.at(i), new_speed);

      // If they won't be the same, we won't show the value
      if (this_clip_duration != new_duration) {
        same_durations = false;
        break;
      }
    }

    if (same_durations) {
      duration_slider_->SetValue(new_duration);
    } else {
      duration_slider_->SetTristate();
    }
  }
}

void SpeedDurationDialog::DurationChanged()
{
  if (link_speed_and_duration_->isChecked()) {
    int64_t new_duration = duration_slider_->GetValue();

    bool same_speeds = true;
    double new_speed = GetAdjustedSpeed(clips_.first(), new_duration);

    for (int i=1;i<clips_.size();i++) {
      double this_clip_speed = GetAdjustedSpeed(clips_.at(i), new_duration);

      if (!qFuzzyCompare(this_clip_speed, new_speed)) {
        same_speeds = false;
        break;
      }
    }

    if (same_speeds) {
      speed_slider_->SetValue(new_speed);
    } else {
      speed_slider_->SetTristate();
    }
  }
}
