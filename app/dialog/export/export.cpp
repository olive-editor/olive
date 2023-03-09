/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "common/digit.h"
#include "common/qtutils.h"
#include "dialog/task/task.h"
#include "exportsavepresetdialog.h"
#include "node/project.h"
#include "node/project/sequence/sequence.h"
#include "task/taskmanager.h"
#include "ui/icons/icons.h"
#include "widget/timeruler/timeruler.h"

namespace olive {

#define super QDialog

ExportDialog::ExportDialog(ViewerOutput *viewer_node, bool stills_only_mode, QWidget *parent) :
  super(parent),
  viewer_node_(viewer_node),
  stills_only_mode_(stills_only_mode),
  loading_presets_(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  preferences_area_ = new QWidget();
  QGridLayout* preferences_layout = new QGridLayout(preferences_area_);
  preferences_layout->setContentsMargins(0, 0, 0, 0);

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
  preset_combobox_ = new QComboBox();
  LoadPresets();
  connect(preset_combobox_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ExportDialog::PresetComboBoxChanged);
  preferences_layout->addWidget(preset_combobox_, row, 1, 1, 2);

  /*QPushButton* preset_load_btn = new QPushButton();
  preset_load_btn->setIcon(icon::Open);
  preset_load_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_load_btn, row, 2);*/

  QPushButton* preset_save_btn = new QPushButton();
  preset_save_btn->setIcon(icon::Save);
  preset_save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_save_btn, row, 3);
  connect(preset_save_btn, &QPushButton::clicked, this, &ExportDialog::SavePreset);

  row++;

  preferences_layout->addWidget(QtUtils::CreateHorizontalLine(), row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Range:")), row, 0);

  range_combobox_ = new QComboBox();
  range_combobox_->addItem(tr("Entire Sequence"));
  range_combobox_->addItem(tr("In to Out"));
  range_combobox_->setEnabled(viewer_node_->GetWorkArea()->enabled());

  preferences_layout->addWidget(range_combobox_, row, 1, 1, 3);

  row++;

  preferences_layout->addWidget(QtUtils::CreateHorizontalLine(), row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  format_combobox_ = new ExportFormatComboBox();
  preferences_layout->addWidget(format_combobox_, row, 1, 1, 3);

  row++;

  QHBoxLayout* av_enabled_layout = new QHBoxLayout();

  video_enabled_ = new QCheckBox(tr("Export Video"));
  av_enabled_layout->addWidget(video_enabled_);

  audio_enabled_ = new QCheckBox(tr("Export Audio"));
  av_enabled_layout->addWidget(audio_enabled_);

  subtitles_enabled_ = new QCheckBox(tr("Export Subtitles"));
  av_enabled_layout->addWidget(subtitles_enabled_);

  preferences_layout->addLayout(av_enabled_layout, row, 0, 1, 4);

  row++;

  preferences_tabs_ = new QTabWidget();

  color_manager_ = viewer_node_->project()->color_manager();
  video_tab_ = new ExportVideoTab(color_manager_);
  AddPreferencesTab(video_tab_, tr("Video"));

  // Set video tab time and make connections
  connect(viewer_node, &ViewerOutput::PlayheadChanged, video_tab_, &ExportVideoTab::SetTime);
  connect(video_tab_, &ExportVideoTab::TimeChanged, viewer_node, &ViewerOutput::SetPlayhead);
  video_tab_->SetTime(viewer_node->GetPlayhead());

  audio_tab_ = new ExportAudioTab();
  AddPreferencesTab(audio_tab_, tr("Audio"));

  subtitle_tab_ = new ExportSubtitlesTab();
  AddPreferencesTab(subtitle_tab_, tr("Subtitles"));

  preferences_layout->addWidget(preferences_tabs_, row, 0, 1, 4);

  row++;

  {
    QGroupBox *options_group = new QGroupBox();
    preferences_layout->addWidget(options_group, row, 0, 1, 4);

    QGridLayout *options_layout = new QGridLayout(options_group);

    int opt_row = 0;

    export_bkg_box_ = new QCheckBox(tr("Run In Background"));
    export_bkg_box_->setToolTip(tr("Exporting in the background allows you to continue using Olive while "
                                   "exporting, but may result in slower export speeds, and may"
                                   "severely impact editing and playback performance."));
    options_layout->addWidget(export_bkg_box_, opt_row, 0);

    import_file_after_export_ = new QCheckBox(tr("Import Result After Export"));
    options_layout->addWidget(import_file_after_export_, opt_row, 1);

    connect(export_bkg_box_, &QCheckBox::toggled, import_file_after_export_, [this](bool e){
      import_file_after_export_->setEnabled(!e);
    });
  }

  row++;

  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->setContentsMargins(0, 0, 0, 0);
  preferences_layout->addLayout(btn_layout, row, 0, 1, 4);

  btn_layout->addStretch();

  QPushButton *export_btn = new QPushButton(tr("Export"));
  btn_layout->addWidget(export_btn);
  connect(export_btn, &QPushButton::clicked, this, &ExportDialog::StartExport);

  QPushButton *cancel_btn = new QPushButton(tr("Cancel"));
  btn_layout->addWidget(cancel_btn);
  connect(cancel_btn, &QPushButton::clicked, this, &ExportDialog::reject);

  btn_layout->addStretch();

  splitter->addWidget(preferences_area_);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_viewer_ = new ViewerWidget();
  preview_viewer_->ruler()->SetMarkerEditingEnabled(false);
  preview_viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  preview_layout->addWidget(preview_viewer_);
  splitter->addWidget(preview_area);

  // Prioritize preview area
  splitter->setSizes({1, 99999});

  // Set default filename
  SetDefaultFilename();

  // Set defaults
  previously_selected_format_ = ExportFormat::kFormatMPEG4Video;
  connect(format_combobox_, &ExportFormatComboBox::FormatChanged, this, &ExportDialog::FormatChanged);

  VideoParams vp = viewer_node_->GetVideoParams();
  video_aspect_ratio_ = static_cast<double>(vp.width()) / static_cast<double>(vp.height());

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
  connect(video_tab_,
          &ExportVideoTab::ImageSequenceCheckBoxChanged,
          this,
          &ExportDialog::ImageSequenceCheckBoxChanged);

  // We don't check if the codec supports subtitles because we can always export to a sidecar file
  bool has_subtitle_tracks = SequenceHasSubtitles();
  connect(subtitles_enabled_, &QCheckBox::toggled, subtitle_tab_, &QWidget::setEnabled);
  subtitles_enabled_->setEnabled(has_subtitle_tracks);

  // If the viewer already has cached params, use them
  if (!stills_only_mode_ && viewer_node_->GetLastUsedEncodingParams().IsValid()) {
    // This will automatically set the param data
    QtUtils::SetComboBoxData(preset_combobox_, kPresetLastUsed);
  } else {
    SetDefaults();
  }

  // Set viewer to view the node and set its colorspace
  preview_viewer_->ConnectViewerNode(viewer_node_);
  preview_viewer_->SetColorMenuEnabled(false);
  preview_viewer_->SetColorTransform(video_tab_->CurrentOCIOColorSpace());

  qApp->installEventFilter(this);

  connect(video_enabled_, &QCheckBox::toggled, video_tab_, &QWidget::setEnabled);
  video_tab_->setEnabled(video_enabled_->isChecked());
  connect(audio_enabled_, &QCheckBox::toggled, audio_tab_, &QWidget::setEnabled);
  audio_tab_->setEnabled(audio_enabled_->isChecked());
  connect(subtitles_enabled_, &QCheckBox::toggled, subtitle_tab_, &QWidget::setEnabled);
  subtitle_tab_->setEnabled(subtitles_enabled_->isChecked());
}

rational ExportDialog::GetSelectedTimebase() const
{
  return video_tab_->GetSelectedFrameRate().flipped();
}

void ExportDialog::SetSelectedTimebase(const rational &r)
{
  video_tab_->SetSelectedFrameRate(r.flipped());
}

void ExportDialog::StartExport()
{
  if (!video_enabled_->isChecked() && !audio_enabled_->isChecked() && !subtitles_enabled_->isChecked()) {
    QtUtils::MsgBox(this, QMessageBox::Critical, tr("Invalid parameters"),
                        tr("Video, audio, and subtitles are disabled. There's nothing to export."));
    return;
  }

  // Validate if the entered filename contains the correct extension (the extension is necessary
  // for both FFmpeg and OIIO to determine the output format)
  QString necessary_ext = QStringLiteral(".%1").arg(ExportFormat::GetExtension(format_combobox_->GetFormat()));
  QString proposed_filename = filename_edit_->text().trimmed();

  // If it doesn't, see if the user wants to append it automatically. If not, we don't abort the export.
  if (!proposed_filename.endsWith(necessary_ext, Qt::CaseInsensitive)) {
    if (QtUtils::MsgBox(this, QMessageBox::Warning, tr("Invalid filename"),
                            tr("The filename must contain the extension \"%1\". Would you like to append it "
                                             "automatically?").arg(necessary_ext),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      filename_edit_->setText(proposed_filename.append(necessary_ext));
    } else {
      return;
    }
  }

  // Validate the intended path
  QFileInfo file_info(proposed_filename);
  QFileInfo dir_info(file_info.path());

  // If the directory does not exist, try to create it
  QDir dest_dir(file_info.path());
  if (!FileFunctions::DirectoryIsValid(dest_dir)) {
    QtUtils::MsgBox(this, QMessageBox::Critical, tr("Failed to create output directory"),
                        tr("The intended output directory doesn't exist and Olive couldn't create it. "
                                         "Please choose a different filename."));
    return;
  }

  // Validate if this is an image sequence and if the filename contains enough digits
  if (video_tab_->IsImageSequenceSet()) {
    // Ensure filename contains digits
    if (!Encoder::FilenameContainsDigitPlaceholder(proposed_filename)) {
      QtUtils::MsgBox(this, QMessageBox::Critical, tr("Invalid filename"),
                          tr("Export is set to an image sequence, but the filename does not have a section for digits "
                                             "(formatted as [#####] where the amount of # is the amount of digits)."));
      return;
    }

    int64_t frame_count = GetExportLengthInTimebaseUnits();
    int64_t needed_digit_count = GetDigitCount(frame_count);
    int current_digit_count = Encoder::GetImageSequencePlaceholderDigitCount(proposed_filename);
    if (current_digit_count < needed_digit_count) {
      QtUtils::MsgBox(this, QMessageBox::Critical, tr("Invalid filename"),
                          tr("Filename doesn't contain enough digits for the amount of frames "
                             "this export will need (need %1 for %n frame(s)).", nullptr, frame_count)
                                .arg(QString::number(needed_digit_count)));
      return;
    }
  }

  // Validate if the file exists and whether the user wishes to overwrite it
  if (file_info.exists()) {
    if (QtUtils::MsgBox(this, QMessageBox::Warning, tr("Confirm Overwrite"),
                            tr("The file \"%1\" already exists. Do you want to overwrite it?")
                                          .arg(proposed_filename),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
      return;
    }
  }

  // Validate video resolution
  if (video_enabled_->isChecked()
      && (video_tab_->GetSelectedCodec() == ExportCodec::kCodecH264 || video_tab_->GetSelectedCodec() == ExportCodec::kCodecH265)
      && (video_tab_->width_slider()->GetValue()%2 != 0 || video_tab_->height_slider()->GetValue()%2 != 0)) {
    QtUtils::MsgBox(this, QMessageBox::Critical, tr("Invalid Parameters"),
                        tr("Width and height must be multiples of 2."));
    return;
  }

  ExportTask* task = new ExportTask(viewer_node_, color_manager_, GenerateParams());

  if (export_bkg_box_->isChecked()) {
    // Send to TaskManager to export in background
    TaskManager::instance()->AddTask(task);
    this->accept();
  } else {
    // Use modal dialog box
    TaskDialog* td = new TaskDialog(task, tr("Export"), this);
    connect(td, &TaskDialog::TaskSucceeded, this, &ExportDialog::ExportFinished);
    td->open();
  }
}

void ExportDialog::ExportFinished()
{
  TaskDialog* td = static_cast<TaskDialog*>(sender());

  if (td->GetTask()->IsCancelled()) {
    // If this task was cancelled, we stay open so the user can potentially queue another export
  } else {
    // Accept this dialog and close
    if (import_file_after_export_->isEnabled() && import_file_after_export_->isChecked()) {
      QString filename = filename_edit_->text().trimmed();
      emit RequestImportFile(filename);
    }

    this->accept();
  }
}

void ExportDialog::ImageSequenceCheckBoxChanged(bool e)
{
  QFileInfo current_fileinfo(filename_edit_->text());

  QString basename = current_fileinfo.completeBaseName();
  QString suffix = current_fileinfo.suffix();

  if (e) {
    if (!Encoder::FilenameContainsDigitPlaceholder(basename)) {
      basename.append(QStringLiteral("_[#####]"));
    }
  } else {
    basename = Encoder::FilenameRemoveDigitPlaceholder(basename);
  }

  // Set filename
  if (!suffix.isEmpty()) {
    basename.append('.');
    basename.append(suffix);
  }
  filename_edit_->setText(current_fileinfo.dir().filePath(basename));
}

void ExportDialog::SavePreset()
{
  ExportSavePresetDialog d(GenerateParams(), this);
  if (d.exec() == QDialog::Accepted) {
    LoadPresets();
    preset_combobox_->setCurrentText(d.GetSelectedPresetName());
  }
}

void ExportDialog::PresetComboBoxChanged()
{
  if (loading_presets_) {
    return;
  }

  QComboBox *c = static_cast<QComboBox *>(sender());

  int preset_number = c->currentData().toInt();
  if (preset_number == kPresetDefault) {
    SetDefaults();
  } else if (preset_number == kPresetLastUsed) {
    SetParams(viewer_node_->GetLastUsedEncodingParams());
  } else {
    SetParams(presets_.at(preset_number));
  }
}

void ExportDialog::AddPreferencesTab(QWidget *inner_widget, const QString &title)
{
  QScrollArea* scroll_area = new QScrollArea();
  scroll_area->setWidgetResizable(true);
  scroll_area->setWidget(inner_widget);
  preferences_tabs_->addTab(scroll_area, title);
}

void ExportDialog::BrowseFilename()
{
  ExportFormat::Format f = format_combobox_->GetFormat();

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

void ExportDialog::FormatChanged(ExportFormat::Format current_format)
{
  QString current_filename = filename_edit_->text().trimmed();
  QString previously_selected_ext = ExportFormat::GetExtension(previously_selected_format_);
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
  bool has_video_codecs = video_tab_->SetFormat(current_format);
  video_enabled_->setChecked(has_video_codecs);
  video_enabled_->setEnabled(has_video_codecs);

  bool has_audio_codecs = audio_tab_->SetFormat(current_format);
  audio_enabled_->setChecked(has_audio_codecs);
  audio_enabled_->setEnabled(has_audio_codecs);

  if (subtitles_enabled_->isEnabled()) {
    subtitle_tab_->SetFormat(current_format);
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
  loading_presets_ = true;

  preset_combobox_->clear();
  presets_.clear();

  preset_combobox_->addItem(tr("Default"), kPresetDefault);

  if (viewer_node_->GetLastUsedEncodingParams().IsValid()) {
    preset_combobox_->addItem(tr("Last Used"), kPresetLastUsed);
  }

  preset_combobox_->insertSeparator(preset_combobox_->count());

  QStringList l = EncodingParams::GetListOfPresets();
  presets_.reserve(l.size());

  for (const QString &preset : l) {
    EncodingParams p;

    QFile f(EncodingParams::GetPresetPath().filePath(preset));
    if (f.open(QFile::ReadOnly)) {
      if (p.Load(&f)) {
        preset_combobox_->addItem(preset, int(presets_.size()));
        presets_.push_back(p);
      }
      f.close();
    }
  }

  loading_presets_ = false;
}

void ExportDialog::SetDefaultFilename()
{
  Project* p = viewer_node_->project();

  QDir doc_location;

  if (p->filename().isEmpty()) {
    doc_location.setPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  } else {
    doc_location = QFileInfo(p->filename()).dir();
  }

  QString file_location = doc_location.filePath(viewer_node_->GetLabel());
  filename_edit_->setText(file_location);
}

bool ExportDialog::SequenceHasSubtitles() const
{
  if (Sequence *s = dynamic_cast<Sequence*>(viewer_node_)) {
    TrackList *tl = s->track_list(Track::kSubtitle);
    for (Track *t : tl->GetTracks()) {
      if (!t->IsMuted() && !t->Blocks().empty()) {
        return true;
      }
    }
  }

  return false;
}

void ExportDialog::SetDefaults()
{
  if (!stills_only_mode_) {
    format_combobox_->SetFormat(ExportFormat::kFormatMPEG4Video);
  } else {
    format_combobox_->SetFormat(ExportFormat::kFormatPNG);
  }
  FormatChanged(format_combobox_->GetFormat());

  VideoParams vp = viewer_node_->GetVideoParams();
  AudioParams ap = viewer_node_->GetAudioParams();

  video_tab_->width_slider()->SetValue(vp.width());
  video_tab_->width_slider()->SetDefaultValue(vp.width());
  video_tab_->height_slider()->SetValue(vp.height());
  video_tab_->height_slider()->SetDefaultValue(vp.height());
  video_tab_->SetSelectedFrameRate(vp.frame_rate());
  video_tab_->pixel_aspect_combobox()->SetPixelAspectRatio(vp.pixel_aspect_ratio());
  video_tab_->pixel_format_field()->SetPixelFormat(static_cast<PixelFormat::Format>(OLIVE_CONFIG("OnlinePixelFormat").toInt()));
  video_tab_->interlaced_combobox()->SetInterlaceMode(vp.interlacing());
  audio_tab_->sample_rate_combobox()->SetSampleRate(ap.sample_rate());
  audio_tab_->sample_format_combobox()->SetAttemptToRestoreFormat(false);
  audio_tab_->channel_layout_combobox()->SetChannelLayout(ap.channel_layout());
  subtitles_enabled_->setChecked(SequenceHasSubtitles());
  subtitle_tab_->SetSidecarFormat(ExportFormat::kFormatSRT);
}

EncodingParams ExportDialog::GenerateParams() const
{
  VideoParams video_render_params(static_cast<int>(video_tab_->width_slider()->GetValue()),
                                  static_cast<int>(video_tab_->height_slider()->GetValue()),
                                  GetSelectedTimebase(),
                                  video_tab_->pixel_format_field()->GetPixelFormat(),
                                  VideoParams::kInternalChannelCount,
                                  video_tab_->pixel_aspect_combobox()->GetPixelAspectRatio(),
                                  video_tab_->interlaced_combobox()->GetInterlaceMode(),
                                  1);

  AudioParams audio_render_params(audio_tab_->sample_rate_combobox()->GetSampleRate(),
                                  audio_tab_->channel_layout_combobox()->GetChannelLayout(),
                                  audio_tab_->sample_format_combobox()->GetSampleFormat());

  EncodingParams params;
  params.set_format(format_combobox_->GetFormat());
  params.SetFilename(filename_edit_->text().trimmed());
  params.SetExportLength(viewer_node_->GetLength());

  if (ExportCodec::IsCodecAStillImage(video_tab_->GetSelectedCodec()) && !video_tab_->IsImageSequenceSet()) {
    // Exporting as image without exporting image sequence, only export one frame
    rational export_time = video_tab_->GetStillImageTime();
    params.set_custom_range(TimeRange(export_time, export_time + GetSelectedTimebase()));
  } else if (range_combobox_->currentIndex() == kRangeInToOut) {
    // Assume if this combobox is enabled, workarea is enabled - a check that we make in this dialog's constructor
    params.set_custom_range(viewer_node_->GetWorkArea()->range());
  }

  if (video_tab_->scaling_method_combobox()->isEnabled()) {
    params.set_video_scaling_method(static_cast<EncodingParams::VideoScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()));
  }

  if (video_enabled_->isChecked()) {
    ExportCodec::Codec video_codec = video_tab_->GetSelectedCodec();

    video_render_params.set_color_range(video_tab_->color_range());

    params.EnableVideo(video_render_params, video_codec);

    params.set_video_threads(video_tab_->threads());

    if (video_tab_->isVisible()) {
      video_tab_->GetCodecSection()->AddOpts(&params);
    }

    params.set_color_transform(video_tab_->CurrentOCIOColorSpace());

    params.set_video_pix_fmt(video_tab_->pix_fmt());

    params.set_video_is_image_sequence(video_tab_->IsImageSequenceSet());
  }

  if (audio_enabled_->isChecked()) {
    ExportCodec::Codec audio_codec = audio_tab_->GetCodec();
    params.EnableAudio(audio_render_params, audio_codec);

    params.set_audio_bit_rate(audio_tab_->bit_rate_slider()->GetValue() * 1000);
  }

  if (subtitles_enabled_->isEnabled()
      && subtitles_enabled_->isChecked()) {
    if (!subtitle_tab_->GetSidecarEnabled()) {
      // Export subtitles embedded in container
      params.EnableSubtitles(subtitle_tab_->GetSubtitleCodec());
    } else {
      // Export subtitles to a sidecar file
      params.EnableSidecarSubtitles(subtitle_tab_->GetSidecarFormat(), subtitle_tab_->GetSubtitleCodec());
    }
  }

  return params;
}

void ExportDialog::SetParams(const EncodingParams &e)
{
  format_combobox_->SetFormat(e.format());
  FormatChanged(format_combobox_->GetFormat());

  if (e.has_custom_range() && viewer_node_->GetWorkArea()->enabled()) {
    range_combobox_->setCurrentIndex(kRangeInToOut);
  }

  QtUtils::SetComboBoxData(video_tab_->scaling_method_combobox(), e.video_scaling_method());

  video_enabled_->setChecked(e.video_enabled());
  if (e.video_enabled()) {
    video_tab_->width_slider()->SetValue(e.video_params().width());
    video_tab_->height_slider()->SetValue(e.video_params().height());
    SetSelectedTimebase(e.video_params().time_base());
    video_tab_->pixel_format_field()->SetPixelFormat(e.video_params().format());
    video_tab_->pixel_aspect_combobox()->SetPixelAspectRatio(e.video_params().pixel_aspect_ratio());
    video_tab_->interlaced_combobox()->SetInterlaceMode(e.video_params().interlacing());

    video_tab_->SetSelectedCodec(e.video_codec());

    video_tab_->SetColorRange(e.video_params().color_range());

    video_tab_->SetThreads(e.video_threads());

    if (video_tab_->isVisible()) {
      video_tab_->GetCodecSection()->SetOpts(&e);
    }

    video_tab_->SetOCIOColorSpace(e.color_transform().output());

    video_tab_->SetPixFmt(e.video_pix_fmt());

    video_tab_->SetImageSequence(e.video_is_image_sequence());
  }

  audio_enabled_->setChecked(e.audio_enabled());
  if (e.audio_enabled()) {
    audio_tab_->sample_rate_combobox()->SetSampleRate(e.audio_params().sample_rate());
    audio_tab_->channel_layout_combobox()->SetChannelLayout(e.audio_params().channel_layout());
    audio_tab_->sample_format_combobox()->SetSampleFormat(e.audio_params().format());

    audio_tab_->SetCodec(e.audio_codec());

    audio_tab_->bit_rate_slider()->SetValue(e.audio_bit_rate() / 1000);
  }

  if (subtitles_enabled_->isEnabled()) {
    subtitles_enabled_->setChecked(e.subtitles_enabled());
    subtitle_tab_->SetSidecarEnabled(e.subtitles_are_sidecar());
    if (e.subtitles_enabled()) {
      subtitle_tab_->SetSubtitleCodec(e.subtitles_codec());
      if (e.subtitles_are_sidecar()) {
        subtitle_tab_->SetSidecarFormat(e.subtitle_sidecar_fmt());
      }
    }
  }
}

bool ExportDialog::eventFilter(QObject *o, QEvent *e)
{
  // Any parameters in scrollable areas, ignore wheel events so the user doesn't unwittingly change
  // them while trying to scroll through the pages
  if (e->type() == QEvent::Wheel) {
    while ((o = o->parent())) {
      if (o == video_tab_ || o == audio_tab_ || o == subtitle_tab_) {
        e->ignore();
        return true;
      }
    }
  }

  return super::eventFilter(o, e);
}

void ExportDialog::done(int r)
{
  preview_viewer_->ConnectViewerNode(nullptr);

  if (!stills_only_mode_) {
    viewer_node_->SetLastUsedEncodingParams(GenerateParams());
  }

  super::done(r);
}

rational ExportDialog::GetExportLength() const
{
  if (range_combobox_->currentIndex() == kRangeInToOut) {
    return viewer_node_->GetWorkArea()->range().length();
  } else {
    return viewer_node_->GetLength();
  }
}

int64_t ExportDialog::GetExportLengthInTimebaseUnits() const
{
  return Timecode::time_to_timestamp(GetExportLength(), GetSelectedTimebase());
}

void ExportDialog::UpdateViewerDimensions()
{
  preview_viewer_->SetViewerResolution(static_cast<int>(video_tab_->width_slider()->GetValue()),
                                       static_cast<int>(video_tab_->height_slider()->GetValue()));

  VideoParams vp = viewer_node_->GetVideoParams();

  QMatrix4x4 transform = EncodingParams::GenerateMatrix(
        static_cast<EncodingParams::VideoScalingMethod>(video_tab_->scaling_method_combobox()->currentData().toInt()),
        vp.width(),
        vp.height(),
        static_cast<int>(video_tab_->width_slider()->GetValue()),
        static_cast<int>(video_tab_->height_slider()->GetValue())
        );

  preview_viewer_->SetMatrix(transform);
}

}
