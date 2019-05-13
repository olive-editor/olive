#ifndef BOOLINPUT_H
#define BOOLINPUT_H

#include "nodes/nodeio.h"

class BoolInput : public NodeIO
{
  Q_OBJECT
public:
  BoolInput(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  /**
   * @brief Get the boolean value at a given timecode
   *
   * A wrapper for BoolField::GetBoolAt().
   *
   * @param timecode
   *
   * The timecode to retrieve the value at
   *
   * @return
   *
   * The boolean value at this timecode
   */
  bool GetBoolAt(double timecode);

signals:
  /**
   * @brief Emitted whenever the UI widget's boolean value has changed
   *
   * Wrapper for BoolField::Toggled().
   */
  void Toggled(bool);
};

#endif // BOOLINPUT_H
