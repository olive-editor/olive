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

#include "text.h"

#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextDocument>

#include "common/cpuoptimize.h"
#include "common/functiontimer.h"

namespace olive {

#define super ShapeNodeBase

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGenerator::kTextInput = QStringLiteral("text_in");
const QString TextGenerator::kHtmlInput = QStringLiteral("html_in");
const QString TextGenerator::kVAlignInput = QStringLiteral("valign_in");
const QString TextGenerator::kFontInput = QStringLiteral("font_in");
const QString TextGenerator::kFontSizeInput = QStringLiteral("font_size_in");

TextGenerator::TextGenerator()
{
  AddInput(kTextInput, NodeValue::kText, tr("Sample Text"));

  AddInput(kHtmlInput, NodeValue::kBoolean, false);

  AddInput(kVAlignInput, NodeValue::kCombo, kVerticalAlignTop);

  AddInput(kFontInput, NodeValue::kFont);

  AddInput(kFontSizeInput, NodeValue::kFloat, 72.0f);

  SetStandardValue(kColorInput, QVariant::fromValue(Color(1.0f, 1.0f, 1.0)));
  SetStandardValue(kSizeInput, QVector2D(400, 300));
}

QString TextGenerator::Name() const
{
  return tr("Text");
}

QString TextGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.text2");
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
  super::Retranslate();

  SetInputName(kTextInput, tr("Text"));
  SetInputName(kHtmlInput, tr("Enable HTML"));
  SetInputName(kFontInput, tr("Font"));
  SetInputName(kFontSizeInput, tr("Font Size"));
  SetInputName(kVAlignInput, tr("Vertical Align"));
  SetComboBoxStrings(kVAlignInput, {tr("Top"), tr("Center"), tr("Bottom")});
}

void TextGenerator::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  GenerateJob job;
  job.InsertValue(value);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
  job.SetRequestedFormat(VideoParams::kFormatFloat32);

  if (!job.GetValue(kTextInput).data().toString().isEmpty()) {
    table->Push(NodeValue::kGenerateJob, QVariant::fromValue(job), this);
  }
}

void TextGenerator::GenerateFrame(FramePtr frame, const GenerateJob& job) const
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
  default_font.setFamily(job.GetValue(kFontInput).data().toString());
  default_font.setPointSizeF(job.GetValue(kFontSizeInput).data().toFloat());
  text_doc.setDefaultFont(default_font);

  QString html = job.GetValue(kTextInput).data().toString();
  if (job.GetValue(kHtmlInput).data().toBool()) {
    html.replace('\n', QStringLiteral("<br>"));
    text_doc.setHtml(html);
  } else {
    text_doc.setPlainText(html);
  }

  QVector2D size = job.GetValue(kSizeInput).data().value<QVector2D>();
  text_doc.setTextWidth(size.x());

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());


  QVector2D pos = job.GetValue(kPositionInput).data().value<QVector2D>();
  p.translate(pos.x() - size.x()/2, pos.y() - size.y()/2);
  p.translate(frame->video_params().width()/2, frame->video_params().height()/2);
  p.setClipRect(0, 0, size.x(), size.y());

  TextVerticalAlign valign = static_cast<TextVerticalAlign>(job.GetValue(kVAlignInput).data().toInt());
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
  Color rgba = job.GetValue(kColorInput).data().value<Color>();
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
