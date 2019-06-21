#ifndef TOOL_PANEL_H
#define TOOL_PANEL_H

#include "widget/panel/panel.h"

class ToolPanel : public PanelWidget
{
  Q_OBJECT
public:
  ToolPanel(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();
};

#endif // TOOL_PANEL_H
