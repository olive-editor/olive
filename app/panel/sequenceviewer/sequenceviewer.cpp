#include "sequenceviewer.h"

SequenceViewerPanel::SequenceViewerPanel(QWidget *parent) :
  ViewerPanel(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("SequenceViewerPanel");

  // Set strings
  Retranslate();
}

void SequenceViewerPanel::Retranslate()
{
  ViewerPanel::Retranslate();

  SetTitle(tr("Sequence Viewer"));
}
