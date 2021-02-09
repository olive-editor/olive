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

#include "text.h"

#include <QTextDocument>

namespace olive {

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGenerator::kTextInput = QStringLiteral("text_in");
const QString TextGenerator::kColorInput = QStringLiteral("color_in");
const QString TextGenerator::kVAlignInput = QStringLiteral("valign_in");
const QString TextGenerator::kFontInput = QStringLiteral("font_in");
const QString TextGenerator::kFontSizeInput = QStringLiteral("font_size_in");

TextGenerator::TextGenerator()
{
  AddInput(kTextInput, NodeValue::kText, tr("Sample Text"));

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0f, 1.0f, 1.0)));

  AddInput(kVAlignInput, NodeValue::kCombo, 1);

  AddInput(kFontInput, NodeValue::kFont);

  AddInput(kFontSizeInput, NodeValue::kFloat, 72.0f);
}

Node *TextGenerator::copy() const
{
  return new TextGenerator();
}

QString TextGenerator::Name() const
{
  return tr("Text");
}

QString TextGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.textgenerator");
}

QVector<Node::CategoryID> TextGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString TextGenerator::Description() const
{
  return tr("Generate rich text.");
}

void TextGenerator::Retranslate()
{
  SetInputName(kTextInput, tr("Text"));
  SetInputName(kFontInput, tr("Font"));
  SetInputName(kFontSizeInput, tr("Font Size"));
  SetInputName(kColorInput, tr("Color"));
  SetInputName(kVAlignInput, tr("Vertical Align"));
  SetComboBoxStrings(kVAlignInput, {tr("Top"), tr("Center"), tr("Bottom")});
}

NodeValueTable TextGenerator::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  GenerateJob job;
  job.InsertValue(this, kTextInput, value);
  job.InsertValue(this, kColorInput, value);
  job.InsertValue(this, kVAlignInput, value);
  job.InsertValue(this, kFontInput, value);
  job.InsertValue(this, kFontSizeInput, value);
  job.SetAlphaChannelRequired(true);

  NodeValueTable table = value.Merge();

  if (!job.GetValue(kTextInput).data().toString().isEmpty()) {
    table.Push(NodeValue::kGenerateJob, QVariant::fromValue(job), this);
  }

  return table;
}

void TextGenerator::GenerateFrame(FramePtr frame, const GenerateJob& job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img(frame->width(), frame->height(), QImage::Format_Grayscale8);
  img.fill(0);

  QTextDocument text_doc;

  // Set default font
  QFont default_font;
  default_font.setFamily(job.GetValue(kFontInput).data().toString());
  default_font.setPointSizeF(job.GetValue(kFontSizeInput).data().toFloat());
  text_doc.setDefaultFont(default_font);

  // Center by default
  text_doc.setDefaultTextOption(QTextOption(Qt::AlignCenter));

  text_doc.setHtml(job.GetValue(kTextInput).data().toString());

  // Align to 80% width because that's considered the "title safe" area
  int tenth_of_width = frame->video_params().width() / 10;
  text_doc.setTextWidth(tenth_of_width * 8);

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());

  // Push 10% inwards to compensate for title safe area
  p.translate(tenth_of_width, 0);

  TextVerticalAlign valign = static_cast<TextVerticalAlign>(job.GetValue(kVAlignInput).data().toInt());
  int doc_height = text_doc.size().height();

  switch (valign) {
  case kVerticalAlignTop:
    // Push 10% inwards for title safe area
    p.translate(0, frame->video_params().height() / 10);
    break;
  case kVerticalAlignCenter:
    // Center align
    p.translate(0, frame->video_params().height() / 2 - doc_height / 2);
    break;
  case kVerticalAlignBottom:
    // Push 10% inwards for title safe area
    p.translate(0, frame->video_params().height() - doc_height - frame->video_params().height() / 10);
    break;
  }

  text_doc.drawContents(&p);

  // Transplant alpha channel to frame
  Color rgb = job.GetValue(kColorInput).data().value<Color>();
  for (int x=0; x<frame->width(); x++) {
    for (int y=0; y<frame->height(); y++) {
      uchar src_alpha = img.bits()[img.bytesPerLine() * y + x];
      float alpha = float(src_alpha) / 255.0f;

      frame->set_pixel(x, y, Color(rgb.red() * alpha, rgb.green() * alpha, rgb.blue() * alpha, alpha));
    }
  }
}

}
