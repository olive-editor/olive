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

#include "codec/exportcodec.h"
#include "codec/exportformat.h"
#include "exportaudiotab.h"
#include "exportvideotab.h"
#include "task/export/export.h"
#include "widget/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER

class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  ExportDialog(ViewerOutput* viewer_node, QWidget* parent = nullptr);

protected:
  virtual void closeEvent(QCloseEvent *e) override;

private:
  void LoadPresets();
  void SetDefaultFilename();

  ExportParams GenerateParams() const;

  ViewerOutput* viewer_node_;

  ExportFormat::Format previously_selected_format_;

  QCheckBox* video_enabled_;
  QCheckBox* audio_enabled_;

  ViewerWidget* preview_viewer_;
  QLineEdit* filename_edit_;
  QComboBox* format_combobox_;

  ExportVideoTab* video_tab_;
  ExportAudioTab* audio_tab_;

  double video_aspect_ratio_;

  ColorManager* color_manager_;

  QWidget* preferences_area_;
  QDialogButtonBox* buttons_;

private slots:
  void BrowseFilename();

  void FormatChanged(int index);

  void ResolutionChanged();

  void UpdateViewerDimensions();

  void StartExport();

  void ExportFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTDIALOG_H
