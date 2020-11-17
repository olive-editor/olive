/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "export.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardPaths>

#include "common/qtutils.h"
#include "core.h"
#include "dialog/task/task.h"
#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

ExportDialog::ExportDialog(ViewerOutput *viewer_node, TimelinePoints *points, QWidget *parent) :
  QDialog(parent),
  viewer_node_(viewer_node),
  points_(points)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  preferences_area_ = new QWidget();
  QGridLayout* preferences_layout = new QGridLayout(preferences_area_);
  preferences_layout->setMargin(0);

  int row = 0;

  QLabel* fn_lbl = new QLabel(tr("Filename:"));
  fn_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(fn_lbl, row, 0);

  filename_edit_ = new QLineEdit();
  preferences_layout->addWidget(filename_edit_, row, 1, 1, 2);

  QPushButton* file_browse_btn = new QPushButton();
  file_browse_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  file_browse_btn->setIcon(icon::Folder);
  file_browse_btn->setToolTip(tr("Browse for exported file filename"));
  connect(file_browse_btn,
          &QPushButton::clicked,
          this,
          &ExportDialog::BrowseFilename);
  preferences_layout->addWidget(file_browse_btn, row, 3);

  row++;

  QLabel* preset_lbl = new QLabel(tr("Preset:"));
  preset_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_lbl, row, 0);
  QComboBox* preset_combobox = new QComboBox();
  preset_combobox->addItem(tr("Same As Source - High Quality"));
  preset_combobox->addItem(tr("Same As Source - Medium Quality"));
  preset_combobox->addItem(tr("Same As Source - Low Quality"));
  preferences_layout->addWidget(preset_combobox, row, 1);

  QPushButton* preset_load_btn = new QPushButton();
  preset_load_btn->setIcon(icon::Open);
  preset_load_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_load_btn, row, 2);

  QPushButton* preset_save_btn = new QPushButton();
  preset_save_btn->setIcon(icon::Save);
  preset_save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_save_btn, row, 3);

  row++;

  preferences_layout->addWidget(QtUtils::CreateHorizontalLine(), row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Range:")), row, 0);

  range_combobox_ = new QComboBox();
  range_combobox_->addItem(tr("Entire Sequence"));
  range_combobox_->addItem(tr("In to Out"));
  if (!points_) {
    range_combobox_->setEnabled(false);
  }
  preferences_layout->addWidget(range_combobox_, row, 1, 1, 3);

  row++;

  preferences_layout->addWidget(QtUtils::CreateHorizontalLine(), row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  format_combobox_ = new QComboBox();
  preferences_layout->addWidget(format_combobox_, row, 1, 1, 3);

  row++;

  QHBoxLayout* av_enabled_layout = new QHBoxLayout();

  video_enabled_ = new QCheckBox(tr("Export Video"));
  video_enabled_->setChecked(true);
  av_enabled_layout->addWidget(video_enabled_);

  audio_enabled_ = new QCheckBox(tr("Export Audio"));
  audio_enabled_->setChecked(true);
  av_enabled_layout->addWidget(audio_enabled_);

  preferences_layout->addLayout(av_enabled_layout, row, 0, 1, 4);

  row++;

  QTabWidget* preferences_tabs = new QTabWidget();
  QScrollArea* video_area = new QScrollArea();
  color_manager_ = static_cast<Sequence*>(viewer_node_->parent())->project()->color_manager();
  video_tab_ = new ExportVideoTab(color_manager_);
  video_area->setWidgetResizable(true);
  video_area->setWidget(video_tab_);
  preferences_tabs->addTab(video_area, tr("Video"));
  QScrollArea* audio_area = new QScrollArea();
  audio_tab_ = new ExportAudioTab();
  audio_area->setWidgetResizable(true);
  audio_area->setWidget(audio_tab_);
  preferences_tabs->addTab(audio_area, tr("Audio"));
  preferences_layout->addWidget(preferences_tabs, row, 0, 1, 4);

  row++;

  buttons_ = new QDialogButtonBox();
  buttons_->setCenterButtons(true);
  buttons_->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
  buttons_->addButton(QDialogButtonBox::Cancel);
  connect(buttons_, &QDialogButtonBox::accepted, this, &ExportDialog::StartExport);
  connect(buttons_, &QDialogButtonBox::rejected, this, &ExportDialog::reject);
  preferences_layout->addWidget(buttons_, row, 0, 1, 4);

  splitter->addWidget(preferences_area_);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_viewer_ = new ViewerWidget();
  preview_viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  preview_layout->addWidget(preview_viewer_);
  splitter->addWidget(preview_area);

  // Set default filename
  SetDefaultFilename();

  // Populate combobox formats
  for (int i=0; i<ExportFormat::kFormatCount; i++) {
    format_combobox_->addItem(ExportFormat::GetName(static_cast<ExportFormat::Format>(i)));
  }

  // Set defaults
  previously_selected_format_ = ExportFormat::kFormatMPEG4;
  format_combobox_->setCurrentIndex(ExportFormat::kFormatMPEG4);
  connect(format_combobox_,
          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &ExportDialog::FormatChanged);
  FormatChanged(ExportFormat::kFormatMPEG4);

  video_tab_->width_slider()->SetValue(viewer_node_->video_params().width());
  video_tab_->width_slider()->SetDefaultValue(viewer_node_->video_params().width());
  video_tab_->height_slider()->SetValue(viewer_node_->video_params().height());
  video_tab_->height_slider()->SetDefaultValue(viewer_node_->video_params().height());
  video_tab_->frame_rate_combobox()->SetFrameRate(viewer_node_->video_params().time_base().flipped());
  video_tab_->pixel_aspect_combobox()->SetPixelAspectRatio(viewer_node_->video_params().pixel_aspect_ratio());
  video_tab_->pixel_format_field()->SetPixelFormat(static_cast<VideoParams::Format>(Config::Current()["OnlinePixelFormat"].toInt()));
  video_tab_->interlaced_combobox()->SetInterlaceMode(viewer_node_->video_params().interlacing());
  audio_tab_->sample_rate_combobox()->SetSampleRate(viewer_node_->audio_params().sample_rate());
  audio_tab_->channel_layout_combobox()->SetChannelLayout(viewer_node_->audio_params().channel_layout());

  video_aspect_ratio_ = static_cast<double>(viewer_node_->video_params().width()) / static_cast<double>(viewer_node_->video_params().height());

  connect(video_tab_->width_slider(),
          &IntegerSlider::ValueChanged,
          this,
          &ExportDialog::ResolutionChanged);

  connect(video_tab_->height_slider(),
          &IntegerSlider::ValueChanged,
          this,
          &ExportDialog::ResolutionChanged);

  connect(video_tab_->scaling_method_combobox(),
          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &ExportDialog::UpdateViewerDimensions);

  connect(video_tab_->maintain_aspect_checkbox(),
          &QCheckBox::toggled,
          this,
          &ExportDialog::ResolutionChanged);

  connect(video_tab_,
          &ExportVideoTab::ColorSpaceChanged,
          preview_viewer_,
          static_cast<void(ViewerWidget::*)(const ColorTransform&)>(&ViewerWidget::SetColorTransform));

  // Set viewer to view the node
  preview_viewer_->ConnectViewerNode(viewer_node_);
  preview_viewer_->ruler()->ConnectTimelinePoints(points_);
  preview_viewer_->SetColorMenuEnabled(false);
  preview_viewer_->SetColorTransform(video_tab_->CurrentOCIOColorSpace());
}

void ExportDialog::StartExport()
{
  if (!video_enabled_->isChecked() && !audio_enabled_->isChecked()) {
    QMessageBox b(this);
    b.setIcon(QMessageBox::Critical);
    b.setWindowModality(Qt::WindowModal);
    b.setWindowTitle(tr("Invalid parameters"));
    b.setText(tr("Both video and audio are disabled. There's nothing to export."));
    b.addButton(QMessageBox::Ok);
    b.exec();
    return;
  }

  // Validate if the entered filename contains the correct extension (the extension is necessary
  // for both FFmpeg and OIIO to determine the output format)
  QString necessary_ext = QStringLiteral(".%1").arg(ExportFormat::GetExtension(static_cast<ExportFormat::Format>(format_combobox_->currentIndex())));
  QString proposed_filename = filename_edit_->text().trimmed();

  // If it doesn't, see if the user wants to append it automatically. If not, we don't abort the export.
  if (!proposed_filename.endsWith(necessary_ext, Qt::CaseInsensitive)) {
    QMessageBox b(this);
    b.setIcon(QMessageBox::Warning);
    b.setWindowModality(Qt::WindowModal);
    b.setWindowTitle(tr("Invalid filename"));
    b.setText(tr("The filename must contain the extension \"%1\". Would you like to append it "
                 "automatically?").arg(necessary_ext));
    b.addButton(QMessageBox::Yes);
    b.addButton(QMessageBox::No);

    if (b.exec() == QMessageBox::Yes) {
      filename_edit_->setText(proposed_filename.append(necessary_ext));
    } else {
      return;
    }
  }

  // Validate the intended path
  QFileInfo file_info(proposed_filename);
  QFileInfo dir_info(file_info.path());

  // If the directory does not exist, try to create it
  if (!QDir(file_info.path()).mkpath(QStringLiteral("."))) {
    QMessageBox b(this);
    b.setIcon(QMessageBox::Critical);
    b.setWindowModality(Qt::WindowModal);
    b.setWindowTitle(tr("Failed to create output directory"));
    b.setText(tr("The intended output directory doesn't exist and Olive couldn't create it. "
                 "Please choose a different filename."));
    b.addButton(QMessageBox::Ok);
    b.exec();
    return;
  }

  // Validate if the file exists and whether the user wishes to overwrite it
  if (file_info.exists()) {
    QMessageBox b(this);
    b.setIcon(QMessageBox::Warning);
    b.setWindowModality(Qt::WindowModal);
    b.setWindowTitle(tr("Confirm Overwrite"));
    b.setText(tr("The file \"%1\" already exists. Do you want to overwrite it?")
              .arg(proposed_filename));
    b.addButton(QMessageBox::Yes);
    b.addButton(QMessageBox::No);

    if (b.exec() == QMessageBox::No) {
      return;
    }
  }

  // Validate video resolution
  if (video_enabled_->isChecked()
      && video_tab_->GetSelectedCodec() == ExportCodec::kCodecH264
      && (video_tab_->width_slider()->GetValue()%2 != 0 || video_tab_->height_slider()->GetValue()%2 != 0)) {
    QMessageBox b(this);
    b.setIcon(QMessageBox::Critical);
    b.setWindowModality(Qt::WindowModal);
    b.setWindowTitle(tr("Invalid Parameters"));
    b.setText(tr("Width and height must be multiples of 2."));
    b.exec();
    return;
  }

  ExportTask* task = new ExportTask(viewer_node_, color_manager_, GenerateParams());
  TaskDialog* td = new TaskDialog(task, tr("Export"), this);
  connect(td, &TaskDialog::TaskSucceeded, this, &ExportDialog::ExportFinished);
  td->open();
}

void ExportDialog::ExportFinished()
{
  TaskDialog* td = static_cast<TaskDialog*>(sender());

  if (td->GetTask()->IsCancelled()) {
    // If this task was cancelled, we stay open so the user can potentially queue another export
  } else {
    // Accept this dialog and close
    this->accept();
  }
}

void ExportDialog::closeEvent(QCloseEvent *e)
{
  preview_viewer_->ConnectViewerNode(nullptr);

  QDialog::closeEvent(e);
}

void ExportDialog::BrowseFilename()
{
  ExportFormat::Format f = static_cast<ExportFormat::Format>(format_combobox_->currentIndex());

  QString browsed_fn = QFileDialog::getSaveFileName(this,
                                                    "",
                                                    filename_edit_->text().trimmed(),
                                                    QStringLiteral("%1 (*.%2)").arg(ExportFormat::GetName(f), ExportFormat::GetExtension(f)),
                                                    nullptr,

                                                    // We don't confirm overwrite here because we do it later
                                                    QFileDialog::DontConfirmOverwrite);

  if (!browsed_fn.isEmpty()) {
    filename_edit_->setText(browsed_fn);
  }
}

void ExportDialog::FormatChanged(int index)
{
  QString current_filename = filename_edit_->text().trimmed();
  QString previously_selected_ext = ExportFormat::GetExtension(previously_selected_format_);
  ExportFormat::Format current_format = static_cast<ExportFormat::Format>(index);
  QString currently_selected_ext = ExportFormat::GetExtension(current_format);

  // If the previous extension was added, remove it
  if (current_filename.endsWith(previously_selected_ext, Qt::CaseInsensitive)) {
    current_filename.resize(current_filename.size() - previously_selected_ext.size() - 1);
  }

  // Add the extension and set it
  current_filename.append('.');
  current_filename.append(currently_selected_ext);
  filename_edit_->setText(current_filename);

  previously_selected_format_ = current_format;

  // Update video and audio comboboxes
  video_tab_->codec_combobox()->clear();
  foreach (ExportCodec::Codec vcodec, ExportFormat::GetVideoCodecs(current_format)) {
    video_tab_->codec_combobox()->addItem(ExportCodec::GetCodecName(vcodec), vcodec);
  }

  audio_tab_->codec_combobox()->clear();
  foreach (ExportCodec::Codec acodec, ExportFormat::GetAudioCodecs(current_format)) {
    audio_tab_->codec_combobox()->addItem(ExportCodec::GetCodecName(acodec), acodec);
  }
}

void ExportDialog::ResolutionChanged()
{
  if (video_tab_->maintain_aspect_checkbox()->isChecked()) {
    // Keep aspect ratio maintained
    if (sender() == video_tab_->height_slider()) {

      // Convert height to float
      double new_width = video_tab_->height_slider()->GetValue();

      // Generate width from aspect ratio
      new_width *= video_aspect_ratio_;

      // Align to even number and set
      video_tab_->width_slider()->SetValue(new_width);

    } else {

      // Convert width to float
      double new_height = video_tab_->width_slider()->GetValue();

      // Generate height from aspect ratio
      new_height /= video_aspect_ratio_;

      // Align to even number and set
      video_tab_->height_slider()->SetValue(new_height);

    }
  }

  UpdateViewerDimensions();
}

void ExportDialog::LoadPresets()
{

}

void ExportDialog::SetDefaultFilename()
{
  Sequence* s = static_cast<Sequence*>(viewer_node_->parent());
  Project* p = s->project();

  QDir doc_location;

  if (p->filename().isEmpty()) {
    doc_location.setPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  } else {
    doc_location = QFileInfo(p->filename()).dir();
  }

  QString file_location = doc_location.filePath(s->name());
  filename_edit_->setText(file_location);
}

ExportParams ExportDialog::GenerateParams() const
{
  VideoParams video_render_params(static_cast<int>(video_tab_->width_slider()->GetValue()),
                                  static_cast<int>(video_tab_->height_slider()->GetValue()),
                                  video_tab_->frame_rate_combobox()->GetFrameRate().flipped(),
                                  video_tab_->pixel_format_field()->GetPixelFormat(),
                                  VideoParams::kInternalChannelCount,
                                  video_tab_->pixel_aspect_combobox()->GetPixelAspectRatio(),
                                  video_tab_->interlaced_combobox()->GetInterlaceMode(),
                                  1);

  AudioParams audio_render_params(audio_tab_->sample_rate_combobox()->currentData().toInt(),
                                  audio_tab_->channel_layout_combobox()->GetChannelLayout(),
                                  AudioParams::kInternalFormat);

  ExportParams params;
  params.SetFilename(filename_edit_->text().trimmed());
  params.SetExportLength(viewer_node_->GetLength());

  if (range_combobox_->currentIndex() == kRangeInToOut
      && points_
      && points_->workarea()->enabled()) {
    params.set_custom_range(points_->workarea()->range());
  }

  if (video_tab_->scaling_method_combobox()->isEnabled()) {
    params.set_video_scaling_method(static_cast<ExportParams::VideoScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()));
  }

  if (video_enabled_->isChecked()) {
    ExportCodec::Codec video_codec = video_tab_->GetSelectedCodec();
    params.EnableVideo(video_render_params, video_codec);

    params.set_video_threads(video_tab_->threads());

    video_tab_->GetCodecSection()->AddOpts(&params);

    params.set_color_transform(video_tab_->CurrentOCIOColorSpace());

    params.set_video_pix_fmt(video_tab_->pix_fmt());
  }

  if (audio_enabled_->isChecked()) {
    ExportCodec::Codec audio_codec = static_cast<ExportCodec::Codec>(audio_tab_->codec_combobox()->currentData().toInt());
    params.EnableAudio(audio_render_params, audio_codec);
  }

  return params;
}

void ExportDialog::UpdateViewerDimensions()
{
  preview_viewer_->SetViewerResolution(static_cast<int>(video_tab_->width_slider()->GetValue()),
                                       static_cast<int>(video_tab_->height_slider()->GetValue()));

  QMatrix4x4 transform =
      ExportParams::GenerateMatrix(static_cast<ExportParams::VideoScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()),
                                   viewer_node_->video_params().width(),
                                   viewer_node_->video_params().height(),
                                   static_cast<int>(video_tab_->width_slider()->GetValue()),
                                   static_cast<int>(video_tab_->height_slider()->GetValue()));

  preview_viewer_->SetMatrix(transform);
}

OLIVE_NAMESPACE_EXIT
