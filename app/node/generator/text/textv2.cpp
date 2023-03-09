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

#include "textv2.h"

#include <olive/core/core.h>
#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextDocument>

namespace olive {

#define super ShapeNodeBase

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGeneratorV2::kTextInput = QStringLiteral("text_in");
const QString TextGeneratorV2::kHtmlInput = QStringLiteral("html_in");
const QString TextGeneratorV2::kVAlignInput = QStringLiteral("valign_in");
const QString TextGeneratorV2::kFontInput = QStringLiteral("font_in");
const QString TextGeneratorV2::kFontSizeInput = QStringLiteral("font_size_in");

TextGeneratorV2::TextGeneratorV2()
{
  AddInput(kTextInput, NodeValue::kText, tr("Sample Text"));

  AddInput(kHtmlInput, NodeValue::kBoolean, false);

  AddInput(kVAlignInput, NodeValue::kCombo, kVerticalAlignTop);

  AddInput(kFontInput, NodeValue::kFont);

  AddInput(kFontSizeInput, NodeValue::kFloat, 72.0f);

  SetStandardValue(kColorInput, QVariant::fromValue(Color(1.0f, 1.0f, 1.0)));
  SetStandardValue(kSizeInput, QVector2D(400, 300));

  SetFlag(kDontShowInCreateMenu);
}

QString TextGeneratorV2::Name() const
{
  return tr("Text (Legacy)");
}

QString TextGeneratorV2::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.text2");
}

QVector<Node::CategoryID> TextGeneratorV2::Category() const
{
  return {kCategoryGenerator};
}

QString TextGeneratorV2::Description() const
{
  return tr("Generate rich text.");
}

void TextGeneratorV2::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextInput, tr("Text"));
  SetInputName(kHtmlInput, tr("Enable HTML"));
  SetInputName(kFontInput, tr("Font"));
  SetInputName(kFontSizeInput, tr("Font Size"));
  SetInputName(kVAlignInput, tr("Vertical Align"));
  SetComboBoxStrings(kVAlignInput, {tr("Top"), tr("Center"), tr("Bottom")});
}

void TextGeneratorV2::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (!value[kTextInput].toString().isEmpty()) {
    GenerateJob job(value);
    auto text_params = globals.vparams();
    text_params.set_format(PixelFormat::F32);
    table->Push(NodeValue::kTexture, Texture::Job(text_params, job), this);
  }
}

void TextGeneratorV2::GenerateFrame(FramePtr frame, const GenerateJob& job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img(frame->width(), frame->height(), QImage::Format_Grayscale8);
  img.fill(Qt::transparent);

  // 72 DPI in DPM (72 / 2.54 * 100)
  const int dpm = 2835;
  img.setDotsPerMeterX(dpm);
  img.setDotsPerMeterY(dpm);

  QTextDocument text_doc;
  text_doc.documentLayout()->setPaintDevice(&img);

  // Set default font
  QFont default_font;
  default_font.setFamily(job.Get(kFontInput).toString());
  default_font.setPointSizeF(job.Get(kFontSizeInput).toDouble());
  text_doc.setDefaultFont(default_font);

  QString html = job.Get(kTextInput).toString();
  if (job.Get(kHtmlInput).toBool()) {
    html.replace('\n', QStringLiteral("<br>"));
    text_doc.setHtml(html);
  } else {
    text_doc.setPlainText(html);
  }

  QVector2D size = job.Get(kSizeInput).toVec2();
  text_doc.setTextWidth(size.x());

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());


  QVector2D pos = job.Get(kPositionInput).toVec2();
  p.translate(pos.x() - size.x()/2, pos.y() - size.y()/2);
  p.translate(frame->video_params().width()/2, frame->video_params().height()/2);
  p.setClipRect(0, 0, size.x(), size.y());

  TextVerticalAlign valign = static_cast<TextVerticalAlign>(job.Get(kVAlignInput).toInt());
  int doc_height = text_doc.size().height();

  switch (valign) {
  case kVerticalAlignTop:
    // Do nothing
    break;
  case kVerticalAlignCenter:
    // Center align
    p.translate(0, size.y() / 2 - doc_height / 2);
    break;
  case kVerticalAlignBottom:
    p.translate(0, size.y() - doc_height);
    break;
  }

  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette.setColor(QPalette::Text, Qt::white);

  text_doc.documentLayout()->draw(&p, ctx);

  // Transplant alpha channel to frame
  Color rgba = job.Get(kColorInput).toColor();
#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
  __m128 sse_color = _mm_loadu_ps(rgba.data());
#endif

  float *frame_dst = reinterpret_cast<float*>(frame->data());
  for (int y=0; y<frame->height(); y++) {
    uchar *src_y = img.bits() + img.bytesPerLine() * y;
    float *dst_y = frame_dst + y*frame->linesize_pixels()*VideoParams::kRGBAChannelCount;

    for (int x=0; x<frame->width(); x++) {
      float alpha = float(src_y[x]) / 255.0f;
      float *dst = dst_y + x*VideoParams::kRGBAChannelCount;

#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
      __m128 sse_alpha = _mm_load1_ps(&alpha);
      __m128 sse_res = _mm_mul_ps(sse_color, sse_alpha);

      _mm_store_ps(dst, sse_res);
#else
      for (int i=0; i<VideoParams::kRGBAChannelCount; i++) {
        dst[i] = rgba.data()[i] * alpha;
      }
#endif
    }
  }
}

}
