#include "exportsubtitlestab.h"

#include <QGridLayout>
#include <QLabel>

namespace olive {

ExportSubtitlesTab::ExportSubtitlesTab(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  layout->addWidget(codec_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Encoding:")), row, 0);

  encoding_combobox_ = new QComboBox();
  for (int i=0; i<SubtitleParams::kEncodingCount; i++) {
    encoding_combobox_->addItem(SubtitleParams::GetEncodingName(static_cast<SubtitleParams::Encoding>(i)), i);
  }
  layout->addWidget(encoding_combobox_, row, 1);

  outer_layout->addStretch();
}

int ExportSubtitlesTab::SetFormat(ExportFormat::Format format)
{
  auto scodecs = ExportFormat::GetSubtitleCodecs(format);
  setEnabled(!scodecs.isEmpty());
  codec_combobox_->clear();
  foreach (ExportCodec::Codec scodec, scodecs) {
    codec_combobox_->addItem(ExportCodec::GetCodecName(scodec), scodec);
  }
  return scodecs.size();
}

}
