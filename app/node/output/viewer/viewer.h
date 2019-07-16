#ifndef VIEWER_H
#define VIEWER_H

#include "node/node.h"
#include "panel/viewer/viewer.h"

class ViewerOutput : public Node
{
  Q_OBJECT
public:
  ViewerOutput();

  virtual QString Name() override;
  virtual QString Category() override;
  virtual QString Description() override;

  NodeInput* texture_input();

  void AttachViewer(ViewerPanel* viewer);

public slots:
  virtual void Process(const rational &time) override;

private:
  NodeInput* texture_input_;

  ViewerPanel* attached_viewer_;
};

#endif // VIEWER_H
