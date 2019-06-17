#ifndef VIEWER_PANEL_H
#define VIEWER_PANEL_H

#include "widget/panel/panel.h"

/**
 * @brief The ViewerPanel class
 *
 * Dockable wrapper around a ViewerWidget
 */
class ViewerPanel : public PanelWidget {
  Q_OBJECT
public:
  ViewerPanel(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();
};

#endif // VIEWER_PANEL_H
