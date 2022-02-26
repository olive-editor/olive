/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "codec/exportcodec.h"
#include "codec/exportformat.h"
#include "exportaudiotab.h"
#include "exportsubtitlestab.h"
#include "exportvideotab.h"
#include "task/export/export.h"
#include "widget/viewer/viewer.h"

namespace olive {

class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  ExportDialog(ViewerOutput* viewer_node, QWidget* parent = nullptr);

  ExportFormat::Format GetSelectedFormat() const;

  rational GetSelectedTimebase() const;

protected:
  virtual void closeEvent(QCloseEvent *e) override;

private:
  void AddPreferencesTab(QWidget *inner_widget, const QString &title);

  void LoadPresets();
  void SetDefaultFilename();

  ExportParams GenerateParams() const;

  void SetCurrentFormat(ExportFormat::Format format);

  ViewerOutput* viewer_node_;

  ExportFormat::Format previously_selected_format_;

  rational GetExportLength() const;
  int64_t GetExportLengthInTimebaseUnits() const;

  enum RangeSelection {
    kRangeEntireSequence,
    kRangeInToOut
  };

  QTabWidget* preferences_tabs_;

  QComboBox* range_combobox_;

  QCheckBox* video_enabled_;
  QCheckBox* audio_enabled_;
  QCheckBox* subtitles_enabled_;

  ViewerWidget* preview_viewer_;
  QLineEdit* filename_edit_;
  QComboBox* format_combobox_;

  ExportVideoTab* video_tab_;
  ExportAudioTab* audio_tab_;
  ExportSubtitlesTab* subtitle_tab_;

  double video_aspect_ratio_;

  ColorManager* color_manager_;

  QWidget* preferences_area_;
  QCheckBox *export_bkg_box_;

private slots:
  void BrowseFilename();

  void FormatChanged(int index);

  void ResolutionChanged();

  void UpdateViewerDimensions();

  void StartExport();

  void ExportFinished();

  void ImageSequenceCheckBoxChanged(bool e);

};

}

#endif // EXPORTDIALOG_H
