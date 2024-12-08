#include "textanimationxmlformatter.h"

#include <QXmlStreamWriter>

namespace olive {

TextAnimationXmlFormatter::TextAnimationXmlFormatter() :
  descriptor_(nullptr)
{
}

QString TextAnimationXmlFormatter::Format() const
{
  QString out;
  QXmlStreamWriter writer( & out);

  if (descriptor_) {
    writer.writeStartElement( TextAnimation::FEATURE_TABLE.value(descriptor_->feature, ""));

    writer.writeAttribute( "ch_from", QString("%1").arg(descriptor_->character_from));
    writer.writeAttribute( "ch_to", QString("%1").arg(descriptor_->character_to));
    writer.writeAttribute( "last_to_first", QString("%1").arg((int)descriptor_->last_to_first));
    writer.writeAttribute( "stride", QString("%1").arg(descriptor_->stride));
    writer.writeAttribute( "overlap_in", QString("%1").arg(descriptor_->overlap_in));
    writer.writeAttribute( "overlap_out", QString("%1").arg(descriptor_->overlap_out));
    writer.writeAttribute( "curve", TextAnimation::CURVE_TABLE.value(descriptor_->curve, ""));
    writer.writeAttribute( "c1", QString("%1").arg(descriptor_->c1));
    writer.writeAttribute( "c2", QString("%1").arg(descriptor_->c2));
    writer.writeAttribute( "value", QString("%1").arg(descriptor_->value));
    writer.writeAttribute( "progress", QString("%1").arg(descriptor_->progress));

    writer.writeEndElement();
  }

  return out;
}

}  // olive
