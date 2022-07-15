#include "textanimationxmlparser.h"
#include <QXmlStreamReader>


namespace olive {

TextAnimationXmlParser::TextAnimationXmlParser()
{
}

QList<TextAnimation::Descriptor> TextAnimationXmlParser::Parse(const QString& xmlInput)
{
  QList<TextAnimation::Descriptor> descriptors;
  QXmlStreamReader reader( xmlInput);
  TextAnimation::Descriptor item;

  while (reader.atEnd() == false) {

    switch( reader.readNext()) {
    case QXmlStreamReader::StartElement:
      item.feature = TextAnimation::FEATURE_TABLE_REV.value( reader.name().toString(), TextAnimation::None);
      item.character_from = reader.attributes().value(QString("ch_from")).toInt();
      item.character_to = reader.attributes().value(QString("ch_to")).toInt();
      item.overlap_in = reader.attributes().value(QString("overlap_in")).toDouble();
      item.overlap_out = reader.attributes().value(QString("overlap_out")).toDouble();
      item.curve = TextAnimation::CURVE_TABLE_REV.value( reader.attributes().value("curve").toString());
      item.c1 = reader.attributes().value(QString("c1")).toDouble();
      item.c2 = reader.attributes().value(QString("c2")).toDouble();
      item.value = reader.attributes().value(QString("value")).toDouble();
      item.alpha = reader.attributes().value(QString("alpha")).toDouble();

      if (item.feature != TextAnimation::None) {
        descriptors << item;
      }
      break;

    default:
      break;
    }
  }

  return descriptors;
}

}  // olive
