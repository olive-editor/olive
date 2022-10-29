#include "html.h"

#include <QDebug>
#include <QTextBlock>

#include "xmlutils.h"

namespace olive {

const QVector<QString> Html::kBlockTags = {
  QStringLiteral("p"),
  QStringLiteral("div")
};

inline bool StrEquals(const QStringView &a, const QStringView &b)
{
  return !a.compare(b, Qt::CaseInsensitive);
}

QString Html::DocToHtml(const QTextDocument *doc)
{
  QString html;
  QXmlStreamWriter writer(&html);

  //writer.setAutoFormatting(true);

  for (auto it=doc->begin(); it!=doc->end(); it=it.next()) {
    WriteBlock(&writer, it);
  }

  return html;
}

struct HtmlNode {
  QString tag;
  QTextCharFormat format;
};

QTextCharFormat MergeHtmlFormats(const QVector<HtmlNode> &stack)
{
  QTextCharFormat f;

  for (int i=0; i<stack.size(); i++) {
    f.merge(stack.at(i).format);
  }

  return f;
}

void Html::HtmlToDoc(QTextDocument *doc, const QString &html)
{
  // Empty doc
  doc->clear();
  bool inside_block = true;

  // Create cursor, which appears to be Qt's official way of inserting blocks and fragments
  QTextCursor c(doc);

  QString wrapped = QStringLiteral("<html>").append(html).append("</html>");
  QXmlStreamReader reader(wrapped);

  QVector<HtmlNode> fmt_stack;

  QTextCharFormat default_fmt;
  default_fmt.setFontWeight(QFont::Normal);
  fmt_stack.append({QStringLiteral("html"), default_fmt});

  QTextCharFormat current_fmt;

  while (!reader.atEnd()) {
    reader.readNext();

    if (reader.tokenType() == QXmlStreamReader::StartElement) {

      QString tag = reader.name().toString().toLower();

      fmt_stack.append({tag, ReadCharFormat(reader.attributes())});
      current_fmt = MergeHtmlFormats(fmt_stack);

      if (kBlockTags.contains(tag)) {
        QTextBlockFormat block_fmt = ReadBlockFormat(reader.attributes());
        if (inside_block) {
          c.setBlockFormat(block_fmt);
          c.setBlockCharFormat(current_fmt);
        } else {
          c.insertBlock(block_fmt, current_fmt);
          inside_block = true;
        }
      }

    } else if (reader.tokenType() == QXmlStreamReader::Characters) {

      QString characters = reader.text().toString();
      c.insertText(characters, current_fmt);

    } else if (reader.tokenType() == QXmlStreamReader::EndElement) {

      QString tag = reader.name().toString().toLower();

      for (int i=fmt_stack.size()-1; i>=0; i--) {
        if (fmt_stack.at(i).tag == tag) {
          fmt_stack.removeAt(i);
          current_fmt = MergeHtmlFormats(fmt_stack);

          if (kBlockTags.contains(tag)) {
            inside_block = false;
          }
          break;
        }
      }

    }
  }

  if (reader.error()) {
    qCritical() << "Failed to parse HTML:" << reader.errorString();
  }
}

void Html::WriteBlock(QXmlStreamWriter *writer, const QTextBlock &block)
{
  writer->writeStartElement(QStringLiteral("p"));

  const QTextBlockFormat &fmt = block.blockFormat();

  // Write block alignment
  if (!(fmt.alignment() & Qt::AlignLeft)) {
    if (fmt.alignment() & Qt::AlignRight) {
      writer->writeAttribute(QStringLiteral("align"), QStringLiteral("right"));
    } else if (fmt.alignment() & Qt::AlignHCenter) {
      writer->writeAttribute(QStringLiteral("align"), QStringLiteral("center"));
    } else if (fmt.alignment() & Qt::AlignJustify) {
      writer->writeAttribute(QStringLiteral("align"), QStringLiteral("justify"));
    }
  }

  // RTL support
  if (block.textDirection() == Qt::RightToLeft) {
    writer->writeAttribute(QStringLiteral("dir"), QStringLiteral("rtl"));
  }

  // Write CSS attributes
  QString style;

  if (fmt.lineHeightType() != QTextBlockFormat::SingleHeight) {
    WriteCSSProperty(&style, QStringLiteral("line-height"), QStringLiteral("%1%").arg(fmt.lineHeight()));
  }

  WriteCharFormat(&style, block.charFormat());

  if (!style.isEmpty()) {
    writer->writeAttribute(QStringLiteral("style"), style);
  }

  auto it = block.begin();

  if (it != block.end()) {
    for (; it!=block.end(); it++) {
      WriteFragment(writer, it.fragment());
    }
  }

  writer->writeEndElement(); // p
}

void Html::WriteFragment(QXmlStreamWriter *writer, const QTextFragment &fragment)
{
  const QTextCharFormat &fmt = fragment.charFormat();

  writer->writeStartElement(QStringLiteral("span"));

  // Write CSS attributes
  QString style;

  WriteCharFormat(&style, fmt);

  if (!style.isEmpty()) {
    writer->writeAttribute(QStringLiteral("style"), style);
  }

  QStringList lines = fragment.text().split(QChar::LineSeparator);
  bool first_line = true;
  foreach (const QString &l, lines) {
    if (first_line) {
      first_line = false;
    } else {
      writer->writeEmptyElement(QStringLiteral("br"));
    }
    writer->writeCharacters(l);
  }

  writer->writeEndElement(); // span
}

void Html::WriteCSSProperty(QString *style, const QString &key, const QStringList &values)
{
  QString value;
  foreach (QString v, values) {
    if (v.contains(' ')) {
      v = QStringLiteral("'%1'").arg(v);
    }

    AppendStringAutoSpace(&value, v);
  }

  AppendStringAutoSpace(style, QStringLiteral("%1: %2;").arg(key, value));
}

void Html::WriteCharFormat(QString *style, const QTextCharFormat &fmt)
{
  if (!fmt.fontFamily().isEmpty()) {
    WriteCSSProperty(style, QStringLiteral("font-family"), fmt.fontFamily());
  }

  if (fmt.hasProperty(QTextFormat::FontPointSize)) {
    WriteCSSProperty(style, QStringLiteral("font-size"), QStringLiteral("%1pt").arg(QString::number(fmt.fontPointSize())));
  }

  if (fmt.hasProperty(QTextFormat::FontWeight)) {
    WriteCSSProperty(style, QStringLiteral("font-weight"), QString::number(fmt.fontWeight() * 8));
  }

  if (fmt.hasProperty(QTextFormat::FontItalic)) {
    WriteCSSProperty(style, QStringLiteral("font-style"), fmt.fontItalic() ? QStringLiteral("italic") : QStringLiteral("normal"));
  }

  if (fmt.hasProperty(QTextFormat::FontStyleName)) {
    WriteCSSProperty(style, QStringLiteral("-ove-font-style"), fmt.fontStyleName().toString());
  }

  QStringList deco;

  if (fmt.fontUnderline()) {
    deco.append(QStringLiteral("underline"));
  }

  if (fmt.fontStrikeOut()) {
    deco.append(QStringLiteral("line-through"));
  }

  if (fmt.fontOverline()) {
    deco.append(QStringLiteral("overline"));
  }

  if (!deco.isEmpty()) {
    WriteCSSProperty(style, QStringLiteral("text-decoration"), deco);
  }

  if (fmt.foreground().style() != Qt::NoBrush) {
    const QColor &color = fmt.foreground().color();
    QString cs;

    if (color.alpha() == 255) {
      cs = color.name();
    } else if (color.alpha()) {
      cs = QStringLiteral("rgba(%1, %2, %3, %4)").arg(QString::number(color.red()), QString::number(color.green()), QString::number(color.blue()), QString::number(color.alphaF()));
    }

    WriteCSSProperty(style, QStringLiteral("color"), cs);
  }

  if (fmt.fontCapitalization() != QFont::MixedCase) {
    if (fmt.fontCapitalization() == QFont::SmallCaps) {
      WriteCSSProperty(style, QStringLiteral("font-variant"), QStringLiteral("small-caps"));
      // TODO: Add others
    }
  }

  if (fmt.fontLetterSpacing() != 0.0) {
    WriteCSSProperty(style, QStringLiteral("letter-spacing"), QStringLiteral("%1%").arg(QString::number(fmt.fontLetterSpacing())));
  }

  if (fmt.fontStretch() != 0) {
    WriteCSSProperty(style, QStringLiteral("font-stretch"), QStringLiteral("%1%").arg(QString::number(fmt.fontStretch())));
  }
}

QTextCharFormat Html::ReadCharFormat(const QXmlStreamAttributes &attributes)
{
  QTextCharFormat fmt;

  foreach (const QXmlStreamAttribute &attr, attributes) {
    if (StrEquals(attr.name(), QStringLiteral("style"))) {
      auto css = GetCSSFromStyle(attr.value().toString());

      for (auto it=css.begin(); it!=css.end(); it++) {
        const QString &first_val = it.value().first();

        if (it.key() == QStringLiteral("font-family")) {
          fmt.setFontFamily(first_val);
        } else if (it.key() == QStringLiteral("font-size")) {
          if (first_val.endsWith(QStringLiteral("pt"), Qt::CaseInsensitive)) {
            fmt.setFontPointSize(first_val.chopped(2).toDouble());
          }
        } else if (it.key() == QStringLiteral("font-weight")) {
          fmt.setFontWeight(first_val.toInt()/8);
        } else if (it.key() == QStringLiteral("font-style")) {
          fmt.setFontItalic(StrEquals(first_val, QStringLiteral("italic")));
        } else if (it.key() == QStringLiteral("text-decoration")) {
          foreach (const QString &v, it.value()) {
            if (StrEquals(v, QStringLiteral("underline"))) {
              fmt.setFontUnderline(true);
            } else if (StrEquals(v, QStringLiteral("line-through"))) {
              fmt.setFontStrikeOut(true);
            } else if (StrEquals(v, QStringLiteral("overline"))) {
              fmt.setFontOverline(true);
            }
          }
        } else if (it.key() == QStringLiteral("color")) {
          if (first_val.startsWith(QStringLiteral("rgba"), Qt::CaseInsensitive)) {
            QString vals_only = first_val;
            vals_only.remove(QStringLiteral("rgba"));
            vals_only.remove(QStringLiteral("("));
            vals_only.remove(QStringLiteral(")"));
            QStringList rgba = vals_only.split(',');
            if (rgba.size() == 4) {
              QColor c;
              c.setRedF(rgba.at(0).toDouble());
              c.setGreenF(rgba.at(1).toDouble());
              c.setBlueF(rgba.at(2).toDouble());
              c.setAlphaF(rgba.at(3).toDouble());
              fmt.setForeground(c);
            }
          } else {
            fmt.setForeground(QColor(first_val));
          }
        } else if (it.key() == QStringLiteral("font-variant")) {
          if (StrEquals(first_val, QStringLiteral("small-caps"))) {
            fmt.setFontCapitalization(QFont::SmallCaps);
          }
        } else if (it.key() == QStringLiteral("letter-spacing")) {
          if (first_val.contains(QChar('%'))) {
            fmt.setFontLetterSpacing(first_val.chopped(1).toDouble());
          }
        } else if (it.key() == QStringLiteral("font-stretch")) {
          if (first_val.contains(QChar('%'))) {
            fmt.setFontStretch(first_val.chopped(1).toInt());
          }
        } else if (it.key() == QStringLiteral("-ove-font-style")) {
          fmt.setFontStyleName(first_val);
        }
      }
    }
  }
  return fmt;
}

QTextBlockFormat Html::ReadBlockFormat(const QXmlStreamAttributes &attributes)
{
  QTextBlockFormat block_fmt;

  foreach (const QXmlStreamAttribute &attr, attributes) {
    if (StrEquals(attr.name(), QStringLiteral("align"))) {
      if (StrEquals(attr.value(), QStringLiteral("right"))) {
        block_fmt.setAlignment(Qt::AlignRight);
      } else if (StrEquals(attr.value(), QStringLiteral("center"))) {
        block_fmt.setAlignment(Qt::AlignHCenter);
      } else if (StrEquals(attr.value(), QStringLiteral("justify"))) {
        block_fmt.setAlignment(Qt::AlignJustify);
      }
    } else if (StrEquals(attr.name(), QStringLiteral("dir"))) {
      if (StrEquals(attr.value(), QStringLiteral("rtl"))) {
        block_fmt.setLayoutDirection(Qt::RightToLeft);
      }
    } else if (StrEquals(attr.name(), QStringLiteral("style"))) {
      auto css = GetCSSFromStyle(attr.value().toString());

      for (auto it=css.begin(); it!=css.end(); it++) {
        if (it.key() == QStringLiteral("line-height")) {
          const QString &first_val = it.value().constFirst();
          if (first_val.contains(QChar('%'))) {
            block_fmt.setLineHeight(first_val.chopped(1).toDouble(), QTextBlockFormat::ProportionalHeight);
          }
        }
      }
    }
  }

  return block_fmt;
}

void Html::AppendStringAutoSpace(QString *s, const QString &append)
{
  if (!s->isEmpty()) {
    s->append(QChar(' '));
  }

  s->append(append);
}

QMap<QString, QStringList> Html::GetCSSFromStyle(const QString &s)
{
  QMap<QString, QStringList> map;

  QStringList list = s.split(QChar(';'));

  foreach (const QString &a, list) {
    QStringList kv = a.split(QChar(':'));

    if (kv.size() != 2) {
      continue;
    }

    // I'm sure there's regex that could do this, but I couldn't figure it out. It needs to split
    // by space EXCEPT within quotes OR double-quotes, and said quotes should be EXCLUDED from each
    // match. Also commas should be filtered out.
    QStringList values;
    const QString &val = kv.at(1);
    QChar in_quote(0);
    QString current_str;
    for (int i=0; i<val.size(); i++) {
      const QChar &current_char = val.at(i);

      if (!in_quote.isNull()) {
        // If inside quotes and character isn't quote, indiscriminately append char
        if (current_char == in_quote) {
          in_quote = QChar(0);
        } else {
          current_str.append(current_char);
        }
      } else if (current_char.isSpace() || current_char == QChar(',')) {
        // Dump current
        if (!current_str.isEmpty()) {
          values.append(current_str);
          current_str.clear();
        }
      } else if (in_quote.isNull() && (current_char == QChar('\'') || current_char == QChar('"'))) {
        in_quote = current_char;
      } else {
        current_str.append(current_char);
      }
    }

    if (!current_str.isEmpty()) {
      values.append(current_str);
    }

    // Not sure if this will ever happen, but just in case, we will avoid assert failures with this
    if (values.isEmpty()) {
      values.append(QString());
    }

    map[kv.at(0).trimmed().toLower()] = values;
  }

  return map;
}

}
