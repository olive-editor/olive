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

#include "textv1.h"

#include <QAbstractTextDocumentLayout>
#include <QTextDocument>

namespace olive {

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGeneratorV1::kTextInput = QStringLiteral("text_in");
const QString TextGeneratorV1::kHtmlInput = QStringLiteral("html_in");
const QString TextGeneratorV1::kColorInput = QStringLiteral("color_in");
const QString TextGeneratorV1::kVAlignInput = QStringLiteral("valign_in");
const QString TextGeneratorV1::kFontInput = QStringLiteral("font_in");
const QString TextGeneratorV1::kFontSizeInput = QStringLiteral("font_size_in");

#define super Node

TextGeneratorV1::TextGeneratorV1()
{
  AddInput(kTextInput, NodeValue::kText, tr("Sample Text"));

  AddInput(kHtmlInput, NodeValue::kBoolean, false);

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0f, 1.0f, 1.0)));

  AddInput(kVAlignInput, NodeValue::kCombo, 1);

  AddInput(kFontInput, NodeValue::kFont);

  AddInput(kFontSizeInput, NodeValue::kFloat, 72.0f);

  SetFlag(kDontShowInCreateMenu);
}

QString TextGeneratorV1::Name() const
{
  return tr("Text (Legacy)");
}

QString TextGeneratorV1::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.textgenerator");
}

QVector<Node::CategoryID> TextGeneratorV1::Category() const
{
  return {kCategoryGenerator};
}

QString TextGeneratorV1::Description() const
{
  return tr("Generate rich text.");
}

void TextGeneratorV1::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextInput, tr("Text"));
  SetInputName(kHtmlInput, tr("Enable HTML"));
  SetInputName(kFontInput, tr("Font"));
  SetInputName(kFontSizeInput, tr("Font Size"));
  SetInputName(kColorInput, tr("Color"));
  SetInputName(kVAlignInput, tr("Vertical Align"));
  SetComboBoxStrings(kVAlignInput, {tr("Top"), tr("Center"), tr("Bottom")});
}

void TextGeneratorV1::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (!value[kTextInput].toString().isEmpty()) {
    table->Push(NodeValue::kTexture, Texture::Job(globals.vparams(), GenerateJob(value)), this);
  }
}

void TextGeneratorV1::GenerateFrame(FramePtr frame, const GenerateJob& job) const
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
  default_font.setFamily(job.Get(kFontInput).toString());
  default_font.setPointSizeF(job.Get(kFontSizeInput).toDouble());
  text_doc.setDefaultFont(default_font);

  // Center by default
  text_doc.setDefaultTextOption(QTextOption(Qt::AlignCenter));

  QString html = job.Get(kTextInput).toString();
  if (job.Get(kHtmlInput).toBool()) {
    html.replace('\n', QStringLiteral("<br>"));
    text_doc.setHtml(html);
  } else {
    text_doc.setPlainText(html);
  }

  // Align to 80% width because that's considered the "title safe" area
  int tenth_of_width = frame->video_params().width() / 10;
  text_doc.setTextWidth(tenth_of_width * 8);

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());

  // Push 10% inwards to compensate for title safe area
  p.translate(tenth_of_width, 0);

  TextVerticalAlign valign = static_cast<TextVerticalAlign>(job.Get(kVAlignInput).toInt());
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

  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette.setColor(QPalette::Text, Qt::white);
  text_doc.documentLayout()->draw(&p, ctx);

  // Transplant alpha channel to frame
  Color rgb = job.Get(kColorInput).toColor();
  for (int x=0; x<frame->width(); x++) {
    for (int y=0; y<frame->height(); y++) {
      uchar src_alpha = img.bits()[img.bytesPerLine() * y + x];
      float alpha = float(src_alpha) / 255.0f;

      frame->set_pixel(x, y, Color(rgb.red() * alpha, rgb.green() * alpha, rgb.blue() * alpha, alpha));
    }
  }
}

}
