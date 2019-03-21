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

#include "advancedvideodialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

AdvancedVideoDialog::AdvancedVideoDialog(QWidget *parent,
                                         AVCodecID encoding_codec,
                                         VideoCodecParams &iparams) :
  QDialog(parent),
  params_(iparams)
{
  setWindowTitle(tr("Advanced Video Settings"));

  // use variable for row to assist adding new fields to the grid layout
  int row = 0;

  // get encoder information for this codec from FFmpeg
  AVCodec* codec_info = avcodec_find_encoder(static_cast<AVCodecID>(encoding_codec));

  // set up grid layout for dialog
  QGridLayout* layout = new QGridLayout(this);

  // create row for codec pixel formats
  layout->addWidget(new QLabel(tr("Pixel Format:")), row, 0);
  pix_fmt_combo_ = new QComboBox();

  // loop through available pixel formats for this codec
  int pix_fmt_index = 0;
  while (codec_info->pix_fmts[pix_fmt_index] != -1) { // AVCodec->pix_fmts is terminated by "-1"

    // get the name of the pixel format and add it to the combobox (with the pixel format constant)
    pix_fmt_combo_->addItem(av_get_pix_fmt_name(codec_info->pix_fmts[pix_fmt_index]),
                           codec_info->pix_fmts[pix_fmt_index]);

    // if the user has already selected a pixel format, set the combobox to it as well
    if (codec_info->pix_fmts[pix_fmt_index] == params_.pix_fmt) {
      pix_fmt_combo_->setCurrentIndex(pix_fmt_combo_->count()-1);
    }

    pix_fmt_index++;
  }

  layout->addWidget(pix_fmt_combo_, row, 1);

  row++;

  // create row for multithreading thread count
  layout->addWidget(new QLabel(tr("Threads:")), row, 0);

  thread_spinbox_ = new QSpinBox();

  // with the thread count, "0" is considered automatics
  thread_spinbox_->setMinimum(0);
  thread_spinbox_->setSpecialValueText("Auto");

  // load current thread value
  thread_spinbox_->setValue(params_.threads);

  layout->addWidget(thread_spinbox_);

  row++;

  // buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttons->setCenterButtons(true);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  layout->addWidget(buttons, row, 0, 1, 2);
}

void AdvancedVideoDialog::accept() {
  // store settings back into struct

  params_.pix_fmt = pix_fmt_combo_->currentData().toInt();
  params_.threads = thread_spinbox_->value();

  QDialog::accept();
}
