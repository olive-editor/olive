#ifndef SEQUENCEVIEWERPANEL_H
#define SEQUENCEVIEWERPANEL_H

#include "panel/viewer/viewer.h"

class SequenceViewerPanel : public ViewerPanel
{
public:
  SequenceViewerPanel(QWidget* parent);

protected:
  virtual void Retranslate() override;

};

#endif // SEQUENCEVIEWERPANEL_H
