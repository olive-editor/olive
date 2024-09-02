#ifndef ANIMATIONXMLFORMATTER_H
#define ANIMATIONXMLFORMATTER_H

#include <QString>

#include "textanimation.h"

namespace olive {

/// This class takes an animation descriptor and generates
/// an XML tag with all animation parameters.
class TextAnimationXmlFormatter
{
public:
  TextAnimationXmlFormatter();

  /// to be called before \a format
  void SetAnimationData( const struct TextAnimation::Descriptor * descr) {
    descriptor_ = descr;
  }

  /// main method to build the XML tag for a text animation
  QString Format() const;

private:
  const struct TextAnimation::Descriptor * descriptor_;
};

}  // olive

#endif // ANIMATIONXMLFORMATTER_H
