#ifndef COLORINPUT_H
#define COLORINPUT_H

#include "effects/effectrow.h"

class ColorInput : public EffectRow
{
  Q_OBJECT
public:
  ColorInput(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  /**
   * @brief Get the color value at a given timecode
   *
   * Wrapper for ColorField::GetColorAt().
   *
   * @param timecode
   *
   * The timecode to retrieve the color at
   *
   * @return
   *
   * The color value at this timecode
   */
  QColor GetColorAt(double timecode);
};

#endif // COLORINPUT_H
