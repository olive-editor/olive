#ifndef FONTINPUT_H
#define FONTINPUT_H

#include "effects/effectrow.h"

class FontInput : public EffectRow
{
  Q_OBJECT
public:
  FontInput(Effect* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  /**
   * @brief Get the font family name at the given timecode
   *
   * Wrapper for FontField::GetFontAt().
   *
   * @param timecode
   *
   * The timecode to retrieve the font family name at
   *
   * @return
   *
   * The font family name at this timecode
   */
  QString GetFontAt(double timecode);
};

#endif // FONTINPUT_H
