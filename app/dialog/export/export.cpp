#include "export.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardPaths>

#include "ui/icons/icons.h"

ExportDialog::ExportDialog(QWidget *parent) :
  QDialog(parent),
  previously_selected_format_(0)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  QWidget* preferences_area = new QWidget();
  QGridLayout* preferences_layout = new QGridLayout(preferences_area);

  int row = 0;

  QLabel* fn_lbl = new QLabel(tr("Filename:"));
  fn_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(fn_lbl, row, 0);

  filename_edit_ = new QLineEdit();
  preferences_layout->addWidget(filename_edit_, row, 1, 1, 2);

  QPushButton* file_browse_btn = new QPushButton();
  file_browse_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  file_browse_btn->setIcon(olive::icon::Folder);
  file_browse_btn->setToolTip(tr("Browse for exported file filename"));
  connect(file_browse_btn, SIGNAL(clicked(bool)), this, SLOT(BrowseFilename()));
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
  preset_load_btn->setIcon(olive::icon::Open);
  preset_load_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_load_btn, row, 2);

  QPushButton* preset_save_btn = new QPushButton();
  preset_save_btn->setIcon(olive::icon::Save);
  preset_save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_save_btn, row, 3);

  row++;

  QFrame* horizontal_line = new QFrame();
  horizontal_line->setFrameShape(QFrame::HLine);
  horizontal_line->setFrameShadow(QFrame::Sunken);
  preferences_layout->addWidget(horizontal_line, row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  format_combobox_ = new QComboBox();
  connect(format_combobox_, SIGNAL(currentIndexChanged(int)), this, SLOT(FormatChanged(int)));
  preferences_layout->addWidget(format_combobox_, row, 1, 1, 3);

  row++;

  QTabWidget* preferences_tabs = new QTabWidget();
  QScrollArea* video_area = new QScrollArea();
  video_tab_ = new ExportVideoTab();
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

  QDialogButtonBox* buttons = new QDialogButtonBox();
  buttons->setCenterButtons(true);
  buttons->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
  buttons->addButton(QDialogButtonBox::Cancel);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  preferences_layout->addWidget(buttons, row, 0, 1, 4);

  splitter->addWidget(preferences_area);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_viewer_ = new ViewerWidget();
  preview_viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  preview_layout->addWidget(preview_viewer_);
  splitter->addWidget(preview_area);

  // Set default filename
  // FIXME: Use Sequence name and project filename to construct this
  SetDefaultFilename();

  // Set up available export formats
  SetUpFormats();

  // Populate combobox formats
  foreach (const ExportFormat& format, formats_) {
    format_combobox_->addItem(format.name());
  }

  // Set defaults
  format_combobox_->setCurrentIndex(kFormatMPEG4);
}

void ExportDialog::BrowseFilename()
{
  const ExportFormat& current_format = formats_.at(format_combobox_->currentIndex());

  QString browsed_fn = QFileDialog::getSaveFileName(this,
                                                    "",
                                                    filename_edit_->text(),
                                                    QStringLiteral("%1 (*.%2)").arg(current_format.name(), current_format.extension()));

  if (!browsed_fn.isEmpty()) {
    filename_edit_->setText(browsed_fn);
  }
}

void ExportDialog::FormatChanged(int index)
{
  QString current_filename = filename_edit_->text();
  const QString& previously_selected_ext = formats_.at(previously_selected_format_).extension();
  const ExportFormat& current_format = formats_.at(index);
  const QString& currently_selected_ext = current_format.extension();

  // If the previous extension was added, remove it
  if (current_filename.endsWith(previously_selected_ext, Qt::CaseInsensitive)) {
    current_filename.resize(current_filename.size() - previously_selected_ext.size() - 1);
  }

  // Add the extension and set it
  current_filename.append('.');
  current_filename.append(currently_selected_ext);
  filename_edit_->setText(current_filename);

  previously_selected_format_ = index;

  // Update video and audio comboboxes
  video_tab_->codec_combobox()->clear();
  foreach (int vcodec, current_format.video_codecs()) {
    video_tab_->codec_combobox()->addItem(codecs_.at(vcodec).name());
  }

  audio_tab_->codec_combobox()->clear();
  foreach (int acodec, current_format.audio_codecs()) {
    audio_tab_->codec_combobox()->addItem(codecs_.at(acodec).name());
  }
}

void ExportDialog::SetUpFormats()
{
  // Set up codecs
  for (int i=0;i<kCodecCount;i++) {
    switch (static_cast<Codec>(i)) {
    case kCodecDNxHD:
      codecs_.append(ExportCodec(tr("DNxHD"), "dnxhd"));
      break;
    case kCodecH264:
      codecs_.append(ExportCodec(tr("H.264"), "libx264"));
      break;
    case kCodecH265:
      codecs_.append(ExportCodec(tr("H.265"), "libx265"));
      break;
    case kCodecOpenEXR:
      codecs_.append(ExportCodec(tr("OpenEXR"), "exr", ExportCodec::kStillImage));
      break;
    case kCodecPNG:
      codecs_.append(ExportCodec(tr("PNG"), "png", ExportCodec::kStillImage));
      break;
    case kCodecProRes:
      codecs_.append(ExportCodec(tr("ProRes"), "prores"));
      break;
    case kCodecTIFF:
      codecs_.append(ExportCodec(tr("TIFF"), "tiff", ExportCodec::kStillImage));
      break;
    case kCodecMP2:
      codecs_.append(ExportCodec(tr("MP2"), "mp2"));
      break;
    case kCodecMP3:
      codecs_.append(ExportCodec(tr("MP3"), "libmp3lame"));
      break;
    case kCodecAAC:
      codecs_.append(ExportCodec(tr("AAC"), "aac"));
      break;
    case kCodecPCMS16LE:
      codecs_.append(ExportCodec(tr("PCM (Signed 16-Bit Little Endian)"), "pcms16le"));
      break;
    case kCodecCount:
      break;
    }
  }

  // Set up formats
  for (int i=0;i<kFormatCount;i++) {
    switch (static_cast<Format>(i)) {
    case kFormatDNxHD:
      formats_.append(ExportFormat(tr("DNxHD"), "mxf", {kCodecDNxHD}, {kCodecPCMS16LE}));
      break;
    case kFormatMatroska:
      formats_.append(ExportFormat(tr("Matroska Video"), "mkv", {kCodecH264, kCodecH265}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCMS16LE}));
      break;
    case kFormatMPEG4:
      formats_.append(ExportFormat(tr("MPEG-4 Video"), "mp4", {kCodecH264, kCodecH265}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCMS16LE}));
      break;
    case kFormatOpenEXR:
      formats_.append(ExportFormat(tr("OpenEXR"), "exr", {kCodecOpenEXR}, {}));
      break;
    case kFormatQuickTime:
      formats_.append(ExportFormat(tr("QuickTime"), "mov", {kCodecH264, kCodecH265, kCodecProRes}, {kCodecAAC, kCodecMP2, kCodecMP3, kCodecPCMS16LE}));
      break;
    case kFormatPNG:
      formats_.append(ExportFormat(tr("PNG"), "png", {kCodecPNG}, {}));
      break;
    case kFormatTIFF:
      formats_.append(ExportFormat(tr("TIFF"), "tiff", {kCodecTIFF}, {}));
      break;
    case kFormatCount:
      break;
    }
  }
}

void ExportDialog::LoadPresets()
{

}

void ExportDialog::SetDefaultFilename()
{
  QString doc_location = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString file_location = QDir(doc_location).filePath("export");
  filename_edit_->setText(file_location);
}
