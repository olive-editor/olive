#ifndef FILEINPUT_H
#define FILEINPUT_H

#include "nodes/nodeio.h"

class FileInput : public NodeIO
{
  Q_OBJECT
public:
  FileInput(OldEffectNode* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  /**
   * @brief Get the filename at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the filename at
   *
   * @return
   *
   * The filename at this timecode
   */
  QString GetFileAt(double timecode);
};

#endif // FILEINPUT_H
