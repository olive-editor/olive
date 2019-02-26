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

#ifndef ADVANCEDVIDEODIALOG_H
#define ADVANCEDVIDEODIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>

#include "io/exportthread.h"

/**
 * @brief The AdvancedVideoDialog class
 *
 * A dialog for interfacing with VideoCodecParams, a struct for more advanced video settings sometimes specific to
 * one codec.
 */
class AdvancedVideoDialog : public QDialog {
  Q_OBJECT
public:
  AdvancedVideoDialog(QWidget* parent,
                      int encoding_codec,
                      VideoCodecParams& iparams);

public slots:
  virtual void accept() override;
private:
  /**
   * @brief Internal reference to VideoCodecParams struct provided by ExportDialog.
   */
  VideoCodecParams& params_;

  /**
   * @brief ComboBox to show available pixel formats for this codec
   */
  QComboBox* pix_fmt_combo_;

  /**
   * @brief SpinBox for multithreading settings
   */
  QSpinBox* thread_spinbox_;
};

#endif // ADVANCEDVIDEODIALOG_H
