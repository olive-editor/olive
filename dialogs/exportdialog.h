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

  /**
   * @brief Slot for the export thread to update the progress bar's value
   *
   * @param value
   *
   * An value between 0 - 100. A percentage of the Sequence that has been exported so far.
   *
   * @param remaining_ms
   *
   * The estimated time in milliseconds that it will take to complete the rest of the Sequence.
   */
  void update_progress_bar(int value, qint64 remaining_ms);

  /**
   * @brief Slot for the export thread completing (both succeeding and failing)
   *
   * Runs whenever the thread has finished. Determines whether the thread succeeded or not (and shows an error message
   * if not), cleans up the ExportThread object, sets the UI state back to normal.
   *
   * Connect to ExportThread::finished().
   */
  void export_thread_finished();

  /**
   * @brief Slot for when the video codec changes
   *
   * Some video codecs require different settings. In the case of that, this function sorts through those.
   *
   * @param index
   *
   * Current vcodecCombobox index - its item data contains the AVCodecID.
   */
  void vcodec_changed(int index);

  /**
   * @brief Slot for when the compression type changes
   *
   * Different UI objects should be displayed for different compression types.
   *
   * @param index
   *
   * Unused.
   */
  void comp_type_changed(int index);

  /**
   * @brief Slot to open the Advanced Video Dialog
   *
   * Opens a dialog for setting more advanced video settings and passes a reference to vcodec_params to it.
   */
  void open_advanced_video_dialog();

private:
  /**
   * @brief Function to create UI objects.
   */
  void setup_ui();

  /**
   * @brief Enables/disables certain UI objects based on the exporting state.
   *
   * Some UI controls don't need to be set while exporting. This function enables/disables them appropriately.
   *
   * @param r
   *
   * TRUE if we're exporting, FALSE if we finished.
   */
  void prep_ui_for_render(bool r);

  /**
   * @brief Retrieves the human-readable name of an AVCodecID and adds it to a QComboBox
   *
   * Also sets that item's data to the AVCodecID so it can be retrieved directly from the QComboBox.
   *
   * @param box
   *
   * The QComboBox to add the item to.
   *
   * @param codec
   *
   * The codec to add to the QComboBox.
   */
  void add_codec_to_combobox(QComboBox* box, enum AVCodecID codec);

  /**
   * @brief Internal array of human-readable names corresponding to enum ExportFormats
   */
  QVector<QString> format_strings;

  /**
   * @brief Pointer to an ExportThread
   *
   * Set when exporting starts, and deleted by export_thread_finished() when the thread is complete.
   */
  ExportThread* export_thread_;

  /**
   * @brief Struct for advanced video codec parameters.
   *
   * More advanced video encoding parameters to be sent to the ExportThread. These variables are not directly editable
   * in this dialog, instead calling open_advanced_video_dialog() will open an AdvancedVideoDialog for setting these
   * values directly. vcodec_changed() should also set these to the defaults for that codec where appropriate.
   */
  VideoCodecParams vcodec_params;

  /**
   * @brief ComboBox for selecting the time range of the Sequence to export
   */
  QComboBox* rangeCombobox;

  /**
   * @brief SpinBox for the exported video's width
   */
  QSpinBox* widthSpinbox;

  /**
   * @brief SpinBox for the exported video's bitrate
   */
  QDoubleSpinBox* videobitrateSpinbox;

  /**
   * @brief Label for the exported video's bitrate - changes depending on the compression type
   */
  QLabel* videoBitrateLabel;

  /**
   * @brief SpinBox for the exported video's frame rate
   */
  QDoubleSpinBox* framerateSpinbox;

  /**
   * @brief ComboBox for the exported video codec
   */
  QComboBox* vcodecCombobox;

  /**
   * @brief ComboBox for the exported audio's codec
   */
  QComboBox* acodecCombobox;

  /**
   * @brief SpinBox for the exported audio's sample rate
   */
  QSpinBox* samplingRateSpinbox;

  /**
   * @brief SpinBox for the exported audio's bitrate
   */
  QSpinBox* audiobitrateSpinbox;

  /**
   * @brief Progress bar for visually showing the export progress
   */
  QProgressBar* progressBar;

  /**
   * @brief ComboBox for the exported video's format
   */
  QComboBox* formatCombobox;

  /**
   * @brief SpinBox for the exported video's height
   */
  QSpinBox* heightSpinbox;

  /**
   * @brief Export button to trigger the start of an export
   */
  QPushButton* export_button;

  /**
   * @brief Dialog cancel button to close this dialog
   */
  QPushButton* cancel_button;

  /**
   * @brief Cancel button to abort the export before completion
   */
  QPushButton* renderCancel;

  /**
   * @brief GroupBox containing all video-related UI objects
   */
  QGroupBox* videoGroupbox;

  /**
   * @brief GroupBox containing all audio-related UI objects
   */
  QGroupBox* audioGroupbox;

  /**
   * @brief ComboBox for the exported video compression type
   */
  QComboBox* compressionTypeCombobox;

  /**
   * @brief Time value set when exporting begins to determine the total duration of the export
   */
  qint64 total_export_time_start;
};

#endif // EXPORTDIALOG_H
