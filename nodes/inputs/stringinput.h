#ifndef STRINGINPUT_H
#define STRINGINPUT_H

#include "effects/effectrow.h"

class StringInput : public EffectRow
{
  Q_OBJECT
public:
  StringInput(Effect* parent,
              const QString& id,
              const QString& name,
              bool rich_text = true,
              bool savable = true,
              bool keyframable = true);

  /**
   * @brief Get the string at the given timecode
   *
   * Wrappre for StringField::GetStringAt().
   *
   * @param timecode
   *
   * The timecode to retrieve the string at
   *
   * @return
   *
   * The string at this timecode
   */
  QString GetStringAt(double timecode);
};

#endif // STRINGINPUT_H
