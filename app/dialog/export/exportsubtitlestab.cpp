#include "exportsubtitlestab.h"

#include <QGridLayout>

namespace olive {

ExportSubtitlesTab::ExportSubtitlesTab(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  sidecar_checkbox_ = new QCheckBox(tr("Export to sidecar file"));
  layout->addWidget(sidecar_checkbox_, row, 0, 1, 2);

  row++;

  sidecar_format_label_ = new QLabel(tr("Sidecar Format:"));
  sidecar_format_label_->setVisible(false);
  layout->addWidget(sidecar_format_label_, row, 0);

  sidecar_format_combobox_ = new ExportFormatComboBox(ExportFormatComboBox::kShowSubtitlesOnly);
  sidecar_format_combobox_->setVisible(true);
  layout->addWidget(sidecar_format_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  layout->addWidget(codec_combobox_, row, 1);

  outer_layout->addStretch();

  connect(sidecar_checkbox_, &QCheckBox::toggled, sidecar_format_label_, &QWidget::setVisible);
  connect(sidecar_checkbox_, &QCheckBox::toggled, sidecar_format_combobox_, &QWidget::setVisible);
}

int ExportSubtitlesTab::SetFormat(ExportFormat::Format format)
{
  auto vcodecs = ExportFormat::GetVideoCodecs(format);
  auto acodecs = ExportFormat::GetAudioCodecs(format);

  auto scodecs = ExportFormat::GetSubtitleCodecs(format);

  if (!scodecs.empty() && vcodecs.empty() && acodecs.empty()) {
    // If format supports ONLY scodecs, default this to off and disable it
    sidecar_checkbox_->setChecked(false);
    sidecar_checkbox_->setEnabled(false);
  } else {
    // If format does not support scodecs, default this to checked and disable it
    sidecar_checkbox_->setChecked(scodecs.empty());
    sidecar_checkbox_->setEnabled(!scodecs.empty());
  }

  scodecs = ExportFormat::GetSubtitleCodecs(sidecar_format_combobox_->GetFormat());

  codec_combobox_->clear();
  foreach (ExportCodec::Codec scodec, scodecs) {
    codec_combobox_->addItem(ExportCodec::GetCodecName(scodec), scodec);
  }

  return scodecs.size();
}

}
