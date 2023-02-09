#ifndef HTML_H
#define HTML_H

#include <QTextDocument>
#include <QTextFragment>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace olive {

/**
 * @brief Functions for converting HTML to QTextDocument and vice versa
 *
 * Qt does contain its own functions for this, however they have some limitations. Some things that
 * we want to support (e.g. kerning/spacing and font stretch) are not implemented in Qt's
 * QTextHtmlExporter and QTextHtmlParser. Additionally, since these functions are not part of Qt's
 * public API, and make many references to other parts of Qt that are not part of the public API,
 * there is no way to subclass or extend their functionality without forking Qt as a whole.
 *
 * Therefore, it became necessary to write a custom class for the conversion so that we can
 * ensure support for the features we need.
 *
 * If someone wishes to extend this class for more feature support, feel free to open a pull
 * request. But this is NOT intended to be an exhaustive HTML implementation, and is primarily
 * designed to store rich text in a standard format for the purpose of text formatting for video.
 */
class Html {
public:
  static QString DocToHtml(const QTextDocument *doc);

  static void HtmlToDoc(QTextDocument *doc, const QString &html);

private:
  static void WriteBlock(QXmlStreamWriter *writer, const QTextBlock &block);

  static void WriteFragment(QXmlStreamWriter *writer, const QTextFragment &fragment);

  static void WriteCSSProperty(QString *style, const QString &key, const QStringList &value);
  static void WriteCSSProperty(QString *style, const QString &key, const QString &value)
  {
    WriteCSSProperty(style, key, QStringList({value}));
  }

  static void WriteCharFormat(QString *style, const QTextCharFormat &fmt);

  static QTextCharFormat ReadCharFormat(const QXmlStreamAttributes &attributes);

  static QTextBlockFormat ReadBlockFormat(const QXmlStreamAttributes &attributes);

  static void AppendStringAutoSpace(QString *s, const QString &append);

  static QMap<QString, QStringList> GetCSSFromStyle(const QString &s);

  static const QVector<QString> kBlockTags;

};

}

#endif // HTML_H
