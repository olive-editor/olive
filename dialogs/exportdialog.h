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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>

#include "timeline/sequence.h"
#include "rendering/exportthread.h"

/**
 * @brief The ExportDialog class
 *
 * The dialog to initiate an export.
 */
class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief ExportDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  explicit ExportDialog(QWidget *parent);

private slots:
  /**
   * @brief Slot for when the user changes the format
   *
   * Used to populate the available codecs list for this format.
   *
   * @param index
   *
   * Current format index (corresponding to enum ExportFormats)
   */
  void format_changed(int index);

  /**
   * @brief Slot for when the user clicks the Export button
   *
   * Asks the user for the file to save to.
   */
  void StartExport();
  void update_progress_bar(int value, qint64 remaining_ms);
  void render_thread_finished();
  void vcodec_changed(int index);
  void comp_type_changed(int index);
  void open_advanced_video_dialog();

private:
  void setup_ui();
  void prep_ui_for_render(bool r);

  QVector<QString> format_strings;
  ExportThread* et;

  void add_codec_to_combobox(QComboBox* box, enum AVCodecID codec);

  VideoCodecParams vcodec_params;

  QComboBox* rangeCombobox;
  QSpinBox* widthSpinbox;
  QDoubleSpinBox* videobitrateSpinbox;
  QLabel* videoBitrateLabel;
  QDoubleSpinBox* framerateSpinbox;
  QComboBox* vcodecCombobox;
  QComboBox* acodecCombobox;
  QSpinBox* samplingRateSpinbox;
  QSpinBox* audiobitrateSpinbox;
  QProgressBar* progressBar;
  QComboBox* formatCombobox;
  QSpinBox* heightSpinbox;
  QPushButton* export_button;
  QPushButton* cancel_button;
  QPushButton* renderCancel;
  QGroupBox* videoGroupbox;
  QGroupBox* audioGroupbox;
  QComboBox* compressionTypeCombobox;

  qint64 total_export_time_start;
};

#endif // EXPORTDIALOG_H
