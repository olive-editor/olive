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

#include "autocutsilencedialog.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

#include "timeline/sequence.h"
#include "rendering/renderfunctions.h"
#include "panels/panels.h"
#include "panels/timeline.h"

AutoCutSilenceDialog::AutoCutSilenceDialog(QWidget *parent, QVector<Clip*> clips) :
  QDialog(parent),
  clips_(clips)
{
  setWindowTitle(tr("Cut Silence"));

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  QGridLayout* grid = new QGridLayout();
  grid->setSpacing(6);

  grid->addWidget(new QLabel(tr("Attack Threshold:"), this), 0, 0);
  attack_threshold = new LabelSlider(this);
  attack_threshold->SetDecimalPlaces(0);
  grid->addWidget(attack_threshold, 0, 1);

  grid->addWidget(new QLabel(tr("Attack Time:"), this), 1, 0);
  attack_time = new LabelSlider(this);
  attack_time->SetDecimalPlaces(0);
  grid->addWidget(attack_time, 1, 1);

  grid->addWidget(new QLabel(tr("Release Threshold:"), this), 2, 0);
  release_threshold = new LabelSlider(this);
  release_threshold->SetDecimalPlaces(0);
  grid->addWidget(release_threshold, 2, 1);

  grid->addWidget(new QLabel(tr("Release Time:"), this), 3, 0);
  release_time = new LabelSlider(this);
  release_time->SetDecimalPlaces(0);
  grid->addWidget(release_time, 3, 1);

  main_layout->addLayout(grid);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->setCenterButtons(true);
  main_layout->addWidget(buttonBox);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

}

int AutoCutSilenceDialog::exec()
{
  default_attack_threshold = 5;
  current_attack_threshold = 5;
  default_attack_time = 2;
  current_attack_time = 2;
  default_release_threshold = 2;
  current_release_threshold = 2;
  default_release_time = 5;
  current_release_time = 5;

  attack_threshold->SetMinimum(1);
  attack_threshold->setEnabled(true);
  attack_threshold->SetDefault(default_attack_threshold);
  attack_threshold->SetValue(current_attack_threshold);

  attack_time->SetMinimum(1);
  attack_time->setEnabled(true);
  attack_time->SetDefault(default_attack_time);
  attack_time->SetValue(current_attack_time);

  release_threshold->SetMinimum(1);
  release_threshold->setEnabled(true);
  release_threshold->SetDefault(default_release_threshold);
  release_threshold->SetValue(current_release_threshold);

  release_time->SetMinimum(1);
  release_time->setEnabled(true);
  release_time->SetDefault(default_release_time);
  release_time->SetValue(current_release_time);

  return QDialog::exec();
}

void AutoCutSilenceDialog::accept() {

  current_attack_threshold = attack_threshold->value();
  current_attack_time = attack_time->value();
  current_release_threshold = release_threshold->value();
  current_release_time = release_time->value();

  cut_silence();

  update_ui(true);
  QDialog::accept();
}

void AutoCutSilenceDialog::cut_silence() {
  // Loop over clips provided to this dialog
  for (int j=0;j<clips_.size();j++) {

    Clip* clip = clips_.at(j);

    // Check if this clip is an audio footage clip
    if (clip->track() >= 0
        && clip->media() != nullptr
        && clip->media_stream()->preview_done) { // TODO provide warning for preview not being done

      QVector<long> split_positions;

      int clip_start = clip->timeline_in();
      const FootageStream* ms = clip->media_stream();

      long media_length = clip->media_length();
      int preview_size = ms->audio_preview.length();
      float chunk_size = (float)preview_size/media_length;  // how many audio samples to read for each fotogram

      int sample_size = qMax(current_attack_time, current_release_time)+1;

      bool attack = false;    // status flags
      bool release = false;

      QVector<qint8> vols;
      vols.resize(sample_size);
      vols.fill(0);

      // loop through the entire sequence
      for (long i=clip_start;i<media_length+clip_start;i++) {
        long start = ((i-clip_start)*chunk_size);     //audio samples are read relative to the clip, not absolute to the timeline
        int circular_index = i%sample_size;

        // read the current sample into the circular array
        qint8 tmp = 0;
        for (int k=start; k<start+chunk_size; k++){
          tmp = qMax(tmp, qint8(qRound(double(ms->audio_preview.at(k)))));
        }
        vols[circular_index] = tmp;

        //for debug:
        //qInfo() << "i:" << i <<" - "<< i/30 <<":"<< i%30 << " - volume:" << vols[circular_index] <<"\n";

        int overthreshold = 0;
        int cut_idx = 0;  //how much to cut (backwards)

        // if current volume value is above threshold
        if (vols[circular_index] >= current_attack_threshold && !attack){   // if we get one sample over the threshold
          for(int k=0; k<sample_size; k++){                       // count how many times this happened before within sample_size range (avoid false activations)
            int back_idx = (((circular_index-k)%sample_size)+sample_size)%sample_size;  // positive modulus
            if(vols[back_idx] > current_attack_threshold){
              overthreshold++;
              cut_idx = k+1;
            }
          }
          // if we reached threshold over the set tolerance
          if(overthreshold >= current_attack_time){
            split_positions.append(i-cut_idx);
            attack = true;
            release = false;
            //qInfo() << "\n\n Current vol: "<<vols[circular_index]<<" attack at " << i-cut_idx << "\n\n";
          }
          overthreshold = 0;
          cut_idx = 0;
        }else if (vols[circular_index] < current_release_threshold && !release){   // if we get one sample under the threshold
          for(int k=0; k<sample_size; k++){                               // count how many times this happened before within sample_size range
            int back_idx = (((circular_index-k)%sample_size)+sample_size)%sample_size;  // positive modulus
            if(vols[back_idx] < current_release_threshold)
              overthreshold++;
          }
          // if we reached threshold over the set tolerance
          if(overthreshold >= current_release_time){        // must be <= sample_size
            attack = false;
            release = true;
            split_positions.append(i);
            //qInfo() << "\n\n Current vol: "<<vols[circular_index]<<" release at " << i << "\n\n";
          }
          overthreshold = 0;
        }
      }

      ComboAction* ca = new ComboAction();

      // NO GOOD VERY BAD TEST CODE
      int clip_index = -1;
      for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
        if (olive::ActiveSequence->clips.at(i).get() == clip) {
          clip_index = i;
          break;
        }
      }

      Q_ASSERT(clip_index > -1);

      panel_timeline->split_clip_at_positions(ca, clip_index, split_positions);
      olive::UndoStack.push(ca);
    }


  }
}
