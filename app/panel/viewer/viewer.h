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
private:
};

#endif // VIEWER_PANEL_H
