#ifndef VIEWER_H
#define VIEWER_H

#include "node/node.h"
#include "panel/viewer/viewer.h"

/**
 * @brief A bridge between a node system and a ViewerPanel
 *
 * Receives update/time change signals from ViewerPanels and responds by sending them a texture of that frame
 */
class ViewerOutput : public Node
{
  Q_OBJECT
public:
  ViewerOutput();

  virtual QString Name() override;
  virtual QString id() override;
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
