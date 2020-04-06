/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QProgressBar>

#ifdef Q_OS_WINDOWS
#include <shobjidl.h>
#endif

#include "exportaudiotab.h"
#include "exportcodec.h"
#include "exportformat.h"
#include "exportvideotab.h"
#include "render/backend/exporter.h"
#include "widget/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER

class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  ExportDialog(ViewerOutput* viewer_node, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

protected:
  virtual void closeEvent(QCloseEvent *e) override;

private:
  void SetUpFormats();
  void LoadPresets();
  void SetDefaultFilename();

  QMatrix4x4 GenerateMatrix(ExportVideoTab::ScalingMethod method, int source_width, int source_height, int dest_width, int height);

  void SetUIElementsEnabled(bool enabled);

  ViewerOutput* viewer_node_;

  QList<ExportFormat> formats_;
  int previously_selected_format_;

  QCheckBox* video_enabled_;
  QCheckBox* audio_enabled_;

  QList<ExportCodec> codecs_;

  ViewerWidget* preview_viewer_;
  QLineEdit* filename_edit_;
  QComboBox* format_combobox_;

  Exporter* exporter_;

  ExportVideoTab* video_tab_;
  ExportAudioTab* audio_tab_;

  double video_aspect_ratio_;

  ColorManager* color_manager_;

  QProgressBar* progress_bar_;

  QWidget* preferences_area_;
  QDialogButtonBox* buttons_;
  QPushButton* export_cancel_btn_;

#ifdef Q_OS_WINDOWS
  ITaskbarList3* taskbar_list_;
#endif

  bool cancelled_;

  enum Format {
    kFormatDNxHD,
    kFormatMatroska,
    kFormatMPEG4,
    kFormatOpenEXR,
    kFormatQuickTime,
    kFormatPNG,
    kFormatTIFF,

    kFormatCount
  };

  enum Codec {
    kCodecDNxHD,
    kCodecH264,
    kCodecH265,
    kCodecOpenEXR,
    kCodecPNG,
    kCodecProRes,
    kCodecTIFF,

    kCodecMP2,
    kCodecMP3,
    kCodecAAC,
    kCodecPCM,

    kCodecCount
  };

private slots:
  void BrowseFilename();

  void FormatChanged(int index);

  void ResolutionChanged();

  void VideoCodecChanged();

  void UpdateViewerDimensions();

  void ExporterIsDone();

  void CancelExport();

#ifdef Q_OS_WINDOWS
  void UpdateTaskbarProgress(int progress);
#endif

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTDIALOG_H
